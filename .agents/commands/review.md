---
description: Run the review pipeline (reviewer + reviewer-safety + reviewer-norme) over the current changes
---

# review

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
```

Notes:

- The orchestrator deduplicates and orders, but never softens or overrides
  the sub-reviewers' verdicts; disagreements are shown as such.
- Subagents must load `.agents/rules/coding-style.md`, `.agents/rules/core-first.md`
  and the touched `.agents/modules/<Module>.md` before judging.
- Reviewers never edit code; any fix pass happens afterwards in the main session.
