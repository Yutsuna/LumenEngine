# Lumen — Agent, Skill, and Rule Configuration

This repository contains the project-level instructions, rules, and skills that guide every AI-assisted contribution to **Lumen**.

The authoritative location for the full agent infrastructure is `.agents/`:

- `.agents/AGENTS.md` — project overview, workflow, instruction precedence, known repository state
- `.agents/rules/*.md` — always-on constraints every change must satisfy
- `.agents/agents/*.md` — specialist role charters (responsibilities and collaboration boundaries)
- `.agents/skills/*.md` — reusable step-by-step workflows

Before modifying code, agents must read the relevant rules and skills. The rules in `.agents/rules/` are non-negotiable. The skills in `.agents/skills/` provide actionable checklists for common tasks.

## Instruction Precedence

When two instructions disagree, the higher entry wins:

```text
1. Repository / build system / compiler reality   (what is actually true)
2. Root AGENTS.md                                 (this file; loaded automatically)
3. .agents/AGENTS.md                              (loaded automatically; refines this file)
4. .agents/rules/                                 (read before changing code; binding once read)
5. .agents/agents/                                (role charters; apply the matching charter)
6. .agents/skills/                                (procedures, loaded on demand)
7. Ad-hoc notes, prior-agent summaries, task scratchpads
```

Two clarifications:

- The current task defines **scope** (what to do). The rules define **constraints** (how changes must be made). If a task explicitly demands something the rules forbid, surface the conflict to the user instead of silently choosing one.
- No instruction may override repository reality or the honesty rules in `.agents/rules/validation.md`. Never report a command as executed, or a result as observed, unless it was.
