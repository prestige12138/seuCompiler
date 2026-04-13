# ECC-Derived Project Instructions

This project keeps the useful ECC workflow patterns, but it is configured for **Codex**, not Claude Code.

**Version:** 1.10.0

## Codex Note

- Active Codex surfaces in this repo are `AGENTS.md`, `.codex/config.toml`, and `.codex/agents/`.
- Optional surfaces such as `.agents/skills/`, `.codex/commands/`, and `.codex-plugin/` should only be documented when they actually exist in the repo.
- Codex has native slash commands, but repository-local `.codex/commands/*.md` files are not a supported custom-command registration mechanism.

## Core Principles

1. Agent-first for bounded, useful delegation.
2. Test-driven implementation where practical.
3. Security-first handling of inputs and secrets.
4. Prefer immutable transformations over mutation.
5. Plan before significant implementation.

## Active Roles

The project currently registers these Codex roles:

- `explorer`
- `reviewer`
- `docs_researcher`
- `planner`
- `architect`
- `tdd_guide`
- `code_reviewer`
- `security_reviewer`
- `refactor_cleaner`
- `build_error_resolver`
- `loop_operator`
- `harness_optimizer`

Only assume these roles exist unless the project adds more to `.codex/config.toml`.

## Skills Surface

- No project-local `.agents/skills/` directory is currently checked into this repo.
- If project skills are added later, document only the directories and skill names that actually exist.

## Project Guidance

- For Lex, Yacc, AST, semantic analysis, and IR generation, prefer the `seu-compiler` skill.
- Treat `resources/` as reference inputs, not generated outputs.
- Prefer staged `minic`-first delivery over attempting full C99 support immediately.

## Workflow Policy

- Keep `.codex/config.toml` focused on Codex-supported settings only.
- If this repo later adds reusable workflows, prefer adding them as real skills instead of `.codex/commands/*.md` files.
- Do not claim command-compatibility skills exist unless they are actually present in the repo.

## Success Metrics

- Codex reads the project-local config and skills correctly.
- No misleading Claude-only runtime instructions remain in the active surface.
- Skills and roles align with the actual project setup.
