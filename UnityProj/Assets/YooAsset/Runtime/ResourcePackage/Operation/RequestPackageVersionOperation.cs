namespace YooAsset
{
    public abstract class RequestPackageVersionOperation : AsyncOperationBase
    {
        /// <summary>
        /// 当前最新的包裹版本
        /// </summary>
        public string PackageVersion { protected set; get; }
    }

    /// <summary>
    /// 请求包裹版本号，支持弱联网 fallback：
    ///   1. 优先走主 FS（CacheFS）联网拉版本
    ///   2. 主 FS 失败 → 自动回退 BuildinFS 读内置版本
    ///   3. 两者都失败 → 整体失败
    /// 适用于 HostPlayMode（BuildinFS + CacheFS 双文件系统）。
    /// 对单 FS 模式（OfflineMode / EditorMode）行为与之前完全相同。
    /// </summary>
    internal sealed class RequestPackageVersionImplOperation : RequestPackageVersionOperation
    {
        private enum ESteps
        {
            None,
            RequestMainVersion,
            RequestBuildinFallback,
            Done,
        }

        private readonly PlayModeImpl _impl;
        private readonly bool _appendTimeTicks;
        private readonly int _timeout;
        private FSRequestPackageVersionOperation _mainVersionOp;
        private FSRequestPackageVersionOperation _buildinVersionOp;
        private ESteps _steps = ESteps.None;

        internal RequestPackageVersionImplOperation(PlayModeImpl impl, bool appendTimeTicks, int timeout)
        {
            _impl = impl;
            _appendTimeTicks = appendTimeTicks;
            _timeout = timeout;
        }

        internal override void InternalStart()
        {
            _steps = ESteps.RequestMainVersion;
        }

        internal override void InternalUpdate()
        {
            if (_steps == ESteps.None || _steps == ESteps.Done)
                return;

            // ── Step 1：向主 FS（CacheFS）请求版本 ────────────────────
            if (_steps == ESteps.RequestMainVersion)
            {
                if (_mainVersionOp == null)
                {
                    var mainFS = _impl.GetMainFileSystem();
                    _mainVersionOp = mainFS.RequestPackageVersionAsync(_appendTimeTicks, _timeout);
                    _mainVersionOp.StartOperation();
                    AddChildOperation(_mainVersionOp);
                }

                _mainVersionOp.UpdateOperation();
                if (_mainVersionOp.IsDone == false)
                    return;

                if (_mainVersionOp.Status == EOperationStatus.Succeed)
                {
                    _steps = ESteps.Done;
                    PackageVersion = _mainVersionOp.PackageVersion;
                    Status = EOperationStatus.Succeed;
                    return;
                }

                // 主 FS 失败：判断是否有 Buildin FS 可回退
                var buildinFS = _impl.GetBuildinFileSystem();
                bool hasFallback = buildinFS != null && !ReferenceEquals(buildinFS, _impl.GetMainFileSystem());
                if (hasFallback)
                {
                    YooLogger.Warning(
                        $"[RequestPackageVersion] Main FS failed ({_mainVersionOp.Error}), " +
                        $"falling back to BuildinFS.");
                    _steps = ESteps.RequestBuildinFallback;
                }
                else
                {
                    // 单 FS 模式，直接失败
                    _steps = ESteps.Done;
                    Status = EOperationStatus.Failed;
                    Error = _mainVersionOp.Error;
                }
            }

            // ── Step 2：回退到 BuildinFS 读内置版本 ───────────────────
            if (_steps == ESteps.RequestBuildinFallback)
            {
                if (_buildinVersionOp == null)
                {
                    var buildinFS = _impl.GetBuildinFileSystem();
                    // BuildinFS 的 appendTimeTicks / timeout 参数无意义（本地读文件）
                    _buildinVersionOp = buildinFS.RequestPackageVersionAsync(false, _timeout);
                    _buildinVersionOp.StartOperation();
                    AddChildOperation(_buildinVersionOp);
                }

                _buildinVersionOp.UpdateOperation();
                if (_buildinVersionOp.IsDone == false)
                    return;

                _steps = ESteps.Done;
                if (_buildinVersionOp.Status == EOperationStatus.Succeed)
                {
                    PackageVersion = _buildinVersionOp.PackageVersion;
                    Status = EOperationStatus.Succeed;
                    YooLogger.Log(
                        $"[RequestPackageVersion] Buildin fallback succeeded, version={PackageVersion}");
                }
                else
                {
                    Status = EOperationStatus.Failed;
                    Error = $"Main: {_mainVersionOp.Error} | Buildin: {_buildinVersionOp.Error}";
                }
            }
        }
    }
}
