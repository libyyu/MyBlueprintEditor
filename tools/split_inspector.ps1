# Split BlueprintEditor/InspectorPanel.cpp (2313 lines) into 4 per-panel files under inspector/.
#
# Source method ranges (1-based, inclusive):
#   DrawNodeListPanel(int)                : 11-668     -> inspector/NodeListPanel.cpp
#   DrawVariablePanel()                   : 674-1399   -> inspector/VariablePanel.cpp
#   DrawDetailsPanel()                    : 1405-1751  -> inspector/DetailsPanel.cpp
#   DrawFunctionDetailsPanel(...) +       : 1757-2172
#   SyncFunctionPinsToNodes(...)          : 2177-2313  -> inspector/FunctionDetailsPanel.cpp
#
# All four methods are BlueprintEditor:: members; their bodies translate verbatim
# to the new TUs (no friend / no internal helpers needed).
#
# After extraction, InspectorPanel.cpp is deleted (becomes empty stub otherwise).
# CMakeLists uses file(GLOB_RECURSE) on BlueprintEditor/*.cpp so the new sub-files
# under inspector/ are auto-picked up — no CMakeLists edit needed.

$src = "BlueprintEditor/InspectorPanel.cpp"
$bytes = [System.IO.File]::ReadAllBytes($src)
$text  = [System.Text.Encoding]::UTF8.GetString($bytes)
$lines = $text -split "`r?`n"
$N = $lines.Length
Write-Host "Loaded $src : $N lines"

# Inclusive 1-based ranges
$rNodeList    = @{Start=11;   End=668}
$rVariable    = @{Start=674;  End=1399}
$rDetails     = @{Start=1405; End=1751}
$rFuncDetails = @{Start=1757; End=2313}   # includes SyncFunctionPinsToNodes too

function Slice($s, $e) { return ($lines[($s-1)..($e-1)] -join "`n") }

$includesBlock = @"
// Auto-generated from InspectorPanel.cpp by tools/split_inspector.ps1
// Implements BlueprintEditor::{0} (member of the editor class declared in BlueprintEditor.h).
#include "BlueprintEditor.h"
#include "ThemeManager.h"
#include "../Utils/Json/crude_json.h"

"@

function WriteFile {
    param([string]$path, [string]$methodName, [string]$body)
    if (-not (Test-Path "BlueprintEditor/inspector")) {
        New-Item -Path "BlueprintEditor/inspector" -ItemType Directory | Out-Null
    }
    $hdr = $includesBlock -f $methodName
    $content = $hdr + $body + "`n"
    [System.IO.File]::WriteAllText($path, $content, [System.Text.UTF8Encoding]::new($false))
    $bc = [System.IO.File]::ReadAllBytes($path)
    $lc = ([System.Text.Encoding]::UTF8.GetString($bc) -split "`r?`n").Length
    Write-Host ("  Wrote {0,-50} {1,4} lines" -f $path, $lc)
}

WriteFile "BlueprintEditor/inspector/NodeListPanel.cpp"        "DrawNodeListPanel"        (Slice $rNodeList.Start    $rNodeList.End)
WriteFile "BlueprintEditor/inspector/VariablePanel.cpp"        "DrawVariablePanel"        (Slice $rVariable.Start    $rVariable.End)
WriteFile "BlueprintEditor/inspector/DetailsPanel.cpp"         "DrawDetailsPanel"         (Slice $rDetails.Start     $rDetails.End)
WriteFile "BlueprintEditor/inspector/FunctionDetailsPanel.cpp" "DrawFunctionDetailsPanel + SyncFunctionPinsToNodes" (Slice $rFuncDetails.Start $rFuncDetails.End)

# Delete the now-empty source
Remove-Item $src -Force
Write-Host "Deleted $src (all 5 methods moved to inspector/)."
