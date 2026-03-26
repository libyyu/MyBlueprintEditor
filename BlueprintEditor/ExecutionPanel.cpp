// ExecutionPanel.cpp -- 蓝图执行 & 执行面板 UI
#include "BlueprintEditor.h"
#include "BuiltinHandlers.h"

// ============================================================================
// 执行蓝图
// ============================================================================

void BlueprintEditor::ExecuteBlueprint()
{
    ActiveDoc()->executionLog.clear();
    ActiveDoc()->flowLinks.clear();
    ActiveDoc()->isExecuting = true;

    ActiveDoc()->executionLog.push_back("========================================");
    ActiveDoc()->executionLog.push_back("  Blueprint Execution Started");
    ActiveDoc()->executionLog.push_back("========================================");

    // ================================================================
    // 执行前验证（UE4 风格）
    // ================================================================
    bool hasErrors = false;
    int errorCount = 0;
    int warningCount = 0;

    for (const auto& node : ActiveDoc()->nodes)
    {
        // 检查错误节点（定义不存在或有孤立引脚）
        if (node.HasError)
        {
            ActiveDoc()->executionLog.push_back("[ERROR] Node '" + node.Name + "': " + node.ErrorMessage);
            hasErrors = true;
            ++errorCount;
        }

        // 检查孤立引脚
        for (const auto& pin : node.Inputs)
        {
            if (pin.IsOrphaned)
            {
                std::string pinName = pin.Name.empty() ? "(unnamed)" : pin.Name;
                ActiveDoc()->executionLog.push_back("[WARN] Node '" + node.Name + "': Input pin '" + pinName + "' is orphaned (removed from definition)");
                ++warningCount;
            }
        }
        for (const auto& pin : node.Outputs)
        {
            if (pin.IsOrphaned)
            {
                std::string pinName = pin.Name.empty() ? "(unnamed)" : pin.Name;
                ActiveDoc()->executionLog.push_back("[WARN] Node '" + node.Name + "': Output pin '" + pinName + "' is orphaned (removed from definition)");
                ++warningCount;
            }
        }

        // 检查必须连接但未连线的输入引脚（如 Delegate 引脚）
        for (const auto& pin : node.Inputs)
        {
            if (pin.IsRequired && !pin.IsOrphaned && !IsPinLinked(pin.ID))
            {
                std::string pinName = pin.Name.empty() ? "(unnamed)" : pin.Name;
                ActiveDoc()->executionLog.push_back("[ERROR] Node '" + node.Name + "': Required input pin '" + pinName + "' is not connected");
                hasErrors = true;
                ++errorCount;
            }
        }
    }

    if (errorCount > 0 || warningCount > 0)
    {
        ActiveDoc()->executionLog.push_back("");
        ActiveDoc()->executionLog.push_back("Validation: " + std::to_string(errorCount) + " error(s), " + std::to_string(warningCount) + " warning(s)");
    }

    if (hasErrors)
    {
        ActiveDoc()->executionLog.push_back("");
        ActiveDoc()->executionLog.push_back("========================================");
        ActiveDoc()->executionLog.push_back("  Execution ABORTED: Fix errors first");
        ActiveDoc()->executionLog.push_back("========================================");
        ActiveDoc()->lastExecutionStatus = "FAILED: " + std::to_string(errorCount) + " error(s) found";
        ActiveDoc()->isExecuting = false;
        ActiveDoc()->executionLogDirty = true;
        return;
    }

    // 1. 构建 Runtime 数据
    RTBlueprintData bp = BuildRuntimeData();

    ActiveDoc()->executionLog.push_back("Nodes: " + std::to_string(bp.nodes.size()));
    ActiveDoc()->executionLog.push_back("Links: " + std::to_string(bp.links.size()));
    ActiveDoc()->executionLog.push_back("");

    // 2. 使用持久 Runner（这样 Delay 等异步操作注册的 timer 不会随局部变量销毁）
    ActiveDoc()->persistentRunner.ResetState();
    ActiveDoc()->persistentRunner.m_withEditor = true;  // friend 权限：标记在编辑器环境下运行
    ActiveDoc()->persistentRunner.SetLogCallback([this](::NodeEditor::Runtime::LogLevel /*level*/, const std::string& msg) {
        ActiveDoc()->executionLog.push_back(msg);
        ActiveDoc()->executionLogDirty = true;
    });

    if (!ActiveDoc()->persistentRunner.Load(bp))
    {
        ActiveDoc()->executionLog.push_back("[ERROR] Failed to load: " + ActiveDoc()->persistentRunner.GetLastError());
        ActiveDoc()->lastExecutionStatus = "Load Failed";
        ActiveDoc()->isExecuting = false;
        return;
    }

    // 3. 用当前文件所在目录重新注册 handlers（确保 ExecuteBlueprint 节点能解析子蓝图的相对路径）
    //    basePath 约定为目录路径，需从完整文件路径中提取目录部分
    {
        std::string basePath;
        const std::string& fp = ActiveDoc()->filePath;
        auto pos = fp.find_last_of("/\\");
        if (pos != std::string::npos)
            basePath = fp.substr(0, pos);  // "C:/foo/bar/NLoop.json" → "C:/foo/bar"
        else
            basePath = ".";               // 无目录分隔符时使用当前目录
        ::NodeEditor::Runtime::RegisterBuiltinHandlers(
            ActiveDoc()->persistentRunner, basePath, &m_HandlerRegistry);
    }
    if (m_DefaultHandler)
        ActiveDoc()->persistentRunner.SetDefaultHandler(m_DefaultHandler);

    // 4. 执行
    auto startTime = std::chrono::high_resolution_clock::now();
    auto result = ActiveDoc()->persistentRunner.Execute();
    ActiveDoc()->lastExecutionResult = result;  // 保存执行结果供面板展示
    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    ActiveDoc()->executionLog.push_back("");
    ActiveDoc()->executionLog.push_back("========================================");
    if (result.success)
    {
        ActiveDoc()->executionLog.push_back("  Execution Completed Successfully!");
        ActiveDoc()->lastExecutionStatus = "OK (" + std::to_string(result.nodesExecuted) + " nodes, " +
            std::to_string(elapsed).substr(0, std::to_string(elapsed).find('.') + 3) + "ms)";
    }
    else
    {
        ActiveDoc()->executionLog.push_back("  Execution FAILED: " + result.errorMessage);
        ActiveDoc()->lastExecutionStatus = "FAILED: " + result.errorMessage;
    }
    ActiveDoc()->executionLog.push_back("  Nodes executed: " + std::to_string(result.nodesExecuted));
    ActiveDoc()->executionLog.push_back("  Elapsed: " + std::to_string(elapsed) + " ms");
    ActiveDoc()->executionLog.push_back("========================================");

    // 5. 执行可视化 —— 精确高亮已执行的节点 & 触发 Flow 动画
    ActiveDoc()->executedNodeHighlight.clear();
    for (const auto& runtimeId : result.executedNodeIds)
    {
        // runtime NodeId → editor NodeId 映射：通过 definitionId 比对
        // runtime node id = reinterpret_cast<uintptr_t>(editorNode.ID.AsPointer())
        ActiveDoc()->executedNodeHighlight[runtimeId] = 3.0f;  // 3 秒高亮
    }

    for (const auto& link : ActiveDoc()->links)
    {
        ActiveDoc()->flowLinks.push_back(link.ID);
    }

    ActiveDoc()->isExecuting = false;
    ActiveDoc()->executionLogDirty = true;
}

// ============================================================================
// 执行面板 UI
// ============================================================================

void BlueprintEditor::ShowExecutionPanel(float paneWidth)
{
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImGui::GetCursorScreenPos(),
        ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
        ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
    ImGui::Spacing(); ImGui::SameLine();
    ImGui::TextUnformatted("Execution");

    // ── 工具栏 ──────────────────────────────────────────────────────────
    auto& runner = ActiveDoc()->persistentRunner;
    bool isRunning = runner.IsRunning();
    bool isPaused  = runner.IsPaused();
    bool isStopped = runner.IsStopped();
    bool isIdle    = runner.IsIdle();

    ImGui::BeginHorizontal("ExecButtons", ImVec2(paneWidth, 0));

    // Run（Idle / Stopped 时可点击）
    if (!isIdle && !isStopped) ImGui::BeginDisabled();
    if (ImGui::Button(ICON_FA_PLAY " Run", ImVec2(60, 0)))
    {
        if (isStopped) runner.ResetState();  // 停止后需 reset 才能再次执行
        ExecuteBlueprint();
    }
    if (!isIdle && !isStopped) ImGui::EndDisabled();

    ImGui::Spring(0.0f);

    // Pause / Resume 切换（Running 时显示 Pause，Paused 时显示 Resume）
    if (!isRunning && !isPaused) ImGui::BeginDisabled();
    if (isPaused)
    {
        if (ImGui::Button(ICON_FA_PLAY " Resume", ImVec2(80, 0)))
            runner.Resume();
    }
    else
    {
        if (ImGui::Button(ICON_FA_PAUSE " Pause", ImVec2(70, 0)))
            runner.Pause();
    }
    if (!isRunning && !isPaused) ImGui::EndDisabled();

    ImGui::Spring(0.0f);

    // Stop（Running 或 Paused 时可点击）
    if (isIdle || isStopped) ImGui::BeginDisabled();
    if (ImGui::Button(ICON_FA_STOP " Stop", ImVec2(60, 0)))
        runner.Stop();
    if (isIdle || isStopped) ImGui::EndDisabled();

    ImGui::Spring(0.0f);
    if (ImGui::Button(ICON_FA_COPY " Copy", ImVec2(55, 0)))
    {
        if (!ActiveDoc()->executionLog.empty())
        {
            std::string allText;
            for (const auto& line : ActiveDoc()->executionLog)
            { allText += line; allText += '\n'; }
            ImGui::SetClipboardText(allText.c_str());
        }
    }
    ImGui::Spring(0.0f);
    if (ImGui::Button(ICON_FA_ERASER " Clear", ImVec2(55, 0)))
    {
        ActiveDoc()->executionLog.clear();
        ActiveDoc()->executionLogDirty = false;
        ActiveDoc()->lastExecutionStatus.clear();
        ActiveDoc()->lastExecutionResult = RTExecutionResult{};
    }
    ImGui::Spring();
    ImGui::EndHorizontal();

    // 运行状态指示条
    {
        ImVec4 stateCol;
        const char* stateText;
        if (isRunning)      { stateCol = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);  stateText = ICON_FA_CIRCLE_PLAY  " Running..."; }
        else if (isPaused)  { stateCol = ImVec4(1.0f, 0.7f, 0.1f, 1.0f);  stateText = ICON_FA_PAUSE " Paused";     }
        else if (isStopped) { stateCol = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);  stateText = ICON_FA_CIRCLE_STOP  " Stopped";    }
        else                { stateCol = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);  stateText = "Idle";       }
        ImGui::TextColored(stateCol, "%s", stateText);
    }

    // ── 状态摘要 ─────────────────────────────────────────────────────────
    if (!ActiveDoc()->lastExecutionStatus.empty())
    {
        bool isOk = ActiveDoc()->lastExecutionStatus.rfind("OK", 0) == 0;
        ImGui::TextColored(
            isOk ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
            "%s  %s", isOk ? ICON_FA_CIRCLE_CHECK : ICON_FA_CIRCLE_XMARK,
            ActiveDoc()->lastExecutionStatus.c_str());
    }

    const auto& res = ActiveDoc()->lastExecutionResult;
    if (res.nodesExecuted > 0)
    {
        ImGui::SameLine(0, 16);
        ImGui::TextDisabled("%d nodes  %.1f ms",
            res.nodesExecuted, res.elapsedMs);
    }

    // ── Tab: Log | Nodes ─────────────────────────────────────────────────
    if (ImGui::BeginTabBar("##ExecTabs"))
    {
        // ── Tab: Log ────────────────────────────────────────────────────
        if (ImGui::BeginTabItem(ICON_FA_TERMINAL " Log"))
        {
            // 过滤框
            ImGui::SetNextItemWidth(paneWidth - 16.0f);
            ImGui::InputTextWithHint("##LogFilter", ICON_FA_MAGNIFYING_GLASS " Filter...",
                ActiveDoc()->execLogFilter, sizeof(ActiveDoc()->execLogFilter));

            float logH = ImGui::GetContentRegionAvail().y - 4.0f;
            if (logH < 40.0f) logH = 40.0f;
            ImGui::BeginChild("##ExecLog", ImVec2(paneWidth, logH), true,
                ImGuiWindowFlags_HorizontalScrollbar);

            std::string filter(ActiveDoc()->execLogFilter);
            for (const auto& line : ActiveDoc()->executionLog)
            {
                if (!filter.empty() && line.find(filter) == std::string::npos)
                    continue;
                DrawColoredLogLine(line);
            }

            if (ActiveDoc()->executionLogDirty)
            {
                ImGui::SetScrollHereY(1.0f);
                ActiveDoc()->executionLogDirty = false;
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // ── Tab: Nodes ───────────────────────────────────────────────────
        if (ImGui::BeginTabItem(ICON_FA_DIAGRAM_PROJECT " Nodes"))
        {
            if (res.executedNodeIds.empty())
            {
                ImGui::TextDisabled("No execution data yet. Press Run.");
            }
            else
            {
                ImGui::Text("Executed %d node(s) in %.2f ms:",
                    res.nodesExecuted, res.elapsedMs);
                ImGui::Separator();

                float nodeH = ImGui::GetContentRegionAvail().y - 4.0f;
                if (nodeH < 40.0f) nodeH = 40.0f;
                ImGui::BeginChild("##ExecNodes", ImVec2(paneWidth, nodeH), false);

                for (int i = 0; i < (int)res.executedNodeIds.size(); ++i)
                {
                    uint64_t rid = res.executedNodeIds[i];

                    // 找对应的编辑器节点名
                    std::string nodeName;
                    ed::NodeId edId = 0;
                    for (const auto& n : ActiveDoc()->nodes)
                    {
                        uint64_t eid = static_cast<uint64_t>(
                            reinterpret_cast<uintptr_t>(n.ID.AsPointer()));
                        if (eid == rid)
                        {
                            nodeName = n.Name.empty() ? n.DefinitionId : n.Name;
                            edId = n.ID;
                            break;
                        }
                    }

                    // 行着色：最后一个=红色(失败结束点) or 绿色
                    bool isLast = (i == (int)res.executedNodeIds.size() - 1);
                    ImVec4 rowCol = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
                    if (isLast && !res.success)
                        rowCol = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);

                    ImGui::TextColored(rowCol, "%3d. %s",
                        i + 1, nodeName.empty() ?
                            ("[id=" + std::to_string(rid) + "]").c_str() :
                            nodeName.c_str());

                    // 悬停时高亮对应节点
                    if (edId && ImGui::IsItemHovered())
                    {
                        ActiveDoc()->executedNodeHighlight[rid] = 1.5f;
                        if (ImGui::IsItemClicked())
                            ed::SelectNode(edId, false);
                    }
                }

                ImGui::EndChild();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
