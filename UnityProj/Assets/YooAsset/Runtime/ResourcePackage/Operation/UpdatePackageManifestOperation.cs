namespace YooAsset
{
    /// <summary>
    /// 更新资源清单，支持弱联网 fallback：
    ///   1. 优先走主 FS（CacheFS）：下载 .hash + .manifest 并缓存
    ///   2. 主 FS 失败 → 自动回退 BuildinFS 读内置 Manifest
    ///   3. 两者都失败 → 整体失败
    /// 适用于 HostPlayMode（BuildinFS + CacheFS 双文件系统）。
    /// 对单 FS 模式行为与之前完全相同。
    /// </summary>
    public sealed class UpdatePackageManifestOperation : AsyncOperationBase
    {
        private enum ESteps
        {
            None,
            CheckParams,
            CheckActiveManifest,
            LoadMainManifest,
            LoadBuildinFallback,
            Done,
        }

        private readonly PlayModeImpl _impl;
        private readonly string _packageVersion;
        private readonly int _timeout;
        private FSLoadPackageManifestOperation _mainManifestOp;
        private FSLoadPackageManifestOperation _buildinManifestOp;
        private ESteps _steps = ESteps.None;

        internal UpdatePackageManifestOperation(PlayModeImpl impl, string packageVersion, int timeout)
        {
            _impl = impl;
            _packageVersion = packageVersion;
            _timeout = timeout;
        }
        internal override void InternalStart()
        {
            _steps = ESteps.CheckParams;
        }
        internal override void InternalUpdate()
        {
            if (_steps == ESteps.None || _steps == ESteps.Done)
                return;

            if (_steps == ESteps.CheckParams)
            {
                if (string.IsNullOrEmpty(_packageVersion))
                {
                    _steps = ESteps.Done;
                    Status = EOperationStatus.Failed;
                    Error = "Package version is null or empty.";
                }
                else
                {
                    _steps = ESteps.CheckActiveManifest;
                }
            }

            if (_steps == ESteps.CheckActiveManifest)
            {
                // 当前已激活的 Manifest 版本与目标版本一致，直接复用
                if (_impl.ActiveManifest != null && _impl.ActiveManifest.PackageVersion == _packageVersion)
                {
                    _steps = ESteps.Done;
                    Status = EOperationStatus.Succeed;
                }
                else
                {
                    _steps = ESteps.LoadMainManifest;
                }
            }

            // ── Step 1：向主 FS（CacheFS）加载 Manifest ────────────────
            if (_steps == ESteps.LoadMainManifest)
            {
                if (_mainManifestOp == null)
                {
                    var mainFS = _impl.GetMainFileSystem();
                    _mainManifestOp = mainFS.LoadPackageManifestAsync(_packageVersion, _timeout);
                    _mainManifestOp.StartOperation();
                    AddChildOperation(_mainManifestOp);
                }

                _mainManifestOp.UpdateOperation();
                Progress = _mainManifestOp.Progress;
                if (_mainManifestOp.IsDone == false)
                    return;

                if (_mainManifestOp.Status == EOperationStatus.Succeed)
                {
                    _steps = ESteps.Done;
                    _impl.ActiveManifest = _mainManifestOp.Manifest;
                    Status = EOperationStatus.Succeed;
                    return;
                }

                // 主 FS 失败：判断是否有 BuildinFS 可回退
                var buildinFS = _impl.GetBuildinFileSystem();
                bool hasFallback = buildinFS != null && !ReferenceEquals(buildinFS, _impl.GetMainFileSystem());
                if (hasFallback)
                {
                    YooLogger.Warning(
                        $"[UpdatePackageManifest] Main FS failed ({_mainManifestOp.Error}), " +
                        $"falling back to BuildinFS for version={_packageVersion}");
                    _steps = ESteps.LoadBuildinFallback;
                }
                else
                {
                    _steps = ESteps.Done;
                    Status = EOperationStatus.Failed;
                    Error = _mainManifestOp.Error;
                }
            }

            // ── Step 2：回退到 BuildinFS 读内置 Manifest ──────────────
            if (_steps == ESteps.LoadBuildinFallback)
            {
                if (_buildinManifestOp == null)
                {
                    var buildinFS = _impl.GetBuildinFileSystem();
                    _buildinManifestOp = buildinFS.LoadPackageManifestAsync(_packageVersion, _timeout);
                    _buildinManifestOp.StartOperation();
                    AddChildOperation(_buildinManifestOp);
                }

                _buildinManifestOp.UpdateOperation();
                Progress = _buildinManifestOp.Progress;
                if (_buildinManifestOp.IsDone == false)
                    return;

                _steps = ESteps.Done;
                if (_buildinManifestOp.Status == EOperationStatus.Succeed)
                {
                    _impl.ActiveManifest = _buildinManifestOp.Manifest;
                    Status = EOperationStatus.Succeed;
                    YooLogger.Log(
                        $"[UpdatePackageManifest] Buildin fallback succeeded, version={_packageVersion}");
                }
                else
                {
                    Status = EOperationStatus.Failed;
                    Error = $"Main: {_mainManifestOp.Error} | Buildin: {_buildinManifestOp.Error}";
                }
            }
        }
        internal override string InternalGetDesc()
        {
            return $"PackageVersion : {_packageVersion}";
        }
    }
}
