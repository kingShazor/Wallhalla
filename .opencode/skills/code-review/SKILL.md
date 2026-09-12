---
name: code-review-cpp
description: review the cpp source code
---

Review the the code changes for real bugs and concrete problems.

priority for finding code changes:
 1. local unstaged changes
 2. staged changes
 3. last commit

The goal of this project is a high-performance JavaScript runtime written in C++.

Focus on:

 - correctness and logic errors
 - undefined behavior, memory and lifetime issues
 - performance-critical code
 - unnecessary allocations, copies, indirections, and synchronization
 - maintainability issues that are likely to cause bugs

Prefer simple, direct and efficient code.

Do not introduce complexity or "clean code".
Unnecessary GoF design patterns, convoluted OOP constructs, excessive abstraction, unnecessary interfaces, indirection, or hiding straightforward implementation details are explicitly discouraged.

The rule is simple:
If a simple and efficient solution exists, prefer it over an abstract or elaborate one.

Programmers who unnecessarily introduce GoF patterns, convoluted OOP, or abstraction that merely hides the implementation will be banned from the codebase.

Only report issues that are actionable and worth fixing.

For each issue, provide:

file and line
severity
short explanation
suggested fix
