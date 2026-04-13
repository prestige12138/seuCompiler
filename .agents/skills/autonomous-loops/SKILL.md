---
name: autonomous-loops
description: Patterns for controlled autonomous loops and multi-agent execution in Codex.
origin: ECC
---

# Autonomous Loops Skill

Use this skill to design or operate longer-running autonomous workflows in Codex without relying on Claude-specific runtimes.

## When to Use

- Repeating a build -> fix -> verify loop until a clear stop condition is met
- Splitting work across multiple native Codex agents
- Defining checkpoints, rollback rules, and failure handling for autonomous work
- Coordinating multi-step sessions that need structured operator oversight

## Recommended Patterns

### 1. Sequential Loop

Best for one active writer and tight feedback cycles.

1. Define a single goal.
2. Run one step.
3. Verify.
4. Stop on failure or proceed to the next step.

### 2. Native Multi-Agent Split

Best for bounded parallel tasks with clear ownership.

1. Keep the critical path in the main session.
2. Delegate sidecar tasks to specialized agents.
3. Merge results only after verification.

### 3. Checkpointed Execution

Best for longer tasks with non-trivial risk.

Checkpoint after each of:

- planning
- first implementation slice
- verification pass
- final review

## Stop Conditions

Always define explicit stop conditions before starting:

- no progress across two checkpoints
- repeated failures with the same root cause
- quality gates cannot be restored quickly
- scope drift beyond the original task boundary

## Safety Rules

1. One writer per file at a time unless ownership is explicit.
2. Verify after every meaningful batch.
3. Reduce scope when the loop stalls.
4. Prefer native Codex agents and local scripts over harness-specific wrappers.

## Codex Notes

- Do not depend on `claude -p`, `.claude/commands/`, or `~/.claude/*` runtime state.
- When external orchestration is needed, treat it as optional infrastructure, not as the default skill contract.
