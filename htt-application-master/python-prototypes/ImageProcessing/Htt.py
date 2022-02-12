# coding: utf-8

import imageio
import matplotlib.pyplot as plt
from scipy import ndimage, misc
import numpy as np
import glob
import math
from PIL import Image, ImageFilter
import random
import traceback, sys
import time
import os


# Optimization Engine
from OptimizationEngine import OptimizationEngine
from ScheimpflugModel import ScheimpflugModel

######################################################################################
# Settings for the Htt class.
######################################################################################
class Settings:
    def __init__(self, load_filename=None):
        self.channel = 1        # R=0, G=1, B=2
        self.threshold = 0.15   # Use a value between 0 and 1
        self.search_width = 50  # Search with in the threshold function

        if load_filename != None:
            self.load(load_filename)

    ##########################################################################
    def load(self, filename):
        config = configparser.ConfigParser()
        config._interpolation = configparser.ExtendedInterpolation()
        config.read(filename)

        # Parameters
        self.channel = int(config.get('Parameters', 'channel'))
        self.threshold = float(config.get('Parameters', 'threshold'))
        self.search_width = int(config.get('Parameters', 'search_width'))

    ##########################################################################
    def save(self, filename):
        config = configparser.ConfigParser()
        config._interpolation = configparser.ExtendedInterpolation()

        config['Parameters'] = {'threshold': str(self.threshold),
                                'channel': str(self.channel),
                                'search_width': str(self.search_width)}


        with open(filename, 'w') as configfile:
            config.write(configfile)
            configfile.close()

    ##########################################################################
    def print(self):
        print('Parameters ------------------------')
        print('\tChannel:             ' + str(self.channel))
        print('\tThreshold:           ' + str(self.threshold))
        print('\tSearch width:        ' + str(self.threshold) + ' pixels')

class Htt:
    def __init__(self):
        self.version = '0.1.0'
        self.in_path = ''
        self.out_path = ''

    ##############################################################################
    # Get the selected channel from an RGB image, converted to float32 and normalized
    #
    # Inputs:
    #   img     Input RGB image
    #   sel_ch  Selected channel
    #
    # Outputs:
    #   ch      Image (Grayscale) with the selected channel.
    ##############################################################################
    def getChannelAsF32(self, img, sel_ch):
        ch = img[:,:,sel_ch:sel_ch+1].reshape(img.shape[0], img.shape[1]).astype(np.float32)
        return ch

    ##############################################################################
    # Apply the sobel filter
    #
    # Inputs:
    #   img     Input image (grayscale)
    #
    # Outputs:
    #   sobel   Sobelfiltered image
    #   sobel_h Horizontal sobel-filtered image
    #   sobel_v Vertical sobel-filtered image
    ##############################################################################
    def sobel(self, img):
        sobel_v = ndimage.sobel(img, axis=0)
        sobel_h = ndimage.sobel(img, axis=1)
        sobel = np.sqrt(sobel_h * sobel_h +  sobel_v * sobel_v)
        return sobel, sobel_h, sobel_v

    ##############################################################################
    # Convert a float32 image to uint8
    #
    # Inputs:
    #   img     Input float32 image
    #
    # Outputs:
    #   img     Output uint8 image
    ##############################################################################
    def toUint8(self, img):
        img = (img - np.min(img)) * 255.0 / (np.max(img) - np.min(img))
        return img.astype(np.uint8)

    ##############################################################################
    # Normalize the image.
    #
    # Inputs:
    #   img      Input image in float32
    #   max_val  Maximum value (default = 1.0)
    #
    # Outputs:
    #   img     The normalized image (values between 0 and max_val)
    ##############################################################################
    def normalize(self, img, max_val=1.0):
        img = img = (img - np.min(img)) * max_val / (np.max(img) - np.min(img))
        return img

    ##############################################################################
    # Remove isolated points (USED)
    ##############################################################################
    def removeIsolatedPoints(self, x, y, distance_threshold=5.0, window_size=3, counter_threshold=3):
        L = len(x)
        xf = []
        yf = []

        distance_threshold_squared = distance_threshold * distance_threshold

        for l in range(L):
            current_x = x[l]
            current_y = y[l]

            counter = 0
            # The current value of x in l is compared with values
            # in an interval [-window_size, window_size]. We count the
            # number of elements that are within distance_threshold.
            for w in range(-window_size, window_size + 1, 1):
                t = l + w
                if t >= 0 and t < L:
                    #distance = abs(float(x[t] - current_x))

                    dx = float(x[t] - current_x)
                    dy = float(y[t] - current_y)
                    distance = dx * dx + dy * dy

                    if distance < distance_threshold_squared:
                        counter += 1

            if counter >= counter_threshold:
                xf.append(x[l])
                yf.append(y[l])

        xf = np.array(xf)
        yf = np.array(yf)
        return xf, yf

    ##############################################################################
    # Smooth curves (USED)
    ##############################################################################
    def smoothCurves__(self, x, y, label, window_size=3):
        L = len(x)

        xf = []
        yf = []

        # Get the maximum value of label
        maxLabel = np.max(label)

        for l in range(0, maxLabel + 1, 1):
            xl = x[label == l]
            yl = y[label == l]

            T = len(xl)

            for t in range(T):
                lo = t - window_size
                if lo < 0:
                    lo = 0
                hi = t + window_size + 1
                if hi > T:
                    hi = T

                mean_x = np.mean(xl[lo:hi])
                mean_y = np.mean(yl[lo:hi])

                xf.append(round(mean_x))
                yf.append(round(mean_y))

        xf = np.array(xf)
        yf = np.array(yf)

        return xf, yf

    ##############################################################################
    # Thresholding of the edge image with search range
    #
    # Inputs:
    #   edge_image        Edge image (normalized between 0 and 1)
    #   channel           Selected channel on the input image
    #   threshold         Threshold for detecting the profile
    #   search_width      Search width for determining the raw profile
    # Outputs:
    #   raw_profile_max   The detected raw_profile on the maximum of the channel image
    #   raw_profile_edge  The detected raw profile on the edge image
    ##############################################################################
    def threshold(self, edge_image, channel, threshold, search_width, intv_adjustment = 0):
        ###########################################################
        # Scan the image from right to left till the first boundary
        # is found
        ###########################################################

        height, width = edge_image.shape

        # Create an output image with the same dimensions of img
        x = []
        y = []
        y_th = []

        # Use only the right half image.
        #left_boundary = int(width / 2)
        left_boundary = int(width / 4)


        # For each row, we start from the right side of the image and
        # go backward. When the intensity is higher than the chosen
        # threshold, the right boundary rw is set.
        # The maximum value on the search interval [rw-search_width, rw]
        # is then used for determining the position of the maximum, along
        # that row. This is considered as a raw point of the profile on that
        # particular raw. Observe that if we set the search_width to zero,
        # then then we fall back onto the previous version of the thresholding
        # algorithm.
        for h in range(height):
            rw = -1
            for w in range(width - 1, left_boundary - 1, -1):
                # Find the beginning of the profile.
                if rw == -1 and edge_image[h, w] > threshold:
                    rw = w
                    break

            # Find the maximum within the given search range L
            if rw >= 0:

                # Adjust the interval
                rw += intv_adjustment
                if rw >= width:
                    rw = width - 1

                lw = rw - search_width
                if lw <= left_boundary:
                    lw = left_boundary

                max_w = np.argmax(channel[h, lw:rw+1]) + lw

                x.append(h)     # Integer type
                y.append(max_w) # Integer type.
                y_th.append(rw) # Integer type.


        # Transform x and y into numpy arrays
        x = np.array(x)
        y = np.array(y)
        y_th = np.array(y_th)
        raw_profile_max = (x, y)
        raw_profile_edge = (x, y_th)

        return raw_profile_max, raw_profile_edge

    ##############################################################################
    # Draw the profile on the passed image
    #
    # Inputs:
    #   image       Input image on which the trace is written
    #   profile     Profile
    #
    # Output:
    #   image       The input image with the profile marked on it.
    ##############################################################################
    def traceProfile(self, image, profile):
        height, width = image.shape
        x, y = profile

        L = len(x)
        for l in range(L):
            h = int(round(x[l]))
            w = int(round(y[l]))
            if h >= 0 and h < height and w >= 0 and w < width:
                image[h, w] = 1.0
        return image

    ##############################################################################
    # Merge two images into a single representation. This is used for superimposing
    # the input image with the computed profile.
    #
    # Input:
    #   input_img       Input RGB image
    #   profile_img     Profile image (Grayscale)
    #   alpha           Mixing factor, default = 0.35 (0.0 -> outputs img,
    #                                                  1.0 -> outputs profile_img
    # Output:
    #   merged_img      Merged image
    ##############################################################################
    def merge(self, input_img, profile_img, alpha=0.35):
        merged_img = np.zeros_like(input_img)

        height = input_img.shape[0]
        width = input_img.shape[1]
        channels = input_img.shape[2]

        tmp_img = alpha * profile_img.reshape(height, width, 1)
        merged_img[:,:,0:1] = (1.0 - alpha) * input_img[:,:,0:1] + tmp_img
        merged_img[:,:,1:2] = (1.0 - alpha) * input_img[:,:,1:2] + tmp_img
        merged_img[:,:,2:3] = (1.0 - alpha) * input_img[:,:,2:3] + tmp_img

        return merged_img

    def merge2(self, input_img, profile_img, profile_img_red, alpha=0.35):
        merged_img = np.zeros_like(input_img)

        height = input_img.shape[0]
        width = input_img.shape[1]
        channels = input_img.shape[2]

        tmp_img = alpha * profile_img.reshape(height, width, 1)
        tmp_img2 = alpha * profile_img_red.reshape(height, width, 1)
        merged_img[:, :, 0:1] = (1.0 - alpha) * input_img[:, :, 0:1] + (tmp_img + tmp_img2)
        merged_img[:, :, 1:2] = (1.0 - alpha) * input_img[:, :, 1:2] + tmp_img
        merged_img[:, :, 2:3] = (1.0 - alpha) * input_img[:, :, 2:3] + tmp_img

        return merged_img

    ##############################################################################
    # Low-pass filtering of an RGB image
    ##############################################################################
    def lowPassFilter_3(self, input_image, radius=1):
        im = Image.fromarray(input_image, mode='RGB')
        input_image = im.filter(ImageFilter.BoxBlur(radius))
        input_image = np.array(input_image.getdata()).reshape(input_image.size[0], input_image.size[1], 3)
        return input_image

    ##############################################################################
    # Low-pass filtering of a single channel
    ##############################################################################
    def lowPassFilter_1(self, edge_image, radius=3):
        edge_image = self.normalize(edge_image, 255.0)
        edge_image = edge_image.astype(np.uint8)

        im = Image.fromarray(edge_image, mode='L')
        #im = im.convert()
        #print(im.size)
        edge_image = im.filter(ImageFilter.BoxBlur(radius))
        edge_image = np.array(edge_image.getdata()).reshape(edge_image.size[0], edge_image.size[1]).astype(np.float)
        edge_image = self.normalize(edge_image)

        return edge_image

    ##############################################################################
    # Process a single image.
    #
    # Input:
    #   input_image      Input image (RGB)
    #   channel          Channel to process (0 = R, 1 = G, 2 = B), default = 1
    #   threshold        Threshold for raw profile detection
    #   search_width     Width of the search window
    #   debug            Flag used for debugging
    #
    # Output:
    #   merged_image     Image with processed profile
    #   merged_raw_image Image with raw profile
    ##############################################################################
    def process(self, input_image, params):
        # Get parameters
        debug = params['debug']
        channel = params['pre_channel']     # Channel to process (R = 0, G = 1, B = 2)
        threshold = params['pre_threshold']
        search_width = params['pre_search_width']
        opt_get_timings = params['opt_get_timings']

        ################################################################
        # Low pass filter the input image.
        ################################################################
        input_image = self.lowPassFilter_3(input_image, 3)

        if False:
            imageio.imwrite('input_image.png', input_image)

        ################################################################
        # Get the selected channel, convert it to float32 and normalize.
        ################################################################
        channel = self.getChannelAsF32(input_image, channel)
        channel = self.normalize(channel)

        # Extract also the blue channel (experimental)
        #blue_channel = self.getChannelAsF32(input_image, 2)
        #blue_channel = self.normalize(blue_channel)

        #############################################################################################################
        # Apply the Sobel filter to the selected channel and normalize.
        #############################################################################################################
        edge_image, _ , _ = self.sobel(channel)
        edge_image = self.normalize(edge_image)

        if False:
            imageio.imwrite('edge_image.png', edge_image)

        #############################################################################################################
        # Get the rightmost profiles by thresholding the edge image. Create also an image with the profile drawn on
        # black background
        # Note: Perform the thresholding operation. Two profile are returned, the one extracted from the selected
        # channel on the actual edge (raw_profile_edge), and the other on the local maximum value.
        #############################################################################################################

        # Thresholding
        raw_profile_max, raw_profile_edge = self.threshold(edge_image, channel, threshold, search_width)

        # Postprocess the profile
        y_tmp, x_tmp = raw_profile_max

        # Remove isolated points (applied twice for more filtering)
        x_tmp, y_tmp = self.removeIsolatedPoints(x_tmp, y_tmp, distance_threshold=5.0, window_size=3, counter_threshold=5)
        x_tmp, y_tmp = self.removeIsolatedPoints(x_tmp, y_tmp, distance_threshold=5.0, window_size=3, counter_threshold=5)
        raw_profile_max = (y_tmp, x_tmp)

        #############################################################################################################
        # Optimization
        #############################################################################################################
        start_time = time.time()
        opt_out = OptimizationEngine.optimizeModel(raw_profile_max, params)
        end_time = time.time()
        print("Optimization Time: %.5f" % (end_time - start_time))

        # Do not compute output images
        if opt_get_timings:
            return opt_out


        # Get output variables
        loss_field = opt_out['loss_field']
        opt_bevel = opt_out['bevel']

        if loss_field is not None:
            loss_field = self.normalize(loss_field)

        #############################################################################################################
        # Trace the profile and bevel on a black images.
        raw_profile_img = self.traceProfile(np.zeros([400, 400]), raw_profile_max)
        opt_bevel_img = self.traceProfile(np.zeros([400, 400]), opt_bevel)

        ######################################################
        # Merge input RGB image with the computed profile image and bevel image
        ######################################################
        normalized_img_f32 = input_image.astype(np.float) / 255.0
        merged_interp_image = self.merge2(normalized_img_f32, opt_bevel_img, raw_profile_img)
        merged_interp_image = self.toUint8(merged_interp_image)

        # Append the merged image
        opt_out['merged_interp_image'] = merged_interp_image

        # This is not used any more.
        #merged_raw_image = self.merge(normalized_img_f32, raw_profile_img)
        #merged_raw_image = self.toUint8(merged_raw_image)

        # Convert the loss field to RGB
        if params['opt_visualize_loss_field'] and  loss_field is not None:
            loss_field_rgb = np.zeros([400,400, 3])
            loss_field_rgb[:,:,0] = loss_field
            loss_field_rgb[:,:,1] = loss_field
            loss_field_rgb[:,:,2] = loss_field

            # Merge the raw profile and bevel on the loss field.
            loss_field = self.merge2(loss_field_rgb, opt_bevel_img, raw_profile_img)
            loss_field = self.toUint8(loss_field)

            # Append some results to the output dictionary
            opt_out['loss_field'] = loss_field  # The new loss field if it was computed

        return opt_out


    def testScheimpflug(self):


        sm = ScheimpflugModel()

        start_time = time.time()

        R = 0.0
        H = 0.0

        #################################################
        # Convert from object to image
        #################################################


        #M0 = np.array([[0, 0, 0],
        #               [0, 0, 0],
        #               [0, 0, 0]], dtype=np.float)


        # Z sempre 0
        M0 = np.array([[0, 1.0],
                       [0, 1.0],
                       [0, 0]], dtype=np.float)



        MSp = sm.object_to_image(R, H, M0, R_min=-20.0, R_max=20.0, H_min=0.0, H_max=30.0)

        print(MSp)
        a = input()

        MSp = np.array([[200, 300, 100],
                       [200, 200, 200],
                       [0, 0, 0]], dtype=np.float)


        # Convert back from image to point
        R0 = sm.image_to_object(R, H, MSp, R_min=-20.0, R_max=20.0, H_min=0.0, H_max=30.0)


        print("M0", M0)


        print(M0[0,1], M0[1,0])

        print("R0", R0)



        #########################################
        avg_time = (time.time() - start_time)
        print("Timing: %.5f" % (avg_time))


    ##############################################################################
    # Process the images contained in the input folder. Processing results are
    # written to the output folder.
    #
    # Input:
    #   in_path     Input folder
    #   out_path    Output folder
    ##############################################################################
    def processFolder(self, in_path, out_path, hyperopt_set=None):
        # Get images in the input folder.
        files = glob.glob(in_path + '/*.png')
        files.sort()

        R = 1 # Repetitions
        N = len(files)

        # Define the processing parameters
        params = dict()
        params['debug'] = False
        params['pre_channel'] = 1           # Channel to process (R = 0, G = 1, B = 2)
        params['pre_threshold'] = 0.07      # Threshold value for profile detection
        params['pre_search_width'] = 50     # Search width for profile detection

        params['opt_max_retries'] = 30      # Maximum number of retries to find a valid solution
        params['opt_swarm_size'] = 100
        params['opt_iterations'] = 100 #175 #200
        params['opt_win_size_x'] = 24 #10#24
        params['opt_win_size_y'] = 55 #34#55
        params['opt_visualize_loss_field'] = False
        params['opt_get_timings'] = False

        # T Model optimization search space
        params['opt_tmodel_min_S1'] = 30
        params['opt_tmodel_max_S1'] = 100
        params['opt_tmodel_min_E'] = 20
        params['opt_tmodel_max_E'] = 100

        # Define the Bevel model to use:
        # MiniBevelModel_1: FIXME removed initial mini bevel model with fixed angle
        # MiniBevelModel_2: Mini bevel model with independent bevel angles
        # MiniBevelModel_3: Mini bevel model with Scheimpflug deformation.
        # CustomBevelModel_1: Custom_bevel model with Scheimpflug deformation
        # TBevelModel_1: TBevelModel with Scheimpflug deformation
        #params['opt_model_type'] = 'MiniBevelModel_3'

        # New available models are: MiniBevelModel, CustomBevelModel, TModel

        params['opt_model_type'] = 'MiniBevelModel'


        # T Model optimization search space
        params['opt_tmodel_min_S1'] = 30
        params['opt_tmodel_max_S1'] = 100
        params['opt_tmodel_min_E'] = 20
        params['opt_tmodel_max_E'] = 100

        ###############################################################################
        # Hyperopt overrides

        # Substitutes the key in params with those in hyperopt_set
        store_images = True
        if hyperopt_set is not None:
            for key in hyperopt_set.keys():
                params[key] = hyperopt_set[key]
                #print("HyperOpt key substitution: " + key + " with new value" + str(hyperopt_set[key]))
            store_images = False


        if params['opt_get_timings'] == True:
            store_images = False

        ###############################################################################
        # Create the output file
        fout = open(out_path + '/results.csv','w')
        fout.write("Filename;Time;Valid;Retries;Model Type;Model Params\n");

        global_avg_loss = 0.0
        global_avg_time = 0.0

        image_counter = 1
        for fn in files:
            print("Processing image " + str(image_counter) + ' out of ' + str(N), fn)

            if os.name == 'nt': # Windows
                file_name = fn.split('\\')[-1].strip()
            else: # other (unix)
                file_name = fn.split('/')[-1].strip()

            base_name = file_name.split('.png')[0]

            # Read input image
            input_image = imageio.imread(fn)

            ############################################
            # Statistics variables
            start_time = time.time()
            valid_solution_counter = 0.0
            retries_counter = 0
            loss = 0.0

            for l in range(R):

                # Processing
                opt_out = self.process(input_image, params)

                # Counts the number of valid solutions found so far
                if opt_out['valid_solution'] == True:
                    valid_solution_counter += 1.0
                    loss += opt_out['best_loss']

                # Updates total tries
                retries_counter += opt_out['retries']

            # Compute averages
            avg_time = (time.time() - start_time) / R
            avg_valid = valid_solution_counter / R
            avg_retries = retries_counter / R
            if valid_solution_counter > 0 :
                avg_loss = loss / valid_solution_counter
            else:
                avg_loss = 1e200


            ### Global statistics
            global_avg_loss += avg_loss
            global_avg_time += avg_time

            print("Timing: %.5f - Valid: %.2f - Retries = %.2f" % (avg_time, avg_valid, avg_retries))

            # File output
            fout.write(file_name + ";" + str(avg_time) + ";" + str(avg_valid) + ";" + str(avg_retries) + ";" +
                       str(opt_out["model_type"]) + "\n") #";" + str(opt_out["best_solution"]) + '\n')
            fout.flush()

            # Write results
            if store_images:
                imageio.imwrite(out_path + '/' + base_name + '_interp.png', opt_out['merged_interp_image'])
            image_counter += 1
        fout.close()

        if N > 0:
            global_avg_loss = global_avg_loss / N
            global_avg_time = global_avg_time / N

        return global_avg_loss, global_avg_time

    ##############################################################################
    # Process the images contained in the input folder. Processing results are
    # written to the output folder.
    #
    # Input:
    #   in_path     Input folder
    #   out_path    Output folder
    ##############################################################################
    def processFolderForStatistics(self, in_path, out_path, hyperopt_set=None):
        # Get images in the input folder.
        files = glob.glob(in_path + '/*.png')
        files.sort()

        R = 30 # Repetitions
        N = len(files)

        # Define the processing parameters
        params = dict()
        params['debug'] = False
        params['pre_channel'] = 1           # Channel to process (R = 0, G = 1, B = 2)
        params['pre_threshold'] = 0.07      # Threshold value for profile detection
        params['pre_search_width'] = 50     # Search width for profile detection

        params['opt_max_retries'] = 30      # Maximum number of retries to find a valid solution
        params['opt_swarm_size'] = 100
        params['opt_iterations'] = 200      #175 #200
        params['opt_win_size_x'] = 24       #10 #24
        params['opt_win_size_y'] = 55       #34 #55
        params['opt_visualize_loss_field'] = False
        params['opt_get_timings'] = False

        # T Model optimization search space
        params['opt_tmodel_min_S1'] = 30
        params['opt_tmodel_max_S1'] = 100
        params['opt_tmodel_min_E'] = 20
        params['opt_tmodel_max_E'] = 100

        # Define the Bevel model to use:
        # MiniBevelModel_1: FIXME removed initial mini bevel model with fixed angle
        # MiniBevelModel_2: Mini bevel model with independent bevel angles
        # MiniBevelModel_3: Mini bevel model with Scheimpflug deformation.
        # CustomBevelModel_1: Custom_bevel model with Scheimpflug deformation
        # TBevelModel_1: TBevelModel with Scheimpflug deformation
        #params['opt_model_type'] = 'MiniBevelModel_3'

        # New available models are: MiniBevelModel, CustomBevelModel, TModel
        params['opt_model_type'] = 'MiniBevelModel'


        # Container for collecting the statistics
        stats = dict()


        # T Model optimization search space
        params['opt_tmodel_min_S1'] = 30
        params['opt_tmodel_max_S1'] = 100
        params['opt_tmodel_min_E'] = 20
        params['opt_tmodel_max_E'] = 100

        ###############################################################################
        # Hyperopt overrides

        # Substitutes the key in params with those in hyperopt_set
        store_images = True
        if hyperopt_set is not None:
            for key in hyperopt_set.keys():
                params[key] = hyperopt_set[key]
            store_images = False


        if params['opt_get_timings'] == True:
            store_images = False

        ###############################################################################
        # Create the output file
        fout = open(out_path + '/results_stats.csv','w')
        fout.write("Filename;Time;Valid;Retries;Model Type;avg.B[mm];std.B[mm];min.B[mm];max.B[mm];avg.M[mm];std.M[mm];min.M[mm];max.M[mm]\n");

        global_avg_loss = 0.0
        global_avg_time = 0.0

        image_counter = 1
        for fn in files:
            print("Processing image " + str(image_counter) + ' out of ' + str(N), fn)

            if os.name == 'nt': # Windows
                file_name = fn.split('\\')[-1].strip()
            else: # other (unix)
                file_name = fn.split('/')[-1].strip()


            base_name = file_name.split('.png')[0]

            # Read input image
            input_image = imageio.imread(fn)

            ############################################
            # Statistics variables
            start_time = time.time()
            valid_solution_counter = 0.0
            retries_counter = 0
            loss = 0.0


            # These are the parameters measured on the MiniBevel
            measured_A = []
            measured_B = []
            measured_C = []
            measured_M = []

            for l in range(R):

                # Processing
                opt_out = self.process(input_image, params)

                # Counts the number of valid solutions found so far
                if opt_out['valid_solution'] == True:
                    valid_solution_counter += 1.0
                    loss += opt_out['best_loss']

                    # update measurements only if the solution is valid
                    (A, B, C, M) = opt_out['model'].getMeasures()
                    measured_A.append(A)
                    measured_B.append(B)
                    measured_C.append(C)
                    measured_M.append(M)


                # Updates total tries
                retries_counter += opt_out['retries']

            # Store the statistics of each run
            stats['model_type'] = params['opt_model_type']

            measured_A = np.array(measured_A)
            measured_B = np.array(measured_B)
            measured_C = np.array(measured_C)
            measured_M = np.array(measured_M)

            stats['mean_A'] = np.mean(measured_A)
            stats['std_A'] = np.std(measured_A)
            stats['min_A'] = np.min(measured_A)
            stats['max_A'] = np.max(measured_A)

            stats['mean_B'] = np.mean(measured_B)
            stats['std_B'] = np.std(measured_B)
            stats['min_B'] = np.min(measured_B)
            stats['max_B'] = np.max(measured_B)

            stats['mean_C'] = np.mean(measured_C)
            stats['std_C'] = np.std(measured_C)
            stats['min_C'] = np.min(measured_C)
            stats['max_C'] = np.max(measured_C)

            stats['mean_M'] = np.mean(measured_M)
            stats['std_M'] = np.std(measured_M)
            stats['min_M'] = np.min(measured_M)
            stats['max_M'] = np.max(measured_M)


            # Compute averages
            avg_time = (time.time() - start_time) / R
            avg_valid = valid_solution_counter / R
            avg_retries = retries_counter / R
            if valid_solution_counter > 0 :
                avg_loss = loss / valid_solution_counter
            else:
                avg_loss = 1e200


            ### Global statistics
            global_avg_loss += avg_loss
            global_avg_time += avg_time

            print("Timing: %.5f - Valid: %.2f - Retries = %.2f" % (avg_time, avg_valid, avg_retries))

            # File output
            fout.write(file_name + ";" + str(avg_time) + ";" + str(avg_valid) + ";" + str(avg_retries) + ";" +
                       str(opt_out["model_type"]) + ";" +
                       str(stats['mean_B']) + ";" + str(stats['std_B']) + ";" + str(stats['min_B']) + ";" + str(stats['max_B']) + ";" +
                       str(stats['mean_M']) + ";" + str(stats['std_M']) + ";" + str(stats['min_M']) + ";" + str(stats['max_M']) +
                        "\n")
            fout.flush()

            # Write results
            if store_images:
                imageio.imwrite(out_path + '/' + base_name + '_interp.png', opt_out['merged_interp_image'])
            image_counter += 1
        fout.close()

        if N > 0:
            global_avg_loss = global_avg_loss / N
            global_avg_time = global_avg_time / N

        return global_avg_loss, global_avg_time



    # Threaded version of process folder
    def processFolder_threaded(self, in_path, out_path, hyperopt_set=None, n_threads = 1):
        import threading

        # Get images in the input folder.
        files = glob.glob(in_path + '/*.png')
        files.sort()

        #R = 1  # Repetitions
        #N = len(files)

        ###############################################################################
        # Hyperopt overrides

        # Substitutes the key in params with those in hyperopt_set
        store_images = True
        if hyperopt_set is not None:
            for key in hyperopt_set.keys():
                params[key] = hyperopt_set[key]
                # print("HyperOpt key substitution: " + key + " with new value" + str(hyperopt_set[key]))
            store_images = False

        ###############################################################################
        # Create the output file
        fout = open(out_path + '/results.csv', 'w')
        fout.write("Filename;Time;Valid;Retries;Model Type;Model Params\n");

        global_avg_loss = 0.0
        global_avg_time = 0.0

        # Thread mod:
        chunks = [files[i:i + n_threads] for i in range(0, len(files), n_threads)]

        T = dict()

        thread_counter = 0
        for c in chunks:
            x = threading.Thread(target=self.thread_worker, args=(c, out_path))
            x.start()

            T[str(thread_counter)] = x
            thread_counter += 1

        return 0.0, 0.0

    ################################################################################
    def thread_worker(self, files, out_path):
        print("Threaded started")

        store_images = True
        R = 1
        N = len(files)

        params = dict()
        params['debug'] = False
        params['pre_channel'] = 1  # Channel to process (R = 0, G = 1, B = 2)
        params['pre_threshold'] = 0.07  # Threshold value for profile detection
        params['pre_search_width'] = 50  # Search width for profile detection

        params['opt_max_retries'] = 30  # Maximum number of retries to find a valid solution
        params['opt_swarm_size'] = 100
        params['opt_iterations'] = 100  # 200
        params['opt_win_size_x'] = 24  # 24
        params['opt_win_size_y'] = 55  # 55
        params['opt_visualize_loss_field'] = False
        params['opt_get_timings'] = False

        # Define the Bevel model to use:
        # MiniBevelModel_1: initial mini bevel model with fixed angle
        # MiniBevelModel_2: Mini bevel model with independent bevel angles
        # MiniBevelModel_3: Mini bevel model with Scheimpflug deformation.
        # CustomBevelModel_1: Custom bevel model with Scheimpflug deformation
        # TBevelModel_1: T bevel model with Scheimpflug deformation
        params['opt_model_type'] = 'MiniBevelModel_4'

        # T Model optimization search space
        params['opt_tmodel_min_S1'] = 30
        params['opt_tmodel_max_S1'] = 100
        params['opt_tmodel_min_E'] = 20
        params['opt_tmodel_max_E'] = 100

        enable_multi_bevel_test = False


        if params['opt_get_timings'] == True:
            store_images = False


        image_counter = 1
        for fn in files:
            print("Processing image " + str(image_counter) + ' out of ' + str(N), fn)

            if os.name == 'nt': # Windows
                file_name = fn.split('\\')[-1].strip()
            else: # other (unix)
                file_name = fn.split('/')[-1].strip()

            base_name = file_name.split('.png')[0]

            # Read input image
            input_image = imageio.imread(fn)

            ############################################
            # Statistics variables
            start_time = time.time()
            valid_solution_counter = 0.0
            retries_counter = 0
            loss = 0.0

            for l in range(R):

                # Processing
                if enable_multi_bevel_test == False:
                    opt_out = self.process(input_image, params) # Original

                else:
                    # Test solves the computation with two models and select the best
                    params['opt_model_type'] = 'MiniBevelModel_4'
                    opt_out_mini = self.process(input_image, params)

                    params['opt_model_type'] = 'CustomBevelModel_1'
                    opt_out_custom = self.process(input_image, params)

                    params['opt_model_type'] = 'TBevelModel_1'
                    opt_out_T = self.process(input_image, params)

                    if opt_out_mini['best_loss'] < opt_out_custom['best_loss']:
                        opt_out = opt_out_mini
                    else:
                        opt_out = opt_out_custom

                    if opt_out['best_loss'] > opt_out_T['best_loss']:
                        opt_out = opt_out_T

                # Counts the number of valid solutions found so far
                if opt_out['valid_solution'] == True:
                    valid_solution_counter += 1.0
                    loss += opt_out['best_loss']

                # Updates total tries
                retries_counter += opt_out['retries']

            # Compute averages
            avg_time = (time.time() - start_time) / R
            avg_valid = valid_solution_counter / R
            avg_retries = retries_counter / R
            if valid_solution_counter > 0:
                avg_loss = loss / valid_solution_counter
            else:
                avg_loss = 1e200

            ### Global statistics
            # global_avg_loss += avg_loss
            # global_avg_time += avg_time

            # print("Timing: %.5f - Valid: %.2f - Retries = %.2f" % (avg_time, avg_valid, avg_retries))

            # File output
            # fout.write(file_name + ";" + str(avg_time) + ";" + str(avg_valid) + ";" + str(avg_retries) + ";" +
            #           str(opt_out["model_type"]) + ";" + str(opt_out["best_solution"]) + '\n')
            # fout.flush()

            # Write results
            if store_images:
                imageio.imwrite(out_path + '/' + base_name + '_interp.png', opt_out['merged_interp_image'])
            image_counter += 1
        # fout.close()

        # if N > 0:
        #    global_avg_loss = global_avg_loss / N
        #    global_avg_time = global_avg_time / N

        # return global_avg_loss, global_avg_time

