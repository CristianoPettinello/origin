# -*- coding: utf-8 -*-

import numpy as np

from Model import Model

#
# Custom Bevel Model class
#
class CustomBevelModel_S(Model):
    def __init__(self, independent_angles=False, R=0, H=0):
        Model.__init__(self)

        # FIXME: independent angles va rimossa
        self.independent_angles = independent_angles
        self.R = R
        self.H = H

        # Load the slope with the Scheimpflug deformation
        self.slope = self.loadDeformationAngle('CustomBevelModel_ScheimpflugLUT.csv')
        (up, down) = self.getScheimpflugAngles(self.slope, R, H)

        #print((up, down))
        #a = input()

        # Normal angles
        self.slopeDown = abs(down) / 180 * 3.141592654
        self.slopeUp =	abs(up) / 180 * 3.141592654
        # Inverted angles
        #self.slopeDown = abs(up) / 180 * 3.141592654
        #self.slopeUp =	abs(down) / 180 * 3.141592654
        #print("Custom Bevel Model using Inverted angles")


        # Other parameters
        self.x1 = 0
        self.y1 = 0
        self.x3 = 0
        self.y3 = 0
        self.x3B = 0
        self.y3B = 0

        self.A = 0.0
        self.B = 0.0
        self.C = 0.0
        self.delta1 = 0.0  # This should be fixed
        self.delta2 = 0.0  # This should be fixed

        # Computed values
        self.M = 0.0
        self.alpha = 0.0
        self.x0 = 0
        self.y0 = 0
        self.x2 = 0
        self.y2 = 0
        self.x2B = 0 # ADG added
        self.y2B = 0 # ADG added

        self.x4 = 0
        self.y4 = 0

    def loadDeformationAngle(self, file_name):
        import csv
        slope = dict()

        #####################################################
        # Provided parameters are R, H, omega, rho
        # where:
        # R and H are the angles of the HTT stylus.
        # omega and Rho are the up slope and downslope.
        #####################################################
        with open(file_name, 'r') as csvfile:
            reader = csv.reader(csvfile, delimiter=';', quotechar='|')

            # Skip the first row
            next(reader, None)
            for row in reader:
                # The key is R_H
                key = row[0] + '_' + row[1]
                slope[key] = (float(row[2]), float(row[3]))
        return slope

    def getScheimpflugAngles(self, slope, R, H=0, minR=-20, maxR=20, minH=0, maxH=25):
        # FIXME must throw an exception if R and H are outside bounds.

        # Slope is organized as a dictionary where the first key is R.
        intR = str(int(R))
        intH = str(int(H))
        key = intR + '_' + intH
        angles = slope[key]
        return angles

    # Set the model parameters.
    def setModel(self, v):
        if not self.independent_angles:
            # This is the original version of the model.
            # In this specification, the angles of the bevel slopes are
            # kept the same.
            self.x1 = v[0]
            self.y1 = v[1]
            self.alpha = v[2]
            self.M = v[3]
            self.A = v[4]
            self.C = v[5]
            self.delta1 = v[6]
        else:
            self.x1 = v[0]
            self.y1 = v[1]
            self.alpha = v[2]
            self.S1 = v[3]
            self.S2 = v[4]
            # A and C are the size of the bevel shoulders. In this model
            # they are kept fixed.
            self.A = 40
            self.C = 40
            # delta1 and delta2 are the angles of the bevel slopes.
            # The parameters that are optimized are the offset
            # between a central value.
            #self.delta1 = v[4] + 3.141592654 * 0.5 - self.slopeUp
            #self.delta2 = v[5] + 3.141592654 * 0.5 - self.slopeDown
            # For the moment I don't take into account the Scheimpflug deformation but bigger values for v[4] and v[5]

            #####################################################################################
            # NOTE: this is the definition before introducing the Scheimpflug deformation
            #self.delta1 = v[5] + 0.52
            ##self.delta2 = v[6] + 0.78

            ## FIX 2020-04-21: working on the T Model resulted that the definition of delta2 should be corrected like this:
            #self.delta2 = 1.570796327 - (v[6] + 0.78)
            ####################################################################################

            # Take into consideration the scheimpflug deformation

            self.delta1 = v[5] + 3.141592654 * 0.5 - self.slopeUp
            # FIX 2020-04-21: working on the T Model resulted that the definition of delta2 should be corrected like this:
            self.delta2 = 1.570796327 - (v[6] + 3.141592654 * 0.5 - self.slopeDown)

            ####################################################################################

            self.E = v[7]
            self.F = v[8]

        # Compute the model
        self.__computeModel()

    # Compute the model parameters
    def __computeModel(self):
        self.x0 = round(self.x1 - self.A * np.cos(self.alpha))
        self.y0 = round(self.y1 - self.A * np.sin(self.alpha))

        self.x2 = round(self.x1 + self.S1 * np.cos(self.delta1))
        self.y2 = round(self.y1 - self.S1 * np.sin(self.delta1)) ####### + -> -

        self.x2B = round(self.x2 + self.E * np.cos(self.alpha))
        self.y2B = round(self.y2 + self.E * np.sin(self.alpha)) # + -> -

        self.x3 = round(self.x2B + self.S2 * np.sin(self.delta2))
        self.y3 = round(self.y2B + self.S2 * np.cos(self.delta2))######

        self.x3B = round(self.x3 + self.F * np.cos(self.alpha))
        self.y3B = round(self.y3 + self.F * np.sin(self.alpha))

        self.x4 = round(self.x3B + self.C * np.cos(self.alpha))
        self.y4 = round(self.y3B + self.C * np.sin(self.alpha))

        X = [self.x0, self.x2, self.x2B, self.x3, self.x3B, self.x4]
        Y = [self.y0, self.y2, self.y2B, self.y3, self.y3B, self.y4]
        Model.setModelPoints(self, X, Y)

    # Return the values
    def getMeasures(self):
        return self.A, self.B, self.C
