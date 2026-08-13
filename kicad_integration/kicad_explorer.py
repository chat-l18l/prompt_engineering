#!/usr/bin/env python3
"""
KiCad Project Explorer and Checker Tool
This tool inspects a KiCad PCB design (.kicad_pcb) and checks various parameters like footprint fields, trace widths, board outline, and generates statistics.
"""

import sys
import argparse
import os
import math

# Use pcbnewTransition for KiCad 8 API compatibility if installed, fallback to pcbnew
try:
    from pcbnewTransition import pcbnew
except ImportError:
    import pcbnew

def load_board(file_path):
    """Loads a KiCad PCB file."""
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"PCB file not found: {file_path}")

    print(f"Loading board: {file_path}")
    return pcbnew.LoadBoard(file_path)

def check_footprint_fields(board, required_fields=None):
    """Checks if components have the required order info like Part Number or Supplier."""
    if required_fields is None:
        required_fields = ["Part Number", "Supplier"]

    print("\n--- Checking Footprint Fields ---")
    missing_info = []

    # In KiCad 7/8, GetFootprints is used
    for footprint in board.GetFootprints():
        ref = footprint.GetReference()

        # We might not care about mounting holes or fiducials having part numbers
        if ref.startswith("H") or ref.startswith("FID"):
            continue

        fields = footprint.GetProperties()
        missing = []
        for req_field in required_fields:
            if req_field not in fields or not fields[req_field].strip():
                missing.append(req_field)

        if missing:
            missing_info.append(f"{ref}: Missing {', '.join(missing)}")

    if not missing_info:
        print("All required fields are present on components.")
    else:
        for msg in missing_info:
            print(msg)
    return len(missing_info) == 0

def check_trace_widths(board, min_width_mm=0.2):
    """Checks if there are any tracks narrower than the given width (in mm)."""
    print(f"\n--- Checking Trace Widths (Min {min_width_mm} mm) ---")

    min_width_iu = int(min_width_mm * 1e6) # Convert mm to nanometers (internal units)
    too_narrow = []

    for track in board.GetTracks():
        if isinstance(track, pcbnew.PCB_TRACK):
            width = track.GetWidth()
            if width < min_width_iu:
                # convert to mm for display
                width_mm = width / 1e6
                start = track.GetStart()
                too_narrow.append(f"Track too narrow: {width_mm:.3f} mm at ({start.x/1e6:.2f}, {start.y/1e6:.2f})")

    if not too_narrow:
        print(f"All tracks are >= {min_width_mm} mm.")
    else:
        for msg in too_narrow:
            print(msg)

    return len(too_narrow) == 0

def check_board_outline_radius(board):
    """
    Checks the board outline (Edge.Cuts) for sharp corners.
    We look for segments on Edge.Cuts. If they are all lines and form a right angle without an arc, warn the user.
    """
    print("\n--- Checking Board Outline ---")
    edge_cuts_layer = pcbnew.Edge_Cuts

    drawings = board.GetDrawings()
    edge_items = [d for d in drawings if d.GetLayer() == edge_cuts_layer]

    if not edge_items:
        print("Warning: No board outline found on Edge.Cuts.")
        return False

    arcs_found = any(isinstance(item, pcbnew.PCB_SHAPE) and item.GetShape() == pcbnew.SHAPE_T_ARC for item in edge_items)

    if not arcs_found:
        print("Warning: No arcs (rounded corners) found on the board outline. Consider adding a radius to corners for safety and handling.")
    else:
        print("Arcs found on board outline. Rounded corners are present.")

    return arcs_found

def generate_statistics(board):
    """Generates general statistics for the board."""
    print("\n--- Board Statistics ---")

    footprints = list(board.GetFootprints())
    tracks = list(board.GetTracks())
    vias = [t for t in tracks if isinstance(t, pcbnew.PCB_VIA)]
    actual_tracks = [t for t in tracks if isinstance(t, pcbnew.PCB_TRACK)]
    nets = board.GetNetInfo()

    print(f"Total Components: {len(footprints)}")
    print(f"Total Tracks: {len(actual_tracks)}")
    print(f"Total Vias: {len(vias)}")
    # Number of nets minus 1 for the empty net 0
    print(f"Total Nets: {max(0, nets.GetNetCount() - 1)}")

def main():
    parser = argparse.ArgumentParser(description="KiCad PCB Explorer and Checker")
    parser.add_argument("pcb_file", help="Path to the .kicad_pcb file")
    parser.add_argument("--min-trace", type=float, default=0.2, help="Minimum trace width in mm (default: 0.2)")
    parser.add_argument("--fields", type=str, nargs='+', default=["Part Number", "Supplier"], help="Required footprint fields")

    args = parser.parse_args()

    try:
        board = load_board(args.pcb_file)
        print("Board loaded successfully.\n")

        generate_statistics(board)
        check_footprint_fields(board, required_fields=args.fields)
        check_trace_widths(board, min_width_mm=args.min_trace)
        check_board_outline_radius(board)

    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
