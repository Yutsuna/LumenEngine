# Rule: General conduct

**Applies to:** every change, in any file, regardless of task size.

## Repository honesty

- Never fabricate repository state, file contents, build output, or test results.
- Inspect existing code, build files, and documentation before modifying anything. Do not rely on a prior agent's summary without verifying it against the repository.
- If a task is ambiguous, ask for clarification instead of guessing.
- Treat the "Known repository state" table in `.agents/AGENTS.md` as a map of existing inconsistencies: recognize them, report them when relevant, and do not silently rewrite unrelated files to remove them.

## Change discipline

- Prefer the smallest coherent change that satisfies the task.
- Do not rewrite unrelated code, rename symbols opportunistically, or refactor outside the task scope.
- Do not silently change public APIs. If a public API must change, document the change and its migration path.
- Preserve existing architecture unless the task explicitly requires architectural change.
- Introduce a new dependency only when an identified need requires it; represent it in the build system (Meson) and the dependency authority (Nix, `Nix/deps.nix`). Do not add dependencies "in case they are needed."

## Project naming

- The project name is **Lumen**. Use it in new documentation, identifiers, package metadata you author, and messages.
- Do not introduce placeholder or legacy names: TrafficEngine, Volt, EngineName, or similar.
- If you encounter a legacy name in an existing file (for example `Nix/package.nix`), treat it as documented repository state: mention it in your report; change it only if renaming it is the task.

## Communication

- Use professional technical English for all identifiers, comments, documentation, error messages, and logs.
- Do not introduce comments or documentation in other languages.
- Report exactly what changed and what was validated, using the format in `rules/validation.md`.
