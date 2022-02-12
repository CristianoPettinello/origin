# -*- coding: utf-8 -*-

import numpy as np

from Model import Model

#
# T Model class
#
class TBevelModel_S(Model):
    def __init__(self, R=0.0, H=0.0):
        Model.__init__(self)

        self.R = R
        self.H = H

        # Load the slope with the Scheimpflug deformation
        self.slope = self.loadDeformationAngle('TModel_ScheimpflugLUT.csv')
        (up, down, beta)  = self.getScheimpflugAngles(self.slope, R, H=H, d = 1) #FIXME

        #print("up " + str(up))
        #print("down " + str(down))
        #a = input()

        self.slopeDown = abs(down) / 180 * 3.141592654
        self.slopeUp =	abs(up) / 180 * 3.141592654

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
        self.x5 = 0
        self.y5 = 0
        self.M = 0.0

        self.A = 0.0
        #self.B = 0.0
        self.C = 0.0
        #self.delta1 = 0.0  # This should be fixed
        #self.delta2 = 0.0  # This should be fixed

        # Computed values

        self.alpha = 0.0


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
            next(reader, None)
            for row in reader:

                # The key is d_R_H
                key = str(float(row[0])) + '_' + row[1] + '_' + row[2]
               # print(key)#.replace('.','p')

                # The slope contains alpha1, alpha2 and beta
                slope[key] = (float(row[3]), float(row[4]), float(row[5]))
        return slope

    def getScheimpflugAngles(self, slope, R, H, d, minR=-20, maxR=20, minH=0, maxH=30, mind=1, maxd=3):
        # FIXME must throw an exception if R and H are outside bounds.
        d = str(float(int(d * 10.0)) / 10.0)
        #d = str(float(d*10)/10)#.replace('.','p')
        intR = str(int(R))
        intH = str(int(H))
        key = d +'_' + intR + '_' + intH
        angles = slope[key]
        return angles

    # Set the model parameters.
    def setModel(self, v):
        self.x1 = v[0]
        self.y1 = v[1]

        self.alpha = v[2]
        self.M = v[3]
        self.S1 = v[4]

        self.A = 40
        self.C = 40

        # Must be transformed from um to pixels in order to get the
        # correct value of d
        um_to_pixel = 1000 / 18.0
        #3um a pixel

        d = float(self.M) / um_to_pixel

        #print("M " + str(self.M))
        #print("d " + str(d))

        d_min = 1.0
        d_max = 3.0

        if d < d_min:
            d = d_min
        elif d > d_max:
            d = d_max

        (alpha1, alpha2, beta)  = self.getScheimpflugAngles(self.slope, self.R, H=self.H, d=d)


        #print("alpha1: " + str(alpha1))
        #print("alpha2: " + str(alpha2))
        #print("beta: " + str(beta))
        #a = input()

        # Convert angles to radians
        alpha1 = alpha1 / 180.0 * 3.141592654
        alpha2 = alpha2 / 180.0 * 3.141592654
        beta = beta / 180.0 * 3.141592654


        #self.S2 = self.S1

        # delta1 and delta2 are the angles of the bevel slopes.
        # The parameters that are optimized are the offset
        # between a central value.
        #self.delta1 = v[4] + 3.141592654 * 0.5 - self.slopeUp
        #self.delta2 = v[5] + 3.141592654 * 0.5 - self.slopeDown
        # For the moment I don't take into account the Scheimpflug deformation but bigger values for v[4] and v[5]
        #self.delta1 = v[4] + 1.0#1.48#85 deg 0.52
        #self.delta2 = 1.570796327 - self.delta1 #v[5] + 1.0#1.48 #0.78

        # FIXME TEST
        #self.delta1 =  + 1.0#1.48#85 deg 0.52
        #self.delta2 = 1.570796327 - self.delta1 #v[5] + 1.0#1.48 #0.78

        #self.E = v[5]

        # Compute the model
        self.__computeModel()

    # Compute the model parameters
    def __computeModel(self):

        alpha = self.alpha# + self.beta
        self.x0 = round(self.x1 - self.A * np.cos(alpha))
        self.y0 = round(self.y1 + self.A * np.sin(alpha))

        delta1 = alpha #+ 3.141592654 / 2
        self.x2 = round(self.x1 - self.S1 * np.sin(delta1))
        self.y2 = round(self.y1 - self.S1 * np.cos(delta1))

        self.x3 = round(self.x2 + self.M * np.cos(alpha))
        self.y3 = round(self.y2 - self.M * np.sin(alpha))

        delta2 = alpha #+ 3.141592654 / 2
        self.x4 = round(self.x3 + self.S1 * np.sin(delta2))
        self.y4 = round(self.y3 + self.S1 * np.cos(delta2))

        self.x5 = round(self.x4 + self.C * np.cos(alpha))
        self.y5 = round(self.y4 - self.C * np.sin(alpha))

        X = [self.x0, self.x1, self.x2, self.x3, self.x4, self.x5]
        Y = [self.y0, self.y1, self.y2, self.y3, self.y4, self.y5]
        Model.setModelPoints(self, X, Y)

    # Return the values
    def getMeasures(self):
        return self.A, self.B, self.C
