// main.cpp -- 蓝图编辑器入口
#include "BlueprintEditor.h"

int Main(int argc, char** argv)
{
    BlueprintEditor editor("Blueprint Editor", argc, argv);

    if (editor.Create())
        return editor.Run();

    return 0;
}
