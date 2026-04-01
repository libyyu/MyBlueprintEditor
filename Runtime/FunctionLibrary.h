// Runtime/FunctionLibrary.h - 公共函数库加载器
//
// 扫描指定目录下的 *.bjson 文件，把 isPublic=true 的函数注册为节点。
// 注册节点 id:  "FuncLib.<filename_without_ext>.<func.id>"
// 注册节点 name: func.name
// 注册节点 category: "FunctionLibrary/" + func.category

#pragma once
#include "BlueprintExport.h"
#include "NodeDefinition.h"
#include "BlueprintData.h"
#include <string>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

namespace NodeEditor {
namespace Runtime {

/// 加载函数库目录，返回成功注册的函数数量。
/// 目录不存在或为空时静默返回 0。
BLUEPRINT_API int LoadFunctionLibrary(INodeRegistry& registry, const std::string& dirPath);

/// 将一个已解析的 Library BlueprintData 中的公开函数直接注册到 INodeRegistry（节点定义）。
/// libFilePath 用于唯一前缀生成（可为空则用 data.metadata.name）。
BLUEPRINT_API int RegisterLibraryFunctions(INodeRegistry& registry,
                                            const BlueprintData& data,
                                            const std::string& libFilePath = "");

} // namespace Runtime
} // namespace NodeEditor

#ifdef _MSC_VER
#pragma warning(pop)
#endif
