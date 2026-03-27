// BlueprintEditor/BpProject.h
// 蓝图工程（*.bp.proj）— 仅编辑器层，Runtime 不依赖此文件
#pragma once
#include <string>
#include <vector>

// ============================================================================
// 蓝图工程数据结构
// ============================================================================

struct BpProjectEntry
{
    std::string relativePath;   // 相对于 .bp.proj 文件的路径，如 "actors/Player.bp.json"
    std::string displayName;    // 可选显示名（空则取文件名）
};

struct BpProject
{
    std::string                 name;           // 工程名
    std::string                 description;
    std::string                 version = "1.0";
    int                         projVersion = 1;// .bp.proj 格式版本
    std::vector<BpProjectEntry> blueprints;     // Actor 蓝图列表（blueprintClass=0）
    std::vector<BpProjectEntry> libraries;      // Library 蓝图列表（blueprintClass=1）

    // ── 运行时辅助（不序列化）──────────────────────────────────────────────
    std::string                 filePath;       // 当前已打开的工程文件绝对路径
    std::string                 projectDir;     // filePath 所在目录

    bool IsOpen() const { return !filePath.empty(); }

    /// 将相对路径转换为绝对路径（基于 projectDir）
    std::string AbsPath(const std::string& relPath) const;

    /// 将绝对路径转换为相对路径（基于 projectDir）
    std::string RelPath(const std::string& absPath) const;
};

// ============================================================================
// 工程序列化 / 反序列化
// ============================================================================

/// 将工程保存到 .bp.proj 文件，成功返回 true
bool SaveBpProject(const BpProject& proj, const std::string& filePath);

/// 从 .bp.proj 文件加载工程，成功返回 true；filePath 会自动写入 proj.filePath
bool LoadBpProject(BpProject& proj, const std::string& filePath);

/// 新建一个空工程（只填名称，blueprints/libraries 为空）
BpProject NewBpProject(const std::string& name);
