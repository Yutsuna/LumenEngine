# Rule: Validation and reporting

**Applies to:** every claim that a change works, compiles, passes tests, is thread-safe, or is faster — and every completion report.

## Status vocabulary (strict)

Every validation item you report must carry exactly one of these statuses:

| Status | Meaning |
| --- | --- |
| `Passed` | The command was executed and its success was observed. |
| `Failed` | The command was executed and its failure was observed (include the error). |
| `Not run` | Not executed; state why (no tool, no test, out of scope, blocked). |
| `Not applicable` | Irrelevant to this change (state the reason). |

Evidence rules:

- Never turn "I believe this compiles" into "Build passed". Never turn "should be thread-safe" into "Thread-safe". A claim may only be stated as observed if you executed the command and observed the result.
- Compilation is not correctness; unit tests are not proof of performance; one backend passing does not validate other backends (see `skills/testing-validation.md` for the validation categories).
- If a claim rests on reasoning rather than execution, label it as reasoning.

## Before claiming done

Validate as much as is relevant, and record the exact commands and results:

1. **Format/style.** `clang-format` on new files (touched lines only for legacy files) — per `rules/cpp.md`.
2. **Compilation.** Build affected Meson targets (`ninja -C <builddir>`; `meson setup --reconfigure <builddir>` first if source files were added or removed — see `rules/build.md`). Warnings are failures.
3. **Tests.** `meson test -C <builddir>`. Note: the project currently registers no tests ("No tests defined."); if no relevant test exists, state that explicitly and consider whether one should be added.
4. **Static analysis.** Compiler warnings always; `clang-tidy` when configured (none is configured yet).
5. **Sanitizers.** When the change touches memory safety, lifetime, or concurrency, build with the mechanisms that work: `-Db_sanitize=address`, `-Db_sanitize=address,undefined`, or `-Db_sanitize=thread`. The `enable_asan`/`enable_ubsan`/`enable_tsan` options are currently inert (see `rules/build.md`) and must not be reported as sanitizer runs.
6. **Build-system correctness.** If build files changed, reconfigure Meson and confirm the target graph is correct.

## Reporting format

Every completed task reports these sections (omit a section only with `Not applicable`):

```text
Summary
- What changed

Affected Areas
- Modules/files affected

Architectural Impact
- API/dependency/backend implications

Validation
- Commands actually executed
- Results actually observed (with statuses from the vocabulary above)

Performance
- Measurements if applicable; otherwise "Not applicable" or "Not run"

Known Limitations
- Remaining issues, unvalidated areas, and assumptions
```

- Never claim a command was executed if it was not.
- If validation is blocked by environment limitations, state the limitation plainly (`Not run`, with the reason).
