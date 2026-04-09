#!/usr/bin/python3
# ref https://www.youtube.com/watch?v=Kln0ZQ7sX8k
import numpy as np
import matplotlib.pyplot as plt


def f1(a, b, x):
    return a * x / (b + x)


def JacobianF1(f, a, b, x):
    grad_a = x / (b + x)
    grad_b = -a * x / (b + x) ** 2
    return np.column_stack([grad_a, grad_b])


def Jacobian(f, a, b, x):
    eps = 1e-6
    grad_a = (f(a + eps, b, x) - f(a - eps, b, x)) / (2 * eps)
    grad_b = (f(a, b + eps, x) - f(a, b - eps, x)) / (2 * eps)
    return np.column_stack([grad_a, grad_b])


def GaussNewton1d(f, x, y, a0, b0, tol, mat_iter):
    old = new = np.array([a0, b0])
    for _ in range(mat_iter):
        old = new
        J = JacobianF1(f, old[0], old[1], x)
        e = y - f(old[0], old[1], x)
        b = J.T @ e
        H = J.T @ J
        new = old + np.linalg.inv(H) @ b

        if np.linalg.norm(old - new) < tol:
            break

    return new


if __name__ == "__main__":
    x = np.linspace(0, 5, 50)
    y = f1(2, 3, x) + np.random.normal(0, 0.1, size=50)

    plt.scatter(x, y)

    a, b = GaussNewton1d(f1, x, y, 5, 1, 1e-5, 10)
    print(a, b)

    y_hat = f1(a, b, x)

    plt.scatter(x, y_hat)
    plt.show()
