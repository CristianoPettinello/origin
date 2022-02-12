# -*- coding: utf-8 -*-

import numpy as np

from RX import RX
from RY import RY
from RZ import RZ

class ScheimpflugModel:
    def __init__(self):
        ##############################
        # Calibrated parameters
        ##############################

        # Total track of the imaging path (axial length from the center of
        # the object space to the center of the imaging space) in mm.
        self.tt = 39

        # Nodal point distance of the object space from the system origin C of the object plane
        self.p1 = 32.33

        # Nodal point distance of the imaging space from the system origin C of the object plane
        self.p2 = 31.24

        # Pixel size in mm
        self.px = 0.003

        # Half sensor height in mm
        self.hs = 0.6

        # Frame size = hs / px * 2
        self.frame_size = self.hs / self.px * 2
        self.half_frame = self.hs / self.px

        self.hFoV = 2.5
        self.pxFoV = 0.01    # Fictitious pixel in the object plane

        self.theta = -53.84
        self.phi = 16.7

        self.alpha = -18.2
        self.beta = 4

        self.magnification = -0.241

        self.delta = 6


    def mm_to_pixel(self, x_in_um):
        return x_in_um / self.px * abs(self.magnification)

    def pixel_to_mm(self, x_in_pixel):
        return x_in_pixel * self.px / abs(self.magnification)


    #############################################################################################
    # Convert from points on the object (3D in mm) to points on the image plane (2D in pixels).
    #
    # R     Stilus R angle
    # H     Stilus H angle
    #############################################################################################
    def object_to_image(self, R, H, M0, R_min=-20.0, R_max=20.0, H_min=0.0, H_max=30.0):

        # FIXME check min and max

        #########################################
        # Raytracing calculation
        #########################################
        Rtf = RY(self.theta) @ RX(self.phi)
        Rab = RX(self.beta) @ RY(self.alpha)

        #print("RY_theta", RY(self.theta)) #DEBUG
        #print("RX_phi", RX(self.phi)) #DEBUG

        #print("Rtf", Rtf) #DEBUG
        #print("Rab", Rab) #DEBUG

        # Vector that identifies the OA
        vr = RY(self.theta) @ RX(self.phi) @ np.array([[0], [0], [1]])

        #print("vr", vr) #DEBUG

        # Parameters to calculate the parametric equation of the straight line

        # Center of the sensor S and nodal points P1 and P2
        S = np.array([[self.tt * vr[0][0]], [self.tt * vr[1][0]], [self.tt * vr[2][0]]])
        P1 = np.array([[self.p1 * vr[0][0]], [self.p1 * vr[1][0]], [self.p1 * vr[2][0]]])
        P2 = np.array([[self.p2 * vr[0][0]], [self.p2 * vr[1][0]], [self.p2 * vr[2][0]]])

        #print("S", S) #DEBUG
        #print("P1", P1) #DEBUG
        #print("P2", P2) #DEBUG

        # Vector perpendicular to the sensor plane
        nS = Rtf @ Rab @ np.array([[0],[0],[1]])

        #print("nS", nS) #DEBUG

        n = 0

        # Punti di campionamento nel mondo reale.
        #M0 = np.array([[0, 0, 0],
        #               [0, 1, -1],
        #               [0, 0, 0]], dtype=np.float)

        #slopeMatrix = np.zeros((len(Rangle) * len(Hangle), 5)) # ADG removed
        vt = np.zeros((3, np.size(M0, 1)))

        xS = np.zeros((1, np.size(vt, 1)))
        yS = xS.copy()
        zS = xS.copy()

        nR = 0
        MS = M0.copy()

        #for H in Hangle:

        M = RZ(H) @ M0
        MforH = M.copy() # FIXME

        #print("M", M)

        #for R in Rangle:

        vg = RY(R) @ RY(self.delta) @ np.array([[0], [0], [1]])
        C = RY(R) @ np.array([[0],[0],[1]]) - np.array([[0], [0], [1]])
        B = np.array([[0], [0], [(vg[2][0] * C[2][0] + vg[0][0] * C[0][0]) / vg[2][0]]])

        dBC = np.sqrt((B[0][0] - C[0][0]) ** 2 + (B[1][0] - C[1][0]) ** 2 + (B[2][0] - C[2][0]) ** 2)

        #print("vg", vg) #DEBUG
        #print("C", C) #DEBUG
        #print("B", B) #DEBUG
        #print("dBC", dBC) # DEBUG


        M[0][:] = MforH[0][:]

        #print("M0", M[0][:]) # DEBUG

        M[0][:] = M[0][:]*np.cos((R + self.delta) / 180.0 * np.pi) + np.sign(C[0][0]) * dBC

        #print("M0", M[0][:])# DEBUG
        #print("P1", P1)

        # Calculation of the angular coefficient of straight line t from grid points to P2


        for n in np.arange(np.size(M0, 1)):
            vt[:, n:n+1] = -M[:, n:n+1] + P1
            vt[:, n:n+1] = vt[:, n:n+1] / np.sqrt(np.sum(vt[:, n:n+1] ** 2))

        #print("vt", vt)# DEBUG

        # Intersection point on the sigma plane of the sensor

        for n in np.arange(np.size(vt, 1)):
            lam = ( (nS[0][0] * S[0][0] + nS[1][0] * S[1][0] + nS[2][0] * S[2][0]) - (nS[0][0] * P2[0][0] + nS[1][0] * P2[1][0] + nS[2][0] * P2[2][0]) ) / (nS[0][0] * vt[0][n] + nS[1][0] * vt[1][n] + nS[2][0] * vt[2][n])

            #print("lam", lam)

            xS[0][n] = P2[0][0] + vt[0][n] * lam
            yS[0][n] = P2[1][0] + vt[1][n] * lam
            zS[0][n] = P2[2][0] + vt[2][n] * lam



        MS[0, :] = xS
        MS[1, :] = yS
        MS[2, :] = zS # This should be always 0

        #print("MS", MS-S)

        # Msp contiene M0 rimappato al centro del sensore.
        # [0, 0, 0] mm del mondo reale è mappato sul centro del sensore (in mm)
        MSp = np.transpose(Rtf @ Rab) @ (MS - S)
        # MSp / px -> è in pixel e varia tra [-200 e 200]

        nR = nR + 1


        # Image coordinates
        MSp = (MSp / self.px)

        # Correct center and convert to int

        n_points = MSp.shape[1]

        #centerx = self.frame_size // 2
        #centery = self.frame_size // 2

        for l in range(n_points):
            MSp[0,l] += self.half_frame
            MSp[1,l] += self.half_frame


        # Conversion to int
        MSp = MSp.astype(int)

        return MSp



    #############################################################################################
    # Convert to points on the object (3D in mm) from points on the image plane (2D in pixels).
    #
    # R     Stilus R angle
    # H     Stilus H angle
    #############################################################################################
    def image_to_object(self, R, H, MS, R_min=-20.0, R_max=20.0, H_min=0.0, H_max=30.0):


        ######################################################################
        # Raytracing calculation
        ######################################################################

        # Rtf = RotationMatrix(theta,phi)
        Rtf = RY(self.theta) @ RX(self.phi);
        Rab = RX(self.beta) @ RY(self.alpha);

        # Vector of the Optical Axis (OA)
        vr = RY(self.theta) @ RX(self.phi) @ np.array([[0], [0], [1]])

        # Center of the sensor and nodal points
        S = np.array([self.tt * vr[0], self.tt * vr[1], self.tt * vr[2]])
        P1 = np.array([self.p1 * vr[0], self.p1 * vr[1], self.p1 * vr[2]])
        P2 = np.array([self.p2 * vr[0], self.p2 * vr[1], self.p2 * vr[2]])

        # Punti di interesse sul sensore.
        # FIXME
        #PSens = np.load(pathframepoints + framename + '.npy')
        #PSens = np.vstack([PSens[0],PSens[1],np.zeros(np.size(PSens,1))])
        #PScenter = np.array([[np.around(cx0)],[np.around(cy0)],[0]])
        #PSens = (PSens - PScenter)*px


        PScenter = np.array([[self.half_frame],[self.half_frame],[0]])
        PSens = (MS - PScenter) * self.px


        # Point of the sensor grid PS in the coordinate system x, y and z
        PSens2 = PSens.copy()

        for n in range(np.size(PSens, 1)):
            PSens2[:, n:n+1] = Rtf @ Rab @ PSens[:, n:n+1] + S

        # Slope of straight line t from PS2 to P2

        vt = np.zeros((3, np.size(PSens, 1)))

        for n in range(np.size(PSens, 1)):
            vt[:, n:n+1] = PSens2[:, n:n+1] - P2
            vt[:, n:n+1] = vt[:, n:n+1] / np.sqrt(sum(vt[:, n:n+1] ** 2))


        # Intersection points onto the plane xy
        xO = np.zeros((1, np.size(vt, 1)))
        yO = xO.copy()
        zO = xO.copy()

        vg = RY(R) @ RY(self.delta) @ np.array([0, 0, 1])
        vg = np.reshape(vg, (3, 1))
        C = RY(R) @ np.array([0, 0, 1]) - np.array([0, 0, 1])
        C = np.reshape(C, (3, 1)) #

        for n in range(np.size(vt, 1)):
            lam = -P1[2, 0] / vt[2, n]
            xO[0, n] = P1[0, 0] + vt[0, n] * lam
            yO[0, n] = P1[1, 0] + vt[1, n] * lam
            zO[0, n] = P1[2, 0] + vt[2, n] * lam

        PO = np.zeros((3, np.size(PSens, 1)))

        PO[0,:] = xO
        PO[1,:] = yO
        PO[2,:] = zO

        # Center the vertex of the bevel in O = [0,0,0]
        #------------------------------------------------------------------------------

        ##################################
        # FIXME: Da controllare se serve (in fase di calibrazione)
        #k = np.where(PO[0, :] == np.max(PO[0, :]))[0]
        #bevelcenter = PO[:, k]
        #B = np.array([0, 0, (vg[2, 0] * C[2, 0] + vg[0, 0] * C[0, 0] / vg[2, 0])])
        #B = np.reshape(B, (3, 1))
        #PO = PO - bevelcenter + B
        ##################################

        # Projection
        #------------------------------------------------------------------------------

        PO[0, :] = PO[0, :] * np.cos((self.delta + R) / 180.0 * np.pi)

        # Rotation H
        #------------------------------------------------------------------------------

        ##################################
        # FIXME da controllare se serve (in fase di calibrazione)
        # Punti Psens convertiti nel mondo reale.
        #PO = RZ(H) @ PO
        ##################################


        # Plot
        #------------------------------------------------------------------------------

        #plt.figure(1)
        ##ax = plt.axes(projection='3d')
        ##ax.plot3D(PO[0,:], PO[1,:], PO[2,:], 'ro')

        #plt.plot(PO[0,:], PO[1,:], 'ro')
        #plt.axis([-1.5, 0.1, -1.5, 1.5])
        #plt.grid(b=True, which='both', color='b', linestyle='-')
        #plt.minorticks_on()
        #plt.grid(b=True, which='minor', color='#999999', linestyle='-', alpha=0.2)
        #plt.show()

        return PO

