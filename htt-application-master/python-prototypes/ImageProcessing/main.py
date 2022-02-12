#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, traceback
from Htt import Htt

########################################################################
# This is the test function that is called by the test launcher
#
# Inputs:
#   config_filename     The file name with the test configuration
#
# Return a string with the model name or the error code. This string
# will be appended to the test launcher report
########################################################################
def testFunction(config_filename):
    try:

        in_path = './sample_data'
        out_path = './results'

        htt = Htt()

        ################################################
        # Tests with Scheimpflug deformation
        #htt.testScheimpflug()

        ################################################
        # Process a folder single threaded
        #htt.processFolderForStatistics(in_path, out_path)
        htt.processFolder(in_path, out_path)

        ################################################
        # Process a folder multi-threaded
        #htt.processFolder_threaded(in_path, out_path, n_threads =1)

        ################################################
        # Optimize the loss function using hyperopt
        #htt.hyperOptLoss(in_path, out_path)

        ################################################
        # Optimize the computation time using hyperopt
        # (Note: experimental and does not work well)
        #htt.hyperOptTime(in_path, out_path)


    except Exception as e:
        print(traceback.format_exc())
        return str(e)


########################################################################
# Main function.
########################################################################
if __name__ == "__main__":

  testFunction(' ')

