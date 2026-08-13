# Pixi and PlatformIO Integration

This guide shows you how to use **Pixi** as the package manager for embedded ESP32 (and RISC-V) projects with **PlatformIO**.

## Why Use Pixi for C++, Python, and ESP32?

Pixi (from prefix.dev) is a fast, reproducible package manager built on the Conda ecosystem. It solves a major problem for hybrid projects:
- **C++ Tools:** It installs native binaries like `cmake`, `ninja`, and compilers directly from Conda-forge.
- **Python:** It manages Python versions and virtual environments automatically.
- **PlatformIO:** Because PlatformIO is fundamentally a Python CLI tool available on PyPI, Pixi can manage it.

You do not need to install PlatformIO globally on your system. Pixi keeps the toolchain isolated per project.

## How to Set Up the Environment

1. Install Pixi on your machine (see [pixi.prefix.dev](https://pixi.prefix.dev/)).
2. Place the `pixi.toml` file from this directory in the root of your ESP32 project.
3. Run the setup command:

```bash
pixi install
```

This command downloads Python, CMake, Ninja, and the PlatformIO CLI into an isolated folder (`.pixi`).

## How to Run PlatformIO Tasks

We defined custom tasks in the `pixi.toml` file. You can run them without activating an environment.

Initialize a new ESP32-P4 project:
```bash
pixi run init
```

Compile the firmware:
```bash
pixi run build
```

Upload the firmware to the board:
```bash
pixi run upload
```

Monitor the serial output:
```bash
pixi run monitor
```

## Running Native C++ (Linux) Tasks

If your project also contains native Linux C++ code (for simulators or tests), you can run the CMake tasks we defined:

```bash
pixi run cmake-config
pixi run cmake-build
```

Using Pixi guarantees that every developer on your team uses the exact same version of CMake, Python, and PlatformIO, regardless of their host operating system.
