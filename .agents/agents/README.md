# agents/

Agent definitions for Lumen.
The agent ID is the filename without extension:

- `reviewer` — review orchestrator; dispatches the specialised reviewers
  below and consolidates their findings.
- `reviewer-safety` — correctness/safety review (read-only).
- `reviewer-norme` — coding-standard review (`rules/coding-style.md`,
  `rules/core-first.md`) (read-only).
- `documentation` — auditor of the `.agents/` corpus itself: verifies that
  the index, rules, module docs, agent definitions, commands and skills
  are coherent with each other and with the repository (read-only).

Scope notes:

- Performance is **not** the reviewer's job — a dedicated
  benchmarks/performance agent will be authored separately.
- All reviewers are strictly read-only: they may read code and run read-only
  git commands (`diff`, `show`, `log`, `status`), never edit files.
- Reviewers must load the relevant `.agents/rules/*.md` and
  `.agents/modules/<Module>.md` files for the code under review, and apply
  them without improvising standards that are not written down.
- The `documentation` auditor is not a code reviewer: it audits the
  `.agents/` instruction corpus (references, statuses, toolchain facts),
  not the source code. It is strictly read-only too.
