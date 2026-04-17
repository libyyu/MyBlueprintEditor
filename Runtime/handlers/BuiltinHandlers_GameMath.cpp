// Runtime/handlers/BuiltinHandlers_GameMath.cpp
// 游戏数学和数值节点
//
// Vec2/Vec3（字符串 "x,y" / "x,y,z" 格式）：
//   Vec2.Make/Get    — 构造/分解 Vec2
//   Vec2.Add/Sub/Mul/Scale — 算术运算
//   Vec2.Length/Normalize/Dot/Distance/Lerp/Angle/Rotate
//   Vec3.Make/Get
//   Vec3.Add/Sub/Mul/Scale
//   Vec3.Length/Normalize/Dot/Cross/Distance/Lerp/Reflect
//
// 物理碰撞辅助（纯数学）：
//   Physics.PointInRect     — 点在矩形内
//   Physics.RectOverlap     — 两矩形相交
//   Physics.CircleOverlap   — 两圆相交
//   Physics.PointInCircle   — 点在圆内
//   Physics.Raycast2D       — 2D 射线 vs AABB
//
// 数值系统（Stat）：
//   Stat.Set         — 设置属性基础值
//   Stat.Get         — 获取属性当前值（基础值 + 修改器）
//   Stat.AddModifier — 添加修改器（add/mul/override）
//   Stat.RemoveModifier — 移除修改器
//   Stat.Reset       — 重置到基础值
//
// 冷却系统（Cooldown）：
//   Cooldown.Start   — 开始冷却
//   Cooldown.IsReady — 是否可用
//   Cooldown.GetRemaining — 剩余时间
//   Cooldown.Reset   — 立即重置（取消冷却）

#include "BuiltinHandlers_GameMath.h"
#include "../BlueprintRunner.h"
#include "../FrameTimerManager.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// Vec2/Vec3 内部工具
// ============================================================================
struct Vec2f { float x = 0, y = 0; };
struct Vec3f { float x = 0, y = 0, z = 0; };

static Vec2f parseVec2(const std::string& s)
{
    Vec2f v;
    // 格式: "x,y" 或 "x y"
    std::istringstream ss(s);
    char sep;
    if (!(ss >> v.x)) return v;
    if (ss.peek() == ',' || ss.peek() == ';') ss >> sep;
    ss >> v.y;
    return v;
}

static Vec3f parseVec3(const std::string& s)
{
    Vec3f v;
    std::istringstream ss(s);
    char sep;
    if (!(ss >> v.x)) return v;
    if (ss.peek() == ',' || ss.peek() == ';') ss >> sep;
    ss >> v.y;
    if (ss.peek() == ',' || ss.peek() == ';') ss >> sep;
    ss >> v.z;
    return v;
}

static std::string toStr2(float x, float y)
{
    std::ostringstream ss;
    ss << x << "," << y;
    return ss.str();
}

static std::string toStr3(float x, float y, float z)
{
    std::ostringstream ss;
    ss << x << "," << y << "," << z;
    return ss.str();
}

// ============================================================================
// Stat 全局存储（进程级）
// ============================================================================
struct StatModifier {
    std::string id;
    std::string type; // "add" | "mul" | "override"
    float value = 0.0f;
};

struct StatEntry {
    float base = 0.0f;
    float minVal = -1e30f;
    float maxVal =  1e30f;
    std::vector<StatModifier> modifiers;
};

static std::mutex s_statMutex;
static std::unordered_map<std::string, StatEntry> s_stats;

static std::string statKey(const std::string& entity, const std::string& name)
{
    return entity.empty() ? name : entity + "::" + name;
}

static float computeStat(const StatEntry& e)
{
    float val = e.base;
    float addSum = 0.0f;
    float mulProduct = 1.0f;
    bool hasOverride = false;
    float overrideVal = 0.0f;

    for (const auto& m : e.modifiers) {
        if (m.type == "add")      addSum += m.value;
        else if (m.type == "mul") mulProduct *= m.value;
        else if (m.type == "override") { hasOverride = true; overrideVal = m.value; }
    }

    if (hasOverride) val = overrideVal;
    else             val = (val + addSum) * mulProduct;

    // Clamp
    if (val < e.minVal) val = e.minVal;
    if (val > e.maxVal) val = e.maxVal;
    return val;
}

// ============================================================================
void RegisterHandlers_GameMath(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner)
{
    // ========================================================================
    // ── Vec2 ─────────────────────────────────────────────────────────────
    // ========================================================================

    handlers["Vec2.Make"] = [](ExecutionContext& ctx) -> bool {
        float x = static_cast<float>(ctx.GetInputValue("X").asFloat());
        float y = static_cast<float>(ctx.GetInputValue("Y").asFloat());
        ctx.SetOutputValue("Vec2",   Variant(toStr2(x, y)));
        ctx.SetOutputValue("X",      Variant(static_cast<double>(x)));
        ctx.SetOutputValue("Y",      Variant(static_cast<double>(y)));
        return true;
    };

    handlers["Vec2.Get"] = [](ExecutionContext& ctx) -> bool {
        Vec2f v = parseVec2(ctx.GetInputValue("Vec2").asString());
        ctx.SetOutputValue("X", Variant(static_cast<double>(v.x)));
        ctx.SetOutputValue("Y", Variant(static_cast<double>(v.y)));
        return true;
    };

    handlers["Vec2.Add"] = [](ExecutionContext& ctx) -> bool {
        Vec2f a = parseVec2(ctx.GetInputValue("A").asString());
        Vec2f b = parseVec2(ctx.GetInputValue("B").asString());
        ctx.SetOutputValue("Result", Variant(toStr2(a.x+b.x, a.y+b.y)));
        return true;
    };

    handlers["Vec2.Sub"] = [](ExecutionContext& ctx) -> bool {
        Vec2f a = parseVec2(ctx.GetInputValue("A").asString());
        Vec2f b = parseVec2(ctx.GetInputValue("B").asString());
        ctx.SetOutputValue("Result", Variant(toStr2(a.x-b.x, a.y-b.y)));
        return true;
    };

    handlers["Vec2.Scale"] = [](ExecutionContext& ctx) -> bool {
        Vec2f v = parseVec2(ctx.GetInputValue("Vec2").asString());
        float s = static_cast<float>(ctx.GetInputValue("Scale").asFloat());
        ctx.SetOutputValue("Result", Variant(toStr2(v.x*s, v.y*s)));
        return true;
    };

    handlers["Vec2.Length"] = [](ExecutionContext& ctx) -> bool {
        Vec2f v = parseVec2(ctx.GetInputValue("Vec2").asString());
        float len = std::sqrt(v.x*v.x + v.y*v.y);
        ctx.SetOutputValue("Length", Variant(static_cast<double>(len)));
        return true;
    };

    handlers["Vec2.Normalize"] = [](ExecutionContext& ctx) -> bool {
        Vec2f v = parseVec2(ctx.GetInputValue("Vec2").asString());
        float len = std::sqrt(v.x*v.x + v.y*v.y);
        if (len > 1e-6f) { v.x /= len; v.y /= len; }
        ctx.SetOutputValue("Result", Variant(toStr2(v.x, v.y)));
        ctx.SetOutputValue("Length", Variant(static_cast<double>(len)));
        return true;
    };

    handlers["Vec2.Dot"] = [](ExecutionContext& ctx) -> bool {
        Vec2f a = parseVec2(ctx.GetInputValue("A").asString());
        Vec2f b = parseVec2(ctx.GetInputValue("B").asString());
        ctx.SetOutputValue("Result", Variant(static_cast<double>(a.x*b.x + a.y*b.y)));
        return true;
    };

    handlers["Vec2.Distance"] = [](ExecutionContext& ctx) -> bool {
        Vec2f a = parseVec2(ctx.GetInputValue("A").asString());
        Vec2f b = parseVec2(ctx.GetInputValue("B").asString());
        float dx = a.x-b.x, dy = a.y-b.y;
        ctx.SetOutputValue("Distance", Variant(static_cast<double>(std::sqrt(dx*dx+dy*dy))));
        return true;
    };

    handlers["Vec2.Lerp"] = [](ExecutionContext& ctx) -> bool {
        Vec2f a = parseVec2(ctx.GetInputValue("A").asString());
        Vec2f b = parseVec2(ctx.GetInputValue("B").asString());
        float t = static_cast<float>(ctx.GetInputValue("T").asFloat());
        ctx.SetOutputValue("Result", Variant(toStr2(a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t)));
        return true;
    };

    handlers["Vec2.Angle"] = [](ExecutionContext& ctx) -> bool {
        Vec2f v = parseVec2(ctx.GetInputValue("Vec2").asString());
        float angle = std::atan2(v.y, v.x) * 180.0f / 3.14159265f;
        ctx.SetOutputValue("Degrees", Variant(static_cast<double>(angle)));
        return true;
    };

    handlers["Vec2.Rotate"] = [](ExecutionContext& ctx) -> bool {
        Vec2f v = parseVec2(ctx.GetInputValue("Vec2").asString());
        float deg = static_cast<float>(ctx.GetInputValue("Degrees").asFloat());
        float rad = deg * 3.14159265f / 180.0f;
        float cosA = std::cos(rad), sinA = std::sin(rad);
        ctx.SetOutputValue("Result", Variant(toStr2(
            v.x*cosA - v.y*sinA,
            v.x*sinA + v.y*cosA)));
        return true;
    };

    handlers["Vec2.Zero"] = [](ExecutionContext& ctx) -> bool {
        ctx.SetOutputValue("Result", Variant(toStr2(0,0)));
        return true;
    };

    handlers["Vec2.One"] = [](ExecutionContext& ctx) -> bool {
        ctx.SetOutputValue("Result", Variant(toStr2(1,1)));
        return true;
    };

    // ========================================================================
    // ── Vec3 ─────────────────────────────────────────────────────────────
    // ========================================================================

    handlers["Vec3.Make"] = [](ExecutionContext& ctx) -> bool {
        float x = static_cast<float>(ctx.GetInputValue("X").asFloat());
        float y = static_cast<float>(ctx.GetInputValue("Y").asFloat());
        float z = static_cast<float>(ctx.GetInputValue("Z").asFloat());
        ctx.SetOutputValue("Vec3", Variant(toStr3(x,y,z)));
        ctx.SetOutputValue("X",   Variant(static_cast<double>(x)));
        ctx.SetOutputValue("Y",   Variant(static_cast<double>(y)));
        ctx.SetOutputValue("Z",   Variant(static_cast<double>(z)));
        return true;
    };

    handlers["Vec3.Get"] = [](ExecutionContext& ctx) -> bool {
        Vec3f v = parseVec3(ctx.GetInputValue("Vec3").asString());
        ctx.SetOutputValue("X", Variant(static_cast<double>(v.x)));
        ctx.SetOutputValue("Y", Variant(static_cast<double>(v.y)));
        ctx.SetOutputValue("Z", Variant(static_cast<double>(v.z)));
        return true;
    };

    handlers["Vec3.Add"] = [](ExecutionContext& ctx) -> bool {
        Vec3f a = parseVec3(ctx.GetInputValue("A").asString());
        Vec3f b = parseVec3(ctx.GetInputValue("B").asString());
        ctx.SetOutputValue("Result", Variant(toStr3(a.x+b.x,a.y+b.y,a.z+b.z)));
        return true;
    };

    handlers["Vec3.Sub"] = [](ExecutionContext& ctx) -> bool {
        Vec3f a = parseVec3(ctx.GetInputValue("A").asString());
        Vec3f b = parseVec3(ctx.GetInputValue("B").asString());
        ctx.SetOutputValue("Result", Variant(toStr3(a.x-b.x,a.y-b.y,a.z-b.z)));
        return true;
    };

    handlers["Vec3.Scale"] = [](ExecutionContext& ctx) -> bool {
        Vec3f v = parseVec3(ctx.GetInputValue("Vec3").asString());
        float s = static_cast<float>(ctx.GetInputValue("Scale").asFloat());
        ctx.SetOutputValue("Result", Variant(toStr3(v.x*s,v.y*s,v.z*s)));
        return true;
    };

    handlers["Vec3.Length"] = [](ExecutionContext& ctx) -> bool {
        Vec3f v = parseVec3(ctx.GetInputValue("Vec3").asString());
        float len = std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
        ctx.SetOutputValue("Length", Variant(static_cast<double>(len)));
        return true;
    };

    handlers["Vec3.Normalize"] = [](ExecutionContext& ctx) -> bool {
        Vec3f v = parseVec3(ctx.GetInputValue("Vec3").asString());
        float len = std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
        if (len > 1e-6f) { v.x/=len; v.y/=len; v.z/=len; }
        ctx.SetOutputValue("Result", Variant(toStr3(v.x,v.y,v.z)));
        ctx.SetOutputValue("Length", Variant(static_cast<double>(len)));
        return true;
    };

    handlers["Vec3.Dot"] = [](ExecutionContext& ctx) -> bool {
        Vec3f a = parseVec3(ctx.GetInputValue("A").asString());
        Vec3f b = parseVec3(ctx.GetInputValue("B").asString());
        ctx.SetOutputValue("Result", Variant(static_cast<double>(a.x*b.x+a.y*b.y+a.z*b.z)));
        return true;
    };

    handlers["Vec3.Cross"] = [](ExecutionContext& ctx) -> bool {
        Vec3f a = parseVec3(ctx.GetInputValue("A").asString());
        Vec3f b = parseVec3(ctx.GetInputValue("B").asString());
        ctx.SetOutputValue("Result", Variant(toStr3(
            a.y*b.z-a.z*b.y,
            a.z*b.x-a.x*b.z,
            a.x*b.y-a.y*b.x)));
        return true;
    };

    handlers["Vec3.Distance"] = [](ExecutionContext& ctx) -> bool {
        Vec3f a = parseVec3(ctx.GetInputValue("A").asString());
        Vec3f b = parseVec3(ctx.GetInputValue("B").asString());
        float dx=a.x-b.x, dy=a.y-b.y, dz=a.z-b.z;
        ctx.SetOutputValue("Distance", Variant(static_cast<double>(std::sqrt(dx*dx+dy*dy+dz*dz))));
        return true;
    };

    handlers["Vec3.Lerp"] = [](ExecutionContext& ctx) -> bool {
        Vec3f a = parseVec3(ctx.GetInputValue("A").asString());
        Vec3f b = parseVec3(ctx.GetInputValue("B").asString());
        float t = static_cast<float>(ctx.GetInputValue("T").asFloat());
        ctx.SetOutputValue("Result", Variant(toStr3(a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t, a.z+(b.z-a.z)*t)));
        return true;
    };

    handlers["Vec3.Reflect"] = [](ExecutionContext& ctx) -> bool {
        Vec3f v = parseVec3(ctx.GetInputValue("Vec3").asString());
        Vec3f n = parseVec3(ctx.GetInputValue("Normal").asString());
        float dot = v.x*n.x+v.y*n.y+v.z*n.z;
        ctx.SetOutputValue("Result", Variant(toStr3(v.x-2*dot*n.x, v.y-2*dot*n.y, v.z-2*dot*n.z)));
        return true;
    };

    handlers["Vec3.Zero"] = [](ExecutionContext& ctx) -> bool {
        ctx.SetOutputValue("Result", Variant(toStr3(0,0,0)));
        return true;
    };

    handlers["Vec3.One"] = [](ExecutionContext& ctx) -> bool {
        ctx.SetOutputValue("Result", Variant(toStr3(1,1,1)));
        return true;
    };

    // ========================================================================
    // ── 物理碰撞辅助 ─────────────────────────────────────────────────────
    // ========================================================================

    // Physics.PointInRect
    // in:  PointX, PointY, RectX, RectY, RectW, RectH
    // out: Inside(Bool)
    handlers["Physics.PointInRect"] = [](ExecutionContext& ctx) -> bool {
        float px = static_cast<float>(ctx.GetInputValue("PointX").asFloat());
        float py = static_cast<float>(ctx.GetInputValue("PointY").asFloat());
        float rx = static_cast<float>(ctx.GetInputValue("RectX").asFloat());
        float ry = static_cast<float>(ctx.GetInputValue("RectY").asFloat());
        float rw = static_cast<float>(ctx.GetInputValue("RectW").asFloat());
        float rh = static_cast<float>(ctx.GetInputValue("RectH").asFloat());
        bool inside = (px >= rx && px <= rx+rw && py >= ry && py <= ry+rh);
        ctx.SetOutputValue("Inside", Variant(inside));
        return true;
    };

    // Physics.RectOverlap
    // in:  AX,AY,AW,AH, BX,BY,BW,BH
    // out: Overlap(Bool), OverlapX, OverlapY (重叠区域左上角), OverlapW, OverlapH
    handlers["Physics.RectOverlap"] = [](ExecutionContext& ctx) -> bool {
        float ax = static_cast<float>(ctx.GetInputValue("AX").asFloat());
        float ay = static_cast<float>(ctx.GetInputValue("AY").asFloat());
        float aw = static_cast<float>(ctx.GetInputValue("AW").asFloat());
        float ah = static_cast<float>(ctx.GetInputValue("AH").asFloat());
        float bx = static_cast<float>(ctx.GetInputValue("BX").asFloat());
        float by = static_cast<float>(ctx.GetInputValue("BY").asFloat());
        float bw = static_cast<float>(ctx.GetInputValue("BW").asFloat());
        float bh = static_cast<float>(ctx.GetInputValue("BH").asFloat());

        float ox1 = std::max(ax, bx), oy1 = std::max(ay, by);
        float ox2 = std::min(ax+aw, bx+bw), oy2 = std::min(ay+ah, by+bh);
        bool overlap = (ox2 > ox1 && oy2 > oy1);

        ctx.SetOutputValue("Overlap",  Variant(overlap));
        ctx.SetOutputValue("OverlapX", Variant(static_cast<double>(overlap ? ox1 : 0)));
        ctx.SetOutputValue("OverlapY", Variant(static_cast<double>(overlap ? oy1 : 0)));
        ctx.SetOutputValue("OverlapW", Variant(static_cast<double>(overlap ? ox2-ox1 : 0)));
        ctx.SetOutputValue("OverlapH", Variant(static_cast<double>(overlap ? oy2-oy1 : 0)));
        return true;
    };

    // Physics.CircleOverlap
    // in:  C1X,C1Y,R1, C2X,C2Y,R2
    // out: Overlap(Bool), Distance(Float), PenetrationDepth(Float)
    handlers["Physics.CircleOverlap"] = [](ExecutionContext& ctx) -> bool {
        float c1x = static_cast<float>(ctx.GetInputValue("C1X").asFloat());
        float c1y = static_cast<float>(ctx.GetInputValue("C1Y").asFloat());
        float r1  = static_cast<float>(ctx.GetInputValue("R1").asFloat());
        float c2x = static_cast<float>(ctx.GetInputValue("C2X").asFloat());
        float c2y = static_cast<float>(ctx.GetInputValue("C2Y").asFloat());
        float r2  = static_cast<float>(ctx.GetInputValue("R2").asFloat());

        float dx = c2x-c1x, dy = c2y-c1y;
        float dist = std::sqrt(dx*dx+dy*dy);
        float sumR = r1+r2;
        bool overlap = dist < sumR;

        ctx.SetOutputValue("Overlap",           Variant(overlap));
        ctx.SetOutputValue("Distance",          Variant(static_cast<double>(dist)));
        ctx.SetOutputValue("PenetrationDepth",  Variant(static_cast<double>(overlap ? sumR-dist : 0.0f)));
        return true;
    };

    // Physics.PointInCircle
    handlers["Physics.PointInCircle"] = [](ExecutionContext& ctx) -> bool {
        float px = static_cast<float>(ctx.GetInputValue("PointX").asFloat());
        float py = static_cast<float>(ctx.GetInputValue("PointY").asFloat());
        float cx = static_cast<float>(ctx.GetInputValue("CX").asFloat());
        float cy = static_cast<float>(ctx.GetInputValue("CY").asFloat());
        float r  = static_cast<float>(ctx.GetInputValue("Radius").asFloat());
        float dx = px-cx, dy = py-cy;
        bool inside = (dx*dx+dy*dy) <= r*r;
        ctx.SetOutputValue("Inside",   Variant(inside));
        ctx.SetOutputValue("Distance", Variant(static_cast<double>(std::sqrt(dx*dx+dy*dy))));
        return true;
    };

    // Physics.Raycast2D
    // 射线 vs AABB，射线起点+方向，返回是否命中和命中距离
    // in:  OriginX,OriginY, DirX,DirY, MaxDist, RectX,RectY,RectW,RectH
    // out: Hit(Bool), HitX,HitY, HitDist, NormalX,NormalY
    handlers["Physics.Raycast2D"] = [](ExecutionContext& ctx) -> bool {
        float ox = static_cast<float>(ctx.GetInputValue("OriginX").asFloat());
        float oy = static_cast<float>(ctx.GetInputValue("OriginY").asFloat());
        float dx = static_cast<float>(ctx.GetInputValue("DirX").asFloat());
        float dy = static_cast<float>(ctx.GetInputValue("DirY").asFloat());
        float maxDist = static_cast<float>(ctx.GetInputValue("MaxDist").asFloat());
        float rx = static_cast<float>(ctx.GetInputValue("RectX").asFloat());
        float ry = static_cast<float>(ctx.GetInputValue("RectY").asFloat());
        float rw = static_cast<float>(ctx.GetInputValue("RectW").asFloat());
        float rh = static_cast<float>(ctx.GetInputValue("RectH").asFloat());

        if (maxDist <= 0) maxDist = 1e6f;

        // Slab method AABB raycast
        float tmin = 0.0f, tmax = maxDist;
        float nx = 0, ny = 0;

        auto slab = [](float o, float d, float bmin, float bmax,
                        float& tmin, float& tmax, float& n, float ns) {
            if (std::abs(d) < 1e-7f) {
                if (o < bmin || o > bmax) { tmin = tmax + 1; } // miss
                return;
            }
            float t1 = (bmin - o) / d;
            float t2 = (bmax - o) / d;
            if (t1 > t2) { std::swap(t1, t2); ns = -ns; }
            if (t1 > tmin) { tmin = t1; n = ns; }
            if (t2 < tmax)   tmax = t2;
        };

        float tmpNx = -1.0f, tmpNy = 0.0f;
        slab(ox, dx, rx, rx+rw, tmin, tmax, nx, tmpNx);
        tmpNy = -1.0f; tmpNx = 0.0f;
        slab(oy, dy, ry, ry+rh, tmin, tmax, ny, tmpNy);

        bool hit = (tmin <= tmax && tmin >= 0 && tmin <= maxDist);
        ctx.SetOutputValue("Hit",     Variant(hit));
        ctx.SetOutputValue("HitX",    Variant(static_cast<double>(hit ? ox+dx*tmin : 0)));
        ctx.SetOutputValue("HitY",    Variant(static_cast<double>(hit ? oy+dy*tmin : 0)));
        ctx.SetOutputValue("HitDist", Variant(static_cast<double>(hit ? tmin : 0)));
        ctx.SetOutputValue("NormalX", Variant(static_cast<double>(hit ? nx : 0)));
        ctx.SetOutputValue("NormalY", Variant(static_cast<double>(hit ? ny : 0)));
        return true;
    };

    // ========================================================================
    // ── Stat 数值系统 ─────────────────────────────────────────────────────
    // 全局进程级；Entity 为空时用 name 作唯一键
    // ========================================================================

    // Stat.Set — 设置基础值和范围
    // in:  exec, Name(String), Base(Float), Min(Float,-∞), Max(Float,+∞), Entity(String)
    // out: exec
    handlers["Stat.Set"] = [](ExecutionContext& ctx) -> bool {
        std::string name   = ctx.GetInputValue("Name").asString();
        std::string entity = ctx.GetInputValue("Entity").asString();
        float base = static_cast<float>(ctx.GetInputValue("Base").asFloat());
        float minV = ctx.GetInputValue("Min").type != PinDataType::Unknown
                    ? static_cast<float>(ctx.GetInputValue("Min").asFloat()) : -1e30f;
        float maxV = ctx.GetInputValue("Max").type != PinDataType::Unknown
                    ? static_cast<float>(ctx.GetInputValue("Max").asFloat()) :  1e30f;

        std::lock_guard<std::mutex> lk(s_statMutex);
        auto& e = s_stats[statKey(entity, name)];
        e.base   = base;
        e.minVal = minV;
        e.maxVal = maxV;
        ctx.Log("[Stat.Set] " + statKey(entity, name) + " base=" + std::to_string(base));
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // Stat.Get — 获取当前值（含修改器）
    // in:  Name(String), Entity(String)
    // out: Value(Float), Base(Float), ModifierCount(Int)
    handlers["Stat.Get"] = [](ExecutionContext& ctx) -> bool {
        std::string name   = ctx.GetInputValue("Name").asString();
        std::string entity = ctx.GetInputValue("Entity").asString();

        std::lock_guard<std::mutex> lk(s_statMutex);
        auto it = s_stats.find(statKey(entity, name));
        if (it == s_stats.end()) {
            ctx.SetOutputValue("Value",         Variant(0.0));
            ctx.SetOutputValue("Base",          Variant(0.0));
            ctx.SetOutputValue("ModifierCount", Variant(int64_t(0)));
            return true;
        }
        const StatEntry& e = it->second;
        float val = computeStat(e);
        ctx.SetOutputValue("Value",         Variant(static_cast<double>(val)));
        ctx.SetOutputValue("Base",          Variant(static_cast<double>(e.base)));
        ctx.SetOutputValue("ModifierCount", Variant(static_cast<int64_t>(e.modifiers.size())));
        return true;
    };

    // Stat.AddModifier — 添加修改器
    // in:  exec, Name, Entity, ModId(String,唯一标识), Type(add/mul/override), Value(Float)
    // out: exec, NewValue(Float)
    handlers["Stat.AddModifier"] = [](ExecutionContext& ctx) -> bool {
        std::string name   = ctx.GetInputValue("Name").asString();
        std::string entity = ctx.GetInputValue("Entity").asString();
        std::string modId  = ctx.GetInputValue("ModId").asString();
        std::string type   = ctx.GetInputValue("Type").asString();
        float value = static_cast<float>(ctx.GetInputValue("Value").asFloat());
        if (type.empty()) type = "add";
        if (modId.empty()) modId = type + "_" + std::to_string(rand());

        std::lock_guard<std::mutex> lk(s_statMutex);
        auto& e = s_stats[statKey(entity, name)];
        // 若同 id 已存在则更新
        for (auto& m : e.modifiers) {
            if (m.id == modId) { m.type = type; m.value = value;
                ctx.SetOutputValue("NewValue", Variant(static_cast<double>(computeStat(e))));
                ctx.ActivateOutputFlow("exec"); return true; }
        }
        e.modifiers.push_back({modId, type, value});
        ctx.SetOutputValue("NewValue", Variant(static_cast<double>(computeStat(e))));
        ctx.Log("[Stat.AddModifier] " + statKey(entity,name) + " +" + modId + "=" + std::to_string(value));
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // Stat.RemoveModifier
    // in:  exec, Name, Entity, ModId
    // out: exec, NewValue(Float)
    handlers["Stat.RemoveModifier"] = [](ExecutionContext& ctx) -> bool {
        std::string name   = ctx.GetInputValue("Name").asString();
        std::string entity = ctx.GetInputValue("Entity").asString();
        std::string modId  = ctx.GetInputValue("ModId").asString();

        std::lock_guard<std::mutex> lk(s_statMutex);
        auto it = s_stats.find(statKey(entity, name));
        if (it != s_stats.end()) {
            auto& mods = it->second.modifiers;
            mods.erase(std::remove_if(mods.begin(), mods.end(),
                [&](const StatModifier& m){ return m.id == modId; }), mods.end());
            ctx.SetOutputValue("NewValue", Variant(static_cast<double>(computeStat(it->second))));
        } else {
            ctx.SetOutputValue("NewValue", Variant(0.0));
        }
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // Stat.Reset — 清除所有修改器，恢复基础值
    // in:  exec, Name, Entity
    // out: exec
    handlers["Stat.Reset"] = [](ExecutionContext& ctx) -> bool {
        std::string name   = ctx.GetInputValue("Name").asString();
        std::string entity = ctx.GetInputValue("Entity").asString();
        std::lock_guard<std::mutex> lk(s_statMutex);
        auto it = s_stats.find(statKey(entity, name));
        if (it != s_stats.end()) it->second.modifiers.clear();
        ctx.Log("[Stat.Reset] " + statKey(entity, name));
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // ── Cooldown 冷却系统 ─────────────────────────────────────────────────
    // 基于变量存储冷却结束时间，无需额外数据结构
    // ========================================================================

    // Cooldown.Start
    // in:  exec, Key(String), Duration(Float)
    // out: exec
    handlers["Cooldown.Start"] = [](ExecutionContext& ctx) -> bool {
        std::string key = ctx.GetInputValue("Key").asString();
        double dur = ctx.GetInputValue("Duration").asFloat();
        double now = static_cast<double>(FrameTimerManager::GetCurrentUnixTime());
        ctx.SetVariable("__cd_end_" + key, Variant(now + dur));
        ctx.Log("[Cooldown.Start] " + key + " dur=" + std::to_string(dur));
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // Cooldown.IsReady
    // in:  Key(String)
    // out: Ready(Bool), Remaining(Float)
    handlers["Cooldown.IsReady"] = [](ExecutionContext& ctx) -> bool {
        std::string key = ctx.GetInputValue("Key").asString();
        double endTime  = ctx.GetVariable("__cd_end_" + key).asFloat();
        double now      = static_cast<double>(FrameTimerManager::GetCurrentUnixTime());
        bool ready      = (endTime <= 0.0 || now >= endTime);
        double remaining = ready ? 0.0 : (endTime - now);
        ctx.SetOutputValue("Ready",     Variant(ready));
        ctx.SetOutputValue("Remaining", Variant(remaining));
        return true;
    };

    // Cooldown.GetRemaining
    // in:  Key(String)
    // out: Remaining(Float), Progress(Float 0~1)
    handlers["Cooldown.GetRemaining"] = [](ExecutionContext& ctx) -> bool {
        std::string key = ctx.GetInputValue("Key").asString();
        double dur      = ctx.GetInputValue("Duration").asFloat(); // 可选，用于计算 Progress
        double endTime  = ctx.GetVariable("__cd_end_" + key).asFloat();
        double now      = static_cast<double>(FrameTimerManager::GetCurrentUnixTime());
        double remaining = (endTime <= 0.0 || now >= endTime) ? 0.0 : (endTime - now);
        double progress  = (dur > 0 && remaining > 0) ? (1.0 - remaining/dur) : 1.0;
        ctx.SetOutputValue("Remaining", Variant(remaining));
        ctx.SetOutputValue("Progress",  Variant(progress));
        return true;
    };

    // Cooldown.Reset — 立即解除冷却
    // in:  exec, Key(String)
    // out: exec
    handlers["Cooldown.Reset"] = [](ExecutionContext& ctx) -> bool {
        std::string key = ctx.GetInputValue("Key").asString();
        ctx.SetVariable("__cd_end_" + key, Variant(0.0));
        ctx.Log("[Cooldown.Reset] " + key);
        ctx.ActivateOutputFlow("exec");
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
