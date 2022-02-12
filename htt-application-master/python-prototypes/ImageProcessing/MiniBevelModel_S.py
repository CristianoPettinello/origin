# -*- coding: utf-8 -*-

import csv
import numpy as np

from Model import Model

#
# Mini Bevel Model class
#
class MiniBevelModel_S(Model):
    def __init__(self, R=0.0, H=0.0):
        Model.__init__(self)

        self.R = R
        self.H = H

        # Other parameters
        self.x0 = 0
        self.y0 = 0
        self.x1 = 0
        self.y1 = 0
        self.x2 = 0
        self.y2 = 0
        self.x3 = 0
        self.y3 = 0
        self.x4 = 0
        self.y4 = 0

        self.A = self.pixel_to_mm(40)
        self.C = self.pixel_to_mm(40)
        self.B = 0.0
        self.delta = 0.610865238  # Fixed angle in the real domain = 90 - 55

        # Computed values
        self.M = 0.0
        self.alpha = 0.0

    # Set the model parameters.
    def setModel(self, v):
        self.x1 = v[0]
        self.y1 = v[1]
        self.alpha = v[2]
        self.M = v[3]

        # Compute the model
        self.__computeModel()

    # Compute the model parameters
    def __computeModel(self):

        sin_alpha = np.sin(self.alpha)
        cos_alpha = np.cos(self.alpha)

        self.x0 = self.x1 - self.A * sin_alpha
        self.y0 = self.y1 + self.A * cos_alpha

        self.x3 = self.x1 + self.M * sin_alpha
        self.y3 = self.y1 - self.M * cos_alpha

        self.x4 = self.x3 + self.C * sin_alpha
        self.y4 = self.y3 - self.C * cos_alpha

        m = self.M / 2
        xm = self.x1 + m * sin_alpha
        ym = self.y1 - m * cos_alpha

        # Y axis down oriented as images
        self.B = np.tan(self.delta) * m
        self.x2 = xm + self.B * cos_alpha
        self.y2 = ym + self.B * sin_alpha


        XX = [self.x0, self.x1, self.x2, self.x3, self.x4]
        YY = [self.y0, self.y1, self.y2, self.y3, self.y4]

        M0 = np.array([XX, YY, [0, 0, 0, 0, 0]], dtype=np.float)

        # Maps to image plane.
        MSp = self.object_to_image(self.R, self.H, M0)

        # FIXME Beware that X and Y are inverted
        Y = [MSp[0,0], MSp[0,1], MSp[0,2], MSp[0,3], MSp[0,4]]
        X = [MSp[1,0], MSp[1,1], MSp[1,2], MSp[1,3], MSp[1,4]]

        Model.setModelPoints(self, X, Y)


    def getObjectModel(self):
        # After that setModel has been called the following variables are in the
        # image domain
        Xobj = [self.x0, self.x1, self.x2, self.x3, self.x4]
        Yobj = [self.y0, self.y1, self.y2, self.y3, self.y4]
        return (Xobj, Yobj)


    def getImageModel(self):
        Xobj = [self.x0, self.x1, self.x2, self.x3, self.x4]
        Yobj = [self.y0, self.y1, self.y2, self.y3, self.y4]

        M0 = np.array([Xobj, Yobj, [0, 0, 0, 0, 0]], dtype=np.float)

        # Maps to image plane.
        MSp = self.object_to_image(self.R, self.H, M0)

        # FIXME Beware that X and Y are inverted
        Y = [MSp[0,0], MSp[0,1], MSp[0,2], MSp[0,3], MSp[0,4]]
        X = [MSp[1,0], MSp[1,1], MSp[1,2], MSp[1,3], MSp[1,4]]

        return (Y, X)

    # Return the values
    def getMeasures(self):
        return (self.A, self.B, self.C, self.M)
