//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include "Pso.h"

#include <limits>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <time.h>

using namespace Nidek::Libraries::HTT;
using namespace std;

static const char* tag = "[PSO ] ";

template class Nidek::Libraries::HTT::Pso<float>;

template <class T>
Pso<T>::Pso()
    : log(Log::getInstance()),
      m_args(nullptr),
      m_nDims(0),
      m_X(nullptr),
      m_bestX(nullptr),
      m_Vx(nullptr),
      m_res(nullptr),
      m_bestRes(nullptr),
      m_optimumX(nullptr),
      m_optimumRes(0),
      m_debug(false)
{
}

template <class T> Pso<T>::~Pso()
{
    releaseMemory();
}

template <class T> void Pso<T>::setArgs(void* args)
{
    m_args = args;
}

template <class T> typename Pso<T>::ReturnCode Pso<T>::optimize(Input& in, Output& out)
{
    m_debug = in.debug;

    // Get the Search space dimensionality
    m_nDims = (int)in.lb.size();

    // Memory allocation
    if (!allocateMemory(in, out))
        return ReturnCode::MEMORY_ALLOCATION_FAILURE;

    // Initializations
    if (!initialize(in, out))
        return ReturnCode::INITIALIZATION_FAILURE;

    const T iRandMax = (T)1.0 / RAND_MAX;
    const int byteVectorSize = sizeof(T) * m_nDims;
    const T cpiRandMax = in.cp * iRandMax;
    const T cgiRandMax = in.cg * iRandMax;
    const T wi = in.w0i;
    const T wf = in.w0f;

    const T infty = std::numeric_limits<T>::max();

    T prevRes = infty;
    T tolerance = 0.0;
    int optimumIteration = -1;

    int nIterations = in.nIterations;
    T iIt = 1.0f / nIterations;

    bool validSolution = false;

    if (m_debug)
    {
        stringstream s;
        s << tag << "Initial optimum value " << m_optimumRes;
        log->debug(s.str());
        for (unsigned int i = 0; i < in.lb.size(); ++i)
        {
            s.str("");
            s << tag << i << ":  " << in.lb[i] << " " << in.ub[i];
            log->debug(s.str());
        }
    }

    int shakeParticleCounter = 0;

    for (int i = 0; i < nIterations; ++i)
    {
        const T w = (wf * (i) + wi * (nIterations - i)) * iIt;

        // Generate random values
        const T cpRp = cpiRandMax * (T)rand();
        const T cgRg = cgiRandMax * (T)rand();

        // Compute the new velocities.
        T* VxBasePtr = m_Vx;
        T* bestXBasePtr = m_bestX;
        T* XBasePtr = m_X;
        T* optimumXBasePtr = m_optimumX;

        for (int m = 0; m < m_nDims; ++m)
        {
            T optX = *optimumXBasePtr++;

            // Assign the pointers for the inner loop and then updates
            // the base pointers of the outer loop.
            T* VxPtr = VxBasePtr++;
            T* bestXPtr = bestXBasePtr++;
            T* XPtr = XBasePtr++;

            // Lower and upper boundary for this variable
            const T lbValue = in.lb[m];
            const T ubValue = in.ub[m];

            for (int s = in.nParticles - 1; s >= 0; --s)
            {
                T x = *XPtr;
                T vx = w * (*VxPtr) + cpRp * (*bestXPtr - x) + cgRg * (optX - x);

                // Update particle position and check boundaries.
                x += vx;

                if (x < lbValue)
                {
                    x = lbValue;
                    const T absDiff = fabs(ubValue - lbValue);
                    const T iRandMaxAbsDiff2 = iRandMax * absDiff * 4.0f;
                    vx = (T)rand() * iRandMaxAbsDiff2 - absDiff;

                } else if (x > ubValue)
                {
                    x = ubValue;
                    const T absDiff = fabs(ubValue - lbValue);
                    const T iRandMaxAbsDiff2 = iRandMax * absDiff * 4.0f;
                    vx = (T)rand() * iRandMaxAbsDiff2 - absDiff;
                }

                // Set the new value.
                *XPtr = x;
                *VxPtr = vx;

                XPtr += m_nDims;
                VxPtr += m_nDims;
                bestXPtr += m_nDims;
            }
        }

        shakeParticleCounter += 50;

        if (shakeParticleCounter >= in.nParticles)
            shakeParticleCounter = 0;

        // Compute the cost function in the new points.
        in.costF(m_res, m_X, m_nDims, in.nParticles, m_args);

        // Search for the minima
        T minRes = infty;

        T* resPtr = m_res;
        T* bestResPtr = m_bestRes;

        int index = 0;
        const int nParticles = in.nParticles;
        for (int s = 0; s < nParticles; ++s, ++resPtr, ++bestResPtr)
        {
            if (*bestResPtr > *resPtr)
            {
                *bestResPtr = *resPtr;
                memcpy(&(m_bestX[index]), &(m_X[index]), byteVectorSize);
            }

            // Find the minimum res.
            if (minRes >= *resPtr)
            {
                minRes = *resPtr;
                m_minPos = s;
            }

            index += m_nDims;
        }

        if (m_optimumRes > minRes && m_minPos >= 0)
        {
            m_optimumRes = minRes;
            optimumIteration = i;
            validSolution = true;

            memcpy(m_optimumX, &(m_bestX[m_minPos * m_nDims]), byteVectorSize);

            if (m_debug)
            {
                stringstream s;
                s << tag << "Optimum Res " << m_optimumRes << "   it: " << i << "  s: " << m_minPos << " : ";
                for (int m = 0; m < m_nDims; ++m)
                {
                    s << m_optimumX[m] << " ";
                }
                log->debug(s.str());
            }

            tolerance = prevRes - minRes;
            prevRes = minRes;
        }
    }

    ReturnCode ret = ReturnCode::SUCCESS;

    {
        stringstream s;
        s << tag << "Optimum Res " << m_optimumRes << "  Valid: " << boolalpha << validSolution;
        log->info(s.str());

        s.str("");
        s << tag << "Boundaries reached: [ ";
        bool bReached = false;
        for (int m = 0; m < m_nDims; ++m)
        {
            // if (m_optimumX[m] < in.lb[m] + 1e-3 || m_optimumX[m] > in.ub[m] - 1e-3)
            if (m_optimumX[m] == in.lb[m] || m_optimumX[m] == in.ub[m])
            {
                s << m << " ";
                bReached = true;
            }
        }
        s << "]";
        if (bReached)
        {
            log->warn(s.str());
            ret = ReturnCode::BOUNDARY_REACHED;
        }
    }

    // Set the output values
    out.optimumRes.clear();
    out.optimumRes.push_back(m_optimumRes);

    out.optimumX.clear();
    for (int m = 0; m < m_nDims; ++m)
        out.optimumX.push_back(m_optimumX[m]);

    out.tolerance = tolerance;
    out.validSolution = validSolution;
    out.optimumIteration = optimumIteration;
    out.loss = m_optimumRes;

    return ret;
}

template <class T> bool Pso<T>::initialize(Input& in, Output& /*out*/)
{
    srand((unsigned int)time(NULL));

    // Precompute some parameters.
    T iRandMax = (T)1.0 / RAND_MAX;
    T infty = std::numeric_limits<T>::max();

    int matrixSize = m_nDims * in.nParticles;
    int byteVectorSize = sizeof(T) * m_nDims;

    // Initialization of the solutions.
    for (int m = 0; m < m_nDims; ++m)
    {
        // Get the boundary values for the current dimension.
        T xMin = in.lb[m];
        T xMax = in.ub[m];
        T absDiff = fabs(xMax - xMin);

        T iRandMaxAbsDiff = iRandMax * absDiff;
        T iRandMaxAbsDiff2 = iRandMaxAbsDiff * 2.0f;

        // Lower and upper boundary for this variable
        T lbValue = in.lb[m];
        T ubValue = in.ub[m];

        int index = m;
        for (int s = 0; s < in.nParticles; ++s)
        {
            m_X[index] = (T)rand() * iRandMaxAbsDiff + xMin;
            m_Vx[index] = (T)rand() * iRandMaxAbsDiff2 - absDiff;

            // Check boundaries
            if (m_X[index] < lbValue)
                m_X[index] = lbValue;
            else if (m_X[index] > ubValue)
                m_X[index] = ubValue;

            index += m_nDims;
        }

        m_optimumX[m] = infty;
    }

    // Init bestX with the values of X.
    memcpy(m_bestX, m_X, sizeof(T) * matrixSize);

    T* bestResPtr = m_bestRes;
    for (int s = in.nParticles - 1; s >= 0; --s)
        *bestResPtr++ = infty;

    // Compute the cost function on the current set of solutions.
    in.costF(m_res, m_X, m_nDims, in.nParticles, m_args);

    //---------------------------------------------
    // Get the minimum value and position in res[]
    //---------------------------------------------
    m_optimumRes = infty;

    T minRes = infty;
    m_minPos = -1;

    T* resPtr = m_res;
    for (int s = 0; s < in.nParticles; ++s, ++resPtr)
    {
        if (minRes >= *resPtr)
        {
            minRes = *resPtr;
            m_minPos = s;
        }
    }

    // Check that minPos is consistent
    if (m_minPos < 0 || m_minPos >= in.nParticles)
        return false;

    // Get the best global solution.
    if (m_optimumRes >= minRes && m_minPos >= 0)
    {
        m_optimumRes = minRes;
        memcpy(m_optimumX, &(m_bestX[m_minPos * m_nDims]), byteVectorSize);
    }

    return true;
}

template <class T> void Pso<T>::releaseMemory()
{
    if (m_X)
    {
        delete[] m_X;
        m_X = nullptr;
    }

    if (m_bestX)
    {
        delete[] m_bestX;
        m_bestX = nullptr;
    }

    if (m_Vx)
    {
        delete[] m_Vx;
        m_Vx = nullptr;
    }

    if (m_res)
    {
        delete[] m_res;
        m_res = nullptr;
    }

    if (m_bestRes)
    {
        delete[] m_bestRes;
        m_bestRes = nullptr;
    }

    if (m_optimumX)
    {
        delete[] m_optimumX;
        m_optimumX = nullptr;
    }
}

template <class T> bool Pso<T>::allocateMemory(Input& in, Output& /*out*/)
{
    releaseMemory();

    try
    {
        int matrixSize = m_nDims * in.nParticles;

        m_X = new T[matrixSize];
        m_bestX = new T[matrixSize];
        m_Vx = new T[matrixSize];
        m_res = new T[in.nParticles];
        m_bestRes = new T[in.nParticles];
        m_optimumX = new T[m_nDims];
    } catch (...)
    {
        return false;
    }

    return true;
}
