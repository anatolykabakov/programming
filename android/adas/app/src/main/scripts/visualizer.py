#!/usr/bin/env python3
"""Shim: run ``python3 vis/visualizer.py`` instead."""

import runpy
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
runpy.run_path(str(Path(__file__).resolve().parent / "vis" / "visualizer.py"), run_name="__main__")
