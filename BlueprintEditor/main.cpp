// main.cpp -- 蓝图编辑器入口
#include "BlueprintEditor.h"
#include "CrashHandler.h"

int Main(int argc, char** argv)
{
    // 安装崩溃处理器：崩溃时自动生成 .dmp 文件到 crashes/ 目录
    CrashHandler::Install();

    BlueprintEditor editor("Blueprint Editor", argc, argv);

    if (editor.Create())
        return editor.Run();

    return 0;
}
