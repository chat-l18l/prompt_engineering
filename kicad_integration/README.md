# KiCad Project Explorer Integration

This experimental project aims to build a set of Python tools for integrating with KiCad PCB designs. The main goal is to automate Design Rule Checks (DRC), verify component metadata, and generate statistics from KiCad `.kicad_pcb` files to improve the hardware engineering workflow.

## 🎯 What we are trying to achieve

In complex PCB designs, it's easy to forget basic design constraints or metadata required for manufacturing. This project explores the KiCad API to perform automated checks, such as:
- **Component Information:** Verifying that all components have appropriate order numbers (e.g., "Part Number" and "Supplier").
- **Manufacturing Constraints:** Checking that PCB traces are not narrower than a user-defined minimum width.
- **Physical Safety/Handling:** Ensuring the board outline (`Edge.Cuts`) features rounded corners (arcs) instead of sharp 90-degree angles to prevent injuries during manual assembly.
- **Data Collection:** Gathering statistics on the number of components, tracks, vias, and nets.

## 🏗️ Architecture & Approach

KiCad is undergoing significant API changes. This repository demonstrates two different approaches to handle these transitions:

1. **The Classic File-Based API (`kicad_explorer.py`)**
   - Built on the traditional SWIG-based Python bindings.
   - Operates offline by parsing the `.kicad_pcb` file directly.
   - Uses the `pcbnewTransition` compatibility layer. This monkey-patches the older API (KiCad 6/7) to behave like the newer KiCad 8 API, allowing scripts to be forwards-compatible.

2. **The New IPC API (`kicad_ipc_explorer.py`)**
   - Built on the modern Inter-Process Communication (IPC) architecture introduced for KiCad 9+.
   - Communicates with a *running instance* of KiCad via UNIX sockets and Protocol Buffers.
   - Uses the official `kicad-python` library (along with `protobuf` and `pynng`).
   - *Note: This script currently serves as a structural skeleton and proof-of-concept, as the protobuf methods for specific hardware queries are actively evolving.*

## 🛠️ Prerequisites

To run the tools in this repository, you need:
- **Python 3**
- **KiCad** installed on your system (e.g., via `apt-get install kicad` on Ubuntu).
- The Python dependencies specified in `requirements.txt`.

Install the dependencies via pip:
```bash
pip install -r requirements.txt
```

## 🚀 Usage

### Using the Classic Explorer

If you are running the script on a system where KiCad is installed globally via a package manager, you might need to point Python to the KiCad libraries:

```bash
PYTHONPATH=/usr/lib/python3/dist-packages python3 kicad_explorer.py /path/to/your/board.kicad_pcb
```

You can customize the required fields and trace widths:
```bash
PYTHONPATH=/usr/lib/python3/dist-packages python3 kicad_explorer.py my_board.kicad_pcb --min-trace 0.25 --fields "Part Number" "Manufacturer"
```

### Exploring the IPC API (KiCad 9+)

To connect to a running instance of KiCad via the IPC API, ensure the API Server is enabled in KiCad's Preferences (Preferences > Plugins). Then run:

```bash
python3 kicad_ipc_explorer.py
```

## 🧑‍💻 About the Author

**Jules** — *AI Software Engineer*

I wrote this code as part of an interactive coding experiment with a human user. This repository stands as a testament to exploring documentation, adapting to changing API landscapes, and establishing automated tooling in the hardware engineering domain.
