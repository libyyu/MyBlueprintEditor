// ExecutionPanel.cpp -- 蓝图执行 & 执行面板 UI
#include "BlueprintEditor.h"

// ============================================================================
// 执行蓝图
// ============================================================================

void BlueprintEditor::ExecuteBlueprint()
{
    m_ExecutionLog.clear();
    m_FlowLinks.clear();
    m_IsExecuting = true;

    m_ExecutionLog.push_back("========================================");
    m_ExecutionLog.push_back("  Blueprint Execution Started");
    m_ExecutionLog.push_back("========================================");

    // ================================================================
    // 执行前验证（UE4 风格）
    // ================================================================
    bool hasErrors = false;
    int errorCount = 0;
    int warningCount = 0;

    for (const auto& node : m_Nodes)
    {
        // 检查错误节点（定义不存在或有孤立引脚）
        if (node.HasError)
        {
            m_ExecutionLog.push_back("[ERROR] Node '" + node.Name + "': " + node.ErrorMessage);
            hasErrors = true;
            ++errorCount;
        }

        // 检查孤立引脚
        for (const auto& pin : node.Inputs)
        {
            if (pin.IsOrphaned)
            {
                std::string pinName = pin.Name.empty() ? "(unnamed)" : pin.Name;
                m_ExecutionLog.push_back("[WARN] Node '" + node.Name + "': Input pin '" + pinName + "' is orphaned (removed from definition)");
                ++warningCount;
            }
        }
        for (const auto& pin : node.Outputs)
        {
            if (pin.IsOrphaned)
            {
                std::string pinName = pin.Name.empty() ? "(unnamed)" : pin.Name;
                m_ExecutionLog.push_back("[WARN] Node '" + node.Name + "': Output pin '" + pinName + "' is orphaned (removed from definition)");
                ++warningCount;
            }
        }

        // 检查必须连接但未连线的输入引脚（如 Delegate 引脚）
        for (const auto& pin : node.Inputs)
        {
            if (pin.IsRequired && !pin.IsOrphaned && !IsPinLinked(pin.ID))
            {
                std::string pinName = pin.Name.empty() ? "(unnamed)" : pin.Name;
                m_ExecutionLog.push_back("[ERROR] Node '" + node.Name + "': Required input pin '" + pinName + "' is not connected");
                hasErrors = true;
                ++errorCount;
            }
        }
    }

    if (errorCount > 0 || warningCount > 0)
    {
        m_ExecutionLog.push_back("");
        m_ExecutionLog.push_back("Validation: " + std::to_string(errorCount) + " error(s), " + std::to_string(warningCount) + " warning(s)");
    }

    if (hasErrors)
    {
        m_ExecutionLog.push_back("");
        m_ExecutionLog.push_back("========================================");
        m_ExecutionLog.push_back("  Execution ABORTED: Fix errors first");
        m_ExecutionLog.push_back("========================================");
        m_LastExecutionStatus = "FAILED: " + std::to_string(errorCount) + " error(s) found";
        m_IsExecuting = false;
        m_ExecutionLogDirty = true;
        return;
    }

    // 1. 构建 Runtime 数据
    RTBlueprintData bp = BuildRuntimeData();

    m_ExecutionLog.push_back("Nodes: " + std::to_string(bp.nodes.size()));
    m_ExecutionLog.push_back("Links: " + std::to_string(bp.links.size()));
    m_ExecutionLog.push_back("");

    // 2. 使用持久 Runner（这样 Delay 等异步操作注册的 timer 不会随局部变量销毁）
    m_PersistentRunner.ResetState();
    m_PersistentRunner.m_withEditor = true;  // friend 权限：标记在编辑器环境下运行
    m_PersistentRunner.SetLogCallback([this](const std::string& msg) {
        m_ExecutionLog.push_back(msg);
        m_ExecutionLogDirty = true;
    });

    if (!m_PersistentRunner.Load(bp))
    {
        m_ExecutionLog.push_back("[ERROR] Failed to load: " + m_PersistentRunner.GetLastError());
        m_LastExecutionStatus = "Load Failed";
        m_IsExecuting = false;
        return;
    }

    // 3. Register all handlers from the handler registry
    if (m_DefaultHandler)
        m_PersistentRunner.SetDefaultHandler(m_DefaultHandler);
    m_PersistentRunner.RegisterHandlers(m_HandlerRegistry);

    // 4. 执行
    auto startTime = std::chrono::high_resolution_clock::now();
    auto result = m_PersistentRunner.Execute();
    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    m_ExecutionLog.push_back("");
    m_ExecutionLog.push_back("========================================");
    if (result.success)
    {
        m_ExecutionLog.push_back("  Execution Completed Successfully!");
        m_LastExecutionStatus = "OK (" + std::to_string(result.nodesExecuted) + " nodes, " +
            std::to_string(elapsed).substr(0, std::to_string(elapsed).find('.') + 3) + "ms)";
    }
    else
    {
        m_ExecutionLog.push_back("  Execution FAILED: " + result.errorMessage);
        m_LastExecutionStatus = "FAILED: " + result.errorMessage;
    }
    m_ExecutionLog.push_back("  Nodes executed: " + std::to_string(result.nodesExecuted));
    m_ExecutionLog.push_back("  Elapsed: " + std::to_string(elapsed) + " ms");
    m_ExecutionLog.push_back("========================================");

    // 5. 执行可视化 —— 高亮已执行的节点 & 触发 Flow 动画
    // 将所有节点标记为高亮（实际运行器会执行拓扑排序后的节点）
    if (result.success)
    {
        ActiveDoc()->executedNodeHighlight.clear();
        for (const auto& node : m_Nodes)
        {
            uint64_t nid = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(node.ID.AsPointer()));
            ActiveDoc()->executedNodeHighlight[nid] = 3.0f;  // 3 秒高亮
        }
    }

    for (const auto& link : m_Links)
    {
        m_FlowLinks.push_back(link.ID);
    }

    m_IsExecuting = false;
    m_ExecutionLogDirty = true;
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
    if (ImGui::Button("Execute", ImVec2(80, 0)))
    {
        ExecuteBlueprint();
    }
    ImGui::Spring(0.0f);
    if (ImGui::Button("Copy Log", ImVec2(80, 0)))
    {
        if (!m_ExecutionLog.empty())
        {
            std::string allText;
            for (const auto& line : m_ExecutionLog)
            {
                allText += line;
                allText += '\n';
            }
            ImGui::SetClipboardText(allText.c_str());
        }
    }
    ImGui::Spring(0.0f);
    if (ImGui::Button("Clear Log", ImVec2(80, 0)))
    {
        m_ExecutionLog.clear();
        m_ExecutionLogText.clear();
        m_ExecutionLogDirty = false;
        m_LastExecutionStatus.clear();
    }
    ImGui::Spring();
    ImGui::EndHorizontal();

    if (!m_LastExecutionStatus.empty())
    {
        bool isOk = m_LastExecutionStatus.find("OK") == 0;
        ImGui::TextColored(isOk ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
            "Status: %s", m_LastExecutionStatus.c_str());
    }

    // 彩色日志输出区域
    float logHeight = ImGui::GetContentRegionAvail().y;
    if (logHeight < 60.0f) logHeight = 60.0f;

    ImGui::BeginChild("##ExecutionLog", ImVec2(paneWidth, logHeight), true,
        ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& line : m_ExecutionLog)
    {
        DrawColoredLogLine(line);
    }

    // 自动滚动到底部
    if (m_ExecutionLogDirty)
    {
        ImGui::SetScrollHereY(1.0f);
        m_ExecutionLogDirty = false;
    }

    ImGui::EndChild();
}
