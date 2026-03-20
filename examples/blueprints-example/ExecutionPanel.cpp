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

    // 1. 构建 Runtime 数据
    RTBlueprintData bp = BuildRuntimeData();

    m_ExecutionLog.push_back("Nodes: " + std::to_string(bp.nodes.size()));
    m_ExecutionLog.push_back("Links: " + std::to_string(bp.links.size()));
    m_ExecutionLog.push_back("");

    // 2. 创建 Runner 并加载
    RTBlueprintRunner runner;
    runner.SetLogCallback([this](const std::string& msg) {
        m_ExecutionLog.push_back(msg);
    });

    if (!runner.Load(bp))
    {
        m_ExecutionLog.push_back("[ERROR] Failed to load: " + runner.GetLastError());
        m_LastExecutionStatus = "Load Failed";
        m_IsExecuting = false;
        return;
    }

    // 3. Register all handlers from the handler registry
    if (m_DefaultHandler)
        runner.SetDefaultHandler(m_DefaultHandler);
    runner.RegisterHandlers(m_HandlerRegistry);

    // 4. 执行
    auto startTime = std::chrono::high_resolution_clock::now();
    auto result = runner.Execute();
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

    // 5. 触发 Flow 动画 —— 让已执行的链接显示流动效果
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

    // 日志输出区域（只读，可选词拷贝）
    float logHeight = ImGui::GetContentRegionAvail().y;
    if (logHeight < 60.0f) logHeight = 60.0f;

    // 日志变化时重建合并文本
    if (m_ExecutionLogDirty)
    {
        m_ExecutionLogText.clear();
        for (const auto& line : m_ExecutionLog)
        {
            m_ExecutionLogText += line;
            m_ExecutionLogText += '\n';
        }
        m_ExecutionLogDirty = false;
    }

    ImGui::InputTextMultiline("##ExecutionLog",
        const_cast<char*>(m_ExecutionLogText.c_str()),
        m_ExecutionLogText.size() + 1,
        ImVec2(paneWidth, logHeight),
        ImGuiInputTextFlags_ReadOnly);
}
