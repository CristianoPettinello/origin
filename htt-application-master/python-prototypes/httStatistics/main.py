#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, traceback
import matplotlib.pyplot as plt
import csv
import numpy as np



def ScheimpflugStatistics(csvFileName):

    # id;loss;tt;theta;alpha;mag;delta;beta;cx0;cy0

    identifier = []
    loss = []
    tt = []
    theta = []
    alpha = []
    mag = []
    delta = []
    beta = []
    cx0 = []
    cy0 = []
    
    # Read the table
    with open(csvFileName, newline='') as csvfile:
        reader = csv.reader(csvfile, delimiter=';', quotechar='|')
        next(reader)
        for row in reader:
            # print(row)
            identifier.append(int(row[0]))
            loss.append(float(row[1]))
            tt.append(float(row[2]))
            theta.append(float(row[3]))
            alpha.append(float(row[4]))
            mag.append(float(row[5]))
            delta.append(float(row[6]))
            beta.append(float(row[7]))
            cx0.append(float(row[8]))
            cy0.append(float(row[9]))

    # (cx0, cy0) vs loss
    minIndex = np.argmin(loss)
    minLoss = loss[minIndex]
    minCx0 = cx0[minIndex]
    minCy0 = cy0[minIndex]

    print("Min loss @ " + str(minCx0) + " " + str(minCy0))     

    fig = plt.figure()
    ax = plt.axes(projection='3d')
    ax.scatter(cx0, cy0, loss, 'gray')
    ax.scatter(minCx0, minCy0, minLoss, 'red')
    ax.set_xlabel('CX0')
    ax.set_ylabel('CY0')
    ax.set_zlabel('Loss')

    filename = csvFileName[:-4] + '_center.png'
    plt.savefig(filename, dpi=300)

    plt.show()

    # 
    #ax = plt.axes()
    fig = plt.figure()
    ax1 = plt.subplot(231)
    ax1.scatter(tt, loss)
    ax1.set_xlabel('tt')
    ax1.set_ylabel('loss')

    ax2 = plt.subplot(232)
    ax2.scatter(theta, loss)
    ax2.set_xlabel('theta')
    ax2.set_ylabel('loss')

    ax2 = plt.subplot(233)
    ax2.scatter(alpha, loss)
    ax2.set_xlabel('alpha')
    ax2.set_ylabel('loss')

    ax2 = plt.subplot(234)
    ax2.scatter(mag, loss)
    ax2.set_xlabel('mag')
    ax2.set_ylabel('loss')

    ax2 = plt.subplot(235)
    ax2.scatter(delta, loss)
    ax2.set_xlabel('delta')
    ax2.set_ylabel('loss')

    ax2 = plt.subplot(236)
    ax2.scatter(beta, loss)
    ax2.set_xlabel('beta')
    ax2.set_ylabel('loss')
    filename = csvFileName[:-4] + '_stats.png'
    plt.savefig(filename, dpi=300)

    plt.show()


########################################################################
# Main function.
########################################################################
if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Usage: python main.py <csv file>")
    else:
        ScheimpflugStatistics(sys.argv[1])#'calibrationGridTest_23-10-2020_173847.csv')

