# -*- coding: utf-8 -*-

import traceback, sys, time
import numpy as np

from pso import pso2
from MiniBevelModel_S import MiniBevelModel_S
from CustomBevelModel_S import CustomBevelModel_S
from TBevelModel_S import TBevelModel_S
from ScheimpflugModel import ScheimpflugModel

def visualizeLossFunction(out, xcc, ycc, min_xcc, max_xcc, gamma, size=400):
    it = 1.0
    ##############################################
    # Get min and max x of profile under test
    # if self.x2 < min_xcc or self.x2 >= max_xcc:
    #    return None
    # if self.x3 < min_xcc or self.x3 >= max_xcc:
    #    return None

    ##############################################
    # Get the max Y and frm CCs

    # TEST:....NEW set of tests instead of + 7 let give some more.
    # Previous
    # max_y_MAX = size - 1
    # max_y_MIN = np.max(ycc) +30#7

    max_y_MIN = np.max(ycc) + 40  # 7
    max_y_MAX = np.max(ycc) + 7

    # Iterazione intera
    # max_y = (max_y_MIN * (99 - it) + max_y_MAX * it) / 99

    # iterazione frazionaria
    # max_y = (max_y_MIN * (1.0 - it) + max_y_MAX * it)
    max_y = (max_y_MAX * (1.0 - it) + max_y_MIN * it)

    min_y = np.min(ycc) - 30

    # instead
    # max_y = np.min([np.max(ycc) + 30, size - 1])
    # min_y = np.min(ycc) - 30
    ######### END TEST

    # ADG optimization - BEGIN
    # Original
    # lb = np.ones(size) * min_y
    # ub = np.ones(size) * max_y
    # bb = np.ones(size)

    # Optimized
    lb = np.ones(size) * min_y
    ub = np.ones(size) * max_y
    ib = np.ones(size) / (max_y - min_y)
    bb = np.ones(size)
    # ADG optimization - END

    j = 0
    for x in xcc:
        # ADG optimization - BEGIN
        # x_ref = int(round(x))
        x_ref = int(x)
        # ADG optimization - END

        # y_ref = int(round(ycc[j]))
        y_ref = ycc[j]
        j += 1
        # ADG optimization - BEGIN
        # Original
        # lb[x_ref] = y_ref + 1   # Position of CC
        # ub[x_ref] = max_y
        # bb[x_ref] = 0

        lb[x_ref] = y_ref + 3  # +1  # Position of CC
        # ub[x_ref] = max_y
        ib[x_ref] = 1.0 / (max_y - lb[x_ref])
        bb[x_ref] = 0

        # ADG optimization - BEGIN

    # This is the new specification of the loss function
    # Trace single line, We use x as the reference axis.
    # mse = 0.0
    LOW_LOSS = 300000
    HI_LOSS = 300000
    HI_LOSS2 = 100

    for y in range(size):
        for x in range(size):
            lb_y = lb[x]
            ub_y = ub[x]
            bb_y = bb[x]

            if bb_y == 0:
                if y <= lb_y:
                    val = LOW_LOSS
                elif y < ub_y:
                    LL = (y - lb_y) * ib[x]
                    val = (LL ** 2) * HI_LOSS
                else:
                    val = HI_LOSS
            else:
                if y <= lb_y:
                    val = LOW_LOSS
                elif y < ub_y:
                    LL = (y - lb_y) * ib[x]
                    val = (LL ** 2) * HI_LOSS2
                else:
                    val = HI_LOSS

            out[x, y] = val

    return out

# Optimizer constraint function
def constraints(x, *args):
    try:
        # Get other parameters passed to the functions
        _, _, _, _, _, _, model = args

        # Set the model
        model.setModel(x)

        # Check constraints
        res = model.checkConstraint_m()

    except:
        traceback.print_exc(file=sys.stdout)
        res = -1

    return [res]

# Optimizer loss function
def loss(x, it, *args):
    try:
        # Get parameters
        _, _, _, _, _, _, model = args

        # Set the model
        model.setModel(x)

        # Compute the loss function
        mse = model.mse_m(it)
    except:
        traceback.print_exc(file=sys.stdout)
        mse = 1e200

    return mse

class OptimizationEngine:
    def precomputeLossMap(xcc, ycc, min_xcc, max_xcc, min_ycc, max_ycc, size=400):
        # This is the same code as computed in mse
        max_y_MIN = max_ycc + 40  # 7
        max_y_MAX = max_ycc + 7

        # Fractionary iteration (NOTE: this is not used anymore)
        it = 1.0
        max_y = (max_y_MAX * (1.0 - it) + max_y_MIN * it)
        max_y = np.min([max_y, size - 1]) # FIX boundary

        # FIXME ADG_01 optimize here with precomputed value
        min_y = min_ycc - 30
        min_y = np.max([min_y, 0]) # FIX boundary

        # Optimized
        lb = np.ones(size) * min_y              # Lower bound
        ub = np.ones(size) * max_y              # Upper bound
        ib = np.ones(size) / (max_y - min_y)    # precomputed values.
        bb = np.ones(size)                      # array marking when there is the profile

        j = 0
        for x in xcc:
            x_ref = int(round(x))
            y_ref = int(round(ycc[j]))

            j += 1

            lb[x_ref] = y_ref + 3  # +1  # Position of CC
            ib[x_ref] = 1.0 / (max_y - lb[x_ref])
            bb[x_ref] = 0

        # This is the new specification of the loss function
        # Trace single line, We use x as the reference axis.
        LOW_LOSS = 300000
        HI_LOSS = 300000
        HI_LOSS2 = 100

        # Allocate the loss_map
        #self.loss_map = np.zeros([size, size])
        loss_map = np.ones([size, size]) * LOW_LOSS

        min_y = int(min_y)
        max_y = int(max_y)
        # Compute the loss function for each point in the loss_map
        for x in range(size):
            lb_y = lb[x] # Lower bound
            ub_y = ub[x] # Upper bound
            bb_y = bb[x] # Indicator function
            #ib_y = ib[x] * LOSS_VALUE

            #for y in range(size):
            #    if bb_y == 0:       # Trace profile present
            #        if y <= lb_y:
            #            self.loss_map[y, x] = LOW_LOSS
            #        elif y < ub_y:
            #            LL = (y - lb_y) * ib[x]
            #            self.loss_map[y, x] = (LL ** 2) * HI_LOSS
            #        else:
            #            self.loss_map[y, x] = HI_LOSS
            #    else:
            #        if y <= lb_y:
            #            self.loss_map[y, x] = LOW_LOSS
            #        elif y < ub_y:
            #            LL = (y - lb_y) * ib[x]
            #           self.loss_map[y, x]  (LL ** 2) * HI_LOSS2
            #        else:
            #           self.loss_map[y, x] = HI_LOSS

            #for y in range(size):
            #for y in range(min_y, size):

            #if bb_y == 0:
            #    LOSS_VALUE = HI_LOSS
            #else:
            #    LOSS_VALUE = HI_LOSS2

            #for y in range(min_y, max_y + 1):
            #    if y <= lb_y:
            #        self.loss_map[y, x] = LOW_LOSS
            #    elif y < ub_y:
            #        LL = (y - lb_y) * ib_y
            #        self.loss_map[y, x] = (LL ** 2) * LOSS_VALUE
            #    else:
            #        self.loss_map[y, x] = HI_LOSS

            if bb_y == 0:
                LOSS_VALUE = 548#HI_LOSS # this is the square value
            else:
                LOSS_VALUE = 10#HI_LOSS2

            ib_y = ib[x] * LOSS_VALUE

            Y = np.arange(min_y, max_y + 1, 1, dtype = int)
            loss_map[ Y[(Y <= lb_y)], x] = LOW_LOSS

            V = Y[(Y > lb_y) & (Y <= ub_y)]
            LL = (V - lb_y) * ib_y
            loss_map[ V, x] = LL * LL
            #loss_map[ V, x] = LL**2

            #loss_map[ Y[np.where(Y > ub_y)], x] = HI_LOSS
            loss_map[ Y[(Y > ub_y)], x] = HI_LOSS

        return loss_map.astype(int)


    ####################################################################################
    # Model optimization
    ####################################################################################
    def optimizeModel(raw_profile, params, R=0.0, H=0.0, size=400):
        # Get the parameters
        debug = params['debug']
        swarm_size = params['opt_swarm_size']
        iterations = params['opt_iterations']
        max_retries = params['opt_max_retries']
        model_type = params['opt_model_type']

        # The bevel model has an anchor point (x1, y2) which is used as the reference
        # position: win_size_x and win_size_y determine the ROI where this point is
        # placed.
        win_size_x = params['opt_win_size_x']
        win_size_y = params['opt_win_size_y']
        visualize_loss_field = params['opt_visualize_loss_field']
        opt_get_timings = params['opt_get_timings']


        # Get the profile which is used to find the best bevel position using the optimizer.
        y, x = raw_profile
        xf = np.array(x)
        yf = np.array(y)

        # Initialize best solution and best loss
        best_opt = []
        best_loss = 1e200
        valid_solution = False
        counter = 0
        start_opt_time = 1
        start_opt_time = 0
        try:
            # Find the minimum and maximum values of the detected profile.

            # FIXME: ADG_01 precompute these values for optimizations.
            min_xf = np.min(xf)
            max_xf = np.max(xf)


            min_yf = np.min(yf)
            max_yf = np.max(yf)

            # Test with larger shoulders and target M boundaries
            mid_y = (min_yf + max_yf) * 0.5

            # Find y for which we have the maxx in the upper half
            upper_x = xf[yf < mid_y]
            lower_x = xf[yf >= mid_y]

            upper_y = yf[yf < mid_y]
            lower_y = yf[yf >= mid_y]

            upper_y = upper_y[np.argmax(upper_x)]
            lower_y = lower_y[np.argmax(lower_x)]

            min_M = (lower_y - upper_y) / 1.2  # 1.4
            max_M = (lower_y - upper_y) * 1.2  # 1.4

            ###
            # TEST: proviamo a restringere la posizione valida del punto di riferimento
            upper_x = upper_x[np.argmax(upper_x)]
            miny = upper_y - win_size_y
            maxy = upper_y + win_size_y
            minx = upper_x - win_size_x
            maxx = upper_x + win_size_x

            # Define the search space according to the model
            print('Current model: ', model_type)

            if model_type == 'MiniBevelModel':

                ###############################################################
                # Mini bevel model in the object domain.
                ###############################################################

                # Create the model
                model = MiniBevelModel_S(R=R, H=H)

                # upper_x and upper_y are coordinates in the image domain, we need to
                # find the equivalent in the real-world domain
                MSp = np.array([[upper_x],
                                [upper_y],
                                [0]], dtype=np.float)

                R0 = model.image_to_object(R, H, MSp)

                miny_mm = R0[1,0] - model.pixel_to_mm(win_size_y)
                maxy_mm = R0[1,0] + model.pixel_to_mm(win_size_y)
                minx_mm = R0[0,0] - model.pixel_to_mm(win_size_x)
                maxx_mm = R0[0,0] + model.pixel_to_mm(win_size_x)

                lb = [minx_mm, miny_mm, -0.7, model.pixel_to_mm(min_M)]
                ub = [maxx_mm, maxy_mm,  0.7, model.pixel_to_mm(max_M)]

                print(upper_x, upper_y)
                print(lb)
                print(ub)
                a = input()

            elif model_type == 'CustomBevelModel':

                ##############################################################
                # Note: Model specified in the image domain
                ##############################################################

                # Create the model
                # FIXME: We need to pass the angle.
                model = CustomBevelModel_S(independent_angles = True, R=0.0)

                ##############################################################
                # Mini bevel model with asymmetrical bevel angles.
                # X1, Y1    Coordinates of the anchor point
                # alpha     angle of the bevel w.r.t. the horizontal axis.
                # A, C      Have been fixed in the model
                # delta1    Bevel first slope angle
                # delta2    Bevel second slope angle
                #
                #       Y1   X1   alpha  M     delta1 delta2
                #lb = [miny, minx, -0.7, min_M, -0.17, -0.17]
                #ub = [maxy, maxx,  0.7, max_M, 0.17, 0.17]
                min_S1 = 30
                max_S1 = 200
                min_S2 = 30
                max_S2 = 200
                min_E =  20
                max_E = 100
                min_F = 10
                max_F =  60

                lb = [miny, minx, -0.7, min_S1, min_S2, -0.10, -0.10, min_E, min_F]
                ub = [maxy, maxx,  0.7, max_S1, max_S2, 0.10, 0.10, max_E, max_F]
            elif model_type == 'TModel':
                # Create the model
                # FIXME: We need to pass the angle.
                model = TBevelModel_S(R=0.0)

                ##############################################################
                # Mini bevel model with asymmetrical bevel angles.
                # X1, Y1    Coordinates of the anchor point
                # alpha     angle of the bevel w.r.t. the horizontal axis.
                # A, C      Have been fixed in the model
                # delta1    Bevel first slope angle
                # delta2    Bevel second slope angle
                #
                #       Y1   X1   alpha  M     delta1 delta2
                #lb = [miny, minx, -0.7, min_M, -0.17, -0.17]
                #ub = [maxy, maxx,  0.7, max_M, 0.17, 0.17]
                #min_S1 = 30
                #max_S1 = 100
                #min_E =  20
                #max_E = 100

                min_S1 = 10#params['opt_tmodel_min_S1']
                max_S1 = 100#params['opt_tmodel_max_S1']
                min_M = 50#params['opt_tmodel_min_E']
                max_M = 200#params['opt_tmodel_max_E']

                lb = [miny, minx, -0.7, min_M, min_S1]
                ub = [maxy, maxx,  0.7, max_M, max_S1]


            # Prepare the arguments passed to the constraint and objective functions
            args = (yf, xf, min_yf, max_yf, min_xf, max_xf, model)

            start_time = time.time()
            loss_map = OptimizationEngine.precomputeLossMap(yf, xf, min_yf, max_yf, min_xf, max_xf, size=size)
            end_time = time.time()
            print("Loss map computation Time: %.5f" % (end_time - start_time))

            # Set the loss map in the model.
            model.setLossMap(loss_map, yf, xf, size)

            # Visualize the loss field.
            loss_field = None
            #if visualize_loss_field:
            #    loss_field = np.zeros([400, 400])
            #    loss_field = visualizeLossFunction(loss_field, yf, xf, miny, maxy, 0.5, size=400)

            ##############################################################################
            # Optimization starts here
            print("Swarm size = " + str(swarm_size))
            print("Iterations = "+ str(iterations))

            start_opt_time = time.time()

            # Try to optimize until a valid solution is found or when the maximum number of
            # trials have been reached.
            while valid_solution == False and counter <= max_retries:

                # xopt:           the best solution,
                # fopt:           the loss value associated to the best solution
                # valid_solution  True if the returned solution is valid, False otherwise.
                xopt, fopt, valid_solution = pso2(loss, lb, ub,
                                                  args=args,
                                                  debug=debug,
                                                  swarmsize=swarm_size,
                                                  maxiter=iterations, minfunc=1e-8, f_ieqcons=constraints)

                # Check if a valid solution has been found.
                if valid_solution == False:
                    print("Solution not found... retrying", counter)

                counter += 1

            stop_opt_time = time.time()

            best_opt = xopt
            best_loss = fopt

            # ADG
            print(xopt)

            #####################################
            # Trace the best model
            model.setModel(best_opt)

            (Xobj, Yobj) = model.getObjectModel()
            print("Object Model (mm)")
            print(Xobj)
            print(Yobj)

            (Ximg, Yimg) = model.getImageModel()
            print("Image Model (pixel)")
            print(Ximg)
            print(Yimg)

            # Print out bevel measures
            if model_type == 'MiniBevelModel':
                (A, B, C, M) = model.getMeasures()
                print("--------------- Bevel Measures ------------- ")
                print("A: %.3f mm" % (A))
                print("B: %.3f mm" % (B))
                print("C: %.3f mm" % (C))
                print("M: %.3f mm" % (M))


            if opt_get_timings == False:
                yc, xc = model.trace_m()
            else:
                xc = x
                yc = y
        except:
            traceback.print_exc(file=sys.stdout)
            xc = x
            yc = y

        # Prepare the output
        opt_out = dict()
        opt_out['profile'] = (yf, xf)
        opt_out['bevel'] = (yc, xc)
        opt_out['valid_solution'] = valid_solution
        opt_out['retries'] = counter
        opt_out['best_loss'] = best_loss
        opt_out['best_solution'] = best_opt
        opt_out['loss_field'] = loss_field
        opt_out['optimization_time'] = stop_opt_time - start_opt_time
        opt_out['model_type'] = model_type
        opt_out['model'] = model

        return opt_out
