#!/usr/bin/env python3
"""Parse openpilot supercombo.onnx output (shape 1×6409).

Layout matches openpilot ~v0.8.x ``driving.cc`` + the demo's slice starts
(plan ends at 4955). Important: values are **not** plain image polylines.

Coordinates are in the **ego / calibrated frame**:
  X forward (m), Y **right** (m) for this ONNX (matches Android overlay
  ``u = cx + fx·Y/X``), Z up (m), sampled at openpilot ``X_IDXS`` (0…192 m).

  Note: classic openpilot ISO docs say Y-left; this build’s lane means have
  leftNear with **negative** Y — treat as Y-right when projecting.

================================================================================
Index map (this ONNX, out=6409)
================================================================================
  [0    : 4955)  PLAN     5 trajectory hypotheses (MHP)
  [4955 : 5483)  LANES    4 lane lines × 33 × (y, z)   then × (y_std, z_std)
  [5483 : 5491)  LANE_PROB  8 logits (use odd indices → sigmoid)
  [5491 : 5755)  ROAD_EDGES 2 edges × 33 × (y, z) then stds
  [5755 : 6409)  LEAD + desire/meta/pose + GRU state (rest; sizes vary by build)

================================================================================
PLAN (the main driving output)
================================================================================
5 hypotheses × 991 floats:
  33 timesteps × 15 dims  (means)
  33 timesteps × 15 dims  (log-stds)
  1  selection logit

Per point, 15 columns (see fill_model / fill_xyzt):
  0..2   position  x, y, z
  3..5   velocity
  6..8   (related / acceleration-ish in some builds)
  9..11  orientation
  12..14 orientation rate

Pick hypothesis with max selection logit. That curve is the **planned path**,
not a painted lane.

================================================================================
LANES / ROAD EDGES
================================================================================
4 lines: leftFar, leftNear, rightNear, rightFar.
2 edges: left, right road edge.

Storage per geometry block (528 for lanes, 264 for edges):
  first half  = means: for each of N lines, 33 pairs (y, z) interleaved
  second half = stds:  same layout (exp(.) for σ)

So index ``i*2`` within a line's 66 floats is **lateral Y**, ``i*2+1`` is **Z**
(height), NOT (y, std) as the GitHub demo's ``seperate_points_and_std_values``
suggests. That demo only works for Y by accident (even indices).

Lane probs: sigmoid(lane_lines_prob[i*2+1]) for line i.

================================================================================
What the toy demo plots
================================================================================
Yellow “lanes” / red “edges” / green “mid” are a **partial, buggy** read of
the lane/edge heads. It never draws the PLAN head. Green is
``0.5*(ll_t2 + l_t)`` (blend of two lane tensors), not the planner path.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import List, Optional, Tuple

import numpy as np

X_IDXS = np.array(
    [
        0.0,
        0.1875,
        0.75,
        1.6875,
        3.0,
        4.6875,
        6.75,
        9.1875,
        12.0,
        15.1875,
        18.75,
        22.6875,
        27.0,
        31.6875,
        36.75,
        42.1875,
        48.0,
        54.1875,
        60.75,
        67.6875,
        75.0,
        82.6875,
        90.75,
        99.1875,
        108.0,
        117.1875,
        126.75,
        136.6875,
        147.0,
        157.6875,
        168.75,
        180.1875,
        192.0,
    ],
    dtype=np.float64,
)

PLAN_END = 4955
LANES_END = PLAN_END + 528  # 5483
LANE_PROB_END = LANES_END + 8  # 5491
ROAD_END = LANE_PROB_END + 264  # 5755

PLAN_MHP_N = 5
PLAN_COLS = 15
PLAN_GROUP = 2 * PLAN_COLS * 33 + 1  # 991
TRAJ_N = 33

LANE_NAMES = ("leftFar", "leftNear", "rightNear", "rightFar")


def _sigmoid(x: np.ndarray) -> np.ndarray:
    return 1.0 / (1.0 + np.exp(-np.asarray(x, dtype=np.float64)))


@dataclass
class LaneXYZ:
    name: str
    y: np.ndarray  # (33,) left-positive
    z: np.ndarray  # (33,) up
    prob: float


@dataclass
class PlanXYZ:
    x: np.ndarray
    y: np.ndarray
    z: np.ndarray
    hyp_index: int
    logit: float


@dataclass
class SupercomboOut:
    plan: PlanXYZ
    lanes: List[LaneXYZ]
    edges: List[LaneXYZ]  # left, right; prob unused


def _yz_from_block(block66: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    """33×(y,z) interleaved → y, z arrays."""
    flat = np.asarray(block66, dtype=np.float64).reshape(-1)
    return flat[0::2].copy(), flat[1::2].copy()


def parse_supercombo(out: np.ndarray) -> SupercomboOut:
    """Parse flat 6409 (or longer) model vector."""
    res = np.asarray(out, dtype=np.float64).reshape(-1)
    if res.size < ROAD_END:
        raise ValueError(f"output too short: {res.size}")

    # --- plan: best of 5 MHP ---
    best_i, best_logit = 0, -1e9
    for i in range(PLAN_MHP_N):
        logit = float(res[(i + 1) * PLAN_GROUP - 1])
        if logit > best_logit:
            best_logit, best_i = logit, i
    base = best_i * PLAN_GROUP
    means = res[base : base + PLAN_COLS * TRAJ_N].reshape(TRAJ_N, PLAN_COLS)
    plan = PlanXYZ(
        x=means[:, 0].copy(),
        y=means[:, 1].copy(),
        z=means[:, 2].copy(),
        hyp_index=best_i,
        logit=best_logit,
    )

    # --- lanes: means in first 264, stds in second 264 ---
    lane_raw = res[PLAN_END:LANES_END]
    probs_raw = res[LANES_END:LANE_PROB_END]
    lanes: List[LaneXYZ] = []
    for i, name in enumerate(LANE_NAMES):
        y, z = _yz_from_block(lane_raw[i * 66 : (i + 1) * 66])
        # official: sigmoid(prob[i*2+1])
        prob = float(_sigmoid(probs_raw[i * 2 + 1]))
        lanes.append(LaneXYZ(name=name, y=y, z=z, prob=prob))

    # --- road edges ---
    edge_raw = res[LANE_PROB_END:ROAD_END]
    edges: List[LaneXYZ] = []
    for i, name in enumerate(("edgeLeft", "edgeRight")):
        y, z = _yz_from_block(edge_raw[i * 66 : (i + 1) * 66])
        edges.append(LaneXYZ(name=name, y=y, z=z, prob=1.0))

    return SupercomboOut(plan=plan, lanes=lanes, edges=edges)


def explain_output() -> str:
    return __doc__ or ""
