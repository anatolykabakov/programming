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
    A = np.array([[np.cos(theta), -np.sin(theta), x],
                  [np.sin(theta),  np.cos(theta), y],
                  [0,  0, 1]])
    return A


def cacl_edges_2d(odom, obs):
    edges = []
    for i in range(1, len(odom)):
        edge = Edge()
        Xi = odom[i - 1]
        Xj = odom[i]
        Zij = obs[i - 1]
        theta_i = Xi[2]

        si = np.sin(theta_i)
        ci = np.cos(theta_i)
            
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
        A[2, 2] = -1.

        B = np.zeros((3, 3))
        B[0:2, 0:2] = R_z.T @ R_i.T
        B[2, 2] = 1.

        e = t2v(np.linalg.inv(T_z) * np.linalg.inv(T_i) * T_j)

        edge.A = A
        edge.B = B
        edge.e = e

        edge.id1 = i - 1
        edge.id2 = i
        edge.OMEGA = np.ones((3, 3))
        edges.append(edge)
    return edges

def build_linear_system(odom, obs):
    N = len(odom) # odom.shape[1] * STATE_SIZE
    H = np.zeros((N, N))
    b = np.zeros((N, 3))

    edges = cacl_edges_2d(odom, obs)

    for edge in edges:
        id1 = edge.id1
        id2 = edge.id2
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


def optimize(x, z, tolerance):
    converged = False

    while not converged:
        H, b = build_linear_system(x, z)
        H[0:STATE_SIZE, 0:STATE_SIZE] += np.identity(STATE_SIZE)
        dx = -np.linalg.inv(H) @ b
        x += dx
        if np.sum(dx) <= tolerance:
            converged = True

    return x


if __name__ == "__main__":
    # xTrue = np.array([[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [2.0, 0.0, 0.0], [3.0, 0.0, 0.0]])
    x = np.array([[0.0, 0.0, 0.0], [0.5, 0.0, 0.0], [1.5, 0.0, 0.0], [2.0, 0.0, 0.0]])
    z = np.array([[1.0, 0.0, 0], [1.0, 0.0, 0], [1.0, 0.0, 0]])
    tolerance = 0.01

    # plt.plot(x, np.zeros(x.shape), '.', markersize=20, label='init guess')
    x_opt = optimize(x, z, tolerance)
    print(x_opt)

    # plt.plot(xTrue, np.zeros(xTrue.shape), '.', markersize=20, label='Ground truth')
    # plt.plot(x_opt, np.zeros(x_opt.shape), '.', markersize=20, label='estimate')
    # plt.show()
    
