// Runtime/Http/IHttpClient.cpp
// 全局 HttpClient 注册实现
#include "IHttpClient.h"
#include <memory>

namespace NodeEditor {
namespace Runtime {

static std::shared_ptr<IHttpClient> g_httpClient;

void BP_SetHttpClient(std::shared_ptr<IHttpClient> client)
{
    g_httpClient = std::move(client);
}

IHttpClient* BP_GetHttpClient()
{
    return g_httpClient.get();
}

} // namespace Runtime
} // namespace NodeEditor
