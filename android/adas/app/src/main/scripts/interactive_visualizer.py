#!/usr/bin/env python3
"""Shim: run ``python3 vis/interactive_visualizer.py`` instead."""

import runpy
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
runpy.run_path(
    str(Path(__file__).resolve().parent / "vis" / "interactive_visualizer.py"), run_name="__main__"
)
