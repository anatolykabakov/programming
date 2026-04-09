#!/usr/bin/python3
import numpy as np
import matplotlib.pyplot as plt

STATE_SIZE = 1


class Edge:
    def __init__(self):
        self.e = 0
        self.A = None  # de / xi
        self.B = None  # de / xj
        self.id1 = None
        self.id2 = None
        self.OMEGA = None


def cacl_edges_1d(odom, obs):
    edges = []
    for i in range(1, len(odom)):
        edge = Edge()
        Xi = odom[i - 1]
        Xj = odom[i]
        Zij = obs[i - 1]
        edge.e = Zij - (Xj - Xi)
        edge.A = np.array([1.0])
        edge.B = np.array([-1.0])
        edge.id1 = i - 1
        edge.id2 = i
        edge.OMEGA = np.array([1.0])
        edges.append(edge)
    return edges


def build_linear_system(odom, obs):
    N = len(odom)
    H = np.zeros((N, N))
    b = np.zeros((N, 1))

    edges = cacl_edges_1d(odom, obs)

    for edge in edges:
        id1 = edge.id1
        id2 = edge.id2
        A = edge.A
        B = edge.B
        OMEGA = edge.OMEGA

        H[id1 : id1 + STATE_SIZE, id1 : id1 + STATE_SIZE] += np.multiply(
            np.multiply(A.T, OMEGA), A
        )
        H[id1 : id1 + STATE_SIZE, id2 : id2 + STATE_SIZE] += np.multiply(
            np.multiply(A.T, OMEGA), B
        )
        H[id2 : id2 + STATE_SIZE, id1 : id1 + STATE_SIZE] += np.multiply(
            np.multiply(B.T, OMEGA), A
        )
        H[id2 : id2 + STATE_SIZE, id2 : id2 + STATE_SIZE] += np.multiply(
            np.multiply(B.T, OMEGA), B
        )

        b[id1 : id1 + STATE_SIZE] += np.multiply(np.multiply(A.T, OMEGA), edge.e)
        b[id2 : id2 + STATE_SIZE] += np.multiply(np.multiply(B.T, OMEGA), edge.e)

    return H, b


def optimize(x, z, tolerance):
    converged = False

    while not converged:
        H, b = build_linear_system(x, z)
        H[(0, 0)] += 1
        dx = -np.linalg.inv(H) @ b
        x += dx
        if np.sum(dx) <= tolerance:
            converged = True

    return x


if __name__ == "__main__":
    xTrue = np.array([[0.0], [1.0], [2.0], [3.0]])
    x = np.array([[0.0], [0.5], [1.5], [2.0]])
    z = np.array([[1.0], [1.0], [1.0]])
    tolerance = 0.01

    plt.plot(x, np.zeros(x.shape), '.', markersize=20, label='init guess')
    x_opt = optimize(x, z, tolerance)
    print(x_opt)

    plt.plot(xTrue, np.zeros(xTrue.shape), '.', markersize=20, label='Ground truth')
    plt.plot(x_opt, np.zeros(x_opt.shape), '.', markersize=20, label='estimate')
    plt.show()
