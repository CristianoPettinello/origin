import numpy as np

def pso(func, lb, ub, ieqcons=[], f_ieqcons=None, args=(), kwargs={},
        swarmsize=100, omega=0.5, phip=0.5, phig=0.5, maxiter=100,
        minstep=1e-8, minfunc=1e-8, debug=False):
    """
    Perform a particle swarm optimization (PSO)

    Parameters
    ==========
    func : function
        The function to be minimized
    lb : array
        The lower bounds of the design variable(s)
    ub : array
        The upper bounds of the design variable(s)

    Optional
    ========
    ieqcons : list
        A list of functions of length n such that ieqcons[j](x,*args) >= 0.0 in
        a successfully optimized problem (Default: [])
    f_ieqcons : function
        Returns a 1-D array in which each element must be greater or equal
        to 0.0 in a successfully optimized problem. If f_ieqcons is specified,
        ieqcons is ignored (Default: None)
    args : tuple
        Additional arguments passed to objective and constraint functions
        (Default: empty tuple)
    kwargs : dict
        Additional keyword arguments passed to objective and constraint
        functions (Default: empty dict)
    swarmsize : int
        The number of particles in the swarm (Default: 100)
    omega : scalar
        Particle velocity scaling factor (Default: 0.5)
    phip : scalar
        Scaling factor to search away from the particle's best known position
        (Default: 0.5)
    phig : scalar
        Scaling factor to search away from the swarm's best known position
        (Default: 0.5)
    maxiter : int
        The maximum number of iterations for the swarm to search (Default: 100)
    minstep : scalar
        The minimum stepsize of swarm's best position before the search
        terminates (Default: 1e-8)
    minfunc : scalar
        The minimum change of swarm's best objective value before the search
        terminates (Default: 1e-8)
    debug : boolean
        If True, progress statements will be displayed every iteration
        (Default: False)

    Returns
    =======
    g : array
        The swarm's best known position (optimal design)
    f : scalar
        The objective value at ``g``

    """
    print("ADG")

    assert len(lb)==len(ub), 'Lower- and upper-bounds must be the same length'
    assert hasattr(func, '__call__'), 'Invalid function handle'
    lb = np.array(lb)
    ub = np.array(ub)
    assert np.all(ub>lb), 'All upper-bound values must be greater than lower-bound values'

    vhigh = np.abs(ub - lb)
    vlow = -vhigh

    # Check for constraint function(s) #########################################
    obj = lambda x, it: func(x, it, *args, **kwargs)
    if f_ieqcons is None:
        if not len(ieqcons):
            if debug:
                print('No constraints given.')
            cons = lambda x: np.array([0])
        else:
            if debug:
                print('Converting ieqcons to a single constraint function')
            cons = lambda x: np.array([y(x, *args, **kwargs) for y in ieqcons])
    else:
        if debug:
            print('Single constraint function given in f_ieqcons')
        cons = lambda x: np.array(f_ieqcons(x, *args, **kwargs))

    def is_feasible(x):
        check = np.all(cons(x)>=0)
        return check

    # ADG
    def is_feasible2(x, it, threshold):
        if it > threshold:
            check = np.all(cons(x)>=0)
            return check
        else:
            return True


    # Initialize the particle swarm ############################################
    S = swarmsize
    D = len(lb)  # the number of dimensions each particle has

    good_start = False
    while good_start == False:

        x = np.random.rand(S, D)  # particle positions
        v = np.zeros_like(x)  # particle velocities
        p = np.zeros_like(x)  # best particle positions
        fp = np.zeros(S)  # best particle function values
        g = []  # best swarm position
        fg = 1e100  # artificial best swarm position starting value

        for i in range(S):

            ## Initialize the particle's position
            x[i, :] = lb + x[i, :]*(ub - lb)

            # Initialize the particle's best known position
            p[i, :] = x[i, :]

            # Calculate the objective's value at the current particle's
            fp[i] = obj(p[i, :], 0)

            # At the start, there may not be any feasible starting point, so just
            # give it a temporary "best" point since it's likely to change
            if i==0:
                g = p[0, :].copy()

            # If the current particle's position is better than the swarm's,
            # update the best swarm position
            if fp[i]<fg and is_feasible(p[i, :]):
                fg = fp[i]
                g = p[i, :].copy()
            good_start = True # Pessima idea

            # Initialize the particle's velocity
            v[i, :] = vlow + np.random.rand(D)*(vhigh - vlow)

    # Iterate until termination criterion met ##################################
    it = 1
    # ADG
    threshold = 10

    # ADG
    while it<=maxiter:
        rp = np.random.uniform(size=(S, D))
        rg = np.random.uniform(size=(S, D))

        # FIXME ADG_02 : matrix computation
        v = omega * v + phip * rp * (p - x) + phig * rg * (g - x)
        x = x + v


        for i in range(S):

            # Update the particle's velocity
            #v[i, :] = omega*v[i, :] + phip*rp[i, :]*(p[i, :] - x[i, :]) + \
            #          phig*rg[i, :]*(g - x[i, :])

            # Update the particle's position, correcting lower and upper bound
            # violations, then update the objective function value
            #x[i, :] = x[i, :] + v[i, :]
            mark1 = x[i, :]<lb
            mark2 = x[i, :]>ub
            x[i, mark1] = lb[mark1]
            x[i, mark2] = ub[mark2]

            #fx = obj(x[i, :], float(it)/maxiter) # ADG: iterazione frazionaria
            fx = obj(x[i, :], 1) # ADG: iterazione frazionaria # FIXME ADG_02 iterazione frazionaria non più usata

            # Compare particle's best position (if constraints are satisfied)
            #if fx<fp[i] and is_feasible(x[i, :]):
            if fx<fp[i] and is_feasible2(x[i, :], it, threshold):
                p[i, :] = x[i, :].copy()
                fp[i] = fx

                # Compare swarm's best position to current particle's position
                # (Can only get here if constraints are satisfied)
                if fx<fg:
                    # ADG optimization: commented debug if clause - BEGIN
                    #if debug:
                    #    print('New best for swarm at iteration {:}: {:} {:}'.format(it, x[i, :], fx))
                    # ADG optimization: commented debug if clause - END

                    # ADG optimization - BEGIN
                    # we just don't need to copy x[i,:] twice. See part commented below: g = tmp.copy()
                    #tmp = x[i, :].copy()
                    # ADG optimization - END

                    # ADG optimization - BEGIN
                    # This is commented because not used.
                    #stepsize = np.sqrt(np.sum((g-tmp)**2))
                    # SDG optimization - END

                    # ADG optimization of stopping criteria - BEGIN
                    # ADG: the stop criterion on minfunc is tested only if we have just found a new
                    # best solution
                    #if np.abs(fg - fx)<=minfunc:
                    #    print('Stopping search: Swarm best objective change less than {:}'.format(minfunc))
                    #    return tmp, fx
                    #elif stepsize<=minstep:
                    #    print('Stopping search: Swarm best position change less than {:}'.format(minstep))
                    #    return tmp, fx
                    #else:
                    #    g = tmp.copy()
                    #    fg = fx

                    # ADG Nested optimization: BEGIN
                    # As said before we do not need to copy x[i,:] twice.
                    #g = tmp.copy()
                    # which is substituted with
                    g = x[i, :].copy()
                    fg = fx
                    # ADG Nested optimization: END

                    # ADG optimization of stopping criteria - END


        #if it > 20 and not is_feasible(g):
        if it == 20 and not is_feasible(g):
            return g, fg, False

        if debug:
            print('Best after iteration {:}: {:} {:}'.format(it, g, fg))
        it += 1

    print('Stopping search: maximum iterations reached --> {:}'.format(maxiter))

    goodsolution = True
    if not is_feasible(g):
        print("However, the optimization couldn't find a feasible design. Sorry")
        goodsolution = False

    return g, fg, goodsolution


def pso2(func, lb, ub, ieqcons=[], f_ieqcons=None, args=(), kwargs={},
        swarmsize=100, omega=0.4, phip=0.5, phig=0.5, maxiter=100,
        minstep=1e-8, minfunc=1e-8, debug=False):
    """
    Perform a particle swarm optimization (PSO)

    Parameters
    ==========
    func : function
        The function to be minimized
    lb : array
        The lower bounds of the design variable(s)
    ub : array
        The upper bounds of the design variable(s)

    Optional
    ========
    ieqcons : list
        A list of functions of length n such that ieqcons[j](x,*args) >= 0.0 in
        a successfully optimized problem (Default: [])
    f_ieqcons : function
        Returns a 1-D array in which each element must be greater or equal
        to 0.0 in a successfully optimized problem. If f_ieqcons is specified,
        ieqcons is ignored (Default: None)
    args : tuple
        Additional arguments passed to objective and constraint functions
        (Default: empty tuple)
    kwargs : dict
        Additional keyword arguments passed to objective and constraint
        functions (Default: empty dict)
    swarmsize : int
        The number of particles in the swarm (Default: 100)
    omega : scalar
        Particle velocity scaling factor (Default: 0.5)
    phip : scalar
        Scaling factor to search away from the particle's best known position
        (Default: 0.5)
    phig : scalar
        Scaling factor to search away from the swarm's best known position
        (Default: 0.5)
    maxiter : int
        The maximum number of iterations for the swarm to search (Default: 100)
    minstep : scalar
        The minimum stepsize of swarm's best position before the search
        terminates (Default: 1e-8)
    minfunc : scalar
        The minimum change of swarm's best objective value before the search
        terminates (Default: 1e-8)
    debug : boolean
        If True, progress statements will be displayed every iteration
        (Default: False)

    Returns
    =======
    g : array
        The swarm's best known position (optimal design)
    f : scalar
        The objective value at ``g``

    """
    assert len(lb)==len(ub), 'Lower- and upper-bounds must be the same length'
    assert hasattr(func, '__call__'), 'Invalid function handle'
    lb = np.array(lb)
    ub = np.array(ub)
    assert np.all(ub>lb), 'All upper-bound values must be greater than lower-bound values'

    vhigh = np.abs(ub - lb)
    vlow = -vhigh

    # Check for constraint function(s) #########################################
    obj = lambda x, it: func(x, it, *args, **kwargs)
    if f_ieqcons is None:
        if not len(ieqcons):
            if debug:
                print('No constraints given.')
            cons = lambda x: np.array([0])
        else:
            if debug:
                print('Converting ieqcons to a single constraint function')
            cons = lambda x: np.array([y(x, *args, **kwargs) for y in ieqcons])
    else:
        if debug:
            print('Single constraint function given in f_ieqcons')
        cons = lambda x: np.array(f_ieqcons(x, *args, **kwargs))

    def is_feasible(x):
        check = np.all(cons(x)>=0)
        return check

    # ADG
    def is_feasible2(x, it, threshold):
        if it > threshold:
            check = np.all(cons(x)>=0)
            return check
        else:
            return True


    # Initialize the particle swarm ############################################
    S = swarmsize
    D = len(lb)  # the number of dimensions each particle has

    good_start = False
    while good_start == False:

        x = np.random.rand(S, D)  # particle positions
        v = np.zeros_like(x)  # particle velocities
        p = np.zeros_like(x)  # best particle positions
        fp = np.zeros(S)  # best particle function values
        g = []  # best swarm position
        fg = 1e100  # artificial best swarm position starting value


        for i in range(S):
            ## Initialize the particle's position
            x[i, :] = lb + x[i, :]*(ub - lb)

            # Initialize the particle's best known position
            p[i, :] = x[i, :]

            # Calculate the objective's value at the current particle's
            fp[i] = obj(p[i, :], 0)

            # At the start, there may not be any feasible starting point, so just
            # give it a temporary "best" point since it's likely to change
            if i==0:
                g = p[0, :].copy()

            # If the current particle's position is better than the swarm's,
            # update the best swarm position
            if fp[i]<fg and is_feasible(p[i, :]):
                fg = fp[i]
                g = p[i, :].copy()
            good_start = True # Pessima idea

            # Initialize the particle's velocity
            v[i, :] = vlow + np.random.rand(D)*(vhigh - vlow)

    # Iterate until termination criterion met ##################################
    it = 1
    # ADG
    threshold = 10

    # ADG
    while it <= threshold:
        rp = np.random.uniform(size=(S, D))
        rg = np.random.uniform(size=(S, D))

        # FIXME ADG_02 : matrix computation
        #v = omega * v + phip * rp * (p - x) + phig * rg * (g - x)

        v = omega * (v + rp * (p - x) + rg * (g - x))
        x = x + v

        for i in range(S):
            # Update the particle's velocity
            #v[i, :] = omega*v[i, :] + phip*rp[i, :]*(p[i, :] - x[i, :]) + \
            #          phig*rg[i, :]*(g - x[i, :])

            # ADG: omega = phip = phig = 0.5
            #v[i, :] = omega*(v[i, :] + rp[i, :] * (p[i, :] - x[i, :]) + rg[i, :] * (g - x[i, :]))



            # Update the particle's position, correcting lower and upper bound
            # violations, then update the objective function value
            #x[i, :] = x[i, :] + v[i, :]
            mark1 = x[i, :]<lb
            mark2 = x[i, :]>ub
            x[i, mark1] = lb[mark1]
            x[i, mark2] = ub[mark2]
            fx = obj(x[i, :], it) # ADG (introduced it)

            # Compare particle's best position (if constraints are satisfied)
            if fx<fp[i]: #and is_feasible2(x[i, :], it, threshold):
                p[i, :] = x[i, :].copy()
                fp[i] = fx

                # Compare swarm's best position to current particle's position
                # (Can only get here if constraints are satisfied)
                if fx<fg:
                    g = x[i, :].copy()
                    fg = fx

        #if it == 20 and not is_feasible(g):
        #    return g, fg, False

        #if debug:
        #    print('Best after iteration {:}: {:} {:}'.format(it, g, fg))
        it += 1

    # TEST experimental ADG 2020-03-17
    initial_solution_is_feasible = is_feasible(g)
    print(initial_solution_is_feasible, fg)
    #solutions = []

    while it<=maxiter:
        rp = np.random.uniform(size=(S, D))
        rg = np.random.uniform(size=(S, D))

        v = omega * (v + rp * (p - x) + rg * (g - x))
        x = x + v

        for i in range(S):
            # Update the particle's velocity
            #v[i, :] = omega*v[i, :] + phip*rp[i, :]*(p[i, :] - x[i, :]) + \
            #          phig*rg[i, :]*(g - x[i, :])

            # ADG: omega = phip = phig = 0.5
            #v[i, :] = omega*(v[i, :] + rp[i, :] * (p[i, :] - x[i, :]) + rg[i, :] * (g - x[i, :]))


            # Update the particle's position, correcting lower and upper bound
            # violations, then update the objective function value
            #x[i, :] = x[i, :] + v[i, :]
            mark1 = x[i, :]<lb
            mark2 = x[i, :]>ub
            x[i, mark1] = lb[mark1]
            x[i, mark2] = ub[mark2]
            fx = obj(x[i, :], it) # ADG (introduced it)

            # Compare particle's best position (if constraints are satisfied)
            #if fx<fp[i] and is_feasible2(x[i, :], it, threshold):
            if fx<fp[i] and is_feasible(x[i, :]):
                p[i, :] = x[i, :].copy()
                fp[i] = fx

                # Compare swarm's best position to current particle's position
                # (Can only get here if constraints are satisfied)

                # IDEA: fx < fg potrebbe essere una condizione malposta se la soluzione g è non-feasible ma per qualche
                # ragione ha fg < fx feasible.
                #print("S", fx)
                if initial_solution_is_feasible == False:
                    fg = fx + 1.0 # Small hack so that the new solution is selected
                    initial_solution_is_feasible = True
                    print("new solution forced")

                if fx<fg:
                    g = x[i, :].copy()
                    #delta = fg-fx
                    #print(delta)
                    fg = fx

                    # ADG TEST 2020-03-17 -------
                    #sol_ = g.copy()
                    #solutions.append(sol_)
                    # ADG TEST 2020-03-17 -------

        if it == 20 and not is_feasible(g):
            print("Stop condition")
            return g, fg, False

        #if debug:
        #    print('Best after iteration {:}: {:} {:}'.format(it, g, fg))
        it += 1

    print('Stopping search: maximum iterations reached --> {:}'.format(maxiter))

    goodsolution = True
    if not is_feasible(g):
        print("However, the optimization couldn't find a feasible design. Sorry")
        goodsolution = False

    #solutions = None
    return g, fg, goodsolution#, solutions

