// BlueprintEditor/FileDialogs.h
// 平台相关文件对话框自由函数声明
#pragma once
#include <string>

std::string OpenFileDialog(const char* filter, const char* title);
std::string SaveFileDialog(const char* filter, const char* title, const char* defaultName);
