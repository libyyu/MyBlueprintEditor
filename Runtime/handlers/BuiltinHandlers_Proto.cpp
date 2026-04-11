#include "BuiltinHandlers_Proto.h"
#include "../../Utils/Json/crude_json.h"
#include "../BlueprintRunner.h"

// ============================================================
// Proto nodes are only available when protobuf is linked in.
// ============================================================
#ifdef BLUEPRINT_HAS_PROTOBUF

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/compiler/parser.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <sstream>
#include <mutex>
#include <unordered_map>
#include <string>

namespace pb = google::protobuf;

// ============================================================
// 全局注册表（线程安全）
// ============================================================
namespace {

struct SchemaRegistry {
    std::mutex mtx;
    pb::DescriptorPool pool;
    pb::DynamicMessageFactory factory;

    static SchemaRegistry& Get() {
        static SchemaRegistry inst;
        return inst;
    }
    SchemaRegistry() : factory(&pool) {}
};

// 从 proto 源文本注册到 pool
// 返回 FileDescriptor，失败返回 nullptr
const pb::FileDescriptor* RegisterProtoText(
    const std::string& filename,
    const std::string& protoText,
    std::string& errOut)
{
    auto& reg = SchemaRegistry::Get();
    std::lock_guard<std::mutex> lk(reg.mtx);

    // 已存在则直接返回
    const pb::FileDescriptor* existing = reg.pool.FindFileByName(filename);
    if (existing) return existing;

    std::istringstream ss(protoText);
    pb::io::IstreamInputStream iis(&ss);
    pb::io::Tokenizer tokenizer(&iis, nullptr);
    pb::compiler::Parser parser;
    pb::FileDescriptorProto fdp;
    fdp.set_name(filename);
    if (!parser.Parse(&tokenizer, &fdp)) {
        errOut = "Failed to parse proto schema: " + filename;
        return nullptr;
    }
    const pb::FileDescriptor* fd = reg.pool.BuildFile(fdp);
    if (!fd) {
        errOut = "Failed to build FileDescriptor: " + filename;
        return nullptr;
    }
    return fd;
}

// 从 pb::Message 递归转 crude_json::value
crude_json::value MessageToJson(const pb::Message& msg) {
    crude_json::object obj;
    const pb::Descriptor* desc = msg.GetDescriptor();
    const pb::Reflection* refl = msg.GetReflection();

    for (int i = 0; i < desc->field_count(); ++i) {
        const pb::FieldDescriptor* fd = desc->field(i);
        const std::string& name = fd->name();

        if (fd->is_repeated()) {
            crude_json::array arr;
            int sz = refl->FieldSize(msg, fd);
            for (int j = 0; j < sz; ++j) {
                switch (fd->cpp_type()) {
                case pb::FieldDescriptor::CPPTYPE_INT32:
                    arr.push_back(crude_json::value((double)refl->GetRepeatedInt32(msg, fd, j))); break;
                case pb::FieldDescriptor::CPPTYPE_INT64:
                    arr.push_back(crude_json::value((double)refl->GetRepeatedInt64(msg, fd, j))); break;
                case pb::FieldDescriptor::CPPTYPE_UINT32:
                    arr.push_back(crude_json::value((double)refl->GetRepeatedUInt32(msg, fd, j))); break;
                case pb::FieldDescriptor::CPPTYPE_UINT64:
                    arr.push_back(crude_json::value((double)refl->GetRepeatedUInt64(msg, fd, j))); break;
                case pb::FieldDescriptor::CPPTYPE_FLOAT:
                    arr.push_back(crude_json::value((double)refl->GetRepeatedFloat(msg, fd, j))); break;
                case pb::FieldDescriptor::CPPTYPE_DOUBLE:
                    arr.push_back(crude_json::value(refl->GetRepeatedDouble(msg, fd, j))); break;
                case pb::FieldDescriptor::CPPTYPE_BOOL:
                    arr.push_back(crude_json::value(refl->GetRepeatedBool(msg, fd, j))); break;
                case pb::FieldDescriptor::CPPTYPE_ENUM:
                    arr.push_back(crude_json::value(std::string(
                        refl->GetRepeatedEnum(msg, fd, j)->name()))); break;
                case pb::FieldDescriptor::CPPTYPE_STRING: {
                    std::string tmp;
                    arr.push_back(crude_json::value(
                        refl->GetRepeatedStringReference(msg, fd, j, &tmp))); break;
                }
                case pb::FieldDescriptor::CPPTYPE_MESSAGE:
                    arr.push_back(MessageToJson(refl->GetRepeatedMessage(msg, fd, j))); break;
                default: break;
                }
            }
            obj[name] = crude_json::value(std::move(arr));
        } else {
            if (!refl->HasField(msg, fd) && fd->is_optional()) {
                // skip unset optional fields
                continue;
            }
            switch (fd->cpp_type()) {
            case pb::FieldDescriptor::CPPTYPE_INT32:
                obj[name] = crude_json::value((double)refl->GetInt32(msg, fd)); break;
            case pb::FieldDescriptor::CPPTYPE_INT64:
                obj[name] = crude_json::value((double)refl->GetInt64(msg, fd)); break;
            case pb::FieldDescriptor::CPPTYPE_UINT32:
                obj[name] = crude_json::value((double)refl->GetUInt32(msg, fd)); break;
            case pb::FieldDescriptor::CPPTYPE_UINT64:
                obj[name] = crude_json::value((double)refl->GetUInt64(msg, fd)); break;
            case pb::FieldDescriptor::CPPTYPE_FLOAT:
                obj[name] = crude_json::value((double)refl->GetFloat(msg, fd)); break;
            case pb::FieldDescriptor::CPPTYPE_DOUBLE:
                obj[name] = crude_json::value(refl->GetDouble(msg, fd)); break;
            case pb::FieldDescriptor::CPPTYPE_BOOL:
                obj[name] = crude_json::value(refl->GetBool(msg, fd)); break;
            case pb::FieldDescriptor::CPPTYPE_ENUM:
                obj[name] = crude_json::value(
                    std::string(refl->GetEnum(msg, fd)->name())); break;
            case pb::FieldDescriptor::CPPTYPE_STRING: {
                std::string tmp;
                obj[name] = crude_json::value(
                    refl->GetStringReference(msg, fd, &tmp)); break;
            }
            case pb::FieldDescriptor::CPPTYPE_MESSAGE:
                obj[name] = MessageToJson(refl->GetMessage(msg, fd)); break;
            default: break;
            }
        }
    }
    return crude_json::value(std::move(obj));
}

// 从 crude_json::value 填充 pb::Message
void JsonToMessage(const crude_json::value& jv, pb::Message* msg) {
    if (!jv.is_object() || !msg) return;
    const pb::Descriptor* desc = msg->GetDescriptor();
    const pb::Reflection* refl = msg->GetReflection();
    const auto& obj = jv.get<crude_json::object>();

    for (const auto& kv : obj) {
        const pb::FieldDescriptor* fd = desc->FindFieldByName(kv.first);
        if (!fd) continue;
        const crude_json::value& val = kv.second;

        if (fd->is_repeated()) {
            if (!val.is_array()) continue;
            for (const auto& elem : val.get<crude_json::array>()) {
                switch (fd->cpp_type()) {
                case pb::FieldDescriptor::CPPTYPE_INT32:
                    refl->AddInt32(msg, fd, (int32_t)elem.get<double>()); break;
                case pb::FieldDescriptor::CPPTYPE_INT64:
                    refl->AddInt64(msg, fd, (int64_t)elem.get<double>()); break;
                case pb::FieldDescriptor::CPPTYPE_UINT32:
                    refl->AddUInt32(msg, fd, (uint32_t)elem.get<double>()); break;
                case pb::FieldDescriptor::CPPTYPE_UINT64:
                    refl->AddUInt64(msg, fd, (uint64_t)elem.get<double>()); break;
                case pb::FieldDescriptor::CPPTYPE_FLOAT:
                    refl->AddFloat(msg, fd, (float)elem.get<double>()); break;
                case pb::FieldDescriptor::CPPTYPE_DOUBLE:
                    refl->AddDouble(msg, fd, elem.get<double>()); break;
                case pb::FieldDescriptor::CPPTYPE_BOOL:
                    refl->AddBool(msg, fd, elem.get<bool>()); break;
                case pb::FieldDescriptor::CPPTYPE_STRING:
                    if (elem.is_string())
                        refl->AddString(msg, fd, elem.get<std::string>()); break;
                case pb::FieldDescriptor::CPPTYPE_MESSAGE: {
                    pb::Message* sub = refl->AddMessage(msg, fd);
                    JsonToMessage(elem, sub); break;
                }
                default: break;
                }
            }
        } else {
            switch (fd->cpp_type()) {
            case pb::FieldDescriptor::CPPTYPE_INT32:
                if (val.is_number()) refl->SetInt32(msg, fd, (int32_t)val.get<double>()); break;
            case pb::FieldDescriptor::CPPTYPE_INT64:
                if (val.is_number()) refl->SetInt64(msg, fd, (int64_t)val.get<double>()); break;
            case pb::FieldDescriptor::CPPTYPE_UINT32:
                if (val.is_number()) refl->SetUInt32(msg, fd, (uint32_t)val.get<double>()); break;
            case pb::FieldDescriptor::CPPTYPE_UINT64:
                if (val.is_number()) refl->SetUInt64(msg, fd, (uint64_t)val.get<double>()); break;
            case pb::FieldDescriptor::CPPTYPE_FLOAT:
                if (val.is_number()) refl->SetFloat(msg, fd, (float)val.get<double>()); break;
            case pb::FieldDescriptor::CPPTYPE_DOUBLE:
                if (val.is_number()) refl->SetDouble(msg, fd, val.get<double>()); break;
            case pb::FieldDescriptor::CPPTYPE_BOOL:
                if (val.is_boolean()) refl->SetBool(msg, fd, val.get<bool>()); break;
            case pb::FieldDescriptor::CPPTYPE_STRING:
                if (val.is_string()) refl->SetString(msg, fd, val.get<std::string>()); break;
            case pb::FieldDescriptor::CPPTYPE_MESSAGE: {
                pb::Message* sub = refl->MutableMessage(msg, fd);
                JsonToMessage(val, sub); break;
            }
            default: break;
            }
        }
    }
}

// ============================================================
// 内嵌飞书 pbbp2.Frame schema
// ============================================================
static const char* FEISHU_PBBP2_SCHEMA_NAME = "pbbp2.proto";
static const char* FEISHU_PBBP2_SCHEMA = R"proto(
syntax = "proto3";
package pbbp2;

message Header {
    string key   = 1;
    string value = 2;
}

message Frame {
    repeated Header headers = 1;
    int32  service          = 2;
    int32  method           = 3;
    int64  SeqID            = 4;
    int64  LogID            = 5;
    bytes  payload          = 6;
}
)proto";

bool EnsureFeishuSchema(std::string& err) {
    auto& reg = SchemaRegistry::Get();
    std::lock_guard<std::mutex> lk(reg.mtx);
    if (reg.pool.FindFileByName(FEISHU_PBBP2_SCHEMA_NAME)) return true;

    std::istringstream ss(FEISHU_PBBP2_SCHEMA);
    pb::io::IstreamInputStream iis(&ss);
    pb::io::Tokenizer tok(&iis, nullptr);
    pb::compiler::Parser parser;
    pb::FileDescriptorProto fdp;
    fdp.set_name(FEISHU_PBBP2_SCHEMA_NAME);
    if (!parser.Parse(&tok, &fdp)) { err = "pbbp2 parse failed"; return false; }
    if (!reg.pool.BuildFile(fdp))  { err = "pbbp2 build failed"; return false; }
    return true;
}

} // anonymous namespace

namespace NodeEditor { namespace Runtime {

void RegisterHandlers_Proto(std::unordered_map<std::string, NodeHandler>& handlers)
{
    // ================================================================
    // Proto.LoadSchema
    //   将 proto 源文本注册到全局 schema 注册表
    //   in:  SchemaName(String)  — 唯一标识，如 "my.proto"
    //        ProtoText(String)   — proto 源文本（syntax="proto3";...）
    //   out: onSuccess / onError
    //        ErrorMessage(String)
    // ================================================================
    handlers["Proto.LoadSchema"] = [](ExecutionContext& ctx) -> bool {
        std::string name  = ctx.GetInputValue("SchemaName").asString();
        std::string text  = ctx.GetInputValue("ProtoText").asString();
        if (name.empty() || text.empty()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("SchemaName or ProtoText is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        std::string err;
        const pb::FileDescriptor* fd = RegisterProtoText(name, text, err);
        if (!fd) {
            ctx.SetOutputValue("ErrorMessage", Variant(err));
            ctx.ActivateOutputFlow("onError");
        } else {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
            ctx.ActivateOutputFlow("onSuccess");
        }
        return true;
    };

    // ================================================================
    // Proto.Decode
    //   将 protobuf 二进制数据解码为 JSON 字符串
    //   in:  Data(String/bytes)    — 二进制数据
    //        MessageType(String)   — 完整消息类型名，如 "pbbp2.Frame"
    //                                内置: "pbbp2.Frame"（飞书长连接帧，无需 LoadSchema）
    //   out: JSON(String)          — 解码后的 JSON
    //        onSuccess / onError
    //        ErrorMessage(String)
    // ================================================================
    handlers["Proto.Decode"] = [](ExecutionContext& ctx) -> bool {
        std::string data    = ctx.GetInputValue("Data").asString();
        std::string msgType = ctx.GetInputValue("MessageType").asString();
        if (msgType.empty()) msgType = "pbbp2.Frame";

        // 自动注册内置 schema
        if (msgType == "pbbp2.Frame" || msgType.rfind("pbbp2.", 0) == 0) {
            std::string schemaErr;
            if (!EnsureFeishuSchema(schemaErr)) {
                ctx.SetOutputValue("ErrorMessage", Variant(schemaErr));
                ctx.ActivateOutputFlow("onError");
                return true;
            }
        }

        auto& reg = SchemaRegistry::Get();
        std::lock_guard<std::mutex> lk(reg.mtx);

        const pb::Descriptor* desc = reg.pool.FindMessageTypeByName(msgType);
        if (!desc) {
            ctx.SetOutputValue("ErrorMessage",
                Variant(std::string("Unknown message type: ") + msgType));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        const pb::Message* proto = reg.factory.GetPrototype(desc);
        if (!proto) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("GetPrototype failed")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        std::unique_ptr<pb::Message> msg(proto->New());
        if (!msg->ParseFromString(data)) {
            ctx.SetOutputValue("ErrorMessage",
                Variant(std::string("ParseFromString failed for ") + msgType));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        crude_json::value jv = MessageToJson(*msg);
        ctx.SetOutputValue("JSON",         Variant(jv.dump()));
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
    };

    // ================================================================
    // Proto.Encode
    //   将 JSON 字符串编码为 protobuf 二进制
    //   in:  JSON(String)          — JSON 对象字符串
    //        MessageType(String)   — 完整消息类型名，如 "pbbp2.Frame"
    //   out: Data(String/bytes)    — 序列化后的二进制
    //        onSuccess / onError
    //        ErrorMessage(String)
    // ================================================================
    handlers["Proto.Encode"] = [](ExecutionContext& ctx) -> bool {
        std::string jsonStr = ctx.GetInputValue("JSON").asString();
        std::string msgType = ctx.GetInputValue("MessageType").asString();
        if (msgType.empty()) msgType = "pbbp2.Frame";

        if (msgType == "pbbp2.Frame" || msgType.rfind("pbbp2.", 0) == 0) {
            std::string schemaErr;
            if (!EnsureFeishuSchema(schemaErr)) {
                ctx.SetOutputValue("ErrorMessage", Variant(schemaErr));
                ctx.ActivateOutputFlow("onError");
                return true;
            }
        }

        auto& reg = SchemaRegistry::Get();
        std::lock_guard<std::mutex> lk(reg.mtx);

        const pb::Descriptor* desc = reg.pool.FindMessageTypeByName(msgType);
        if (!desc) {
            ctx.SetOutputValue("ErrorMessage",
                Variant(std::string("Unknown message type: ") + msgType));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        const pb::Message* proto = reg.factory.GetPrototype(desc);
        std::unique_ptr<pb::Message> msg(proto->New());

        crude_json::value jv = crude_json::value::parse(jsonStr);
        if (!jv.is_object()) {
            ctx.SetOutputValue("ErrorMessage",
                Variant(std::string("JSON input is not an object")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        JsonToMessage(jv, msg.get());

        std::string out;
        if (!msg->SerializeToString(&out)) {
            ctx.SetOutputValue("ErrorMessage",
                Variant(std::string("SerializeToString failed")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        ctx.SetOutputValue("Data",         Variant(out));
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
    };

    // ================================================================
    // Proto.GetField
    //   从已解码的 JSON 中提取指定路径的字段
    //   （实际上就是 JSON.GetPath，这里提供一个语义别名方便蓝图组织）
    //   in:  JSON(String), Path(String), Default(Any)
    //   out: Value(Any), Found(Boolean)
    // ================================================================
    handlers["Proto.GetField"] = [](ExecutionContext& ctx) -> bool {
        // 直接复用 JSON.GetPath 语义
        std::string jsonStr = ctx.GetInputValue("JSON").asString();
        std::string path    = ctx.GetInputValue("Path").asString();
        Variant     def     = ctx.GetInputValue("Default");

        if (jsonStr.empty() || path.empty()) {
            ctx.SetOutputValue("Value",  def);
            ctx.SetOutputValue("Found",  Variant(false));
            return true;
        }

        crude_json::value jv = crude_json::value::parse(jsonStr);
        // 支持 dot-path：a.b.c
        crude_json::value* cur = &jv;
        std::istringstream ss(path);
        std::string seg;
        bool found = true;
        while (std::getline(ss, seg, '.')) {
            if (!cur->is_object() || !cur->contains(seg)) { found = false; break; }
            cur = &((*cur)[seg]);
        }

        if (!found) {
            ctx.SetOutputValue("Value", def);
            ctx.SetOutputValue("Found", Variant(false));
        } else {
            std::string val = cur->is_string() ? cur->get<std::string>() : cur->dump();
            ctx.SetOutputValue("Value", Variant(val));
            ctx.SetOutputValue("Found", Variant(true));
        }
        return true;
    };
}

} } // namespace NodeEditor::Runtime

#else // BLUEPRINT_HAS_PROTOBUF not defined

namespace NodeEditor { namespace Runtime {
void RegisterHandlers_Proto(std::unordered_map<std::string, NodeHandler>&) {
    // protobuf not linked, Proto.* nodes silently unavailable
}
} }

#endif
