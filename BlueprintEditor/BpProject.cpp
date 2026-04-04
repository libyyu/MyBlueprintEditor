// BlueprintEditor/BpProject.cpp
#include "BpProject.h"
#include "BpLogger.h"
#include "../Utils/Json/crude_json.h"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================================
// 路径工具
// ============================================================================

std::string BpProject::AbsPath(const std::string& relPath) const
{
    if (relPath.empty()) return relPath;
    if (fs::path(relPath).is_absolute()) return relPath;
    return (fs::path(projectDir) / relPath).lexically_normal().string();
}

std::string BpProject::RelPath(const std::string& absPath) const
{
    if (absPath.empty() || projectDir.empty()) return absPath;
    try
    {
        std::string rel = fs::path(absPath)
               .lexically_relative(fs::path(projectDir))
               .string();
        // 统一使用正斜杠，避免 Windows 下序列化为反斜杠
        for (char& c : rel) if (c == '\\') c = '/';
        return rel;
    }
    catch (...) { return absPath; }
}

// ============================================================================
// 保存
// ============================================================================

bool SaveBpProject(const BpProject& proj, const std::string& filePath)
{
    using namespace crude_json;

    value root = object();
    root["projVersion"]  = value((double)proj.projVersion);
    root["name"]         = value(proj.name);
    root["description"]  = value(proj.description);
    root["version"]      = value(proj.version);

    // blueprints 数组
    value bpArr = array();
    for (const auto& e : proj.blueprints)
    {
        value item = object();
        item["path"] = value(e.relativePath);
        if (!e.displayName.empty())
            item["name"] = value(e.displayName);
        bpArr.push_back(item);
    }
    root["blueprints"] = bpArr;

    // libraries 数组
    value libArr = array();
    for (const auto& e : proj.libraries)
    {
        value item = object();
        item["path"] = value(e.relativePath);
        if (!e.displayName.empty())
            item["name"] = value(e.displayName);
        libArr.push_back(item);
    }
    root["libraries"] = libArr;

    // luaExtensions 数组（可选）
    if (!proj.luaExtensions.empty())
    {
        value luaArr = array();
        for (const auto& p : proj.luaExtensions)
            luaArr.push_back(value(p));
        root["luaExtensions"] = luaArr;
    }

    std::string json = root.dump();

    std::ofstream ofs(filePath);
    if (!ofs.is_open())
    {
        BPERROR("SaveBpProject: cannot write to " + filePath);
        return false;
    }
    ofs << json;
    BPLOG("Project saved: " + filePath);
    return true;
}

// ============================================================================
// 加载
// ============================================================================

bool LoadBpProject(BpProject& proj, const std::string& filePath)
{
    std::ifstream ifs(filePath);
    if (!ifs.is_open())
    {
        BPERROR("LoadBpProject: cannot open " + filePath);
        return false;
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    std::string content = ss.str();

    crude_json::value root = crude_json::value::parse(content);
    if (root.type() != crude_json::type_t::object)
    {
        BPERROR("LoadBpProject: invalid JSON in " + filePath);
        return false;
    }

    proj = BpProject{};  // reset

    auto getStr = [&](const crude_json::value& obj, const std::string& key, const std::string& def = "") -> std::string {
        if (obj.contains(key) && obj[key].type() == crude_json::type_t::string)
            return obj[key].get<std::string>();
        return def;
    };
    auto getInt = [&](const crude_json::value& obj, const std::string& key, int def = 0) -> int {
        if (obj.contains(key) && obj[key].type() == crude_json::type_t::number)
            return (int)obj[key].get<double>();
        return def;
    };

    proj.projVersion = getInt(root, "projVersion", 1);
    proj.name        = getStr(root, "name");
    proj.description = getStr(root, "description");
    proj.version     = getStr(root, "version", "1.0");

    auto parseEntries = [&](const std::string& key, std::vector<BpProjectEntry>& out)
    {
        if (!root.contains(key)) return;
        const auto& arr = root[key];
        if (arr.type() != crude_json::type_t::array) return;
        for (const auto& item : arr.get<crude_json::array>())
        {
            BpProjectEntry e;
            e.relativePath = getStr(item, "path");
            e.displayName  = getStr(item, "name");
            // 规范化路径分隔符为正斜杠（兼容旧版反斜杠存储）
            for (char& c : e.relativePath) if (c == '\\') c = '/';
            if (!e.relativePath.empty())
            {
                // 去重（同一路径可能因新旧版本分隔符不同被重复记录）
                bool dup = false;
                for (const auto& existing : out)
                    if (existing.relativePath == e.relativePath) { dup = true; break; }
                if (!dup)
                    out.push_back(std::move(e));
            }
        }
    };

    parseEntries("blueprints", proj.blueprints);
    parseEntries("libraries",  proj.libraries);

    // luaExtensions（字符串数组，可选）
    if (root.contains("luaExtensions") &&
        root["luaExtensions"].type() == crude_json::type_t::array)
    {
        for (const auto& item : root["luaExtensions"].get<crude_json::array>())
        {
            if (item.type() == crude_json::type_t::string)
                proj.luaExtensions.push_back(item.get<std::string>());
        }
    }

    // 记录文件路径和目录
    proj.filePath = fs::absolute(filePath).string();
    proj.projectDir = fs::path(proj.filePath).parent_path().string();

    BPLOG("Project loaded: " + filePath + " (" +
          std::to_string(proj.blueprints.size()) + " BPs, " +
          std::to_string(proj.libraries.size())  + " libs)");
    return true;
}

// ============================================================================
// 新建
// ============================================================================

BpProject NewBpProject(const std::string& name)
{
    BpProject p;
    p.name = name.empty() ? "NewProject" : name;
    return p;
}
