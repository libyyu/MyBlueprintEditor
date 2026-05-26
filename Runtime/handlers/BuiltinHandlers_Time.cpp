// Runtime/BuiltinHandlers_Time.cpp -- Time 节点处理器
#include "BuiltinHandlers_Time.h"
#include <cmath>
#include <ctime>

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Time(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner)
{
    handlers["GetTime"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        double seconds = static_cast<double>(FrameTimerManager::GetCurrentUnixTime());
        ctx.SetOutputValue("Seconds", Variant(seconds));
        return true;
    };

    handlers["DeltaTime"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        // 从 __DeltaTime 变量读取 Tick() 注入的真实帧时间
        double dt = ctx.GetVariable("__DeltaTime").asFloat();
        if (dt <= 0.0) dt = 0.016; // 未注入时降级为 ~60fps
        ctx.SetOutputValue("Seconds", Variant(dt));
        return true;
    };

    handlers["TimeSince"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        double timestamp = ctx.GetInputValue("Timestamp").asFloat();
        double now = static_cast<double>(FrameTimerManager::GetCurrentUnixTime());
        ctx.SetOutputValue("Elapsed", Variant(now - timestamp));
        return true;
    };

    handlers["BreakTime"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        double seconds = ctx.GetInputValue("Seconds").asFloat();
        int totalSec = static_cast<int>(seconds);
        std::time_t ts = static_cast<std::time_t>(totalSec);
		// 转换为本地时间（或使用 gmtime 得 UTC）
		std::tm* tm = std::localtime(&ts);
		int year = tm->tm_year + 1900;
		int month = tm->tm_mon + 1;
		int day = tm->tm_mday;
		int wday = tm->tm_wday;   // 星期几
		int hour = tm->tm_hour;
		int minute = tm->tm_min;
		int second = tm->tm_sec;
        int ms = static_cast<int>((seconds - totalSec) * 1000);
		ctx.SetOutputValue("Year", Variant(year));
		ctx.SetOutputValue("Mon", Variant(month));
		ctx.SetOutputValue("Day", Variant(day));
		ctx.SetOutputValue("WDay", Variant(wday));
		ctx.SetOutputValue("Hour", Variant(hour));
		ctx.SetOutputValue("Min", Variant(minute));
		ctx.SetOutputValue("Sec", Variant(second));
		ctx.SetOutputValue("MSec", Variant(ms));
        return true;
    };

    handlers["FormatTime"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        double seconds = ctx.GetInputValue("Seconds").asFloat();
		std::string format = ctx.GetInputValue("Format").asString();
        // 转换为本地时间（或使用 gmtime 得 UTC）
		std::time_t ts = static_cast<std::time_t>(seconds);
		std::tm* tm = std::localtime(&ts);
        char buffer[80] = { 0x0 };
        strftime(buffer, sizeof(buffer), format.c_str(), tm);
        ctx.SetOutputValue("Formatted", Variant(std::string(buffer)));
        return true;
    };

    handlers["TimerInfo"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        int64_t handleVal = ctx.GetInputValue("TimerHandle").asInt();
        auto handle = static_cast<TimerHandle>(handleVal);

        bool isActive = false;
        bool isPaused = false;
        double elapsed = 0.0;
        double remaining = 0.0;

        if (handle != 0)
        {
            const auto& timers = runner->GetTimerManager().GetAllTimers();
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
