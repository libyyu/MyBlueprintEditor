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

    // 3. 用当前文件路径重新注册 handlers（确保 ExecuteBlueprint 节点能解析子蓝图的相对路径）
    {
        std::string currentPath = ActiveDoc()->filePath;  // 当前蓝图文件路径
        ::NodeEditor::Runtime::RegisterBuiltinHandlers(
            ActiveDoc()->persistentRunner, currentPath, &m_HandlerRegistry);
    }
    if (m_DefaultHandler)
        ActiveDoc()->persistentRunner.SetDefaultHandler(m_DefaultHandler);

    // 4. 执行
    auto startTime = std::chrono::high_resolution_clock::now();
    auto result = ActiveDoc()->persistentRunner.Execute();
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

    // 5. 执行可视化 —— 高亮已执行的节点 & 触发 Flow 动画
    // 将所有节点标记为高亮（实际运行器会执行拓扑排序后的节点）
    if (result.success)
    {
        ActiveDoc()->executedNodeHighlight.clear();
        for (const auto& node : ActiveDoc()->nodes)
        {
            uint64_t nid = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(node.ID.AsPointer()));
            ActiveDoc()->executedNodeHighlight[nid] = 3.0f;  // 3 秒高亮
        }
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

    ImGui::BeginHorizontal("ExecButtons", ImVec2(paneWidth, 0));
    if (ImGui::Button(ICON_FA_PLAY " Execute", ImVec2(90, 0)))
    {
        ExecuteBlueprint();
    }
    ImGui::Spring(0.0f);
    if (ImGui::Button(ICON_FA_COPY " Copy Log", ImVec2(100, 0)))
    {
        if (!ActiveDoc()->executionLog.empty())
        {
            std::string allText;
            for (const auto& line : ActiveDoc()->executionLog)
            {
                allText += line;
                allText += '\n';
            }
            ImGui::SetClipboardText(allText.c_str());
        }
    }
    ImGui::Spring(0.0f);
    if (ImGui::Button(ICON_FA_ERASER " Clear", ImVec2(80, 0)))
    {
        ActiveDoc()->executionLog.clear();
        ActiveDoc()->executionLogText.clear();
        ActiveDoc()->executionLogDirty = false;
        ActiveDoc()->lastExecutionStatus.clear();
    }
    ImGui::Spring();
    ImGui::EndHorizontal();

    if (!ActiveDoc()->lastExecutionStatus.empty())
    {
        bool isOk = ActiveDoc()->lastExecutionStatus.find("OK") == 0;
        ImGui::TextColored(isOk ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
            "Status: %s", ActiveDoc()->lastExecutionStatus.c_str());
    }

    // 彩色日志输出区域
    float logHeight = ImGui::GetContentRegionAvail().y;
    if (logHeight < 60.0f) logHeight = 60.0f;

    ImGui::BeginChild("##ExecutionLog", ImVec2(paneWidth, logHeight), true,
        ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& line : ActiveDoc()->executionLog)
    {
        DrawColoredLogLine(line);
    }

    // 自动滚动到底部
    if (ActiveDoc()->executionLogDirty)
    {
        ImGui::SetScrollHereY(1.0f);
        ActiveDoc()->executionLogDirty = false;
    }

    ImGui::EndChild();
}
