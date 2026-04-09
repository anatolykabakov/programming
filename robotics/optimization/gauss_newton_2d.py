# ref https://www.youtube.com/watch?v=Kln0ZQ7sX8k
import numpy as np
import matplotlib.pyplot as plt


def f1(b, X):
    return b[0] - (1 / b[1]) * X[:, 0] ** 2 - (1 / b[2]) * X[:, 1] ** 2


def Jacobian(f, b, x):
    eps = 1e-6
    grads = []
    for i in range(len(b)):
        t = np.zeros_like(b).astype(float)
        t[i] = t[i] + eps
        grad = (f(b + t, x) - f(b - t, x)) / (2 * eps)
        grads.append(grad)
    return np.column_stack(grads)


def GaussNewton2d(f, x, y, b0, tol, mat_iter):
    old = new = b0
    for _ in range(mat_iter):
        old = new
        J = Jacobian(f, old, x)
        e = y - f(old, x)
        b = J.T @ e
        H = J.T @ J
        new = old + np.linalg.inv(H) @ b
        if np.linalg.norm(old - new) < tol:
            break
    return new


if __name__ == "__main__":
    x1 = np.linspace(-5, 5, 50)
    x2 = np.linspace(-5, 5, 50)
    X1, X2 = np.meshgrid(x1, x2)
    X = np.column_stack([X1.ravel(), X2.ravel()])
    y = f1(np.array([5, 4, 1]), X) + np.random.normal(0, 1, size=len(X))

    init_guess = np.array([3, 3, 1])
    max_iter = 10
    tolerance = 1e-5
    b = GaussNewton2d(f1, X, y, init_guess, tolerance, max_iter)

    print(b)

    fig = plt.figure()
    ax = plt.axes(projection="3d")
    ax.scatter3D(X[:, 0], X[:, 1], y)
    plt.show()
