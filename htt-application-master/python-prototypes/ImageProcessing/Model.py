# -*- coding: utf-8 -*-

import numpy as np
from ScheimpflugModel import ScheimpflugModel

class Model:
    def __init__(self):

        self.sm = ScheimpflugModel()

        self.loss_map = None
        self.xcc = None
        self.ycc = None
        self.size = None

        self.X_REF = None
        self.Y_REF = None

        self.min_xcc = None
        self.max_xcc = None
        self.min_ycc = None
        self.max_ycc = None

        # The following depends on the actual model.
        self.X = None
        self.Y = None
        self.L = None
        self.K = None


    def object_to_image(self, R, H, M0):
        return self.sm.object_to_image(R, H, M0)


    def image_to_object(self, R, H, MS):
        return self.sm.image_to_object(R, H, MS)

    def mm_to_pixel(self, x_in_um):
        return self.sm.mm_to_pixel(x_in_um)

    def pixel_to_mm(self, x_in_pixel):
        return self.sm.pixel_to_mm(x_in_pixel)

    # Set the loss map used during the model optimization
    def setLossMap(self, loss_map, xcc, ycc, size):
        self.loss_map = loss_map
        self.xcc = xcc
        self.ycc = ycc
        self.size = size
        self.X_REF = self.xcc.astype(int)
        self.Y_REF = self.ycc.astype(int)


        # Compute some index
        self.min_xcc = np.min(xcc)
        self.max_xcc = np.max(xcc)
        self.min_ycc = np.min(ycc)
        self.max_ycc = np.max(ycc)

    # Set the points that represent the model
    # in the image space
    def setModelPoints(self, X, Y):
        self.X = np.array(X)
        self.Y = np.array(Y)
        #self.H = np.zeros_like(self.X)

        self.L = np.zeros_like(self.X)
        self.K = np.zeros_like(self.X)

        num_points = len(self.X)
        for i in range(num_points - 1):
            xi = self.X[i]
            yi = self.Y[i]
            xj = self.X[i + 1]
            yj = self.Y[i + 1]

            # FIXME modifica
            #Lji = xj - xi
            #Hji = (yj - yi) / Lji
            #self.H[i] = Hji

            Lji = xj - xi
            Kji = yj - yi
            self.L[i] = Lji
            self.K[i] = Kji

    def mse_m(self, it):
        # FIXME Get min and max x of profile under test
        #if self.x2 < min_xcc or self.x2 >= max_xcc:
        #    return 1e101
        #if self.x3 < min_xcc or self.x3 >= max_xcc:
        #    return 1e101

        counter = 0
        loss_value = 0.0
        step = 1
        #print("points")
        #print(self.X)
        #print(self.Y)
        #print(self.L)
        #print(self.K)

        # Get the number of points in the model and compute
        # the loss function associated to each segment.
        num_points = len(self.X)
        for i in range(num_points - 1):
            # (xi, yi) is the starting point of the segment,
            # (xj, yj) is the ending point.
            xi = self.X[i]
            yi = self.Y[i]
            xj = self.X[i + 1]
            yj = self.Y[i + 1]

            Kji = self.K[i]
            Lji = self.L[i]

            if Lji >= Kji:
                start_x = int(np.max([xi, 0]))
                end_x = int(np.min([xj, self.size - 1])) + 1

                #print("start_x ", start_x, end_x)
                Xji = np.arange(start_x, end_x, step, dtype = float)

                if abs(Lji) >= 1:
                    Yji = yi + Kji/Lji * (Xji - xi)
                else:
                    Yji = np.array([yi])
                #print(np.min(Yji), np.max(Yji))
                # This should prevent to have the trace going out of the
                # loss_map
                #if len(Yji) and np.max(Yji) >= self.size:
                if len(Yji) > 0 and np.max([Yji[0], Yji[-1]]) >= self.size:
                    return 1e101

                counter += end_x - start_x

            else:
                start_y = int(np.max([yi, 0]))
                end_y = int(np.min([yj, self.size - 1])) + 1

                #print("start_y ", start_y, end_y)
                Yji = np.arange(start_y, end_y, step, dtype = float)

                if abs(Kji) >= 1:
                    Xji = xi + Lji/Kji * (Yji - yi)
                else:
                    Xji = np.array([xi])


                #Xji = Lji/Kji * (Yji - yi) + xi

                # This should prevent to have the trace going out of the
                # loss_map
                #if len(Xji) > 0 and np.max(Xji) >= self.size:
                if len(Xji) > 0 and np.max([Xji[0], Xji[-1]]) >= self.size:
                    return 1e101

                counter += end_y - start_y

            # Conversion to integer
            Xji = Xji.astype(int)
            Yji = Yji.astype(int)
            try:
                loss_value += np.sum(self.loss_map[Yji, Xji])
            except:
                print(Lji)
                print(Yji, Xji)

        if counter > 0:
            return np.sqrt(loss_value) / counter
        else:
            return np.sqrt(loss_value)

    # Check model against connected components.
    def checkConstraint_m(self):
        num_points = len(self.X)
        for i in range(num_points - 1):
            xi = self.X[i]
            yi = self.Y[i]
            xj = self.X[i + 1]
            yj = self.Y[i + 1]
            Lji = self.L[i]
            Kji = self.K[i]

            if Lji >= Kji:
                I = (self.xcc >= xi) & (self.xcc < xj)
                Yji_REF = self.Y_REF[I]
                Xji = self.xcc[I]

                if abs(Lji) >= 1.0:
                    Yji = Kji/Lji * (Xji - xi) + yi
                else:
                    Yji = yi

                if np.count_nonzero(Yji <= Yji_REF) > 0:
                    return -1
            else:
                I = (self.ycc >= yi) & (self.ycc < yj)
                Xji_REF = self.X_REF[I]
                Yji = self.ycc[I]
                if abs(Kji) >= 1.0:
                    Xji = Lji/Kji * (Yji - yi) + xi
                else:
                    Xji = xi
                if np.count_nonzero(Xji <= Xji_REF) > 0:
                    return -1

        return 0

    # Trace the model on an image
    def trace_m(self):
        # Trace single line, We use x as the reference axis.
        xf = np.array([])
        yf = np.array([])

        step = 1

        #self.X = self.X[::-1]
        #self.Y = self.Y[::-1]


        num_points = len(self.X)
        for i in range(num_points - 1):
            xi = self.X[i]
            yi = self.Y[i]
            xj = self.X[i + 1]
            yj = self.Y[i + 1]
            Lji = self.L[i]
            Kji = self.K[i]


            if Lji >= Kji:

                start_x = int(np.max([xi, 0]))
                end_x = int(np.min([xj, self.size - 1])) + 1

                Xji = np.arange(start_x, end_x, step, dtype = float)

                #if abs(Lji) > 1:
                Yji = yi + Kji/Lji * (Xji - xi)
                #else:
                #    Yji = yi
            else:
                start_y = int(np.max([yi, 0]))
                end_y = int(np.min([yj, self.size - 1])) + 1

                Yji = np.arange(start_y, end_y, step, dtype = float)

                #if abs(Kji) > 1:
                Xji = xi + Lji/Kji * (Yji - yi)
                #else:
                #    Xji = xi

            Xji = Xji.astype(int)
            Yji = Yji.astype(int)

            if len(Xji) > 0:
                xf = np.concatenate((xf, Xji))
                yf = np.concatenate((yf, Yji))

        return xf, yf
