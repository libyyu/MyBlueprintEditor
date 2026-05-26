# Extract Recent Files / Recent Projects + GetCanonicalPath helper from
# BlueprintEditor/FileOperations.cpp into BlueprintEditor/RecentFiles.cpp.
#
# Source ranges (1-based, inclusive):
#   GetCanonicalPath helper (static)        : 1082-1086
#   AddRecentFile()                         : 1088-1117
#   SaveRecentFiles()                       : 1119-1128
#   LoadRecentFiles()                       : 1130-1166
#   DrawRecentFilesMenu()                   : 1168-1217
#   AddRecentProject()                      : 1219-1232
#   SaveRecentProjects()                    : 1234-1242
#   LoadRecentProjects()                    : 1244-1266
#   DrawRecentProjectsMenu()                : 1268-1317
#
# Block to extract (continuous): 1082-1317  (236 lines)
# After extraction, FileOperations.cpp lines 1081-1318 collapse to just a
# single comment "// Recent files & recent projects moved to RecentFiles.cpp".
#
# No CMakeLists edit needed (file(GLOB_RECURSE) BlueprintEditor/*.cpp).

$src = "BlueprintEditor/FileOperations.cpp"
$dst = "BlueprintEditor/RecentFiles.cpp"

$bytes = [System.IO.File]::ReadAllBytes($src)
$text  = [System.Text.Encoding]::UTF8.GetString($bytes)
$lines = $text -split "`r?`n"
$N = $lines.Length
Write-Host "Loaded $src : $N lines"

$blkStart = 1082
$blkEnd   = 1313

$body = ($lines[($blkStart-1)..($blkEnd-1)] -join "`n")

$dstHeader = @"
// RecentFiles.cpp -- Recent Files & Recent Projects history (file menu helpers).
// Extracted from FileOperations.cpp by tools/split_recentfiles.ps1.
//
// Methods (all BlueprintEditor:: members):
//   - AddRecentFile / SaveRecentFiles / LoadRecentFiles / DrawRecentFilesMenu
//   - AddRecentProject / SaveRecentProjects / LoadRecentProjects / DrawRecentProjectsMenu
//
// Local helper:
//   - GetCanonicalPath (static) -- path normalization for duplicate-detection.

#include "BlueprintEditor.h"
#include "PathUtils.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;


"@

$dstContent = $dstHeader + $body + "`n"
[System.IO.File]::WriteAllText($dst, $dstContent, [System.Text.UTF8Encoding]::new($false))
$dstLineCount = ([System.Text.Encoding]::UTF8.GetString([System.IO.File]::ReadAllBytes($dst)) -split "`r?`n").Length
Write-Host ("  Wrote {0,-45} {1,4} lines" -f $dst, $dstLineCount)

# Rewrite FileOperations.cpp: keep 1..1080, insert placeholder comment, keep 1318..N
$kept = @()
$kept += $lines[0..1080]   # lines 1..1081
$kept += ""
$kept += "// ============================================================================"
$kept += "// Recent Files / Recent Projects (history menus) -- moved to RecentFiles.cpp"
$kept += "// ============================================================================"
$kept += ""
$kept += $lines[1313..($N-1)]  # lines 1314..N (the SaveDialog section header onward)

$newText = ($kept -join "`n")
[System.IO.File]::WriteAllText($src, $newText, [System.Text.UTF8Encoding]::new($false))
$newCount = ([System.Text.Encoding]::UTF8.GetString([System.IO.File]::ReadAllBytes($src)) -split "`r?`n").Length
Write-Host "Updated ${src}: $N -> $newCount lines"
