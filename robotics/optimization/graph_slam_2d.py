#!/usr/bin/python3
import numpy as np
import matplotlib.pyplot as plt

STATE_SIZE = 3


class Edge:
    def __init__(self):
        self.e = 0
        self.A = None  # de / xi
        self.B = None  # de / xj
        self.id1 = None
        self.id2 = None
        self.OMEGA = None


def t2v(A):
    """
    A = [cos, -sin, x,
              sin, cos,  y,
              0, 0, 1]

    v = [x, y, theta]
    """
    v = np.zeros((3, 1))
    v[0][0] = A[0][2]
    v[1][0] = A[1][2]
    v[2][0] = np.arctan2(A[1][0], A[0][0])
    return v


def v2t(v):
    theta = v[2]
    x = v[0]
    y = v[1]
    A = np.array([[np.cos(theta), -np.sin(theta), x], [np.sin(theta), np.cos(theta), y], [0, 0, 1]])
    return A


def cacl_edges_2d(odom, obs):
    edges = []
    for i in range(1, len(odom)):
        edge = Edge()
        Xi = odom[i - 1]
        Xj = odom[i]
        Zij = obs[i - 1]
        v_i = Xi
        v_j = Xj

        T_i = v2t(v_i)
        T_j = v2t(v_j)
        T_z = v2t(Zij)
        R_i = T_i[0:2, 0:2]
        R_z = T_z[0:2, 0:2]

        si = np.sin(v_i[2])
        ci = np.cos(v_i[2])
        dR_i = np.array([[-si, ci], [-ci, -si]]).T
        dt_ij = v_j[0:2] - v_i[0:2]
        dt_ij = np.reshape(dt_ij, (2, 1))

        # Calculate jacobians
        A = np.zeros((3, 3))
        A[0:2, 0:2] = -R_z.T @ R_i.T
        A[0:2, 2:3] = R_z.T @ dR_i.T @ dt_ij
        A[2, 2] = -1.0

        B = np.zeros((3, 3))
        B[0:2, 0:2] = R_z.T @ R_i.T
        B[2, 2] = 1.0

        e = t2v(np.linalg.inv(T_z) @ np.linalg.inv(T_i) @ T_j)

        edge.A = A
        edge.B = B
        edge.e = e

        edge.id1 = i - 1
        edge.id2 = i
        edge.OMEGA = np.ones((3, 3))
        edges.append(edge)
    return edges


def build_linear_system(odom, obs):
    n_poses = len(odom)
    dim = n_poses * STATE_SIZE
    H = np.zeros((dim, dim))
    b = np.zeros((dim, 1))

    edges = cacl_edges_2d(odom, obs)

    for edge in edges:
        id1 = edge.id1 * STATE_SIZE
        id2 = edge.id2 * STATE_SIZE
        A = edge.A
        B = edge.B
        OMEGA = edge.OMEGA

        H[id1 : id1 + STATE_SIZE, id1 : id1 + STATE_SIZE] += A.T @ OMEGA @ A
        H[id1 : id1 + STATE_SIZE, id2 : id2 + STATE_SIZE] += A.T @ OMEGA @ B
        H[id2 : id2 + STATE_SIZE, id1 : id1 + STATE_SIZE] += B.T @ OMEGA @ A
        H[id2 : id2 + STATE_SIZE, id2 : id2 + STATE_SIZE] += B.T @ OMEGA @ B

        b[id1 : id1 + STATE_SIZE] += A.T @ OMEGA @ edge.e
        b[id2 : id2 + STATE_SIZE] += B.T @ OMEGA @ edge.e

    return H, b


def anchor_first_pose(H, b, weight=1.0):
    """Fix the first pose: delta x_0 = 0."""
    fixed = STATE_SIZE
    H[:fixed, :] = 0
    H[:, :fixed] = 0
    H[:fixed, :fixed] = np.identity(fixed) * weight
    b[:fixed] = 0


def add_component_priors(H, b, x, x_ref, components=(1, 2), weight=1e2):
    """Soft priors on selected pose components (y, theta by default).

    Straight-line odometry does not fully observe global y and theta;
    without these priors the normal equations are singular.
    """
    flat = x.reshape(-1)
    flat_ref = x_ref.reshape(-1)
    for i in range(len(x)):
        for comp in components:
            idx = i * STATE_SIZE + comp
            H[idx, idx] += weight
            b[idx, 0] += weight * (flat[idx] - flat_ref[idx])


def optimize(x, z, tolerance):
    x = np.array(x, dtype=float)
    x_ref = x.copy()
    converged = False

    while not converged:
        H, b = build_linear_system(x, z)
        add_component_priors(H, b, x, x_ref)
        anchor_first_pose(H, b)
        dx = -np.linalg.solve(H, b)
        x += dx.reshape(x.shape)
        x[:, 2] = np.arctan2(np.sin(x[:, 2]), np.cos(x[:, 2]))
        if np.linalg.norm(dx) <= tolerance:
            converged = True

    return x


if __name__ == "__main__":
    x_true = np.array([[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [2.0, 0.0, 0.0], [3.0, 0.0, 0.0]])
    x = np.array([[0.0, 0.0, 0.0], [0.5, 0.0, 0.0], [1.5, 0.0, 0.0], [2.0, 0.0, 0.0]])
    z = np.array([[1.0, 0.0, 0.0], [1.0, 0.0, 0.0], [1.0, 0.0, 0.0]])
    tolerance = 0.01

    plt.plot(x[:, 0], x[:, 1], ".", markersize=20, label="init guess")
    x_opt = optimize(x.copy(), z, tolerance)
    print(x_opt)

    plt.plot(x_true[:, 0], x_true[:, 1], ".", markersize=20, label="ground truth")
    plt.plot(x_opt[:, 0], x_opt[:, 1], ".", markersize=20, label="estimate")
    plt.legend()
    plt.axis("equal")
    plt.show()
