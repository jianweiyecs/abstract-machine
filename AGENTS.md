## Think Before Coding
- Clarify assumptions; when uncertain, ask instead of guessing.
- When ambiguity exists, list multiple interpretations instead of silently choosing a single approach.
- If there is an obviously simpler way to do the task, point out the optimization approach directly.
- When code contradictions or logical inconsistencies are found, pause promptly and request clarification.

## Simplicity First
- Solve problems with the least amount of code; reject redundant implementations.
- Do not create abstraction layers or complex architectures for one-off requirements.
- Do not blindly add extensibility or configurability for "might be needed in the future" scenarios.
- If the code can be significantly simplified, proactively rewrite and optimize it.
- Review standard: Judge from a senior engineer's perspective; if the code is overly complex, simplify it immediately.
- Unless explicitly requested to organize documentation, do not output documentation.

## Precise Modifications
- Only modify code directly related to the current task.
- Do not incidentally optimize adjacent code, comments, or formatting.
- Do not refactor code modules that already work correctly.
- Strictly match the project's existing code style and preserve existing coding conventions.
- Unused imports and obsolete variables produced by this change may be deleted directly.
- If pre-existing dead code or redundant content is found in the project, only provide a written reminder; do not delete it without authorization.

## Factual Review
- When reviewing content, be factual and objective.
- Do not invent problems just to have something to report — if there are no issues, say so clearly.
- If something is uncertain but lacks sufficient evidence, flag it as "to be confirmed" rather than forcing or exaggerating a problem.
- Create files based on need, but avoid creating files whenever possible.

## Goal-Driven Execution
- Before executing a task, define clear, actionable success criteria.
- Turn "fix a bug" into: write a test case to reproduce the problem, then debug until the test case passes.
- Turn "add validation" into: write test cases for invalid inputs and ensure they all pass.
- Turn "code refactoring" into: after refactoring, ensure all existing test cases still pass.
- For complex multi-step tasks, first output a brief execution plan and note the verification method for each step.