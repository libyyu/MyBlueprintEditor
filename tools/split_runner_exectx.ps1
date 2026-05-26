# tools/split_runner_exectx.ps1
# Extract all ExecutionContext:: method implementations from
# Runtime/BlueprintRunner.cpp into a new Runtime/BlueprintRunner_ExecutionContext.cpp
#
# This continues the BlueprintRunner.cpp slimming after the Lua extraction:
#   BlueprintRunner.cpp           — core (load, execute, control flow, topology, ...)
#   BlueprintRunner_Lua.cpp       — Lua extension methods (BLUEPRINT_HAS_LUA only)
#   BlueprintRunner_ExecutionContext.cpp  — all ExecutionContext:: methods (NEW)
#
# Three contiguous blocks are moved:
#   Block A  1590..1601  FireConnectedNode + PauseRunner
#   Block B  1754..2071  Timer/Log/Print/Delay + RunAsync + SetTimer + ActivateOutputFlow + MarkDownstreamAsHandled
#   Block C  2430..2464  EvaluateConditionPin (+ preceding two-line comment block)
#
# Line numbers are 1-based as displayed by the editor.

$ErrorActionPreference = 'Stop'

$src = "$PSScriptRoot/../Runtime/BlueprintRunner.cpp"
$dst = "$PSScriptRoot/../Runtime/BlueprintRunner_ExecutionContext.cpp"

$bytes = [System.IO.File]::ReadAllBytes($src)
$text  = [System.Text.Encoding]::UTF8.GetString($bytes)
$lines = $text -split "`r?`n"
$N = $lines.Length

# Block boundaries (1-based, inclusive)
$blockA = @{ Start = 1590; End = 1601 }
$blockB = @{ Start = 1754; End = 2071 }
$blockC = @{ Start = 2430; End = 2464 }

function GetBlock($b) {
    $startIdx = $b.Start - 1
    $endIdx   = $b.End   - 1
    return ($lines[$startIdx..$endIdx] -join "`n")
}

# ===== 1. Write new file =====
$header = @"
// Runtime/BlueprintRunner_ExecutionContext.cpp
// All ExecutionContext:: method implementations.
//
// Extracted from BlueprintRunner.cpp to keep that file focused on the core
// BlueprintRunner:: methods (loading, execution, topology, control flow).
//
// ExecutionContext is BlueprintRunner's friend, so accessing private members
// (m_blueprint, m_state, executeNodeInternal, propagatePinValues, ...) from
// this TU works without further qualification.

#include "BlueprintRunner.h"
#include <cstdio>   // fprintf fallback inside Log/Print

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// FireConnectedNode + PauseRunner (was at line ~1590 in BlueprintRunner.cpp)
// ============================================================================

"@
$header += (GetBlock $blockA)
$header += @"


// ============================================================================
// Timer context helpers + Log/Print/Delay + RunAsync + SetTimer + ActivateOutputFlow
// (was at lines 1754..2071 in BlueprintRunner.cpp)
// ============================================================================

"@
$header += (GetBlock $blockB)
$header += @"


// ============================================================================
// EvaluateConditionPin (was at line ~2430 in BlueprintRunner.cpp)
// ============================================================================

"@
$header += (GetBlock $blockC)
$header += @"


} // namespace Runtime
} // namespace NodeEditor
"@

[System.IO.File]::WriteAllText($dst, $header, [System.Text.UTF8Encoding]::new($false))
Write-Host "Wrote $dst ($(($header -split "`n").Length) lines)"

# ===== 2. Remove the three blocks from BlueprintRunner.cpp =====
# Build a keep[] mask
$keep = New-Object 'System.Collections.Generic.List[bool]'
for ($i = 0; $i -lt $N; $i++) { $keep.Add($true) }

foreach ($b in @($blockA, $blockB, $blockC)) {
    for ($i = $b.Start - 1; $i -le $b.End - 1; $i++) { $keep[$i] = $false }
}

$newLines = New-Object 'System.Collections.Generic.List[string]'
$placeholders = @{
    1590 = "// FireConnectedNode + PauseRunner moved to BlueprintRunner_ExecutionContext.cpp"
    1754 = "// Timer helpers + Log/Print/Delay + RunAsync + SetTimer + ActivateOutputFlow + MarkDownstreamAsHandled`n// moved to BlueprintRunner_ExecutionContext.cpp"
    2430 = "// EvaluateConditionPin moved to BlueprintRunner_ExecutionContext.cpp"
}

for ($i = 0; $i -lt $N; $i++) {
    if ($keep[$i]) {
        $newLines.Add($lines[$i])
    } elseif ($placeholders.ContainsKey($i + 1)) {
        $newLines.Add($placeholders[$i + 1])
    }
    # otherwise drop the line
}

$newText = ($newLines.ToArray() -join "`n")
[System.IO.File]::WriteAllText($src, $newText, [System.Text.UTF8Encoding]::new($false))
Write-Host "Updated ${src}: $N -> $($newLines.Count) lines"
