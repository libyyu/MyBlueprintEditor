# ============================================================================
# Application — 应用框架（平台抽象层 + 渲染后端）
# ============================================================================
#
# 本目录的构建由根 CMakeLists.txt 直接管理。
#
# 提供 Application 基类：
#   - OnStart() / OnStop() / OnFrame(deltaTime) 生命周期
#   - 纹理加载 / 字体管理
#   - 窗口创建与主循环
#
# 平台后端：
#   - Win32 + DirectX 11
#   - GLFW + OpenGL 3
#
# 入口点：
#   entry_point.cpp 定义 WinMain / main，调用 Main(argc, argv)
#   各应用程序实现 int Main(int argc, char** argv) 函数
