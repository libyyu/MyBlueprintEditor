// Runtime/handlers/BuiltinHandlers_File.cpp
// 文件 I/O 节点 handler 实现
//
// 平台策略：
//   非 Emscripten（Windows/macOS/Linux/Android/iOS）：
//     全功能实现，使用 std::filesystem + std::fstream
//   Emscripten/WebGL：
//     浏览器沙箱内没有真实磁盘访问，所有节点注册为 stub，
//     立即走 onError 并输出 "File I/O not supported on WebGL"。
//     纯数据节点（无 exec flow）输出零值/空字符串。
//
#include "BuiltinHandlers_File.h"
#include "../BlueprintRunner.h"

#ifndef __EMSCRIPTEN__
#  include <fstream>
#  include <sstream>
#  include <filesystem>
#  include <cerrno>
#  include <cstring>
   namespace fs = std::filesystem;
#endif

#include <string>
#include <vector>

namespace NodeEditor {
namespace Runtime {

// WebGL stub 消息
static constexpr const char* kWebGLUnsupported =
    "File I/O not supported on WebGL";

void RegisterHandlers_File(
    std::unordered_map<std::string, NodeHandler>& handlers)
{
#ifndef __EMSCRIPTEN__
    // ================================================================
    // 完整实现（桌面 / 移动平台）
    // ================================================================

    // File.ReadText
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

    // File.WriteText
    handlers["File.WriteText"] = [](ExecutionContext& ctx) {
        std::string path    = ctx.GetInputValue("Path").asString();
        std::string content = ctx.GetInputValue("Content").asString();
        bool append = ctx.GetInputValue("Append").asBool();

        auto mode = std::ios::out | std::ios::binary;
        if (append) mode |= std::ios::app;
        else        mode |= std::ios::trunc;

        {
            std::error_code ec2;
            fs::path p(path);
            if (p.has_parent_path()) fs::create_directories(p.parent_path(), ec2);
        }

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

    // File.AppendText
    handlers["File.AppendText"] = [](ExecutionContext& ctx) {
        std::string path    = ctx.GetInputValue("Path").asString();
        std::string content = ctx.GetInputValue("Content").asString();
        {
            std::error_code ec2;
            fs::path p(path);
            if (p.has_parent_path()) fs::create_directories(p.parent_path(), ec2);
        }
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

    // File.Exists（纯数据节点）
    handlers["File.Exists"] = [](ExecutionContext& ctx) {
        std::string path = ctx.GetInputValue("Path").asString();
        bool exists = !path.empty() && fs::exists(fs::path(path));
        ctx.SetOutputValue("Exists", Variant(exists));
        return true;
    };

    // File.Delete
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

    // File.ListDir
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
        std::string json = "[";
        for (size_t i = 0; i < names.size(); ++i) {
            if (i) json += ",";
            json += "\"";
            for (char c : names[i]) {
                if      (c == '"')  json += "\\\"";
                else if (c == '\\') json += "\\\\";
                else                json += c;
            }
            json += "\"";
        }
        json += "]";
        ctx.SetOutputValue("Files", Variant(json));
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
    };

    // File.MakeDir
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

    // File.GetBaseName（纯数据节点）
    handlers["File.GetBaseName"] = [](ExecutionContext& ctx) {
        std::string path = ctx.GetInputValue("Path").asString();
        ctx.SetOutputValue("BaseName", Variant(fs::path(path).filename().string()));
        return true;
    };

    // File.GetDirName（纯数据节点）
    handlers["File.GetDirName"] = [](ExecutionContext& ctx) {
        std::string path = ctx.GetInputValue("Path").asString();
        ctx.SetOutputValue("DirName", Variant(fs::path(path).parent_path().string()));
        return true;
    };

    // File.JoinPath（纯数据节点）
    handlers["File.JoinPath"] = [](ExecutionContext& ctx) {
        std::string base = ctx.GetInputValue("Base").asString();
        std::string part = ctx.GetInputValue("Part").asString();
        ctx.SetOutputValue("Result", Variant((fs::path(base) / fs::path(part)).string()));
        return true;
    };

    // File.GetSize（纯数据节点）
    handlers["File.GetSize"] = [](ExecutionContext& ctx) {
        std::string path = ctx.GetInputValue("Path").asString();
        std::error_code ec;
        auto sz = fs::file_size(fs::path(path), ec);
        int64_t size = ec ? -1LL : static_cast<int64_t>(sz);
        ctx.SetOutputValue("Size", Variant(size));
        return true;
    };

#else // __EMSCRIPTEN__
    // ================================================================
    // WebGL/Emscripten stub：所有节点立即走 onError 或返回零值
    // ================================================================

    // exec 节点 stub（共用逻辑）
    auto execStub = [](ExecutionContext& ctx) -> bool {
        ctx.SetOutputValue("ErrorMessage",
            Variant(std::string(kWebGLUnsupported)));
        ctx.ActivateOutputFlow("onError");
        return true;
    };

    handlers["File.ReadText"] = [execStub](ExecutionContext& ctx) {
        ctx.SetOutputValue("Content", Variant(std::string("")));
        return execStub(ctx);
    };
    handlers["File.WriteText"]  = execStub;
    handlers["File.AppendText"] = execStub;
    handlers["File.Delete"]     = execStub;
    handlers["File.ListDir"] = [execStub](ExecutionContext& ctx) {
        ctx.SetOutputValue("Files", Variant(std::string("[]")));
        return execStub(ctx);
    };
    handlers["File.MakeDir"] = execStub;

    // 纯数据节点 stub（返回空/零，不触发 flow）
    handlers["File.Exists"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Exists", Variant(false));
        return true;
    };
    handlers["File.GetBaseName"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("BaseName", Variant(std::string("")));
        return true;
    };
    handlers["File.GetDirName"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("DirName", Variant(std::string("")));
        return true;
    };
    handlers["File.JoinPath"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Result", Variant(std::string("")));
        return true;
    };
    handlers["File.GetSize"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Size", Variant(int64_t(-1)));
        return true;
    };

#endif // __EMSCRIPTEN__
}

} // namespace Runtime
} // namespace NodeEditor
