# Agents Notes - Programmeer Experimenten

These instructions apply to the complete project tree. Preserve this file when
moving the project. A coding agent must read it before making changes.

## Context

This repository is for teaching and experimentation with small, deterministic C modules.
Targets include:
- Embedded firmware on ESP32-P4 (RISC-V) using PlatformIO + ESP-IDF
- Linux native code on Ubuntu 24.04 using gcc + CMake (C23 where practical)
- Supporting Python experiments (imgui, zeromq, zenoh, ROS2 via pixi)

Focus: technical vehicle/robot controls, DSP for musical instruments, simulators.
Philosophy: simple, readable code — no clever tricks that are hard to understand.
Minimal abstractions. Code and explanations are designed to be passed on to students
learning hardware + software design.

## Repo shape
- Experiments live under `experiments/<nummer>_<naam>/`
- Embedded ESP32 code goes in `esp32/` subdir (PlatformIO project)
- Linux native C in `linux/` or direct at exp root with CMakeLists.txt
- Each experiment is independent; cd into it to build
- Keep experiments minimal and self-contained for teaching

## PlatformIO for ESP32-P4
- Use `platform = https://github.com/pioarduino/platform-espressif32.git`
- framework = espidf
- board = esp32-p4
- monitor_speed = 921600
- default_envs = ESP32-P4
- sdkconfig.defaults and sdkconfig.ESP32-P4 tracked for repro

## Build commands
- ESP: from the esp32/ dir: `pio run`, `pio run -t upload`, `pio device monitor -b 921600`
- Linux cmake: from exp dir: `cmake -B build -S . && cmake --build build`
- Alternative gcc (small exps): see README in the experiment
- Prefer `ctest` when tests are present

## Coding conventions (CLASS style + Scalable C principles)

Follow a consistent, scalable C style (inspired by Pieter Hintjens' Scalable C):

- snake_case for functions and variables
- ALL_CAPS for macros and constants
- 4 spaces indent, short functions
- Keep modules small, cohesive and independently testable
- Use one public header and one implementation file per module
- Public symbols: `rbt_` prefix (project-specific, e.g. robot)
- Private/file-local: `s_` prefix and declare them `static`
- Prefer plain structures and direct control flow over abstraction layers
- Do not add frameworks or external dependencies without a measured need
- Use C23 (or C11 for broad embedded compatibility) where practical
- Use fixed-width integers where representation matters
- Prefer caller-owned buffers for explicit, deterministic memory use
- Do not use global mutable state
- Put units in domain names (e.g. `width_m`, `resolution_mm`)
- Prefer one parameter struct with designated initializers over long positional argument lists
- assert-heavy for preconditions and errors (in teaching code)

Every function and struct has:
  // Why: ...
  // What: ...
  // Pre: ...
  // Post: ...

## Design By Contract

Every function (including private helpers) checks preconditions at its boundary.
Invalid caller input is a programming defect:

- Fail immediately with explicit contract checks, `abort`, always-active assertion,
  or a hard error return where failure is an expected operational condition.
- Never silently repair, clamp, reorder, or ignore invalid caller input.
- Keep code compact after the boundary checks; avoid repeated defensive checks.
- Distinguish contract violations from expected runtime failures.
- Document important preconditions in the public header.

For expected runtime failures, return explicit tagged result structures by value
when appropriate. Constructors that allocate return a typed result.
Destructors take a double pointer, release resources, and set the caller pointer
to `NULL`.

Do not add result machinery to functions with no expected runtime failure.

## Build System

- Use CMake for Linux experiments.
- Enable testing with `enable_testing()` and prefer `ctest` for test execution.
- Keep build settings strict and explicit (`-Wall -Wextra -Werror -pedantic` where practical).
- Avoid generator-specific or platform-specific behavior unless required.

## Comments And Documentation

- Add a short comment to every function explaining what it does and why.
- Document ownership, input validity, capacities and limitations in headers.
- Explain decisions and invariants, not obvious syntax.
- English comments with intent/contract (Why/What/Pre/Post)
- Assume students read code; explain *why* not line-by-line
- READMEs start with Nederlandse samenvatting, then English details + exercises
- Keep `README.md` current with build commands and a minimal usage example
- Use ASCII unless an existing file requires another character set

## Testing

- Every public behavior needs focused deterministic tests.
- Keep strict compiler flags enabled.
- Use AddressSanitizer and UndefinedBehaviorSanitizer on Linux for changes
  involving indexing, memory, or geometry arithmetic.
- Prefer CMake/CTest for test discovery and execution.
- Do not make performance claims without a repeatable benchmark.

## Adding experiments

- See docs/how_to_add_experiment.md
- Keep minimal, layered only when it teaches something
- Always include build/run + expected output + student exercises
- Read existing code and tests before editing

## Change Discipline

- Read existing code and tests before editing.
- Keep changes scoped to requested behavior.
- Do not rewrite working code solely to impose a new style.
- Do not remove contracts, comments or tests to reduce line count.
- Preserve unrelated user changes.
- Report changed files and verification commands exactly.

## Verification

- Every change must document exact commands to build/run/verify

## Repository alignment notes

- Use C23 (or C11) and CMake for Linux parts.
- Use PlatformIO + ESP-IDF for ESP32-P4 parts.
- Use `rbt_` prefix for public API symbols.
- Prefer `ctest` as the testing workflow when tests are present.
- Adapt this file when repository conventions differ from these generic guidelines.
- The educational focus (simple code for students) takes precedence where it conflicts with production-oriented rules.

## Python Specific Conventions

For Python projects and tools within this repository (such as the KiCad integrations and supporting tools), the same philosophy of simplicity, readability, and deterministic behavior applies. Follow these rules:

- **Style:** Strictly adhere to PEP 8. Use standard `snake_case` for functions/variables and `PascalCase` for classes. Avoid complex meta-programming or overly deep inheritance hierarchies.
- **Type Hinting:** Use static type hinting (`typing` module or modern built-in types) for all public functions, arguments, and return types.
- **Documentation:** Use concise docstrings for all classes and functions. Follow the (Why/What/Pre/Post) structure conceptually within the docstring to maintain alignment with the C code documentation style.
- **Design by Contract:** Similar to the C guidelines, enforce preconditions at the boundary of your functions. Use `assert` statements or explicitly raise `ValueError`/`TypeError` early if input data is invalid. Do not swallow exceptions silently.
- **Dependency Management:** Keep dependencies explicit. For scripts requiring external libraries, always document them via `requirements.txt` or a `pixi` configuration, pinning versions where reproducibility matters.
- **Testing:** Every independent Python module should be testable. Prefer standard `unittest` or simple `pytest` scripts. Keep mock usage to a minimum; favor real deterministic input/output tests.
- **Global State:** Do not use mutable global state. Construct and pass necessary contexts explicitly.
