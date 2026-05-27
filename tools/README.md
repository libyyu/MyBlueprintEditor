# tools/

This directory contains:

1. **Build infrastructure** (used at every build):
   - `busybox.exe` — POSIX shell on Windows, invoked by `build.bat` to run `build.sh`.
   - `iconv.exe` — encoding conversion utility (used by some build steps).
   - `shared_functions.sh` — shared shell functions sourced by `build.sh`.
   - `requirements.txt` — Python deps for the data-migration scripts below.

2. **Refactor extraction scripts** (one-shot, kept for reference):

   These scripts were used to perform large mechanical splits of monolithic
   source files. They are **not part of the build** and **should not be re-run**
   on the current tree (they would corrupt files). They are kept as
   documentation of how the splits were done, and as templates for similar
   future refactors.

   | Script | What it did | When |
   |---|---|---|
   | `split_nodedefs.ps1`         | Split `BuiltinNodeDefs.cpp` (3624 lines) into 25 per-category files under `Runtime/nodedefs/`. | refactor batch |
   | `split_nodedefs_ai.ps1`      | Further split `nodedefs/BuiltinNodeDefs_AI.cpp` (706 lines) into 4 sub-files (JSON/LLM/Tool/Agent). | refactor batch |
   | `split_exporter.ps1`         | Split `BlueprintExporter.cpp` (2064 lines) into 5 per-section files under `Runtime/exporter/`. | refactor batch |
   | `split_runner_exectx.ps1`    | Extracted all `ExecutionContext::` methods from `BlueprintRunner.cpp` into `BlueprintRunner_ExecutionContext.cpp`. | refactor batch |
   | `split_inspector.ps1`        | Split `BlueprintEditor/InspectorPanel.cpp` (2313 lines) into 4 per-panel files under `BlueprintEditor/inspector/`. | refactor batch |
   | `split_onframe.ps1`          | Extracted 5 helpers from `BlueprintEditor::OnFrame` (1986-line function → 1080). | refactor batch |
   | `split_recentfiles.ps1`      | Extracted Recent Files / Recent Projects history out of `FileOperations.cpp` into `RecentFiles.cpp`. | refactor batch |

3. **Refactor analysis helpers** (re-runnable diagnostics):

   | Script | What it does |
   |---|---|
   | `analyze_unused_runner.ps1`  | Counts handlers that declare `auto* runner = ctx.GetRunner();` but never use `runner`. |
   | `remove_unused_runner.ps1`   | Removes the dead template lines reported by `analyze_unused_runner.ps1`. |

4. **Historical data-migration scripts** (one-shot, kept for reference):

   These ran once to fix specific data shapes in committed `.bjson` blueprints
   or `.cs` source files. They are **not idempotent** on the current tree.

   | Script | What it did |
   |---|---|
   | `convert_links.py`           | Converted old `from`/`to` link format to `startPinId`/`endPinId`. |
   | `fix_bjson.py`               | Bulk-renamed wxgame blueprint events: `OnExecute → OnBeginPlay`, etc. |
   | `fix_links.py`               | Repointed `MakeMessage.Message → FormatString` mis-wirings. |
   | `fix_formatstring_pins.py`   | `FormatString` dynamic input pin dataType: 9 (Any) → 4 (String); pin names `arg0..argN → 0..N`. |
   | `fix_namespaces.py`          | Added `using BlueprintRuntime.Samples.AINpc;` etc. to MiniGame `.cs` files. |
   | `fix_all_cs_errors.py`       | Bulk-fix MiniGame C# compile errors. |

## Re-running policy

- **Build infrastructure** (group 1): always invoked by the build system.
- **Diagnostic helpers** (group 3): safe to re-run any time.
- **Refactor / migration scripts** (groups 2, 4): **DO NOT re-run** on the
  current tree. They were one-shot. If you need to do a similar split or
  migration, copy the script as a template and adapt it.
