# CLAUDE.md

This repository contains Arduino Uno / embedded C++ code for the FireGuard / EmberSensor project.

## Goals
- Preserve existing behavior unless a change is explicitly requested
- Prioritize reliability, clarity, and low-risk edits
- Keep the code easy to debug on hardware
- Report all dead code, conflicting logic, race conditions and deadlocks

## Rules
- Do not change pin assignments unless explicitly asked
- Do not change threshold values or fire detection semantics unless explicitly asked
- Do not remove cooldown or safety logic unless explicitly asked
- Prefer small, surgical changes over large rewrites
- Preserve serial logging unless explicitly asked to reduce it
- Avoid dynamic memory usage where possible
- Avoid introducing heavy abstractions that make embedded debugging harder
- Keep loop behavior simple and non-blocking where possible
- When suggesting refactors, explain hardware/runtime risks first

## Code style
- Prefer clear function names
- Keep modules focused by responsibility
- Minimize hidden side effects
- Comment only where it improves maintainability

## When making changes
- First explain the proposed approach
- Then identify files to modify
- Then make the change
- Then summarize exactly what changed and any hardware behavior that could be affected
