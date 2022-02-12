# -*- coding: utf-8 -*-
"""
Created on Mon Feb 24 12:26:46 2020

@author: mminozzi
"""

import math
import numpy as np

def RX(theta):
    theta = theta / 180 * math.pi
    RX = np.array([[1.0, 0.0, 0.0],
                   [0.0, math.cos(theta), -math.sin(theta)],
                   [0.0, math.sin(theta), math.cos(theta)]])

    return RX
