// Runtime/StringPool.h - 字符串驻留池
//
// 相同字符串只存一份，返回稳定的 const char* 指针。
// 线程安全（内部 mutex 保护）。

#pragma once
#include "BlueprintExport.h"
#include <string>
#include <unordered_set>
#include <mutex>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

namespace NodeEditor {
namespace Runtime {

/// 字符串驻留池：相同字符串只存一份，返回稳定的 const char* 指针。
/// 指针在 StringPool 存活期间始终有效（不会因为插入新字符串而失效）。
/// 线程安全（内部加锁）。
class BLUEPRINT_API StringPool
{
public:
    static StringPool& Get();

    /// 驻留字符串，返回稳定指针（池存活期间有效）
    const char* Intern(const std::string& s);
    const char* Intern(const char* s);

    /// 查询池大小（用于调试）
    size_t Size() const;

    /// 清空池（一般不需要调用，仅测试用）
    void Clear();

private:
    StringPool() = default;

    std::unordered_set<std::string> m_pool;
    mutable std::mutex              m_mutex;
};

} // namespace Runtime
} // namespace NodeEditor

#ifdef _MSC_VER
#pragma warning(pop)
#endif
