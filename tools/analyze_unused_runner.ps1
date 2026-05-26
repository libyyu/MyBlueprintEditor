# tools/analyze_unused_runner.ps1
# 分析 handlers 中 "auto* runner = ctx.GetRunner(); (void)runner;" 后
# 实际使用 runner 与否，定位可移除的死代码模板行
#
# 输出每个文件中 unused（可删）vs used（保留）的统计。

$ErrorActionPreference = 'Stop'
$pattern = 'auto\* runner = ctx\.GetRunner\(\); \(void\)runner;'
$lambdaEnd = '^\s*\};\s*$'

$results = @()
foreach ($f in Get-ChildItem "$PSScriptRoot/../Runtime/handlers/*.cpp") {
    $bytes = [System.IO.File]::ReadAllBytes($f.FullName)
    $text  = [System.Text.Encoding]::UTF8.GetString($bytes)
    $lines = $text -split "`r?`n"
    $used = 0; $unused = 0; $unusedLines = @()
    for ($i = 0; $i -lt $lines.Length; $i++) {
        if ($lines[$i] -notmatch $pattern) { continue }
        # 找出该 lambda 的结束行 "};"
        $j = $i + 1
        $endLine = $lines.Length - 1
        while ($j -lt [Math]::Min($lines.Length, $i + 300)) {
            if ($lines[$j] -match $lambdaEnd) { $endLine = $j; break }
            $j++
        }
        $body = ($lines[($i+1)..$endLine] -join "`n")
        # 是否在 body 中真的引用 runner（变量名）？
        if ($body -match '\brunner\b') {
            $used++
        } else {
            $unused++
            $unusedLines += ($i + 1)  # 1-based
        }
    }
    $results += [PSCustomObject]@{
        File = $f.Name
        Total = $used + $unused
        Used = $used
        Unused = $unused
        UnusedLines = ($unusedLines -join ',')
    }
}

$results | Sort-Object Unused -Descending | Format-Table File,Total,Used,Unused -AutoSize
""
"=== Summary ==="
$totalUsed   = ($results | Measure-Object Used -Sum).Sum
$totalUnused = ($results | Measure-Object Unused -Sum).Sum
"Total handlers with template line: $($totalUsed + $totalUnused)"
"  - Actually uses runner: $totalUsed"
"  - Dead code (unused):   $totalUnused"
