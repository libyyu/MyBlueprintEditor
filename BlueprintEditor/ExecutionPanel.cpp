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
    ActiveDoc()->persistentRunner.SetPrintCallback([this](::NodeEditor::Runtime::LogLevel /*level*/, const std::string& msg) {
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
        char statusBuf[128];
        snprintf(statusBuf, sizeof(statusBuf), "OK (%d nodes, %.2fms)",
                 result.nodesExecuted, elapsed);
        ActiveDoc()->lastExecutionStatus = statusBuf;
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
            size_t totalLen = 0;
            for (const auto& line : ActiveDoc()->executionLog)
                totalLen += line.size() + 1;   // +1 for '\n'
            allText.reserve(totalLen);
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

        // ── Tab: Watch ───────────────────────────────────────────────────
        if (ImGui::BeginTabItem(ICON_FA_EYE " Watch"))
        {
            DrawWatchPanel(paneWidth);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

// ============================================================================
// Watch 面板（运行时变量/引脚值监控）
// ============================================================================

void BlueprintEditor::DrawWatchPanel(float paneWidth)
{
    auto* doc = ActiveDoc();
    if (!doc) return;

    auto& runner = doc->persistentRunner;
    const auto& allVars = runner.GetAllVariables();
    const auto& res = doc->lastExecutionResult;

    // Variant 类型→可读名称
    auto dataTypeStr = [](RTPinDataType t) -> const char* {
        switch (t) {
        case RTPinDataType::Boolean: return "Bool";
        case RTPinDataType::Integer: return "Int";
        case RTPinDataType::Float:   return "Float";
        case RTPinDataType::String:  return "String";
        case RTPinDataType::Object:  return "Object";
        case RTPinDataType::Array:   return "Array";
        case RTPinDataType::Map:     return "Map";
        case RTPinDataType::Any:     return "Any";
        default:                     return "Unknown";
        }
    };

    // Variant 类型→颜色
    auto dataTypeColor = [](RTPinDataType t) -> ImVec4 {
        switch (t) {
        case RTPinDataType::Boolean: return ImVec4(0.9f, 0.4f, 0.4f, 1.0f);
        case RTPinDataType::Integer: return ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
        case RTPinDataType::Float:   return ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
        case RTPinDataType::String:  return ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
        case RTPinDataType::Object:  return ImVec4(0.8f, 0.5f, 1.0f, 1.0f);
        case RTPinDataType::Array:   return ImVec4(0.5f, 1.0f, 0.8f, 1.0f);
        case RTPinDataType::Map:     return ImVec4(1.0f, 0.6f, 0.2f, 1.0f);
        default:                     return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        }
    };

    float watchH = ImGui::GetContentRegionAvail().y - 4.0f;
    if (watchH < 40.0f) watchH = 40.0f;
    ImGui::BeginChild("##WatchContent", ImVec2(paneWidth, watchH), false);

    // ── Section 1: Blueprint Variables ───────────────────────────────────
    {
        auto* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float sectionH = ImGui::GetTextLineHeight() + 4.0f;
        ImU32 colL = IM_COL32(35, 52, 60, 210);
        ImU32 colR = IM_COL32(28, 40, 48, 180);
        drawList->AddRectFilledMultiColor(
            cursorPos,
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
            colL, colR, colR, colL);
        drawList->AddText(
            ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
            IM_COL32(140, 210, 220, 230), ICON_FA_LAYER_GROUP " Variables");

        // 变量数量标签
        char countBuf[32];
        snprintf(countBuf, sizeof(countBuf), "(%d)", static_cast<int>(allVars.size()));
        float countW = ImGui::CalcTextSize(countBuf).x;
        drawList->AddText(
            ImVec2(cursorPos.x + paneWidth - countW - 10.0f, cursorPos.y + 2.0f),
            IM_COL32(100, 140, 150, 180), countBuf);

        ImGui::Dummy(ImVec2(paneWidth, sectionH));
    }

    if (allVars.empty())
    {
        ImGui::TextDisabled("  No runtime variables.");
        ImGui::TextDisabled("  Execute the blueprint to populate.");
    }
    else
    {
        // 使用表格展示变量
        if (ImGui::BeginTable("##WatchVars", 3,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Name",  ImGuiTableColumnFlags_WidthStretch, 0.35f);
            ImGui::TableSetupColumn("Type",  ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.50f);
            ImGui::TableHeadersRow();

            for (const auto& kv : allVars)
            {
                ImGui::TableNextRow();

                // Name
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(kv.first.c_str());

                // Type
                ImGui::TableNextColumn();
                ImVec4 tCol = dataTypeColor(kv.second.type);
                ImGui::TextColored(tCol, "%s", dataTypeStr(kv.second.type));

                // Value
                ImGui::TableNextColumn();
                std::string valStr = kv.second.asString();
                if (valStr.size() > 80)
                    valStr = valStr.substr(0, 77) + "...";
                ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "%s", valStr.c_str());
            }

            ImGui::EndTable();
        }
    }

    // ── Section 2: Selected Node Pin Values ─────────────────────────────
    ImGui::Spacing();
    {
        auto* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float sectionH = ImGui::GetTextLineHeight() + 4.0f;
        ImU32 colL = IM_COL32(50, 42, 35, 210);
        ImU32 colR = IM_COL32(38, 32, 28, 180);
        drawList->AddRectFilledMultiColor(
            cursorPos,
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
            colL, colR, colR, colL);
        drawList->AddText(
            ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
            IM_COL32(220, 190, 140, 230), ICON_FA_CUBE " Selected Node Pins");
        ImGui::Dummy(ImVec2(paneWidth, sectionH));
    }

    // 获取选中节点
    std::vector<ed::NodeId> selectedNodes;
    selectedNodes.resize(ed::GetSelectedObjectCount());
    int selCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
    selectedNodes.resize(selCount);

    if (selectedNodes.empty())
    {
        ImGui::TextDisabled("  Select a node to watch its pin values.");
    }
    else
    {
        for (const auto& selNodeId : selectedNodes)
        {
            Node* node = FindNode(selNodeId);
            if (!node) continue;

            // 节点标题
            ImGui::TextColored(ImVec4(0.70f, 0.85f, 1.0f, 1.0f), ICON_FA_CUBE " %s", node->Name.c_str());

            uint64_t nodeIdVal = reinterpret_cast<uintptr_t>(node->ID.AsPointer());

            // 检查该节点是否在上次执行中被执行过
            bool wasExecuted = false;
            for (auto rid : res.executedNodeIds)
            {
                if (rid == nodeIdVal) { wasExecuted = true; break; }
            }

            if (!wasExecuted && res.nodesExecuted > 0)
            {
                ImGui::TextDisabled("    (not executed in last run)");
                continue;
            }

            if (res.nodesExecuted == 0)
            {
                ImGui::TextDisabled("    (no execution data)");
                continue;
            }

            // 输入引脚值
            bool hasAnyPinValue = false;
            for (const auto& pin : node->Inputs)
            {
                if (pin.Type == PinType::Flow) continue;
                uint64_t pinId = reinterpret_cast<uintptr_t>(pin.ID.AsPointer());

                // 从执行结果查找
                auto it = res.outputValues.find(pinId);
                RTVariant val;
                if (it != res.outputValues.end())
                    val = it->second;
                else
                    val = runner.GetPinValue(pinId);

                if (val.type != RTPinDataType::Unknown)
                {
                    hasAnyPinValue = true;
                    ImVec4 tCol = dataTypeColor(val.type);
                    std::string valStr = val.asString();
                    if (valStr.size() > 60) valStr = valStr.substr(0, 57) + "...";
                    ImGui::TextColored(ImVec4(0.50f, 0.55f, 0.65f, 1.0f), "    " ICON_FA_ARROW_RIGHT " %s:", pin.Name.c_str());
                    ImGui::SameLine();
                    ImGui::TextColored(tCol, "%s", valStr.c_str());
                }
            }

            // 输出引脚值
            for (const auto& pin : node->Outputs)
            {
                if (pin.Type == PinType::Flow) continue;
                uint64_t pinId = reinterpret_cast<uintptr_t>(pin.ID.AsPointer());

                auto it = res.outputValues.find(pinId);
                RTVariant val;
                if (it != res.outputValues.end())
                    val = it->second;
                else
                    val = runner.GetPinValue(pinId);

                if (val.type != RTPinDataType::Unknown)
                {
                    hasAnyPinValue = true;
                    ImVec4 tCol = dataTypeColor(val.type);
                    std::string valStr = val.asString();
                    if (valStr.size() > 60) valStr = valStr.substr(0, 57) + "...";
                    ImGui::TextColored(ImVec4(0.50f, 0.55f, 0.65f, 1.0f), "    " ICON_FA_ARROW_LEFT " %s:", pin.Name.c_str());
                    ImGui::SameLine();
                    ImGui::TextColored(tCol, "%s", valStr.c_str());
                }
            }

            if (!hasAnyPinValue)
                ImGui::TextDisabled("    (no pin values captured)");

            ImGui::Spacing();
        }
    }

    // ── Section 3: Output Values Summary ────────────────────────────────
    if (!res.outputValues.empty())
    {
        ImGui::Spacing();
        {
            auto* drawList = ImGui::GetWindowDrawList();
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            float sectionH = ImGui::GetTextLineHeight() + 4.0f;
            ImU32 colL = IM_COL32(35, 42, 55, 210);
            ImU32 colR = IM_COL32(28, 32, 42, 180);
            drawList->AddRectFilledMultiColor(
                cursorPos,
                ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
                colL, colR, colR, colL);

            char titleBuf[64];
            snprintf(titleBuf, sizeof(titleBuf), ICON_FA_CIRCLE_CHECK " All Output Values (%d)",
                static_cast<int>(res.outputValues.size()));
            drawList->AddText(
                ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
                IM_COL32(140, 170, 220, 230), titleBuf);
            ImGui::Dummy(ImVec2(paneWidth, sectionH));
        }

        if (ImGui::BeginTable("##WatchOutputs", 3,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Pin",   ImGuiTableColumnFlags_WidthStretch, 0.35f);
            ImGui::TableSetupColumn("Type",  ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.50f);
            ImGui::TableHeadersRow();

            for (const auto& kv : res.outputValues)
            {
                ImGui::TableNextRow();

                // Pin ID → 找节点和引脚名
                std::string pinLabel;
                for (const auto& n : doc->nodes)
                {
                    bool found = false;
                    for (const auto& p : n.Outputs)
                    {
                        uint64_t pid = reinterpret_cast<uintptr_t>(p.ID.AsPointer());
                        if (pid == kv.first)
                        {
                            pinLabel = n.Name + "." + p.Name;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                    for (const auto& p : n.Inputs)
                    {
                        uint64_t pid = reinterpret_cast<uintptr_t>(p.ID.AsPointer());
                        if (pid == kv.first)
                        {
                            pinLabel = n.Name + "." + p.Name;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }

                // Pin Name
                ImGui::TableNextColumn();
                if (pinLabel.empty())
                    ImGui::Text("pin#%llu", static_cast<unsigned long long>(kv.first));
                else
                    ImGui::TextUnformatted(pinLabel.c_str());

                // Type
                ImGui::TableNextColumn();
                ImVec4 tCol = dataTypeColor(kv.second.type);
                ImGui::TextColored(tCol, "%s", dataTypeStr(kv.second.type));

                // Value
                ImGui::TableNextColumn();
                std::string valStr = kv.second.asString();
                if (valStr.size() > 80) valStr = valStr.substr(0, 77) + "...";
                ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "%s", valStr.c_str());
            }

            ImGui::EndTable();
        }
    }

    ImGui::EndChild();
}
