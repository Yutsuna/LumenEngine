---
description: Run the review pipeline (reviewer + reviewer-safety + reviewer-norme) over the current changes
argument-hint: "<module> | <file> | (empty for git diff)"
---

# review

**Argument resolution (follow strictly in order):**
1. If the user supplied an argument that matches a module name under `.agents/modules/`, review that module's diff.
2. Else if the user supplied an argument that is a file path, review that file's diff.
3. If no argument is supplied, default to `git diff` (working tree or against the harness-provided `<base>`).

Dispatches the review agents defined in `.agents/agents/` (all strictly
read-only; performance is deliberately out of scope) over the current changes.

Give your harness the prompt below, adjusting `<base>`:

```text
Use the reviewer agent on the current diff against <base> (or on the working
tree if uncommitted). Follow .agents/agents/reviewer.md strictly: dispatch
reviewer-safety and reviewer-norme, consolidate their findings into one
report ordered by severity — each finding with path:line, quoted evidence,
and the rule (or defect) it violates. End with a verdict:
approve / needs changes / fails the rules.
