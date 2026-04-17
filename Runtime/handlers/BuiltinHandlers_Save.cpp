// Runtime/handlers/BuiltinHandlers_Save.cpp
// 游戏存档系统节点
//
//   Save.Write       — 写入存档槽的一个键值
//   Save.Read        — 读取存档槽的一个键值
//   Save.Exists      — 检查存档槽是否存在
//   Save.Delete      — 删除存档槽
//   Save.ListSlots   — 列举所有存档槽
//   Save.ExportJSON  — 导出存档槽为 JSON 字符串
//   Save.ImportJSON  — 从 JSON 字符串导入存档槽
//   Save.Clear       — 清空存档槽内所有键
//   Save.GetKeys     — 获取存档槽内所有键名
//
// 存储格式：
//   每个槽对应一个 JSON 文件，路径为 SaveDir/slot.sav
//   SaveDir 默认为 "./saves/"，可通过变量 "__save_dir" 覆盖
//   存档文件格式：{ "version": 1, "data": { "key": value, ... } }

#include "BuiltinHandlers_Save.h"
#include "../BlueprintRunner.h"
#include "../../Utils/Json/crude_json.h"

#ifndef __EMSCRIPTEN__
#  include <filesystem>
#endif

#include <fstream>
#include <sstream>
#include <mutex>
#include <unordered_map>
#include <string>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 内部工具
// ============================================================================
static std::mutex s_saveMutex;

// 获取存档目录（优先读 __save_dir 变量）
static std::string getSaveDir(ExecutionContext& ctx)
{
    std::string dir = ctx.GetVariable("__save_dir").asString();
    if (dir.empty()) dir = "./saves";
    // 确保末尾无斜杠
    while (!dir.empty() && (dir.back() == '/' || dir.back() == '\\'))
        dir.pop_back();
    return dir;
}

static std::string slotPath(const std::string& saveDir, const std::string& slot)
{
    return saveDir + "/" + slot + ".sav";
}

// 读取槽文件，返回 data 对象（解析失败返回空对象）
static crude_json::value loadSlot(const std::string& path)
{
#ifdef __EMSCRIPTEN__
    return crude_json::value(crude_json::object{});
#else
    std::ifstream f(path);
    if (!f.is_open()) return crude_json::value(crude_json::object{});
    std::ostringstream ss; ss << f.rdbuf();
    crude_json::value root = crude_json::value::parse(ss.str());
    if (root.is_object() && root.contains("data") && root["data"].is_object())
        return root["data"];
    return crude_json::value(crude_json::object{});
#endif
}

// 写入槽文件
static bool saveSlot(const std::string& saveDir, const std::string& slot,
                     const crude_json::value& data)
{
#ifdef __EMSCRIPTEN__
    return false;
#else
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(saveDir, ec);

    crude_json::object root;
    root["version"] = crude_json::value(1.0);
    root["slot"]    = crude_json::value(slot);
    root["data"]    = data;

    std::ofstream f(slotPath(saveDir, slot));
    if (!f.is_open()) return false;
    f << crude_json::value(std::move(root)).dump();
    return true;
#endif
}

// Variant → crude_json
static crude_json::value variantToJson(const Variant& v)
{
    switch (v.type) {
    case PinDataType::Boolean: return crude_json::value(v.asBool());
    case PinDataType::Integer: return crude_json::value(static_cast<double>(v.asInt()));
    case PinDataType::Float:   return crude_json::value(v.asFloat());
    default:                   return crude_json::value(v.asString());
    }
}

static Variant jsonToVariant(const crude_json::value& j)
{
    if (j.is_boolean()) return Variant(j.get<bool>());
    if (j.is_number()) {
        double d = j.get<double>();
        if (d == static_cast<double>(static_cast<int64_t>(d)))
            return Variant(static_cast<int64_t>(d));
        return Variant(d);
    }
    if (j.is_string()) return Variant(j.get<std::string>());
    if (!j.is_null())  return Variant(j.dump());
    return Variant();
}

// ============================================================================
void RegisterHandlers_Save(std::unordered_map<std::string, NodeHandler>& handlers)
{
    // ========================================================================
    // Save.Write
    // 向存档槽写入键值
    // in:  exec, Slot(String), Key(String), Value(Any)
    // out: exec, Success(Bool), ErrorMessage(String)
    // ========================================================================
    handlers["Save.Write"] = [](ExecutionContext& ctx) -> bool {
#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("Success", Variant(false));
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("Save not supported on WebGL")));
        ctx.ActivateOutputFlow("exec");
        return true;
#else
        std::string slot = ctx.GetInputValue("Slot").asString();
        std::string key  = ctx.GetInputValue("Key").asString();
        Variant     val  = ctx.GetInputValue("Value");

        if (slot.empty() || key.empty()) {
            ctx.SetOutputValue("Success",      Variant(false));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Slot/Key is empty")));
            ctx.ActivateOutputFlow("exec");
            return true;
        }

        std::string saveDir = getSaveDir(ctx);
        std::lock_guard<std::mutex> lk(s_saveMutex);

        crude_json::value data = loadSlot(slotPath(saveDir, slot));
        if (!data.is_object()) data = crude_json::value(crude_json::object{});
        data.get<crude_json::object>()[key] = variantToJson(val);

        bool ok = saveSlot(saveDir, slot, data);
        ctx.SetOutputValue("Success",      Variant(ok));
        ctx.SetOutputValue("ErrorMessage", Variant(ok ? std::string("") : std::string("Write failed")));
        ctx.Log("[Save.Write] slot=" + slot + " key=" + key + " ok=" + (ok ? "true" : "false"));
        ctx.ActivateOutputFlow("exec");
        return true;
#endif
    };

    // ========================================================================
    // Save.Read
    // 读取存档槽的键值
    // in:  Slot(String), Key(String), Default(Any)
    // out: Value(Any), Found(Bool)
    // ========================================================================
    handlers["Save.Read"] = [](ExecutionContext& ctx) -> bool {
#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("Value", ctx.GetInputValue("Default"));
        ctx.SetOutputValue("Found", Variant(false));
        return true;
#else
        std::string slot    = ctx.GetInputValue("Slot").asString();
        std::string key     = ctx.GetInputValue("Key").asString();
        Variant     defVal  = ctx.GetInputValue("Default");

        if (slot.empty() || key.empty()) {
            ctx.SetOutputValue("Value", defVal);
            ctx.SetOutputValue("Found", Variant(false));
            return true;
        }

        std::string saveDir = getSaveDir(ctx);
        std::lock_guard<std::mutex> lk(s_saveMutex);

        crude_json::value data = loadSlot(slotPath(saveDir, slot));
        bool found = data.is_object() && data.contains(key);
        Variant result = found ? jsonToVariant(data[key]) : defVal;

        ctx.SetOutputValue("Value", result);
        ctx.SetOutputValue("Found", Variant(found));
        return true;
#endif
    };

    // ========================================================================
    // Save.Exists
    // 检查存档槽是否存在
    // in:  Slot(String)
    // out: Exists(Bool)
    // ========================================================================
    handlers["Save.Exists"] = [](ExecutionContext& ctx) -> bool {
#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("Exists", Variant(false));
        return true;
#else
        std::string slot    = ctx.GetInputValue("Slot").asString();
        std::string saveDir = getSaveDir(ctx);
        namespace fs = std::filesystem;
        std::error_code ec;
        bool exists = !slot.empty() && fs::exists(slotPath(saveDir, slot), ec);
        ctx.SetOutputValue("Exists", Variant(exists));
        return true;
#endif
    };

    // ========================================================================
    // Save.Delete
    // 删除存档槽文件
    // in:  exec, Slot(String)
    // out: exec, Success(Bool)
    // ========================================================================
    handlers["Save.Delete"] = [](ExecutionContext& ctx) -> bool {
#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("Success", Variant(false));
        ctx.ActivateOutputFlow("exec");
        return true;
#else
        std::string slot    = ctx.GetInputValue("Slot").asString();
        std::string saveDir = getSaveDir(ctx);
        namespace fs = std::filesystem;
        std::error_code ec;
        bool ok = !slot.empty() && fs::remove(slotPath(saveDir, slot), ec);
        ctx.SetOutputValue("Success", Variant(ok));
        ctx.Log("[Save.Delete] slot=" + slot + " ok=" + (ok ? "true" : "false"));
        ctx.ActivateOutputFlow("exec");
        return true;
#endif
    };

    // ========================================================================
    // Save.ListSlots
    // 列举所有存档槽名称
    // out: Slots(Array<String>), Count(Int)
    // ========================================================================
    handlers["Save.ListSlots"] = [](ExecutionContext& ctx) -> bool {
#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("Slots", Variant(std::vector<Variant>{}));
        ctx.SetOutputValue("Count", Variant(int64_t(0)));
        return true;
#else
        std::string saveDir = getSaveDir(ctx);
        namespace fs = std::filesystem;
        std::vector<Variant> slots;
        std::error_code ec;
        if (fs::exists(saveDir, ec)) {
            for (const auto& entry : fs::directory_iterator(saveDir, ec)) {
                if (entry.path().extension() == ".sav") {
                    std::string stem = entry.path().stem().string();
                    slots.push_back(Variant(stem));
                }
            }
        }
        ctx.SetOutputValue("Slots", Variant(std::move(slots)));
        ctx.SetOutputValue("Count", Variant(static_cast<int64_t>(slots.size())));
        return true;
#endif
    };

    // ========================================================================
    // Save.ExportJSON
    // 将存档槽导出为 JSON 字符串
    // in:  Slot(String)
    // out: JSON(String), Found(Bool)
    // ========================================================================
    handlers["Save.ExportJSON"] = [](ExecutionContext& ctx) -> bool {
#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("JSON",  Variant(std::string("{}")));
        ctx.SetOutputValue("Found", Variant(false));
        return true;
#else
        std::string slot    = ctx.GetInputValue("Slot").asString();
        std::string saveDir = getSaveDir(ctx);
        namespace fs = std::filesystem;
        std::error_code ec;
        std::string path = slotPath(saveDir, slot);
        bool found = !slot.empty() && fs::exists(path, ec);

        if (!found) {
            ctx.SetOutputValue("JSON",  Variant(std::string("{}")));
            ctx.SetOutputValue("Found", Variant(false));
            return true;
        }

        std::ifstream f(path);
        std::ostringstream ss; ss << f.rdbuf();
        ctx.SetOutputValue("JSON",  Variant(ss.str()));
        ctx.SetOutputValue("Found", Variant(true));
        return true;
#endif
    };

    // ========================================================================
    // Save.ImportJSON
    // 从 JSON 字符串导入到存档槽（覆盖）
    // in:  exec, Slot(String), JSON(String)
    // out: exec, Success(Bool), ErrorMessage(String)
    // ========================================================================
    handlers["Save.ImportJSON"] = [](ExecutionContext& ctx) -> bool {
#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("Success", Variant(false));
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("Not supported on WebGL")));
        ctx.ActivateOutputFlow("exec");
        return true;
#else
        std::string slot    = ctx.GetInputValue("Slot").asString();
        std::string json    = ctx.GetInputValue("JSON").asString();
        std::string saveDir = getSaveDir(ctx);

        crude_json::value root = crude_json::value::parse(json);
        if (!root.is_object()) {
            ctx.SetOutputValue("Success",      Variant(false));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Invalid JSON")));
            ctx.ActivateOutputFlow("exec");
            return true;
        }

        // 支持两种格式：完整存档文件 或 纯 data 对象
        crude_json::value data;
        if (root.contains("data") && root["data"].is_object())
            data = root["data"];
        else
            data = root;

        std::lock_guard<std::mutex> lk(s_saveMutex);
        bool ok = saveSlot(saveDir, slot, data);
        ctx.SetOutputValue("Success",      Variant(ok));
        ctx.SetOutputValue("ErrorMessage", Variant(ok ? std::string("") : std::string("Write failed")));
        ctx.ActivateOutputFlow("exec");
        return true;
#endif
    };

    // ========================================================================
    // Save.Clear
    // 清空存档槽内所有键（保留槽文件，data 变为空对象）
    // in:  exec, Slot(String)
    // out: exec, Success(Bool)
    // ========================================================================
    handlers["Save.Clear"] = [](ExecutionContext& ctx) -> bool {
#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("Success", Variant(false));
        ctx.ActivateOutputFlow("exec");
        return true;
#else
        std::string slot    = ctx.GetInputValue("Slot").asString();
        std::string saveDir = getSaveDir(ctx);
        std::lock_guard<std::mutex> lk(s_saveMutex);
        bool ok = saveSlot(saveDir, slot, crude_json::value(crude_json::object{}));
        ctx.SetOutputValue("Success", Variant(ok));
        ctx.Log("[Save.Clear] slot=" + slot);
        ctx.ActivateOutputFlow("exec");
        return true;
#endif
    };

    // ========================================================================
    // Save.GetKeys
    // 获取存档槽内所有键名
    // in:  Slot(String)
    // out: Keys(Array<String>), Count(Int)
    // ========================================================================
    handlers["Save.GetKeys"] = [](ExecutionContext& ctx) -> bool {
#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("Keys",  Variant(std::vector<Variant>{}));
        ctx.SetOutputValue("Count", Variant(int64_t(0)));
        return true;
#else
        std::string slot    = ctx.GetInputValue("Slot").asString();
        std::string saveDir = getSaveDir(ctx);
        std::lock_guard<std::mutex> lk(s_saveMutex);

        crude_json::value data = loadSlot(slotPath(saveDir, slot));
        std::vector<Variant> keys;
        if (data.is_object()) {
            for (const auto& kv : data.get<crude_json::object>())
                keys.push_back(Variant(kv.first));
        }
        ctx.SetOutputValue("Keys",  Variant(std::move(keys)));
        ctx.SetOutputValue("Count", Variant(static_cast<int64_t>(keys.size())));
        return true;
#endif
    };

    // ========================================================================
    // Save.WriteMultiple
    // 批量写入多个键值（JSON 对象格式）
    // in:  exec, Slot(String), Data(String JSON object)
    // out: exec, Success(Bool)
    // ========================================================================
    handlers["Save.WriteMultiple"] = [](ExecutionContext& ctx) -> bool {
#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("Success", Variant(false));
        ctx.ActivateOutputFlow("exec");
        return true;
#else
        std::string slot    = ctx.GetInputValue("Slot").asString();
        std::string dataStr = ctx.GetInputValue("Data").asString();
        std::string saveDir = getSaveDir(ctx);

        crude_json::value newData = crude_json::value::parse(dataStr);
        if (!newData.is_object()) {
            ctx.SetOutputValue("Success", Variant(false));
            ctx.ActivateOutputFlow("exec");
            return true;
        }

        std::lock_guard<std::mutex> lk(s_saveMutex);
        crude_json::value existing = loadSlot(slotPath(saveDir, slot));
        if (!existing.is_object()) existing = crude_json::value(crude_json::object{});
        // Merge
        for (const auto& kv : newData.get<crude_json::object>())
            existing.get<crude_json::object>()[kv.first] = kv.second;

        bool ok = saveSlot(saveDir, slot, existing);
        ctx.SetOutputValue("Success", Variant(ok));
        ctx.Log("[Save.WriteMultiple] slot=" + slot + " keys=" + std::to_string(newData.get<crude_json::object>().size()));
        ctx.ActivateOutputFlow("exec");
        return true;
#endif
    };
}

} // namespace Runtime
} // namespace NodeEditor
