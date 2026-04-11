// Runtime/BuiltinHandlers_Time.cpp -- Time 节点处理器
#include "BuiltinHandlers_Time.h"
#include <cmath>

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Time(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner)
{
    handlers["GetTime"] = [](ExecutionContext& ctx) {
        double seconds = static_cast<double>(FrameTimerManager::GetCurrentUnixTime());
        ctx.SetOutputValue("Seconds", Variant(seconds));
        return true;
    };

    handlers["DeltaTime"] = [](ExecutionContext& ctx) {
        // 从 __DeltaTime 变量读取 Tick() 注入的真实帧时间
        double dt = ctx.GetVariable("__DeltaTime").asFloat();
        if (dt <= 0.0) dt = 0.016; // 未注入时降级为 ~60fps
        ctx.SetOutputValue("Seconds", Variant(dt));
        return true;
    };

    handlers["TimeSince"] = [](ExecutionContext& ctx) {
        double timestamp = ctx.GetInputValue("Timestamp").asFloat();
        double now = static_cast<double>(FrameTimerManager::GetCurrentUnixTime());
        ctx.SetOutputValue("Elapsed", Variant(now - timestamp));
        return true;
    };

    handlers["FormatTime"] = [](ExecutionContext& ctx) {
        double seconds = ctx.GetInputValue("Seconds").asFloat();
        int totalSec = static_cast<int>(seconds);
        int hours = totalSec / 3600;
        int mins = (totalSec % 3600) / 60;
        int secs = totalSec % 60;
        int ms = static_cast<int>((seconds - totalSec) * 1000);

        char buf[64];
        if (hours > 0)
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d", hours, mins, secs, ms);
        else
            snprintf(buf, sizeof(buf), "%02d:%02d.%03d", mins, secs, ms);
        ctx.SetOutputValue("Formatted", Variant(std::string(buf)));
        return true;
    };

    handlers["TimerInfo"] = [&runner](ExecutionContext& ctx) {
        int64_t handleVal = ctx.GetInputValue("TimerHandle").asInt();
        auto handle = static_cast<TimerHandle>(handleVal);

        bool isActive = false;
        bool isPaused = false;
        double elapsed = 0.0;
        double remaining = 0.0;

        if (handle != 0)
        {
            const auto& timers = runner.GetTimerManager().GetAllTimers();
            for (const auto& t : timers)
            {
                if (t.handle == handle && !t.pendingKill)
                {
                    isActive = true;
                    isPaused = t.paused;
                    elapsed = static_cast<double>(t.totalElapsed);
                    remaining = static_cast<double>(t.remaining);
                    if (remaining < 0.0) remaining = 0.0;
                    break;
                }
            }
        }

        ctx.SetOutputValue("IsActive", Variant(isActive));
        ctx.SetOutputValue("IsPaused", Variant(isPaused));
        ctx.SetOutputValue("Elapsed", Variant(elapsed));
        ctx.SetOutputValue("Remaining", Variant(remaining));
        return true;
    };
}

// ============================================================================
// Data 处理器
// ============================================================================

} // namespace Runtime
} // namespace NodeEditor
