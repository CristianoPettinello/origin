//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include "Statistics.h"
#include "Utils.h"
#include <algorithm> /* min_element, max_element, sort */
#include <math.h>
#include <numeric>  /* accumulate */
#include <sstream>  /* stringstream */
#include <string.h> /* strdup */

using namespace std;
using namespace Nidek::Libraries::HTT;

static const char* params[8] = {"B*", "B", "M", "beta", "loss", "It", "pTime", "oTime"};
static const int showFields[8] = {1, 1, 1, 1, 0, 0, 0, 0};
static const int precision[8] = {3, 3, 3, 3, 3, 0, 1, 1};

Statistics::Statistics(string folder, string filename, int repetitions, bool fileSaving /* = true */)
    : m_folder(folder),
      m_filename(filename),
      m_label(nullptr),
      m_fstats(nullptr),
      m_fCurrentStats(nullptr),
      m_fileSaving(fileSaving),
      log(Log::getInstance()),
      m_repetitions(repetitions)
{
    M_Assert(repetitions > 0, "Repetitions must be more than zero");

    if (m_fileSaving)
    {
        // Make parent directories as needed
        stringstream s;
        s << "mkdir -p " << folder;
        system(s.str().c_str());
    } else
    {
        log->info("Statistics file saving disabled");
    }

    m_B.reserve(m_repetitions);
    m_M.reserve(m_repetitions);
    m_Loss.reserve(m_repetitions);
    m_Iteration.reserve(m_repetitions);
    m_PTime.reserve(m_repetitions);
    m_OTime.reserve(m_repetitions);

    if (m_fileSaving)
        initGlobalStats();
}

Statistics::~Statistics()
{
    if (m_fCurrentStats)
    {
        fclose(m_fCurrentStats);
        m_fCurrentStats = nullptr;
    }

    if (m_fstats)
    {
        fclose(m_fstats);
        m_fstats = nullptr;
    }

    if (m_label)
    {
        free(m_label);
        m_label = nullptr;
    }
}

void Statistics::start(const char* label)
{
#ifdef UNIX
    if (m_fileSaving)
    {
        M_Assert(m_label == nullptr, "Start() function already called");
        M_Assert(m_fCurrentStats == nullptr, "File already opened");

        m_label = strdup(label);
        // m_label = _strdup(label);
        string f = m_folder + "/" + m_label + ".csv";
        m_fCurrentStats = fopen(f.c_str(), "w");
        fprintf(m_fCurrentStats, "%s;%s;%s;%s;%s;%s\n", "B*", "B", "M", "beta", "loss", "iteration", "pTime", "oTime");
    }
#endif

    m_Bstar.clear();
    m_B.clear();
    m_beta.clear();
    m_M.clear();
    m_Loss.clear();
    m_Iteration.clear();
    m_PTime.clear();
    m_OTime.clear();

    m_Bstar.reserve(m_repetitions);
    m_beta.reserve(m_repetitions);
    m_B.reserve(m_repetitions);
    m_M.reserve(m_repetitions);
    m_Loss.reserve(m_repetitions);
    m_Iteration.reserve(m_repetitions);
    m_PTime.reserve(m_repetitions);
    m_OTime.reserve(m_repetitions);
}

void Statistics::stop()
{
    // vector<float> avgV = {avg(m_B), avg(m_M), avg(m_Loss), avg(m_Iteration), avg(m_PTime), avg(m_OTime)};
    vector<float> minV = {min(m_Bstar), min(m_B), min(m_M), min(m_beta), min(m_Loss), min(m_Iteration), min(m_PTime), min(m_OTime)};
    vector<float> maxV = {max(m_Bstar), max(m_B), max(m_M), max(m_beta), max(m_Loss), max(m_Iteration), max(m_PTime), max(m_OTime)};
    vector<float> medianV = {median(m_Bstar), median(m_B),         median(m_M),     median(m_beta), median(m_Loss),
                             median(m_Iteration), median(m_PTime), median(m_OTime)};
    vector<float> meanV = {mean(m_Bstar), mean(m_B), mean(m_M), mean(m_beta)};
    vector<float> varV = {variance(m_Bstar), variance(m_B), variance(m_M), variance(m_beta)};
    if (m_fileSaving)
    {
        for (int i = 0; i < (int)m_B.size(); ++i)
        {
            fprintf(m_fCurrentStats, "%f;%f;%f;%f;%f;%d;%f;%f\n", m_Bstar[i], m_B[i], m_M[i], m_beta[i], m_Loss[i], (int)m_Iteration[i], m_PTime[i],
                    m_OTime[i]);
        }

        fclose(m_fCurrentStats);
        m_fCurrentStats = nullptr;

        // Process statistics and save into cvs file
        fprintf(m_fstats, "%s;", m_label);

        for (int i = 0; i < 8; ++i)
        {
            // fprintf(m_fstats, "%f;%f;%f;%f;", avgV[i], minV[i], maxV[i], medianV[i]);

            if (showFields[i])
            {
                int p = precision[i];
                if (i == 0 || i == 1 || i == 2 || i == 3) // B* B, M or beta
                    fprintf(m_fstats, "%.*f;%.*f;%.*f;%.*f;%f;", p, medianV[i], p, minV[i], p, maxV[i], p, meanV[i],
                            varV[i]);
                else
                    fprintf(m_fstats, "%.*f;%.*f;%.*f;", p, medianV[i], p, minV[i], p, maxV[i]);
            }
        }

        fprintf(m_fstats, "\n");
    }

    if (m_label)
    {
        free(m_label);
        m_label = nullptr;
    }
}

/*void Statistics::push_back(float B, float M, float loss, int iteration, float preprocessingTime, float optimizationTime)
{
    m_B.push_back(B);
    m_M.push_back(M);
    m_Loss.push_back(loss);
    m_Iteration.push_back((float)iteration);
    m_PTime.push_back(preprocessingTime * 1000); // Time in ms
    m_OTime.push_back(optimizationTime * 1000);  // Time in ms
}*/

void Statistics::push_back(float Bstar, float B, float M, float beta, float loss, int iteration, float preprocessingTime, float optimizationTime)
{
    m_Bstar.push_back(Bstar);
    m_B.push_back(B);
    m_beta.push_back(beta);
    m_M.push_back(M);
    m_Loss.push_back(loss);
    m_Iteration.push_back((float)iteration);
    m_PTime.push_back(preprocessingTime * 1000); // Time in ms
    m_OTime.push_back(optimizationTime * 1000);  // Time in ms
}

void Statistics::close()
{
    if (m_fstats)
    {
        fclose(m_fstats);
        m_fstats = nullptr;
    }
}

void Statistics::initGlobalStats()
{
    close();

#ifdef UNIX
    string path = m_folder + "/" + m_filename;
    m_fstats = fopen(path.c_str(), "w");
    M_Assert(m_fstats != nullptr, "Failed to open the file for saving the statistics");
    // const char *params[6] = {"B", "M", "loss", "iteration", "pTime", "oTime"};
    fprintf(m_fstats, "%s;", "Image");
    for (int i = 0; i < 8; ++i)
    {
        // if (i > 0)
        //    fprintf(m_fstats, ";");
        // fprintf(m_fstats, "%s%s;%s%s;%s%s;%s%s", params[i], "_avg", params[i], "_min", params[i], "_max", params[i],
        // "_med");

        // Removed the mean
        if (showFields[i])
        {
            if (i > 0)
                fprintf(m_fstats, ";");
            if (i == 0 || i == 1 || i == 2 || i == 3) // B* B or M, delta
                fprintf(m_fstats, "%s%s;%s%s;%s%s;%s%s;%s%s", params[i], " [med]", params[i], " [min]", params[i],
                        " [max]", params[i], " [avg]", params[i], " [var]");
            else
                fprintf(m_fstats, "%s%s;%s%s;%s%s", params[i], " [med]", params[i], " [min]", params[i], " [max]");
        }
    }
    fprintf(m_fstats, "\n");
#endif
}

float Statistics::mean(vector<float> v)
{
    if (v.size() == 0)
        return 0;
    auto n = v.size();
    float mean = 0.0f;
    if (n != 0)
    {
        mean = (float)accumulate(v.begin(), v.end(), 0.0) / n;
    }
    return mean;
}

float Statistics::variance(vector<float> v)
{
    const int sz = v.size();
    if (sz == 1)
        return 0.0;

    // Calculate the mean
    const float m = mean(v);

    // Now calculate the variance
    auto variance_func = [&m, &sz](float accumulator, const float& val) {
        return accumulator + ((val - m) * (val - m) / (sz - 1));
    };

    const float var = accumulate(v.begin(), v.end(), 0.0, variance_func);
    return var;
}

float Statistics::min(vector<float> v)
{
    if (v.size() == 0)
        return 0;
    auto it = min_element(v.begin(), v.end());
    return *it;
}

float Statistics::max(vector<float> v)
{
    if (v.size() == 0)
        return 0;
    auto it = max_element(v.begin(), v.end());
    return *it;
}

float Statistics::median(vector<float> v)
{
    if (v.size() == 0)
        return 0;
    sort(v.begin(), v.end());
    float median = ((v.size() % 2) == 0) ? 0.5f * (v[v.size() / 2 - 1] + v[v.size() / 2]) : v[v.size() / 2];
    return median;
}

float Statistics::round(float v, int n)
{
    if (n <= 0)
        return v;

    float m = (float)pow(10, n);
    return (float)(floor(v * m + 0.5) / m);
}
