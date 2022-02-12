//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __PSO_H__
#define __PSO_H__

#include "Log.h"
#include <vector>

using namespace std;
using namespace Nidek::Libraries::Utils;

namespace Nidek {
namespace Libraries {
namespace HTT {

template <class T> class Pso
{
public:
    enum ReturnCode
    {
        SUCCESS = 0,
        MEMORY_ALLOCATION_FAILURE,
        INITIALIZATION_FAILURE,
        BOUNDARY_REACHED
    };

    class Input
    {
    public:
        void (*costF)(T* res, T* X, int nDims, int nParticles, void* args);

        vector<T> lb; // Lower bound
        vector<T> ub; // Upper bound

        // Algorithm parameters
        T w0i; // Initial velocity
        T w0f; // Final velocity

        T cp; // Cognitive weight
        T cg; // Social weight

        int nIterations;
        int nParticles;

        T tolerance;

        bool debug;
    };

    class Output
    {
    public:
        //
        // Optimum solution determined by the PSO.
        vector<T> optimumX;

        // Minimum value of the cost function
        vector<T> optimumRes;

        T tolerance;
        bool validSolution;

        T loss;

        int optimumIteration;
    };

public:
    // Constructor
    Pso();

    // Destructor
    ~Pso();

    // Optimization
    ReturnCode optimize(Input& in, Output& out);

    void setArgs(void* args);

private:
    // Initialize the PSO.
    bool initialize(Input& in, Output& out);

    // Memory allocation
    bool allocateMemory(Input& in, Output& out);

    // Release memory
    void releaseMemory();

private:
    shared_ptr<Log> log;

    void* m_args;

    // Dimension of the search space
    int m_nDims;

    // Current solutions
    T* m_X;

    // Best solution found so far
    T* m_bestX;

    // Particle velocities
    T* m_Vx;

    // Current fitness function value for each particle.
    T* m_res;

    // Best fitness function value for each particle
    T* m_bestRes;

    // Optimum solution
    T* m_optimumX;

    // Optimum value for the fitness function.
    T m_optimumRes;

    // Minimum position for the fitness function.
    int m_minPos;

    // Flag used for enable debugging
    bool m_debug;
};

} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __PSO_H__
