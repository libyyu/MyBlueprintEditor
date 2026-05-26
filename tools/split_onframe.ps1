# Split BlueprintEditor::OnFrame (lines 342-2324) into 5 helper methods.
#
# Layout decision:
#   - PreTick                (344-478, 135 lines)  no params
#   - DrawMenuBar            (479-764, 286 lines)  no params  (includes 'auto& io = ImGui::GetIO();' from line 479)
#   - DockLayout + 3 floating widgets (766-1826, 1061 lines)  STAYS INLINE in OnFrame (too deeply nested ImGui state)
#   - DrawDebugToolbar       (1828-2191, 364 lines) takes (editorMin, editorMax)
#   - DrawSearchOverlay()    (2196, 1 line)        STAYS INLINE
#   - DrawBottomStatusBar    (2198-2291, 94 lines) takes (editorMin, editorMax)
#   - DrawDialogsAndFloatingPanels (2293-2323, 31 lines) no params
#
# All 5 helpers are appended at end of EditorUI.cpp.
# OnFrame body is rewritten to call them.

$src = "BlueprintEditor/EditorUI.cpp"
$bytes = [System.IO.File]::ReadAllBytes($src)
$text  = [System.Text.Encoding]::UTF8.GetString($bytes)
$lines = $text -split "`r?`n"
$N = $lines.Length
Write-Host "Loaded $src : $N lines"

# 1-based source ranges (inclusive)
$preTickStart   = 344  ; $preTickEnd   = 478   # 帧前置（不含 OnFrame 入口大括号）
$menuBarStart   = 479  ; $menuBarEnd   = 764   # 含 auto& io = ImGui::GetIO();
$dockLayoutStart= 766  ; $dockLayoutEnd= 1826  # 包含 TimerWindow/Minimap/ZoomBar 三个简单浮动
$debugTbStart   = 1828 ; $debugTbEnd   = 2191
$searchOverlay  = 2196                          # 单行 DrawSearchOverlay();
$bottomBarStart = 2198 ; $bottomBarEnd = 2291
$dialogsStart   = 2293 ; $dialogsEnd   = 2323

# Helper to slice 1-based inclusive
function Slice($s, $e) { return ($lines[($s-1)..($e-1)] -join "`n") }

$preTickBody   = Slice $preTickStart   $preTickEnd
$menuBarBody   = Slice $menuBarStart   $menuBarEnd
$dockLayoutBody= Slice $dockLayoutStart $dockLayoutEnd
$debugTbBody   = Slice $debugTbStart   $debugTbEnd
$bottomBarBody = Slice $bottomBarStart $bottomBarEnd
$dialogsBody   = Slice $dialogsStart   $dialogsEnd

# Helper method skeleton (re-indented: original was indented 4 spaces inside OnFrame; keep that)
function WrapHelper {
    param([string]$signature, [string]$body)
    return @"

// ============================================================================
$signature
{
$body
}

"@
}

$preTickHelper   = WrapHelper "void BlueprintEditor::OnFrame_PreTick(float deltaTime)" $preTickBody
$menuBarHelper   = WrapHelper "void BlueprintEditor::OnFrame_DrawMenuBar()"             $menuBarBody
$debugTbHelper   = WrapHelper "void BlueprintEditor::OnFrame_DrawDebugToolbar(const ImVec2& editorMin, const ImVec2& editorMax)" $debugTbBody
$bottomBarHelper = WrapHelper "void BlueprintEditor::OnFrame_DrawBottomStatusBar(const ImVec2& editorMin, const ImVec2& editorMax)" $bottomBarBody
$dialogsHelper   = WrapHelper "void BlueprintEditor::OnFrame_DrawDialogsAndFloatingPanels()" $dialogsBody

# New OnFrame body — keep 4-space indent inside braces
$newOnFrame = @"
void BlueprintEditor::OnFrame(float deltaTime)
{
    OnFrame_PreTick(deltaTime);
    OnFrame_DrawMenuBar();

$dockLayoutBody

    OnFrame_DrawDebugToolbar(editorMin, editorMax);

    // Inline: DrawSearchOverlay (Ctrl+F canvas node search)
    DrawSearchOverlay();

    OnFrame_DrawBottomStatusBar(editorMin, editorMax);
    OnFrame_DrawDialogsAndFloatingPanels();
}
"@

# Compose final file: keep lines 1..341 (everything before OnFrame), new OnFrame, helpers, then anything after old OnFrame (line 2325+)
$prefix = ($lines[0..340] -join "`n")   # lines 1..341 (note: index 340 == line 341)
# Find end of old OnFrame: closing brace at line 2324 in original
$suffix = if (2325 -le $N) { "`n" + ($lines[2324..($N-1)] -join "`n") } else { "" }

$out = $prefix + "`n" + $newOnFrame + "`n" + $preTickHelper + $menuBarHelper + $debugTbHelper + $bottomBarHelper + $dialogsHelper + $suffix

[System.IO.File]::WriteAllText($src, $out, [System.Text.UTF8Encoding]::new($false))

$newBytes = [System.IO.File]::ReadAllBytes($src)
$newLineCount = ([System.Text.Encoding]::UTF8.GetString($newBytes) -split "`r?`n").Length
Write-Host "Updated ${src}: $N -> $newLineCount lines"
