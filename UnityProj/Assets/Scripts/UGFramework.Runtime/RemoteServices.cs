// RemoteServices.cs
// YooAsset 远端服务实现 — 提供 CDN 资源地址解析
// 支持主包（DefaultPackage）和 DLC 分包（DlcChapterXX）

using YooAsset;

namespace UGFramework.Runtime
{
    /// <summary>
    /// 远端资源服务：把 packageName + fileName 拼成 CDN URL
    /// CDN 目录约定：{BaseUrl}/{PackageName}/{FileName}
    /// </summary>
    public class RemoteServices : IRemoteServices
    {
        private readonly string _baseUrl;
        private readonly string _fallbackUrl;

        /// <param name="baseUrl">主 CDN，如 https://cdn.example.com/res</param>
        /// <param name="fallbackUrl">备用 CDN，可为空</param>
        public RemoteServices(string baseUrl, string fallbackUrl = "")
        {
            _baseUrl    = baseUrl.TrimEnd('/');
            _fallbackUrl = string.IsNullOrEmpty(fallbackUrl) ? baseUrl.TrimEnd('/') : fallbackUrl.TrimEnd('/');
        }

        public string GetRemoteMainURL(string fileName)
        {
            return $"{_baseUrl}/{fileName}";
        }

        public string GetRemoteFallbackURL(string fileName)
        {
            return $"{_fallbackUrl}/{fileName}";
        }
    }
}
