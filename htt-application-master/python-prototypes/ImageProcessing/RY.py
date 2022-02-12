# -*- coding: utf-8 -*-
"""
Created on Mon Feb 24 13:35:42 2020

@author: mminozzi
"""

import math
import numpy as np

def RY(theta):
    theta = theta / 180.0 * math.pi
    RY = np.array([[math.cos(theta), 0.0, math.sin(theta)],
                    [0.0, 1.0, 0.0],
                    [-math.sin(theta), 0.0, math.cos(theta)]])

    return RY
