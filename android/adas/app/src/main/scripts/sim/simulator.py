#!/usr/bin/env python3
"""Alias entry point — use ``python3 -m sim.main`` or ``sim/main.py``."""

import _path  # noqa: F401

from sim.main import main

if __name__ == "__main__":
    main()
