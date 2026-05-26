// Runtime/handlers/BuiltinHandlers_GameExtra.cpp
// 游戏扩展节点 handler 实现
//
// 分类：
//   Game/Random   — 随机系统（带权随机、随机打乱、随机种子）
//   Game/Easing   — 缓动函数（30 种标准缓动曲线）
//   Game/Timer    — 游戏时钟（帧计数、游戏时间累计）
//   Game/Entity   — 实体系统（生命周期、Tag 系统）
//   Game/Inventory— 背包系统（添加/移除/查询物品）

#include "BuiltinHandlers_GameExtra.h"
#include "../BlueprintRunner.h"

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <algorithm>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 内部：线程安全随机数引擎（每个 runner 独立，通过变量 __rng_seed 初始化）
// ============================================================================
static std::mt19937& getOrCreateRng(BlueprintRunner& runner)
{
    // 用静态 thread_local 保证每线程独立
    thread_local std::mt19937 rng(std::random_device{}());
    // 检查 runner 是否设置了种子
    auto seedVar = runner.GetVariable("__rng_seed");
    if (seedVar.type == PinDataType::Integer && seedVar.asInt() != 0)
    {
        rng.seed(static_cast<uint32_t>(seedVar.asInt()));
        runner.SetVariable("__rng_seed", Variant(int64_t(0))); // 消费后清零
    }
    return rng;
}

// ============================================================================
// Easing 函数库（标准 Robert Penner 缓动，基于 t ∈ [0,1]）
// ============================================================================
namespace Easing {
    // 线性
    inline double linear(double t) { return t; }

    // Quad
    inline double easeInQuad(double t) { return t * t; }
    inline double easeOutQuad(double t) { return t * (2 - t); }
    inline double easeInOutQuad(double t) { return t < .5 ? 2*t*t : -1+(4-2*t)*t; }

    // Cubic
    inline double easeInCubic(double t) { return t * t * t; }
    inline double easeOutCubic(double t) { double u=1-t; return 1-u*u*u; }
    inline double easeInOutCubic(double t) { return t<.5 ? 4*t*t*t : (t-1)*(2*t-2)*(2*t-2)+1; }

    // Quart
    inline double easeInQuart(double t) { return t*t*t*t; }
    inline double easeOutQuart(double t) { double u=t-1; return 1-u*u*u*u; }
    inline double easeInOutQuart(double t) { double u=t-1; return t<.5 ? 8*t*t*t*t : 1-8*u*u*u*u; }

    // Quint
    inline double easeInQuint(double t) { return t*t*t*t*t; }
    inline double easeOutQuint(double t) { double u=t-1; return 1+u*u*u*u*u; }
    inline double easeInOutQuint(double t) { double u=t-1; return t<.5 ? 16*t*t*t*t*t : 1+16*u*u*u*u*u; }

    // Sine
    inline double easeInSine(double t) { return 1-std::cos(t*M_PI/2); }
    inline double easeOutSine(double t) { return std::sin(t*M_PI/2); }
    inline double easeInOutSine(double t) { return -(std::cos(M_PI*t)-1)/2; }

    // Expo
    inline double easeInExpo(double t) { return t==0 ? 0 : std::pow(2,10*t-10); }
    inline double easeOutExpo(double t) { return t==1 ? 1 : 1-std::pow(2,-10*t); }
    inline double easeInOutExpo(double t)
    {
        if (t==0) return 0; if (t==1) return 1;
        return t<.5 ? std::pow(2,20*t-10)/2 : (2-std::pow(2,-20*t+10))/2;
    }

    // Circ
    inline double easeInCirc(double t) { return 1-std::sqrt(1-t*t); }
    inline double easeOutCirc(double t) { double u=t-1; return std::sqrt(1-u*u); }
    inline double easeInOutCirc(double t)
    {
        double u=t-1; return t<.5 ? (1-std::sqrt(1-4*t*t))/2 : (std::sqrt(1-(-2*t+2)*(-2*t+2))+1)/2;
    }

    // Back
    inline double easeInBack(double t) { double c=1.70158; return (c+1)*t*t*t-c*t*t; }
    inline double easeOutBack(double t) { double c=1.70158; double u=t-1; return 1+(c+1)*u*u*u+c*u*u; }
    inline double easeInOutBack(double t)
    {
        double c=1.70158*1.525; double u=t-1;
        return t<.5 ? (4*t*t*((c+1)*2*t-c))/2 : ((u*u*((c+1)*u+c))*2+1)/2; // simplified
    }

    // Elastic
    inline double easeInElastic(double t)
    {
        if(t==0||t==1) return t;
        return -std::pow(2,10*t-10)*std::sin((t*10-10.75)*2*M_PI/3);
    }
    inline double easeOutElastic(double t)
    {
        if(t==0||t==1) return t;
        return std::pow(2,-10*t)*std::sin((t*10-.75)*2*M_PI/3)+1;
    }
    inline double easeInOutElastic(double t)
    {
        if(t==0||t==1) return t;
        return t<.5 ? -(std::pow(2,20*t-10)*std::sin((20*t-11.125)*2*M_PI/4.5))/2
                    : (std::pow(2,-20*t+10)*std::sin((20*t-11.125)*2*M_PI/4.5))/2+1;
    }

    // Bounce
    inline double easeOutBounce(double t)
    {
        const double n1=7.5625, d1=2.75;
        if(t<1/d1) return n1*t*t;
        if(t<2/d1) { t-=1.5/d1; return n1*t*t+.75; }
        if(t<2.5/d1) { t-=2.25/d1; return n1*t*t+.9375; }
        t-=2.625/d1; return n1*t*t+.984375;
    }
    inline double easeInBounce(double t) { return 1-easeOutBounce(1-t); }
    inline double easeInOutBounce(double t) { return t<.5 ? (1-easeOutBounce(1-2*t))/2 : (1+easeOutBounce(2*t-1))/2; }

    // 统一分发
    double apply(const std::string& easeName, double t)
    {
        t = std::max(0.0, std::min(1.0, t)); // clamp [0,1]
        if (easeName == "Linear")          return linear(t);
        if (easeName == "InQuad")          return easeInQuad(t);
        if (easeName == "OutQuad")         return easeOutQuad(t);
        if (easeName == "InOutQuad")       return easeInOutQuad(t);
        if (easeName == "InCubic")         return easeInCubic(t);
        if (easeName == "OutCubic")        return easeOutCubic(t);
        if (easeName == "InOutCubic")      return easeInOutCubic(t);
        if (easeName == "InQuart")         return easeInQuart(t);
        if (easeName == "OutQuart")        return easeOutQuart(t);
        if (easeName == "InOutQuart")      return easeInOutQuart(t);
        if (easeName == "InQuint")         return easeInQuint(t);
        if (easeName == "OutQuint")        return easeOutQuint(t);
        if (easeName == "InOutQuint")      return easeInOutQuint(t);
        if (easeName == "InSine")          return easeInSine(t);
        if (easeName == "OutSine")         return easeOutSine(t);
        if (easeName == "InOutSine")       return easeInOutSine(t);
        if (easeName == "InExpo")          return easeInExpo(t);
        if (easeName == "OutExpo")         return easeOutExpo(t);
        if (easeName == "InOutExpo")       return easeInOutExpo(t);
        if (easeName == "InCirc")          return easeInCirc(t);
        if (easeName == "OutCirc")         return easeOutCirc(t);
        if (easeName == "InOutCirc")       return easeInOutCirc(t);
        if (easeName == "InBack")          return easeInBack(t);
        if (easeName == "OutBack")         return easeOutBack(t);
        if (easeName == "InOutBack")       return easeInOutBack(t);
        if (easeName == "InElastic")       return easeInElastic(t);
        if (easeName == "OutElastic")      return easeOutElastic(t);
        if (easeName == "InOutElastic")    return easeInOutElastic(t);
        if (easeName == "InBounce")        return easeInBounce(t);
        if (easeName == "OutBounce")       return easeOutBounce(t);
        if (easeName == "InOutBounce")     return easeInOutBounce(t);
        return linear(t);
    }
}

// ============================================================================
// 注册所有 GameExtra handler
// ============================================================================
void RegisterHandlers_GameExtra(
    std::unordered_map<std::string, NodeHandler>& handlers)
{
    // ========================================================================
    // Random.SetSeed — 设置随机种子（影响本 runner 后续所有 Random.* 操作）
    // ========================================================================
    handlers["Random.SetSeed"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        int64_t seed = ctx.GetInputValue("Seed").asInt();
        runner->SetVariable("__rng_seed", Variant(seed));
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // Random.Float — 随机浮点数 [Min, Max)
    // ========================================================================
    handlers["Random.Float"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        double mn = ctx.GetInputValue("Min").asFloat();
        double mx = ctx.GetInputValue("Max").asFloat();
        if (mn > mx) std::swap(mn, mx);
        auto& rng = getOrCreateRng(*runner);
        std::uniform_real_distribution<double> dist(mn, mx);
        ctx.SetOutputValue("Value", Variant(dist(rng)));
        return true;
    };

    // ========================================================================
    // Random.Int — 随机整数 [Min, Max]（含两端）
    // ========================================================================
    handlers["Random.Int"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        int64_t mn = ctx.GetInputValue("Min").asInt();
        int64_t mx = ctx.GetInputValue("Max").asInt();
        if (mn > mx) std::swap(mn, mx);
        auto& rng = getOrCreateRng(*runner);
        std::uniform_int_distribution<int64_t> dist(mn, mx);
        ctx.SetOutputValue("Value", Variant(dist(rng)));
        return true;
    };

    // ========================================================================
    // Random.Bool — 随机布尔值（Probability 为 true 的概率，默认 0.5）
    // ========================================================================
    handlers["Random.Bool"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        double prob = ctx.GetInputValue("Probability").asFloat();
        prob = std::max(0.0, std::min(1.0, prob));
        auto& rng = getOrCreateRng(*runner);
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        bool result = dist(rng) < prob;
        ctx.SetOutputValue("Value", Variant(result));
        return true;
    };

    // ========================================================================
    // Random.WeightedChoice — 按权重从字符串列表中随机选择
    // Items:   Array of String（候选列表）
    // Weights: Array of Float（权重，可为空则均匀分布）
    // 输出: ChosenItem(String), ChosenIndex(Integer)
    // ========================================================================
    handlers["Random.WeightedChoice"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        auto items   = ctx.GetInputValue("Items");
        auto weights = ctx.GetInputValue("Weights");
        auto& rng    = getOrCreateRng(*runner);

        size_t n = items.arraySize();
        if (n == 0) {
            ctx.SetOutputValue("ChosenItem",  Variant(std::string("")));
            ctx.SetOutputValue("ChosenIndex", Variant(int64_t(-1)));
            return true;
        }

        std::vector<double> w(n, 1.0);
        for (size_t i = 0; i < std::min(n, weights.arraySize()); ++i)
            w[i] = std::max(0.0, weights.arrayGet(i).asFloat());

        std::discrete_distribution<size_t> dist(w.begin(), w.end());
        size_t idx = dist(rng);
        ctx.SetOutputValue("ChosenItem",  Variant(items.arrayGet(idx).asString()));
        ctx.SetOutputValue("ChosenIndex", Variant(static_cast<int64_t>(idx)));
        return true;
    };

    // ========================================================================
    // Random.Shuffle — 随机打乱 Array，返回新的打乱后 Array
    // ========================================================================
    handlers["Random.Shuffle"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        auto arr = ctx.GetInputValue("Array");
        auto& rng = getOrCreateRng(*runner);

        std::vector<Variant> vec;
        for (size_t i = 0; i < arr.arraySize(); ++i)
            vec.push_back(arr.arrayGet(i));

        std::shuffle(vec.begin(), vec.end(), rng);

        Variant result;
        result.type = PinDataType::Array;
        result.arrayValue = std::move(vec);
        ctx.SetOutputValue("Result", result);
        return true;
    };

    // ========================================================================
    // Random.Vec2 — 随机二维向量（在矩形区域内 或 单位圆内）
    // Mode: "Rect"（矩形区域）/ "Circle"（单位圆，归一化）/ "CircleSurface"（圆周）
    // ========================================================================
    handlers["Random.Vec2"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string mode = ctx.GetInputValue("Mode").asString();
        if (mode.empty()) mode = "Rect";
        auto& rng = getOrCreateRng(*runner);
        std::uniform_real_distribution<double> dist01(0.0, 1.0);
        std::uniform_real_distribution<double> distAngle(0.0, 2*M_PI);

        double x = 0, y = 0;
        if (mode == "Circle")
        {
            // 在单位圆内均匀分布（半径平方根处理）
            double r = std::sqrt(dist01(rng));
            double a = distAngle(rng);
            x = r * std::cos(a);
            y = r * std::sin(a);
        }
        else if (mode == "CircleSurface")
        {
            double a = distAngle(rng);
            x = std::cos(a); y = std::sin(a);
        }
        else // Rect
        {
            double x0 = ctx.GetInputValue("MinX").asFloat();
            double y0 = ctx.GetInputValue("MinY").asFloat();
            double x1 = ctx.GetInputValue("MaxX").asFloat();
            double y1 = ctx.GetInputValue("MaxY").asFloat();
            std::uniform_real_distribution<double> dx(x0, x1), dy(y0, y1);
            x = dx(rng); y = dy(rng);
        }
        char buf[64];
        snprintf(buf, sizeof(buf), "%.6f,%.6f", x, y);
        ctx.SetOutputValue("Vec2",  Variant(std::string(buf)));
        ctx.SetOutputValue("X",     Variant(x));
        ctx.SetOutputValue("Y",     Variant(y));
        return true;
    };

    // ========================================================================
    // Random.PickFromArray — 随机从 Array 中取 Count 个不重复元素
    // ========================================================================
    handlers["Random.PickFromArray"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        auto arr  = ctx.GetInputValue("Array");
        int64_t count = ctx.GetInputValue("Count").asInt();
        bool withReplace = ctx.GetInputValue("WithReplacement").asBool();
        auto& rng = getOrCreateRng(*runner);

        size_t n = arr.arraySize();
        if (n == 0 || count <= 0) {
            Variant empty; empty.type = PinDataType::Array;
            ctx.SetOutputValue("Result", empty);
            return true;
        }

        std::vector<Variant> src;
        for (size_t i = 0; i < n; ++i) src.push_back(arr.arrayGet(i));

        Variant result; result.type = PinDataType::Array;
        if (withReplace)
        {
            std::uniform_int_distribution<size_t> dist(0, n-1);
            for (int64_t i = 0; i < count; ++i)
                result.arrayValue.push_back(src[dist(rng)]);
        }
        else
        {
            std::shuffle(src.begin(), src.end(), rng);
            size_t take = std::min((size_t)count, n);
            result.arrayValue.assign(src.begin(), src.begin() + take);
        }
        ctx.SetOutputValue("Result", result);
        return true;
    };

    // ========================================================================
    // Easing.Apply — 对 t ∈ [0,1] 应用指定缓动曲线，输出 EasedT
    // ========================================================================
    handlers["Easing.Apply"] = [](ExecutionContext& ctx) -> bool {
        std::string curve = ctx.GetInputValue("Curve").asString();
        double t = ctx.GetInputValue("T").asFloat();
        ctx.SetOutputValue("EasedT", Variant(Easing::apply(curve, t)));
        return true;
    };

    // ========================================================================
    // Easing.Lerp — 将 Easing.Apply 和线性插值合并：
    //   Result = From + (To - From) * Easing(T)
    // ========================================================================
    handlers["Easing.Lerp"] = [](ExecutionContext& ctx) -> bool {
        std::string curve = ctx.GetInputValue("Curve").asString();
        double from = ctx.GetInputValue("From").asFloat();
        double to   = ctx.GetInputValue("To").asFloat();
        double t    = ctx.GetInputValue("T").asFloat();
        double et   = Easing::apply(curve, t);
        ctx.SetOutputValue("Result", Variant(from + (to - from) * et));
        ctx.SetOutputValue("EasedT", Variant(et));
        return true;
    };

    // ========================================================================
    // GameTimer.Tick — 游戏时钟累计（每次触发加 DeltaTime）
    // Key:       计时器名称（支持多个独立计时器）
    // DeltaTime: 本帧增量时间（秒）
    // MaxTime:   最大累计时间（0 = 不限）
    // 输出: ElapsedTime, FrameCount, Normalized (0~1，MaxTime>0 时有效)
    // ========================================================================
    handlers["GameTimer.Tick"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string key = ctx.GetInputValue("Key").asString();
        if (key.empty()) key = "default";
        double dt       = ctx.GetInputValue("DeltaTime").asFloat();
        double maxTime  = ctx.GetInputValue("MaxTime").asFloat();

        std::string timeKey  = "__gtimer_t_" + key;
        std::string frameKey = "__gtimer_f_" + key;

        double elapsed = runner->GetVariable(timeKey).asFloat() + dt;
        int64_t frames = runner->GetVariable(frameKey).asInt() + 1;

        if (maxTime > 0) elapsed = std::min(elapsed, maxTime);

        runner->SetVariable(timeKey,  Variant(elapsed));
        runner->SetVariable(frameKey, Variant(frames));

        ctx.SetOutputValue("ElapsedTime", Variant(elapsed));
        ctx.SetOutputValue("FrameCount",  Variant(frames));
        ctx.SetOutputValue("Normalized",  Variant(maxTime > 0 ? elapsed / maxTime : 0.0));

        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // GameTimer.Reset — 重置指定计时器
    // ========================================================================
    handlers["GameTimer.Reset"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string key = ctx.GetInputValue("Key").asString();
        if (key.empty()) key = "default";
        runner->SetVariable("__gtimer_t_" + key, Variant(0.0));
        runner->SetVariable("__gtimer_f_" + key, Variant(int64_t(0)));
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // GameTimer.Get — 读取计时器当前值（不累加）
    // ========================================================================
    handlers["GameTimer.Get"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string key = ctx.GetInputValue("Key").asString();
        if (key.empty()) key = "default";
        double elapsed = runner->GetVariable("__gtimer_t_" + key).asFloat();
        int64_t frames = runner->GetVariable("__gtimer_f_" + key).asInt();
        ctx.SetOutputValue("ElapsedTime", Variant(elapsed));
        ctx.SetOutputValue("FrameCount",  Variant(frames));
        return true;
    };

    // ========================================================================
    // Entity.Create — 创建实体（分配唯一 EntityId，基于计数器自增）
    // Prefix: EntityId 前缀（如 "Enemy"、"Player"），默认 "entity"
    // 输出: EntityId(String)
    // ========================================================================
    handlers["Entity.Create"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string prefix = ctx.GetInputValue("Prefix").asString();
        if (prefix.empty()) prefix = "entity";
        int64_t counter = runner->GetVariable("__entity_counter").asInt() + 1;
        runner->SetVariable("__entity_counter", Variant(counter));
        std::string eid = prefix + "_" + std::to_string(counter);
        runner->SetVariable("__entity_alive_" + eid, Variant(true));
        ctx.SetOutputValue("EntityId", Variant(eid));
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // Entity.Destroy — 销毁实体（标记 alive=false，清理组件）
    // ========================================================================
    handlers["Entity.Destroy"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string eid = ctx.GetInputValue("EntityId").asString();
        runner->SetVariable("__entity_alive_" + eid, Variant(false));
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // Entity.IsAlive — 检查实体是否存活
    // ========================================================================
    handlers["Entity.IsAlive"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string eid = ctx.GetInputValue("EntityId").asString();
        bool alive = runner->GetVariable("__entity_alive_" + eid).asBool();
        ctx.SetOutputValue("Alive", Variant(alive));
        return true;
    };

    // ========================================================================
    // Tag.Add — 给实体添加标签（存为 Array，自动去重）
    // ========================================================================
    handlers["Tag.Add"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string eid = ctx.GetInputValue("EntityId").asString();
        std::string tag = ctx.GetInputValue("Tag").asString();
        std::string key = "__tags_" + eid;
        auto tags = runner->GetVariable(key);
        if (tags.type != PinDataType::Array) {
            tags.type = PinDataType::Array;
            tags.arrayValue.clear();
        }
        // 去重
        for (size_t i = 0; i < tags.arraySize(); ++i)
            if (tags.arrayGet(i).asString() == tag) {
                ctx.ActivateOutputFlow("exec");
                return true;
            }
        tags.arrayValue.push_back(Variant(tag));
        runner->SetVariable(key, tags);
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // Tag.Remove — 移除标签
    // ========================================================================
    handlers["Tag.Remove"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string eid = ctx.GetInputValue("EntityId").asString();
        std::string tag = ctx.GetInputValue("Tag").asString();
        std::string key = "__tags_" + eid;
        auto tags = runner->GetVariable(key);
        if (tags.type == PinDataType::Array) {
            auto& arr = tags.arrayValue;
            arr.erase(std::remove_if(arr.begin(), arr.end(),
                [&](const Variant& v){ return v.asString() == tag; }), arr.end());
            runner->SetVariable(key, tags);
        }
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // Tag.Has — 检查实体是否有指定标签
    // ========================================================================
    handlers["Tag.Has"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string eid = ctx.GetInputValue("EntityId").asString();
        std::string tag = ctx.GetInputValue("Tag").asString();
        auto tags = runner->GetVariable("__tags_" + eid);
        bool found = false;
        for (size_t i = 0; i < tags.arraySize(); ++i)
            if (tags.arrayGet(i).asString() == tag) { found = true; break; }
        ctx.SetOutputValue("Result", Variant(found));
        return true;
    };

    // ========================================================================
    // Tag.GetAll — 获取实体的所有标签
    // ========================================================================
    handlers["Tag.GetAll"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string eid = ctx.GetInputValue("EntityId").asString();
        auto tags = runner->GetVariable("__tags_" + eid);
        if (tags.type != PinDataType::Array) { tags.type = PinDataType::Array; }
        ctx.SetOutputValue("Tags",  tags);
        ctx.SetOutputValue("Count", Variant(static_cast<int64_t>(tags.arraySize())));
        return true;
    };

    // ========================================================================
    // Inventory.Add — 向背包添加物品
    // SlotId:   背包标识（支持多角色背包）
    // ItemId:   物品 ID
    // Quantity: 数量（默认 1）
    // MaxStack: 最大堆叠数（0 = 不限）
    // 输出: NewQuantity, Overflow（超出 MaxStack 的溢出数量）
    // ========================================================================
    handlers["Inventory.Add"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string slotId = ctx.GetInputValue("SlotId").asString();
        if (slotId.empty()) slotId = "default";
        std::string itemId = ctx.GetInputValue("ItemId").asString();
        int64_t qty     = std::max(int64_t(0), ctx.GetInputValue("Quantity").asInt());
        int64_t maxStack= ctx.GetInputValue("MaxStack").asInt();

        std::string key = "__inv_" + slotId + "_" + itemId;
        int64_t current = runner->GetVariable(key).asInt();
        int64_t newQty  = current + qty;
        int64_t overflow = 0;
        if (maxStack > 0 && newQty > maxStack) {
            overflow = newQty - maxStack;
            newQty = maxStack;
        }
        runner->SetVariable(key, Variant(newQty));

        // 维护 items 列表（用于 GetAll）
        std::string listKey = "__inv_list_" + slotId;
        auto list = runner->GetVariable(listKey);
        if (list.type != PinDataType::Array) list.type = PinDataType::Array;
        bool exists = false;
        for (size_t i = 0; i < list.arraySize(); ++i)
            if (list.arrayGet(i).asString() == itemId) { exists = true; break; }
        if (!exists) list.arrayValue.push_back(Variant(itemId));
        runner->SetVariable(listKey, list);

        ctx.SetOutputValue("NewQuantity", Variant(newQty));
        ctx.SetOutputValue("Overflow",    Variant(overflow));
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // Inventory.Remove — 从背包移除物品
    // 输出: NewQuantity, Removed（实际移除数量）
    // ========================================================================
    handlers["Inventory.Remove"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string slotId = ctx.GetInputValue("SlotId").asString();
        if (slotId.empty()) slotId = "default";
        std::string itemId = ctx.GetInputValue("ItemId").asString();
        int64_t qty = std::max(int64_t(0), ctx.GetInputValue("Quantity").asInt());

        std::string key = "__inv_" + slotId + "_" + itemId;
        int64_t current = runner->GetVariable(key).asInt();
        int64_t removed = std::min(current, qty);
        int64_t newQty  = current - removed;
        runner->SetVariable(key, Variant(newQty));

        ctx.SetOutputValue("NewQuantity", Variant(newQty));
        ctx.SetOutputValue("Removed",     Variant(removed));
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // Inventory.GetQuantity — 查询物品数量（Simple 节点）
    // ========================================================================
    handlers["Inventory.GetQuantity"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string slotId = ctx.GetInputValue("SlotId").asString();
        if (slotId.empty()) slotId = "default";
        std::string itemId = ctx.GetInputValue("ItemId").asString();
        int64_t qty = runner->GetVariable("__inv_" + slotId + "_" + itemId).asInt();
        ctx.SetOutputValue("Quantity", Variant(qty));
        ctx.SetOutputValue("HasItem",  Variant(qty > 0));
        return true;
    };

    // ========================================================================
    // Inventory.GetAll — 获取背包内所有物品列表
    // 输出: Items(Array of String), Quantities(Array of Integer), Count
    // ========================================================================
    handlers["Inventory.GetAll"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string slotId = ctx.GetInputValue("SlotId").asString();
        if (slotId.empty()) slotId = "default";

        auto list = runner->GetVariable("__inv_list_" + slotId);
        Variant itemsArr, qtysArr;
        itemsArr.type = PinDataType::Array;
        qtysArr.type  = PinDataType::Array;

        for (size_t i = 0; i < list.arraySize(); ++i) {
            std::string id = list.arrayGet(i).asString();
            int64_t qty = runner->GetVariable("__inv_" + slotId + "_" + id).asInt();
            if (qty > 0) {
                itemsArr.arrayValue.push_back(Variant(id));
                qtysArr.arrayValue.push_back(Variant(qty));
            }
        }

        ctx.SetOutputValue("Items",      itemsArr);
        ctx.SetOutputValue("Quantities", qtysArr);
        ctx.SetOutputValue("Count", Variant(static_cast<int64_t>(itemsArr.arraySize())));
        return true;
    };

    // ========================================================================
    // Inventory.Clear — 清空背包
    // ========================================================================
    handlers["Inventory.Clear"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string slotId = ctx.GetInputValue("SlotId").asString();
        if (slotId.empty()) slotId = "default";
        auto list = runner->GetVariable("__inv_list_" + slotId);
        for (size_t i = 0; i < list.arraySize(); ++i)
            runner->SetVariable("__inv_" + slotId + "_" + list.arrayGet(i).asString(), Variant(int64_t(0)));
        Variant empty; empty.type = PinDataType::Array;
        runner->SetVariable("__inv_list_" + slotId, empty);
        ctx.ActivateOutputFlow("exec");
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
