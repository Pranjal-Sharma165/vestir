"""Contains functions missing from NumPy that are present and used in Torch."""

import numpy as np
import torch


def softmax(x, axis=-1):
    if isinstance(x, torch.Tensor):
        return torch.softmax(x, dim=axis)

    e = np.exp(x - np.max(x, axis=axis, keepdims=True))
    return e / e.sum(axis=axis, keepdims=True)