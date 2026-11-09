# Git workflow rules

## PR title is a hard gate

Every PR title must match (validated by
`.github/workflows/CheckPullRequest.yml` → `Scripts/CheckPullRequest.py`,
regex in `Scripts/GitHub/Configuration.py`):

```
<PRType>(<PRScope>): #<IssueNumber> <Description>
Example: Feat(AppCore): #142 Add SDL3 window support
```

- `PRType` ∈ `Chore | Fix | Feat | CI | Test | Style | BREAKING`
  (capitalised exactly; `feat:` fails).
- `PRScope` = a top-level directory under `Source/` (auto-discovered at
  validation time, so `Renderer`, `RHI`, … are valid even as WIP modules)
  or `Lumen`.
- The `#<number>` must reference a **real Issue** of this repo — PR numbers
  are rejected (`Not an issue` if it points at a PR).
- `Description` must start with an uppercase letter.
- CI ("Build & Test") runs **only after** the PR Title Check workflow
  succeeds (triggered by `workflow_run`). A bad PR title blocks the whole CI,
  not just the title job. Pushes to `main` trigger CI directly.

## Commit style

Recent history uses `Type(Scope): Description` (same vocabulary as PR titles,
**without** the issue number):

```
Refactor(Scene): Keep internal payloads unexported
Test(Scene): Add ActorMailbox Push & Registry Dead Letter tests
```

Align commits with the module being touched.

## Branches

- Base branch is `main`.
