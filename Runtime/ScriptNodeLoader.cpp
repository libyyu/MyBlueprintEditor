// Runtime/ScriptNodeLoader.cpp -- 从 JSON 文件加载自定义节点定义
#include "ScriptNodeLoader.h"
#include <fstream>
#include <sstream>

#if !defined(BLUEPRINT_NO_FILESYSTEM) && !defined(__EMSCRIPTEN__)
#include <filesystem>
#endif

// crude_json
#include "../Utils/Json/crude_json.h"

namespace NodeEditor {
namespace Runtime {

static PinDataType ParsePinType(const std::string& typeStr, bool& isExec)
{
    isExec = false;
    if (typeStr == "Flow")      { isExec = true; return PinDataType::Unknown; }
    if (typeStr == "Integer")   return PinDataType::Integer;
    if (typeStr == "Float")     return PinDataType::Float;
    if (typeStr == "Boolean")   return PinDataType::Boolean;
    if (typeStr == "String")    return PinDataType::String;
    if (typeStr == "Array")     return PinDataType::Array;
    if (typeStr == "Map")       return PinDataType::Map;
    if (typeStr == "Object")    return PinDataType::Object;
    return PinDataType::Any;
}

int LoadCustomNodesFromFile(INodeRegistry& registry, const std::string& filePath)
{
    // 尝试打开文件，若不存在则静默跳过
    std::ifstream file(filePath);
    if (!file.is_open())
        return 0;

    // 读取文件内容
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    if (content.empty())
        return 0;

    // 解析 JSON
    auto root = crude_json::value::parse(content);
    if (root.type() != crude_json::type_t::object)
        return 0;

    if (!root.contains("nodes") || root["nodes"].type() != crude_json::type_t::array)
        return 0;

    const auto& nodesArr = root["nodes"].get<crude_json::array>();

    int count = 0;
    for (const auto& nodeVal : nodesArr)
    {
        if (nodeVal.type() != crude_json::type_t::object) continue;

        NodeDefinition def;

        // id
        if (nodeVal.contains("id") && nodeVal["id"].type() == crude_json::type_t::string)
            def.id = nodeVal["id"].get<std::string>();
        if (def.id.empty()) continue;

        // name
        if (nodeVal.contains("name") && nodeVal["name"].type() == crude_json::type_t::string)
            def.name = nodeVal["name"].get<std::string>();
        else
            def.name = def.id;

        // category
        if (nodeVal.contains("category") && nodeVal["category"].type() == crude_json::type_t::string)
            def.category = nodeVal["category"].get<std::string>();

        // color
        if (nodeVal.contains("color") && nodeVal["color"].type() == crude_json::type_t::string)
            def.color = nodeVal["color"].get<std::string>();

        // inputs
        if (nodeVal.contains("inputs") && nodeVal["inputs"].type() == crude_json::type_t::array)
        {
            const auto& inputs = nodeVal["inputs"].get<crude_json::array>();
            for (const auto& pinVal : inputs)
            {
                if (pinVal.type() != crude_json::type_t::object) continue;

                PinDefinition pin;
                pin.kind = PinKind::Input;

                if (pinVal.contains("name") && pinVal["name"].type() == crude_json::type_t::string)
                    pin.name = pinVal["name"].get<std::string>();

                std::string typeStr;
                if (pinVal.contains("type") && pinVal["type"].type() == crude_json::type_t::string)
                    typeStr = pinVal["type"].get<std::string>();

                bool isExec = false;
                if (pinVal.contains("isExec") && pinVal["isExec"].type() == crude_json::type_t::boolean)
                    isExec = pinVal["isExec"].get<bool>();

                if (isExec || typeStr == "Flow")
                {
                    pin.dataType = PinDataType::Unknown;
                    pin.isExec = true;
                }
                else
                {
                    bool dummy;
                    pin.dataType = ParsePinType(typeStr, dummy);
                    pin.isExec = false;
                }

                def.inputPins.push_back(pin);
            }
        }

        // outputs
        if (nodeVal.contains("outputs") && nodeVal["outputs"].type() == crude_json::type_t::array)
        {
            const auto& outputs = nodeVal["outputs"].get<crude_json::array>();
            for (const auto& pinVal : outputs)
            {
                if (pinVal.type() != crude_json::type_t::object) continue;

                PinDefinition pin;
                pin.kind = PinKind::Output;

                if (pinVal.contains("name") && pinVal["name"].type() == crude_json::type_t::string)
                    pin.name = pinVal["name"].get<std::string>();

                std::string typeStr;
                if (pinVal.contains("type") && pinVal["type"].type() == crude_json::type_t::string)
                    typeStr = pinVal["type"].get<std::string>();

                bool isExec = false;
                if (pinVal.contains("isExec") && pinVal["isExec"].type() == crude_json::type_t::boolean)
                    isExec = pinVal["isExec"].get<bool>();

                if (isExec || typeStr == "Flow")
                {
                    pin.dataType = PinDataType::Unknown;
                    pin.isExec = true;
                }
                else
                {
                    bool dummy;
                    pin.dataType = ParsePinType(typeStr, dummy);
                    pin.isExec = false;
                }

                def.outputPins.push_back(pin);
            }
        }

        registry.registerNode(def);
        ++count;
    }

    return count;
}

// ─────────────────────────────────────────────────────────────────────
// 自动扫描目录下所有 .json 文件并加载节点定义
// ─────────────────────────────────────────────────────────────────────

#if !defined(BLUEPRINT_NO_FILESYSTEM) && !defined(__EMSCRIPTEN__)

int LoadCustomNodesFromDirectory(INodeRegistry& registry, const std::string& dirPath)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::is_directory(dirPath, ec))
        return 0;

    int total = 0;
    for (const auto& entry : fs::recursive_directory_iterator(dirPath, ec))
    {
        if (ec) break;
        if (!entry.is_regular_file()) continue;

        auto ext = entry.path().extension().string();
        // 支持 .json 和 .bjson
        if (ext == ".json" || ext == ".bjson")
        {
            int n = LoadCustomNodesFromFile(registry, entry.path().string());
            total += n;
        }
    }
    return total;
}

#else
// Emscripten / BLUEPRINT_NO_FILESYSTEM: 无法扫描目录
int LoadCustomNodesFromDirectory(INodeRegistry& /*registry*/, const std::string& /*dirPath*/)
{
    return 0;
}
#endif

} // namespace Runtime
} // namespace NodeEditor
