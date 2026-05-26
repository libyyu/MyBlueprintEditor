# tools/remove_unused_runner.ps1
# 删除 handlers 中未实际使用 runner 的 "auto* runner = ctx.GetRunner(); (void)runner;" 行
#
# 安全策略：
#   - 仅当 lambda body 中不出现独立单词 "runner" 时才删除模板行
#   - 用 BLUEPRINT_HAS_LUA 等 #ifdef 包裹的代码块仍可能触发 false-positive，
#     因此运行后必须立即跑构建 + 测试验证

$ErrorActionPreference = 'Stop'
$pattern = 'auto\* runner = ctx\.GetRunner\(\); \(void\)runner;'
$lambdaEnd = '^\s*\};\s*$'

$totalRemoved = 0
foreach ($f in Get-ChildItem "$PSScriptRoot/../Runtime/handlers/*.cpp") {
    $bytes = [System.IO.File]::ReadAllBytes($f.FullName)
    $text  = [System.Text.Encoding]::UTF8.GetString($bytes)
    $lines = $text -split "`r?`n"
    $keep = New-Object 'System.Collections.Generic.List[bool]'
    for ($i = 0; $i -lt $lines.Length; $i++) { $keep.Add($true) }

    $localRemoved = 0
    for ($i = 0; $i -lt $lines.Length; $i++) {
        if ($lines[$i] -notmatch $pattern) { continue }
        $j = $i + 1
        $endLine = $lines.Length - 1
        while ($j -lt [Math]::Min($lines.Length, $i + 300)) {
            if ($lines[$j] -match $lambdaEnd) { $endLine = $j; break }
            $j++
        }
        $body = ($lines[($i+1)..$endLine] -join "`n")
        if ($body -notmatch '\brunner\b') {
            $keep[$i] = $false
            $localRemoved++
        }
    }

    if ($localRemoved -gt 0) {
        $newLines = @()
        for ($i = 0; $i -lt $lines.Length; $i++) {
            if ($keep[$i]) { $newLines += $lines[$i] }
        }
        $newText = $newLines -join "`n"
        [System.IO.File]::WriteAllText($f.FullName, $newText, [System.Text.UTF8Encoding]::new($false))
        Write-Host "  $($f.Name): removed $localRemoved lines"
        $totalRemoved += $localRemoved
    }
}
""
"Total lines removed: $totalRemoved"
