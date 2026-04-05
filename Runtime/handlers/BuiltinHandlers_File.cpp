// Runtime/handlers/BuiltinHandlers_File.cpp
// 文件 I/O 节点 handler 实现
#include "BuiltinHandlers_File.h"
#include "../BlueprintRunner.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <cerrno>
#include <cstring>

namespace fs = std::filesystem;

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_File(
    std::unordered_map<std::string, NodeHandler>& handlers)
{
    // ------------------------------------------------------------------
    // File.ReadText
    //   exec in → exec out (onSuccess / onError)
    //   in:  Path(String)
    //   out: Content(String), ErrorMessage(String)
    // ------------------------------------------------------------------
    handlers["File.ReadText"] = [](ExecutionContext& ctx) {
        std::string path = ctx.GetInputValue("Path").asString();
        if (path.empty()) {
            ctx.SetOutputValue("Content", Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Path is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) {
            ctx.SetOutputValue("Content", Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string(
                "Cannot open file: " + path + " (" + std::strerror(errno) + ")")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        std::ostringstream ss;
        ss << f.rdbuf();
        ctx.SetOutputValue("Content", Variant(ss.str()));
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
    };

    // ------------------------------------------------------------------
    // File.WriteText
    //   exec in → onSuccess / onError
    //   in:  Path(String), Content(String), Append(Boolean)
    //   out: ErrorMessage(String)
    // ------------------------------------------------------------------
    handlers["File.WriteText"] = [](ExecutionContext& ctx) {
        std::string path    = ctx.GetInputValue("Path").asString();
        std::string content = ctx.GetInputValue("Content").asString();
        bool append = ctx.GetInputValue("Append").asBool();

        auto mode = std::ios::out | std::ios::binary;
        if (append) mode |= std::ios::app;
        else        mode |= std::ios::trunc;

        // 自动建父目录
        try {
            fs::path p(path);
            if (p.has_parent_path())
                fs::create_directories(p.parent_path());
        } catch (...) {}

        std::ofstream f(path, mode);
        if (!f.is_open()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string(
                "Cannot write file: " + path + " (" + std::strerror(errno) + ")")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        f << content;
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
    };

    // ------------------------------------------------------------------
    // File.AppendText  (shortcut，等价 WriteText + Append=true)
    // ------------------------------------------------------------------
    handlers["File.AppendText"] = [](ExecutionContext& ctx) {
        std::string path    = ctx.GetInputValue("Path").asString();
        std::string content = ctx.GetInputValue("Content").asString();

        try {
            fs::path p(path);
            if (p.has_parent_path())
                fs::create_directories(p.parent_path());
        } catch (...) {}

        std::ofstream f(path, std::ios::app | std::ios::binary);
        if (!f.is_open()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string(
                "Cannot append file: " + path + " (" + std::strerror(errno) + ")")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        f << content;
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
    };

    // ------------------------------------------------------------------
    // File.Exists
    //   in:  Path(String)
    //   out: Exists(Boolean)
    //   纯数据节点（无 exec flow）
    // ------------------------------------------------------------------
    handlers["File.Exists"] = [](ExecutionContext& ctx) {
        std::string path = ctx.GetInputValue("Path").asString();
        bool exists = !path.empty() && fs::exists(fs::path(path));
        ctx.SetOutputValue("Exists", Variant(exists));
        return true;
    };

    // ------------------------------------------------------------------
    // File.Delete
    //   exec in → onSuccess / onError
    //   in:  Path(String), IgnoreNotFound(Boolean)
    //   out: ErrorMessage(String)
    // ------------------------------------------------------------------
    handlers["File.Delete"] = [](ExecutionContext& ctx) {
        std::string path = ctx.GetInputValue("Path").asString();
        bool ignore = ctx.GetInputValue("IgnoreNotFound").asBool();
        std::error_code ec;
        bool removed = fs::remove(fs::path(path), ec);
        if (!removed && !ignore && ec) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string(ec.message())));
            ctx.ActivateOutputFlow("onError");
        } else {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
            ctx.ActivateOutputFlow("onSuccess");
        }
        return true;
    };

    // ------------------------------------------------------------------
    // File.ListDir
    //   exec in → onSuccess / onError
    //   in:  Path(String), Pattern(String, glob-like, empty=all)
    //   out: Files(String, JSON array of filenames), ErrorMessage(String)
    // ------------------------------------------------------------------
    handlers["File.ListDir"] = [](ExecutionContext& ctx) {
        std::string path    = ctx.GetInputValue("Path").asString();
        std::string pattern = ctx.GetInputValue("Pattern").asString();

        std::error_code ec;
        if (!fs::is_directory(fs::path(path), ec)) {
            ctx.SetOutputValue("Files", Variant(std::string("[]")));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string(
                "Not a directory: " + path)));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        std::vector<std::string> names;
        for (const auto& entry : fs::directory_iterator(fs::path(path), ec)) {
            std::string name = entry.path().filename().string();
            if (pattern.empty() || name.find(pattern) != std::string::npos)
                names.push_back(name);
        }

        // 序列化成 JSON 数组
        std::string json = "[";
        for (size_t i = 0; i < names.size(); ++i) {
            if (i) json += ",";
            json += "\"";
            for (char c : names[i]) {
                if (c == '"')  json += "\\\"";
                else if (c == '\\') json += "\\\\";
                else json += c;
            }
            json += "\"";
        }
        json += "]";

        ctx.SetOutputValue("Files", Variant(json));
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
    };

    // ------------------------------------------------------------------
    // File.MakeDir
    //   exec in → onSuccess / onError
    //   in:  Path(String)
    //   out: ErrorMessage(String)
    // ------------------------------------------------------------------
    handlers["File.MakeDir"] = [](ExecutionContext& ctx) {
        std::string path = ctx.GetInputValue("Path").asString();
        std::error_code ec;
        fs::create_directories(fs::path(path), ec);
        if (ec) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string(ec.message())));
            ctx.ActivateOutputFlow("onError");
        } else {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
            ctx.ActivateOutputFlow("onSuccess");
        }
        return true;
    };

    // ------------------------------------------------------------------
    // File.GetBaseName  — 纯数据节点
    //   in:  Path(String)
    //   out: BaseName(String)
    // ------------------------------------------------------------------
    handlers["File.GetBaseName"] = [](ExecutionContext& ctx) {
        std::string path = ctx.GetInputValue("Path").asString();
        ctx.SetOutputValue("BaseName",
            Variant(fs::path(path).filename().string()));
        return true;
    };

    // ------------------------------------------------------------------
    // File.GetDirName  — 纯数据节点
    //   in:  Path(String)
    //   out: DirName(String)
    // ------------------------------------------------------------------
    handlers["File.GetDirName"] = [](ExecutionContext& ctx) {
        std::string path = ctx.GetInputValue("Path").asString();
        ctx.SetOutputValue("DirName",
            Variant(fs::path(path).parent_path().string()));
        return true;
    };

    // ------------------------------------------------------------------
    // File.JoinPath  — 纯数据节点
    //   in:  Base(String), Part(String)
    //   out: Result(String)
    // ------------------------------------------------------------------
    handlers["File.JoinPath"] = [](ExecutionContext& ctx) {
        std::string base = ctx.GetInputValue("Base").asString();
        std::string part = ctx.GetInputValue("Part").asString();
        fs::path result = fs::path(base) / fs::path(part);
        ctx.SetOutputValue("Result", Variant(result.string()));
        return true;
    };

    // ------------------------------------------------------------------
    // File.GetSize  — 纯数据节点
    //   in:  Path(String)
    //   out: Size(Integer)  — -1 表示不存在或出错
    // ------------------------------------------------------------------
    handlers["File.GetSize"] = [](ExecutionContext& ctx) {
        std::string path = ctx.GetInputValue("Path").asString();
        std::error_code ec;
        auto sz = fs::file_size(fs::path(path), ec);
        int64_t size = ec ? -1LL : static_cast<int64_t>(sz);
        ctx.SetOutputValue("Size", Variant(size));
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
