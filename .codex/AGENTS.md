# ECC for Codex in SeuCompiler

This supplements the root `AGENTS.md` with Codex-specific guidance for this project.

## What Codex Should Read Here

- Project runtime and multi-agent roles live in `.codex/config.toml` and `.codex/agents/*.toml`.
- The root `AGENTS.md` remains the primary project instruction file.

## Skills Discovery

This repo currently does not include a project-local `.agents/skills/` directory.
If skills are added later, each skill directory should include:

- `SKILL.md` — workflow instructions
- `agents/openai.yaml` — Codex skill metadata

## Commands

Codex supports native slash commands, but it does not auto-register repository-local command markdown files from `.codex/commands/`.
This repo does not currently provide repository-local command shims or same-name compatibility skills.

## Multi-Agent

This project expects Codex multi-agent support to be enabled via:

- `[features] multi_agent = true`
- `[agents.*]` role registration in `.codex/config.toml`

Available roles include explorer, reviewer, docs_researcher, planner, architect, tdd_guide, code_reviewer, security_reviewer, refactor_cleaner, build_error_resolver, loop_operator, and harness_optimizer.

## Project Guidance

- For compiler-course work, prefer `seu-compiler` when the task involves Lex, Yacc, AST, semantic analysis, or IR generation.
- Treat `resources/` as reference inputs, not generated outputs.
- Favor minimal, staged implementation over attempting full C99 support immediately.
