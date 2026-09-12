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

Unnecessary GoF design patterns, convoluted OOP constructs, excessive abstraction, unnecessary interfaces, indirection, or hiding straightforward implementation details are explicitly discouraged.

Programmers who introduce GoF patterns, convoluted OOP, or abstraction that merely hides the implementation will be banned from the codebase.

categorize the findings in critical, major, normal, minor.
show only a list of the findings and the quickfix list for nvim.

make a list of the findings for neovim -format:
# nvim-quickfix
<file1>:<linenumber1>:<column1>:<a very short description1>
<file2>:<linenumber2>:<column2>:<a very short description2>
...

