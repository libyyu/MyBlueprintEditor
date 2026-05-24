// main.cpp -- 蓝图编辑器入口
#include "BlueprintEditor.h"
#include "CrashHandler.h"

int Main(int argc, char** argv)
{
#if defined(_WIN32) || defined(_WIN64)
	// 设置控制台输出为 UTF-8，避免中文乱码
	SetConsoleOutputCP(65001);
	SetConsoleCP(65001);
#endif
    // 安装崩溃处理器：崩溃时自动生成 .dmp 文件到 crashes/ 目录
    CrashHandler::Install();

    BlueprintEditor editor("Blueprint Editor", argc, argv);
    editor.m_Argc = argc;
    editor.m_Argv = argv;

    if (editor.Create())
        return editor.Run();

    return 0;
}
