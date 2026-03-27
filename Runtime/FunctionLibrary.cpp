// Runtime/FunctionLibrary.cpp - 公共函数库加载器实现

#include "FunctionLibrary.h"
#include "BlueprintData.h"
#include "BlueprintExporter.h"
#include "NodeDefinition.h"

#include <string>
#include <vector>
#include <iostream>

#ifndef __EMSCRIPTEN__
#  include <filesystem>
   namespace fs = std::filesystem;
#endif

namespace NodeEditor {
namespace Runtime {

int LoadFunctionLibrary(INodeRegistry& registry, const std::string& dirPath)
{
#ifdef __EMSCRIPTEN__
    // WebGL/Emscripten 环境不支持文件系统扫描，静默返回
    (void)registry; (void)dirPath;
    return 0;
#else
    std::error_code ec;
    if (!fs::is_directory(dirPath, ec))
        return 0; // 目录不存在，静默返回

    int count = 0;
    JsonBlueprintExporter exporter;

    for (const auto& entry : fs::directory_iterator(dirPath, ec))
    {
        if (ec) break;
        if (!entry.is_regular_file()) continue;

        auto p = entry.path();
        if (p.extension() != ".json") continue;

        // 加载蓝图文件
        auto result = exporter.importRuntimeFromFile(p.string());
        if (!result.success)
        {
            std::cerr << "[FunctionLibrary] Failed to load: " << p.string()
                      << " - " << result.errorMessage << "\n";
            continue;
        }

        // 获取文件名（不含扩展名）
        std::string stem = p.stem().string();
        // 去掉 .bp 前缀（如果文件名是 xxx.bp.json，stem = xxx.bp）
        if (stem.size() > 3 && stem.substr(stem.size() - 3) == ".bp")
            stem = stem.substr(0, stem.size() - 3);

        // 注册 isPublic=true 的函数
        for (const auto& funcDef : result.data.functions)
        {
            if (!funcDef.isPublic) continue;

            NodeDefinition nodeDef;
            nodeDef.id       = "FuncLib." + stem + "." + funcDef.id;
            nodeDef.name     = funcDef.name;
            nodeDef.category = "FunctionLibrary/" + funcDef.category;

            // 根据函数 inputs/outputs 构建引脚定义
            {
                PinDefinition flowIn;
                flowIn.name   = "";
                flowIn.isExec = true;
                flowIn.dataType = PinDataType::Unknown;
                nodeDef.inputPins.push_back(flowIn);

                for (const auto& inp : funcDef.inputs)
                {
                    PinDefinition pin;
                    pin.name     = inp.name;
                    pin.dataType = inp.dataType;
                    nodeDef.inputPins.push_back(pin);
                }
            }
            {
                PinDefinition flowOut;
                flowOut.name   = "";
                flowOut.isExec = true;
                flowOut.dataType = PinDataType::Unknown;
                nodeDef.outputPins.push_back(flowOut);

                for (const auto& out : funcDef.outputs)
                {
                    PinDefinition pin;
                    pin.name     = out.name;
                    pin.dataType = out.dataType;
                    nodeDef.outputPins.push_back(pin);
                }
            }

            registry.registerNode(nodeDef);
            ++count;
        }
    }
    return count;
#endif
}

} // namespace Runtime
} // namespace NodeEditor
