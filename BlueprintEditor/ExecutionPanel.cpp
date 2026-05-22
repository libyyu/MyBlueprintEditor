// ExecutionPanel.cpp -- 蓝图执行 & 执行面板 UI
#include "BlueprintEditor.h"
#include "BuiltinHandlers.h"
#include "PathUtils.h"
#include <ctime>
#include <chrono>
#include <unordered_set>
#include <stdexcept>
#include <functional>
#ifndef __EMSCRIPTEN__
#include <filesystem>
#endif


// 返回 "HH:MM:SS.mmm" 格式的时间戳字符串
static std::string NowTimestamp()
{
    using namespace std::chrono;
    auto now   = system_clock::now();
    auto ms    = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = system_clock::to_time_t(now);
    struct tm tm_local{};
#ifdef _WIN32
    localtime_s(&tm_local, &t);
#else
    localtime_r(&t, &tm_local);
#endif
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d",
             tm_local.tm_hour, tm_local.tm_min, tm_local.tm_sec,
             static_cast<int>(ms.count()));
    return buf;
}

// ============================================================================
// InitRunnerForDoc — Runner 初始化公共序列
// 封装 ResetState + SetCallbacks + RegisterLibs + RegisterHandlers + BreakpointCb
// 供 ExecuteBlueprint 和 StepIn 共用，消除重复。
// ============================================================================

void BlueprintEditor::InitRunnerForDoc(BlueprintDocument* doc,
                                        const std::string& basePath,
                                        BlueprintDocument* capturedDoc)
{
    if (!capturedDoc) capturedDoc = doc;

    doc->persistentRunner.ResetState();
    doc->persistentRunner.m_withEditor = true;
    // 编辑器模式下启用引脚快照：每个节点执行后记录所有引脚值，
    // 供 Watch 面板和引脚 tooltip 在执行完成后仍能展示上次的值
    doc->persistentRunner.SetSnapshotEnabled(true);

    doc->persistentRunner.SetLogCallback([capturedDoc](::NodeEditor::Runtime::LogLevel /*level*/, const std::string& msg) {
        capturedDoc->executionLog.push_back(msg);
        capturedDoc->executionLogDirty = true;
    });
    doc->persistentRunner.SetPrintCallback([capturedDoc](::NodeEditor::Runtime::LogLevel /*level*/, const std::string& msg) {
        capturedDoc->executionLog.push_back(msg);
        capturedDoc->executionLogDirty = true;
    });

    // RegisterBuiltinHandlers（含 basePath）
    std::string bp = basePath;
    if (bp.empty()) {
        bp = BpPath::ParentDir(doc->filePath);
        if (bp.empty()) bp = ".";
    }

    ::NodeEditor::Runtime::RegisterBuiltinHandlers(doc->persistentRunner, bp, &m_HandlerRegistry);

    // 扩展脚本集成：让 persistentRunner 拥有自己的脚本引擎绑定，重新加载已加载的全部扩展脚本。
    // 这样脚本 handler 内的 Blueprint.AcquireAsync/ReleaseAsync 打到 persistentRunner，
    // HasPendingWork() 才能感知脚本层的异步活动，编辑器 Tick 循环持续驱动。
    // 无脚本后端时 GetLoadedExtensionScripts() 返回空集合，下面的循环为空操作。
    {
        // 同步搜索路径（把扩展脚本的父目录加到 persistentRunner）
#ifndef __EMSCRIPTEN__
        {
            namespace fs = std::filesystem;
            for (const auto& f : m_extensionRunner.GetLoadedExtensionScripts())
            {
                auto parent = fs::path(f).parent_path().string();
                if (!parent.empty())
                    doc->persistentRunner.AddScriptSearchPath(parent);
            }
        }
#endif

        // 重新加载所有扩展脚本到 persistentRunner（脚本引擎会路由到 persistentRunner）
        for (const auto& f : m_extensionRunner.GetLoadedExtensionScripts())
        {
            if (!doc->persistentRunner.LoadExtensionScript(f))
            {
                capturedDoc->executionLog.push_back(
                    "[WARN] Script: " + f + " — " + doc->persistentRunner.GetLastError());
                capturedDoc->executionLogDirty = true;
            }
        }

        // 节点定义和 handler 已在全局 SharedRegistry 中（HandlerRegistry / NodeDefRegistry），
        // persistentRunner 自动可见，无需手工同步
    }

    if (m_DefaultHandler)
        doc->persistentRunner.SetDefaultHandler(m_DefaultHandler);

    // 断点回调
    doc->persistentRunner.SetNodePreExecuteCallback([capturedDoc](::NodeEditor::Runtime::NodeId nid) -> bool {
        return capturedDoc->breakpoints.count(static_cast<uint64_t>(nid)) > 0;
    });
}

// ============================================================================
// 执行蓝图
// ============================================================================

void BlueprintEditor::ExecuteBlueprint()
{
    ActiveDoc()->executionLog.clear();
    ActiveDoc()->flowLinks.clear();
    ActiveDoc()->isExecuting = true;

    std::string ts = NowTimestamp();
    ActiveDoc()->executionLog.push_back("========================================");
    ActiveDoc()->executionLog.push_back("  [" + ts + "] Blueprint Execution Started");
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

    // 2. 使用持久 Runner
    // 捕获文档指针快照（防止回调期间标签页切换写入错误文档）
    BlueprintDocument* capturedDoc = ActiveDoc();

    InitRunnerForDoc(ActiveDoc(), /*basePath=*/"", capturedDoc);

    // 注入工程库函数（FuncLib.* 节点需要，InitRunnerForDoc 不扫库）
    {
        ::NodeEditor::Runtime::JsonBlueprintExporter exporter;
        auto tryRegisterLibDir = [&](const std::string& dirPath) {
#ifndef __EMSCRIPTEN__
            std::error_code ec;
            if (!std::filesystem::is_directory(dirPath, ec)) return;
            for (const auto& entry : std::filesystem::directory_iterator(dirPath, ec))
            {
                if (ec) break;
                if (!entry.is_regular_file()) continue;
                auto p = entry.path();
                if (p.extension() != ".bjson") continue;
                auto r = exporter.importRuntimeFromFile(p.string());
                if (!r.success) continue;
                if (r.data.metadata.blueprintClass != ::NodeEditor::Runtime::BlueprintClass::FunctionLibrary) continue;
                ActiveDoc()->persistentRunner.RegisterExternalFunctions(r.data.functions);
                ActiveDoc()->persistentRunner.RegisterExternalLibrary(r.data);
            }
#endif
        };
        if (m_Project.IsOpen())
        {
            for (const auto& libEntry : m_Project.libraries)
            {
                std::string absPath = m_Project.AbsPath(libEntry.relativePath);
                if (absPath.empty()) continue;
                auto r = exporter.importRuntimeFromFile(absPath);
                if (!r.success) continue;
                if (r.data.metadata.blueprintClass != ::NodeEditor::Runtime::BlueprintClass::FunctionLibrary) continue;
                ActiveDoc()->persistentRunner.RegisterExternalFunctions(r.data.functions);
                ActiveDoc()->persistentRunner.RegisterExternalLibrary(r.data);
            }
        }
        else
        {
            std::string dir = BpPath::ParentDir(ActiveDoc()->filePath);
            if (dir.empty()) dir = ".";
            tryRegisterLibDir(dir);
        }
    }

    if (!ActiveDoc()->persistentRunner.Load(bp))
    {
        ActiveDoc()->executionLog.push_back("[ERROR] Failed to load: " + ActiveDoc()->persistentRunner.GetLastError());
        ActiveDoc()->lastExecutionStatus = "Load Failed";
        ActiveDoc()->isExecuting = false;
        return;
    }

    // 4. 执行（用 try-catch 防止 bad_function_call / bad_variant_access 等异常崩溃）
    auto startTime = std::chrono::high_resolution_clock::now();
    ::NodeEditor::Runtime::ExecutionResult result;
    try
    {
        result = ActiveDoc()->persistentRunner.Execute();
    }
    catch (const std::bad_function_call& e)
    {
        capturedDoc->executionLog.push_back("[ERROR] Execution exception (bad_function_call): " + std::string(e.what()));
        capturedDoc->executionLog.push_back("  Hint: a callback/handler was called while null.");
        capturedDoc->lastExecutionStatus = "FAILED: bad_function_call";
        capturedDoc->isExecuting = false;
        capturedDoc->executionLogDirty = true;
        return;
    }
    catch (const std::bad_variant_access& e)
    {
        capturedDoc->executionLog.push_back("[ERROR] Execution exception (bad_variant_access): " + std::string(e.what()));
        capturedDoc->executionLog.push_back("  Hint: Variant type mismatch during execution.");
        capturedDoc->lastExecutionStatus = "FAILED: bad_variant_access";
        capturedDoc->isExecuting = false;
        capturedDoc->executionLogDirty = true;
        return;
    }
    catch (const std::exception& e)
    {
        capturedDoc->executionLog.push_back("[ERROR] Execution exception: " + std::string(e.what()));
        capturedDoc->lastExecutionStatus = "FAILED: exception";
        capturedDoc->isExecuting = false;
        capturedDoc->executionLogDirty = true;
        return;
    }

    // ── 触发 OnBeginPlay 事件（若蓝图中存在该事件源节点）──────────────────────
    // Execute() 完成后（数据流节点已求值），再派发 BeginPlay 事件
    // 这与 UE4 的行为一致：构造（CDO 赋值）先于 BeginPlay
    // 注意：若 Execute() 已因断点而 Paused，不能再调用 DispatchEvent，
    //       否则 DispatchEvent 会将 RunState 强制改回 Idle，丢失暂停状态。
    bool executeHitBreakpoint = (result.errorMessage == "Paused at breakpoint");
    if (!executeHitBreakpoint)
    {
        try
        {
            auto beginPlayResult = ActiveDoc()->persistentRunner.DispatchEvent("OnBeginPlay");
            if (beginPlayResult.success && !beginPlayResult.executedNodeIds.empty())
            {
                // 将 BeginPlay 链执行的节点 ID 合并到 result 中（用于编辑器高亮）
                for (auto nid : beginPlayResult.executedNodeIds)
                    result.executedNodeIds.push_back(nid);
                result.nodesExecuted += beginPlayResult.nodesExecuted;
            }
            // 检查 DispatchEvent 期间是否命中断点
            if (ActiveDoc()->persistentRunner.IsPaused())
            {
                result.success = true;
                result.errorMessage = "Paused at breakpoint";
            }
            else if (!beginPlayResult.success && !beginPlayResult.errorMessage.empty())
            {
                capturedDoc->executionLog.push_back("[WARN] OnBeginPlay: " + beginPlayResult.errorMessage);
            }
        }
        catch (const std::exception& e)
        {
            capturedDoc->executionLog.push_back("[ERROR] OnBeginPlay exception: " + std::string(e.what()));
        }
    }

    ActiveDoc()->lastExecutionResult = result;  // 保存执行结果供面板展示
    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    ActiveDoc()->executionLog.push_back("");
    ActiveDoc()->executionLog.push_back("========================================");
    std::string ts2 = NowTimestamp();
    bool isPausedAtBreakpoint = (result.errorMessage == "Paused at breakpoint");
    if (isPausedAtBreakpoint)
    {
        ActiveDoc()->executionLog.push_back("  [" + ts2 + "] Paused at breakpoint");
        char statusBuf[128];
        snprintf(statusBuf, sizeof(statusBuf), "Paused (%d nodes, %.2fms)",
                 result.nodesExecuted, elapsed);
        ActiveDoc()->lastExecutionStatus = statusBuf;
    }
    else if (result.success)
    {
        ActiveDoc()->executionLog.push_back("  [" + ts2 + "] Completed Successfully!");
        char statusBuf[128];
        snprintf(statusBuf, sizeof(statusBuf), "OK (%d nodes, %.2fms)",
                 result.nodesExecuted, elapsed);
        ActiveDoc()->lastExecutionStatus = statusBuf;
    }
    else
    {
        ActiveDoc()->executionLog.push_back("  [" + ts2 + "] FAILED: " + result.errorMessage);
        ActiveDoc()->lastExecutionStatus = "FAILED: " + result.errorMessage;
    }
    ActiveDoc()->executionLog.push_back("  Nodes executed: " + std::to_string(result.nodesExecuted));
    char elapsedBuf[32];
    snprintf(elapsedBuf, sizeof(elapsedBuf), "%.3f ms", elapsed);
    ActiveDoc()->executionLog.push_back("  Elapsed: " + std::string(elapsedBuf));
    ActiveDoc()->executionLog.push_back("========================================");

    // 5. 执行可视化 —— 高亮已执行的节点（按执行顺序错开起始时间，营造逐节点播放感）
    ActiveDoc()->executedNodeHighlight.clear();
    std::unordered_set<uint64_t> executedNodeSet;
    bool execFailed = !result.success && !result.errorMessage.empty() && result.errorMessage != "Paused at breakpoint";
    size_t n = result.executedNodeIds.size();
    for (size_t i = 0; i < n; ++i)
    {
        uint64_t runtimeId = result.executedNodeIds[i];
        bool isFailNode = execFailed && (i + 1 == n);
        BlueprintDocument::NodeHighlight hl;
        // 按顺序错开：最后执行的节点最先开始显示（timeLeft 最大），
        // 最先执行的节点稍后消失，形成"顺序流动"视觉效果
        // 每个节点之间错开 0.08 秒，总持续 3 秒
        float stagger = static_cast<float>(i) * 0.08f;
        hl.timeLeft = 3.0f + stagger;  // 越靠后执行的节点显示时间越长
        hl.color    = isFailNode ? ImColor(255, 60, 60) : ImColor(50, 220, 100);
        ActiveDoc()->executedNodeHighlight[runtimeId] = hl;
        executedNodeSet.insert(runtimeId);
    }

    // 只对连接已执行节点的链接触发 Flow 动画
    for (const auto& link : ActiveDoc()->links)
    {
        auto* startPin = FindPin(link.StartPinID);
        auto* endPin   = FindPin(link.EndPinID);
        if (startPin && endPin && startPin->Node && endPin->Node)
        {
            uint64_t startNid = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(startPin->Node->ID.AsPointer()));
            uint64_t endNid   = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(endPin->Node->ID.AsPointer()));
            if (executedNodeSet.count(startNid) && executedNodeSet.count(endNid))
                ActiveDoc()->flowLinks.push_back(link.ID);
        }
    }

    // isExecuting 在 runner 运行、暂停（断点）或有 pending async（LLM/HTTP 等）时保持 true
    ActiveDoc()->isExecuting = ActiveDoc()->persistentRunner.IsRunning()
                             || ActiveDoc()->persistentRunner.IsPaused()
                             || ActiveDoc()->persistentRunner.HasPendingAsync();
    ActiveDoc()->executionLogDirty = true;
}

// ============================================================================
// 执行面板 UI
// ============================================================================

void BlueprintEditor::ShowExecutionPanel(float paneWidth)
{
    if (!ActiveDoc()) return;   // 无文档时跳过

    // ── 工具栏（按钮已移到浮动 DebugToolbar，此处只保留 Tab 内容）────────────
    auto& runner = ActiveDoc()->persistentRunner;
    bool isRunning = runner.IsRunning();
    bool isPaused  = runner.IsPaused();
    bool isStopped = runner.IsStopped();
    bool isIdle    = runner.IsIdle();

    // 每帧更新 isExecuting：runner 正在运行 / 暂停 / 有待完成的异步任务（LLM/HTTP）时保持 true
    ActiveDoc()->isExecuting = isRunning || isPaused || runner.HasPendingAsync();

    const auto& res = ActiveDoc()->lastExecutionResult;

    // ── Tab: Log | Nodes ─────────────────────────────────────────────────
    if (ImGui::BeginTabBar("##ExecTabs"))
    {
        // ── TabBar 右侧：隐藏底部面板按钮（与左侧面板显隐按钮对称）────────
        // 用 ImGuiTabItemFlags_Trailing 把按钮对齐到 TabBar 最右侧
        ImGui::SetNextItemWidth(0);
        if (ImGui::TabItemButton(ICON_FA_XMARK "##CloseExecPanel",
                                  ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
        {
            m_ShowExecutionWindow = false;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Hide bottom panel\n(View menu to show again)");
        // ── Tab: Log ────────────────────────────────────────────────────
        if (ImGui::BeginTabItem(ICON_FA_TERMINAL " Log"))
        {
            auto* doc = ActiveDoc();


            // ── 工具栏第一行：关键字过滤框 + Copy + Clear ───────────────────────
            float copyW  = ImGui::CalcTextSize(ICON_FA_COPY  " Copy").x  + ImGui::GetStyle().FramePadding.x * 2.0f + 4.0f;
            float clearW = ImGui::CalcTextSize(ICON_FA_ERASER " Clear").x + ImGui::GetStyle().FramePadding.x * 2.0f + 4.0f;
            float filterW = paneWidth - copyW - clearW - ImGui::GetStyle().ItemSpacing.x * 2.0f - 4.0f;
            if (filterW < 80.0f) filterW = 80.0f;

            ImGui::SetNextItemWidth(filterW);
            ImGui::InputTextWithHint("##LogFilter",
                ICON_FA_MAGNIFYING_GLASS " Filter...",
                doc->execLogFilter, sizeof(doc->execLogFilter));

            ImGui::SameLine(0, 4);
            if (ImGui::Button(ICON_FA_COPY " Copy##logcopy"))
            {
                // 复制过滤后的全部日志
                std::string allText;
                std::string flt(doc->execLogFilter);
                for (const auto& line : doc->executionLog)
                {
                    if (!flt.empty() && line.find(flt) == std::string::npos)
                        continue;
                    allText += line;
                    allText += '\n';
                }
                ImGui::SetClipboardText(allText.c_str());
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Copy all filtered lines to clipboard");

            ImGui::SameLine(0, 4);
            if (ImGui::Button(ICON_FA_ERASER " Clear##logclear"))
            {
                doc->executionLog.clear();
                doc->executionLogText.clear();
                doc->execLogCachedFilter.clear();
                doc->executionLogDirty = false;
                doc->logEditorSyncedCount = 0;
                doc->lastExecutionStatus.clear();
                doc->lastExecutionResult = RTExecutionResult{};
            }

            // ── 工具栏第二行：日志级别过滤切换 ────────────────────────────────
            {
                // Error 按钮（红色切换）
                ImVec4 errColor  = doc->logShowErrors   ? ImVec4(0.90f,0.25f,0.25f,1.0f) : ImVec4(0.35f,0.18f,0.18f,0.7f);
                ImVec4 warnColor = doc->logShowWarnings ? ImVec4(0.85f,0.60f,0.10f,1.0f) : ImVec4(0.35f,0.28f,0.08f,0.7f);
                ImVec4 infoColor = doc->logShowInfo     ? ImVec4(0.30f,0.60f,0.90f,1.0f) : ImVec4(0.10f,0.22f,0.35f,0.7f);

                ImGui::PushStyleColor(ImGuiCol_Button,        errColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(errColor.x+0.1f, errColor.y+0.05f, errColor.z+0.05f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  errColor);
                if (ImGui::SmallButton("E##logE")) doc->logShowErrors = !doc->logShowErrors;
                ImGui::PopStyleColor(3);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle: show ERROR lines");

                ImGui::SameLine(0, 3);
                ImGui::PushStyleColor(ImGuiCol_Button,        warnColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(warnColor.x+0.05f, warnColor.y+0.1f, warnColor.z, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  warnColor);
                if (ImGui::SmallButton("W##logW")) doc->logShowWarnings = !doc->logShowWarnings;
                ImGui::PopStyleColor(3);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle: show WARNING lines");

                ImGui::SameLine(0, 3);
                ImGui::PushStyleColor(ImGuiCol_Button,        infoColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(infoColor.x+0.05f, infoColor.y+0.1f, infoColor.z+0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  infoColor);
                if (ImGui::SmallButton("I##logI")) doc->logShowInfo = !doc->logShowInfo;
                ImGui::PopStyleColor(3);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle: show INFO / general lines");

                ImGui::SameLine(0, 8);
                if (!doc->lastExecutionStatus.empty())
                {
                    bool ok = doc->lastExecutionStatus.find("OK") == 0 ||
                              doc->lastExecutionStatus.find("Paused") == 0;
                    ImGui::TextColored(
                        ok ? ImVec4(0.4f,0.9f,0.4f,1.0f) : ImVec4(0.9f,0.35f,0.35f,1.0f),
                        "%s", doc->lastExecutionStatus.c_str());
                }
            }

            // ── 日志颜色分类（按行关键词） ─────────────────────────────────────
            auto getLogLineColor = [](const std::string& line) -> ImVec4 {
                auto has = [&](const char* s){ return line.find(s) != std::string::npos; };
                if (has("[ERROR]") || has("FAILED") || has("ABORTED"))
                    return ImVec4(0.95f, 0.32f, 0.32f, 1.0f); // 红
                if (has("[WARN]"))
                    return ImVec4(0.95f, 0.72f, 0.28f, 1.0f); // 橙
                if (has("[INFO]") || has("[Timer:"))
                    return ImVec4(0.38f, 0.68f, 0.95f, 1.0f); // 蓝
                if (has("Completed Successfully") || has("success"))
                    return ImVec4(0.35f, 0.88f, 0.42f, 1.0f); // 绿
                if (has("========"))
                    return ImVec4(0.50f, 0.52f, 0.56f, 0.90f); // 暗灰
                if (has("[Breakpoint]") || has("[Step]"))
                    return ImVec4(1.00f, 0.80f, 0.20f, 1.0f); // 黄
                return ImVec4(0.82f, 0.84f, 0.90f, 1.0f);     // 默认浅灰白
            };

            // ── 日志滚动列表（BeginChild，使用主 UI 字体，彻底避免字体叠字问题）──
            std::string filter(doc->execLogFilter);
            bool filterChanged2 = (doc->execLogCachedFilter != filter);
            if (filterChanged2)
                doc->execLogCachedFilter = filter;
            doc->executionLogDirty = false;

            float logH = ImGui::GetContentRegionAvail().y - 4.0f;
            if (logH < 40.0f) logH = 40.0f;

            // 级别过滤判断
            auto passLevelFilter = [&doc](const std::string& line) -> bool {
                auto has = [&](const char* s){ return line.find(s) != std::string::npos; };
                bool isError   = has("[ERROR]") || has("FAILED") || has("ABORTED");
                bool isWarning = !isError && has("[WARN]");
                bool isInfo    = !isError && !isWarning;
                if (isError   && !doc->logShowErrors)   return false;
                if (isWarning && !doc->logShowWarnings) return false;
                if (isInfo    && !doc->logShowInfo)     return false;
                return true;
            };

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.082f, 0.086f, 0.102f, 1.0f));
            bool scrollToBottom = false;
            if (ImGui::BeginChild("##ExecLogChild", ImVec2(paneWidth, logH), ImGuiChildFlags_Borders))
            {
                const auto& logLines = doc->executionLog;
                size_t prevCount = doc->logEditorSyncedCount;
                bool hasNewLines = logLines.size() > prevCount;

                for (size_t i = 0; i < logLines.size(); ++i)
                {
                    const auto& line = logLines[i];
                    if (!filter.empty() && line.find(filter) == std::string::npos)
                        continue;
                    if (!passLevelFilter(line))
                        continue;
                    ImVec4 col = getLogLineColor(line);
                    ImGui::TextColored(col, "%s", line.c_str());
                }

                // 有新内容时自动滚到底部
                if (hasNewLines)
                {
                    ImGui::SetScrollHereY(1.0f);
                    doc->logEditorSyncedCount = logLines.size();
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::EndTabItem();
        }

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
                        BlueprintDocument::NodeHighlight hl;
                        hl.timeLeft = 1.5f;
                        hl.color    = ImColor(50, 220, 100);
                        ActiveDoc()->executedNodeHighlight[rid] = hl;
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
        case RTPinDataType::Set:     return "Set";
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
        case RTPinDataType::Set:     return ImVec4(0.7f, 0.4f, 0.9f, 1.0f);
        default:                     return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        }
    };

    float watchH = ImGui::GetContentRegionAvail().y - 4.0f;
    if (watchH < 40.0f) watchH = 40.0f;
    ImGui::BeginChild("##WatchContent", ImVec2(paneWidth, watchH), false);

    // ── Section 1: Blueprint Variables ───────────────────────────────────
    DrawSectionHeader(ICON_FA_LAYER_GROUP " Variables", paneWidth);
    // 变量数量标签（追加绘制到刚渲染的 header 上）
    {
        char countBuf[32];
        snprintf(countBuf, sizeof(countBuf), "(%d)", static_cast<int>(allVars.size()));
        auto* dl = ImGui::GetWindowDrawList();
        // header 刚在上面渲染，光标已前进，需要倒退一行
        ImVec2 headerPos = ImGui::GetCursorScreenPos();
        float sectionH   = ImGui::GetTextLineHeight() + 4.0f;
        float countW     = ImGui::CalcTextSize(countBuf).x;
        dl->AddText(
            ImVec2(headerPos.x + paneWidth - countW - 10.0f, headerPos.y - sectionH + 2.0f),
            IM_COL32(100, 140, 150, 180), countBuf);
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
    DrawSectionHeader(ICON_FA_CUBE " Selected Node Pins", paneWidth);

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
            auto runtimeNodeId = static_cast<::NodeEditor::Runtime::NodeId>(nodeIdVal);

            // 优先使用 PinSnapshot（执行完后保留，比 GetPinValue 更可靠）
            auto pinSnapshot = runner.GetNodePinSnapshot(runtimeNodeId);

            // 检查是否在上次执行中被执行过
            bool wasExecuted = !pinSnapshot.empty();
            if (!wasExecuted)
            {
                for (auto rid : res.executedNodeIds)
                    if (rid == nodeIdVal) { wasExecuted = true; break; }
            }

            if (!wasExecuted && res.nodesExecuted > 0)
            {
                ImGui::TextDisabled("    (not executed in last run)");
                continue;
            }
            if (res.nodesExecuted == 0 && pinSnapshot.empty())
            {
                ImGui::TextDisabled("    (no execution data)");
                continue;
            }

            bool hasAnyPinValue = false;

            // 辅助：从 snapshot 或 runner 获取 pin 值字符串
            auto getPinVal = [&](const Pin& pin) -> std::pair<bool, std::string> {
                // 1. 优先从 snapshot（按名字查）
                auto it = pinSnapshot.find(pin.Name);
                if (it != pinSnapshot.end())
                    return {true, it->second.value.asString()};
                // 2. 回退到实时 pinValues
                ::NodeEditor::Runtime::PinId pid = reinterpret_cast<uintptr_t>(pin.ID.AsPointer());
                auto val = runner.GetPinValue(pid);
                if (val.type != ::NodeEditor::Runtime::PinDataType::Unknown)
                    return {true, val.asString()};
                // 3. lastExecutionResult
                auto it2 = res.outputValues.find(pid);
                if (it2 != res.outputValues.end())
                    return {true, it2->second.asString()};
                return {false, {}};
            };

            // 输入引脚
            for (const auto& pin : node->Inputs)
            {
                if (pin.Type == PinType::Flow) continue;
                auto [hasVal, valStr] = getPinVal(pin);
                if (hasVal)
                {
                    hasAnyPinValue = true;
                    if (valStr.size() > 60) valStr = valStr.substr(0, 57) + "...";
                    ImGui::TextColored(ImVec4(0.50f, 0.55f, 0.65f, 1.0f), "    " ICON_FA_ARROW_RIGHT " %s:", pin.Name.c_str());
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "%s", valStr.c_str());
                }
            }

            // 输出引脚
            for (const auto& pin : node->Outputs)
            {
                if (pin.Type == PinType::Flow) continue;
                auto [hasVal, valStr] = getPinVal(pin);
                if (hasVal)
                {
                    hasAnyPinValue = true;
                    if (valStr.size() > 60) valStr = valStr.substr(0, 57) + "...";
                    ImGui::TextColored(ImVec4(0.50f, 0.55f, 0.65f, 1.0f), "    " ICON_FA_ARROW_LEFT " %s:", pin.Name.c_str());
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "%s", valStr.c_str());
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
            drawList->AddRectFilled(
                cursorPos,
                ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
                IM_COL32(30, 30, 38, 230), 0.0f);
            drawList->AddLine(
                ImVec2(cursorPos.x, cursorPos.y + sectionH - 1.0f),
                ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH - 1.0f),
                IM_COL32(0, 122, 204, 100));

            char titleBuf[64];
            snprintf(titleBuf, sizeof(titleBuf), ICON_FA_CIRCLE_CHECK " All Output Values (%d)",
                static_cast<int>(res.outputValues.size()));
            drawList->AddText(
                ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
                IM_COL32(200, 200, 210, 230), titleBuf);
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
