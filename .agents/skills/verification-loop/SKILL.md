---
name: verification-loop
description: A Codex-friendly verification workflow for build, lint, typecheck, tests, and review.
origin: ECC
---

# Verification Loop Skill

Use this skill after code changes and before final delivery.

## Verification Phases

### 1. Build

Run the project build if one exists. Stop and fix build failures before continuing.

### 2. Type Check

Run the relevant type checker or compiler diagnostics for the stack in use.

### 3. Lint

Run the configured linter if present.

### 4. Tests

Run the smallest relevant test set first, then broader verification if needed.

### 5. Review

Inspect the changed files for correctness, edge cases, and accidental drift.

## Output

Report:

- build status
- typecheck status
- lint status
- test status
- remaining risks

## Rules

1. Do not claim success without actually running the checks you report.
2. If a check is unavailable, say so explicitly.
3. Prefer incremental verification during long tasks, not only at the end.

## Codex Notes

- This skill is Codex-neutral and does not rely on Claude hooks or `.claude/*` files.
- Use it together with `code-review`, `tdd`, and `build-fix` compatibility skills when relevant.
