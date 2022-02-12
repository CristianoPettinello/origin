#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys, traceback
import imageio
import pyHtt as Htt
import numpy as np

def testFunction():
    try:
        # Read input image
        file = "../../../tests/ImageProcessing.Tester/sample_data/test.png"
        input_image = imageio.imread(file)
        # print(type(input_image))
        print(input_image.shape)

        ret = Htt.process(input_image)
        ret = ret * 255
        ret = ret.astype(np.uint8)
        print(ret.shape)

        output = "../../../tests/ImageProcessing.Tester/sample_data/output.png"
        imageio.imwrite(output, ret)

        folder="../../../tests/ImageProcessing.Tester/sample_data/"
        Htt.processFolder(folder)

    except Exception as e:
        print(traceback.format_exc())
        return str(e)        

if __name__ == "__main__":

  testFunction()