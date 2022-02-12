# -*- coding: utf-8 -*-
"""
Created on Mon Feb 24 13:37:57 2020

@author: mminozzi
"""

import math
import numpy as np

def RZ(theta):
    theta = theta / 180.0 * math.pi
    RZ = np.array([[math.cos(theta), -math.sin(theta), 0.0],
                    [math.sin(theta), math.cos(theta), 0.0],
                    [0.0, 0.0, 1.0]])

    return RZ
