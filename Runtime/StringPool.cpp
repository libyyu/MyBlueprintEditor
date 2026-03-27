// Runtime/StringPool.cpp - 字符串驻留池实现

#include "StringPool.h"
#include "Types.h"

namespace NodeEditor {
namespace Runtime {

StringPool& StringPool::Get()
{
    static StringPool instance;
    return instance;
}

const char* StringPool::Intern(const std::string& s)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_pool.insert(s);
    return it.first->c_str();
}

const char* StringPool::Intern(const char* s)
{
    if (!s) return "";
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_pool.insert(std::string(s));
    return it.first->c_str();
}

size_t StringPool::Size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pool.size();
}

void StringPool::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pool.clear();
}

// ── Variant::internedString() ────────────────────────────────────────────────
// 此函数在此处实现，因为 StringPool.h 已被 StringPool.cpp 包含，
// 而 Types.h 中只有声明（避免 Types.h -> StringPool.h 的循环依赖）。

const char* Variant::internedString() const
{
    return StringPool::Get().Intern(asString());
}

} // namespace Runtime
} // namespace NodeEditor
