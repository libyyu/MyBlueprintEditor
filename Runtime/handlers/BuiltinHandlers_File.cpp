// Runtime/handlers/BuiltinHandlers_File.cpp
// 文件 I/O 节点 handler 实现
//
// 所有文件操作统一通过 GetDefaultFileSystem() 调用 IFileSystem 接口。
// 平台差异（WebGL stub / native std::filesystem）均封装在 DefaultFileSystem 内部，
// handler 层不再区分 __EMSCRIPTEN__。
//
#include "BuiltinHandlers_File.h"
#include "../BlueprintRunner.h"
#include "../FileSystem.h"

#include <string>
#include <vector>

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_File(
    std::unordered_map<std::string, NodeHandler>& handlers)
{
    // ── File.ReadText ─────────────────────────────────────────────────────────
    handlers["File.ReadText"] = [](ExecutionContext& ctx) -> bool {
        auto* fs = GetDefaultFileSystem().get();
        std::string path = ctx.GetInputValue("Path").asString();
        if (path.empty()) {
            ctx.SetOutputValue("Content",      Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Path is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        std::string content, err;
        if (fs && fs->ReadFile(path, content, err)) {
            ctx.SetOutputValue("Content",      Variant(content));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
            ctx.ActivateOutputFlow("onSuccess");
        } else {
            ctx.SetOutputValue("Content",      Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage", Variant(err.empty() ? "FileSystem not available" : err));
            ctx.ActivateOutputFlow("onError");
        }
        return true;
    };

    // ── File.WriteText ────────────────────────────────────────────────────────
    handlers["File.WriteText"] = [](ExecutionContext& ctx) -> bool {
        auto* fs      = GetDefaultFileSystem().get();
        std::string path    = ctx.GetInputValue("Path").asString();
        std::string content = ctx.GetInputValue("Content").asString();
        bool        append  = ctx.GetInputValue("Append").asBool();

        std::string err;
        bool ok = fs && (append ? fs->AppendFile(path, content, err)
                                : fs->WriteFile(path, content, err));
        ctx.SetOutputValue("ErrorMessage", Variant(ok ? std::string("") : err));
        ctx.ActivateOutputFlow(ok ? "onSuccess" : "onError");
        return true;
    };

    // ── File.AppendText ───────────────────────────────────────────────────────
    handlers["File.AppendText"] = [](ExecutionContext& ctx) -> bool {
        auto* fs      = GetDefaultFileSystem().get();
        std::string path    = ctx.GetInputValue("Path").asString();
        std::string content = ctx.GetInputValue("Content").asString();
        std::string err;
        bool ok = fs && fs->AppendFile(path, content, err);
        ctx.SetOutputValue("ErrorMessage", Variant(ok ? std::string("") : err));
        ctx.ActivateOutputFlow(ok ? "onSuccess" : "onError");
        return true;
    };

    // ── File.Exists ───────────────────────────────────────────────────────────
    handlers["File.Exists"] = [](ExecutionContext& ctx) -> bool {
        auto* fs = GetDefaultFileSystem().get();
        std::string path = ctx.GetInputValue("Path").asString();
        bool exists = fs && !path.empty() && fs->FileExists(path);
        ctx.SetOutputValue("Exists", Variant(exists));
        return true;
    };

    // ── File.Delete ───────────────────────────────────────────────────────────
    handlers["File.Delete"] = [](ExecutionContext& ctx) -> bool {
        auto* fs     = GetDefaultFileSystem().get();
        std::string path   = ctx.GetInputValue("Path").asString();
        bool ignore        = ctx.GetInputValue("IgnoreNotFound").asBool();
        std::string err;
        bool ok = fs && fs->DeleteFile(path, ignore, err);
        ctx.SetOutputValue("ErrorMessage", Variant(ok ? std::string("") : err));
        ctx.ActivateOutputFlow(ok ? "onSuccess" : "onError");
        return true;
    };

    // ── File.ListDir ──────────────────────────────────────────────────────────
    handlers["File.ListDir"] = [](ExecutionContext& ctx) -> bool {
        auto* fs      = GetDefaultFileSystem().get();
        std::string path    = ctx.GetInputValue("Path").asString();
        std::string pattern = ctx.GetInputValue("Pattern").asString();

        std::vector<std::string> names;
        std::string err;
        bool ok = fs && fs->ListDir(path, pattern, names, err);

        // 序列化为 JSON 数组
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

        ctx.SetOutputValue("Files",        Variant(json));
        ctx.SetOutputValue("ErrorMessage", Variant(ok ? std::string("") : err));
        ctx.ActivateOutputFlow(ok ? "onSuccess" : "onError");
        return true;
    };

    // ── File.MakeDir ──────────────────────────────────────────────────────────
    handlers["File.MakeDir"] = [](ExecutionContext& ctx) -> bool {
        auto* fs = GetDefaultFileSystem().get();
        std::string path = ctx.GetInputValue("Path").asString();
        std::string err;
        bool ok = fs && fs->MakeDir(path, err);
        ctx.SetOutputValue("ErrorMessage", Variant(ok ? std::string("") : err));
        ctx.ActivateOutputFlow(ok ? "onSuccess" : "onError");
        return true;
    };

    // ── File.GetBaseName（纯数据节点）─────────────────────────────────────────
    handlers["File.GetBaseName"] = [](ExecutionContext& ctx) -> bool {
        auto* fs = GetDefaultFileSystem().get();
        std::string path = ctx.GetInputValue("Path").asString();
        ctx.SetOutputValue("BaseName", Variant(fs ? fs->GetBaseName(path) : std::string("")));
        return true;
    };

    // ── File.GetDirName（纯数据节点）─────────────────────────────────────────
    handlers["File.GetDirName"] = [](ExecutionContext& ctx) -> bool {
        auto* fs = GetDefaultFileSystem().get();
        std::string path = ctx.GetInputValue("Path").asString();
        ctx.SetOutputValue("DirName", Variant(fs ? fs->GetDirName(path) : std::string("")));
        return true;
    };

    // ── File.JoinPath（纯数据节点）───────────────────────────────────────────
    handlers["File.JoinPath"] = [](ExecutionContext& ctx) -> bool {
        auto* fs = GetDefaultFileSystem().get();
        std::string base = ctx.GetInputValue("Base").asString();
        std::string part = ctx.GetInputValue("Part").asString();
        ctx.SetOutputValue("Result", Variant(fs ? fs->JoinPath(base, part) : std::string("")));
        return true;
    };

    // ── File.GetSize（纯数据节点）────────────────────────────────────────────
    handlers["File.GetSize"] = [](ExecutionContext& ctx) -> bool {
        auto* fs = GetDefaultFileSystem().get();
        std::string path = ctx.GetInputValue("Path").asString();
        int64_t size = fs ? fs->GetFileSize(path) : -1LL;
        ctx.SetOutputValue("Size", Variant(size));
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
