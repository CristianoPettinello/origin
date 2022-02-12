//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __STATISTICS_H__
#define __STATISTICS_H__

#include "Log.h"
#include <string>
#include <vector>

using namespace std;
using namespace Nidek::Libraries::Utils;

namespace Nidek {
namespace Libraries {
namespace HTT {
class Statistics
{
private:
    vector<float> m_B;
    vector<float> m_Bstar;
    vector<float> m_beta;

    vector<float> m_M;
    vector<float> m_Loss;
    vector<float> m_Iteration;
    vector<float> m_PTime;
    vector<float> m_OTime;
    string m_folder;
    string m_filename;
    char* m_label;
    FILE* m_fstats;
    FILE* m_fCurrentStats;
    bool m_fileSaving;
    shared_ptr<Log> log;
    int m_repetitions;

public:
    Statistics(string folder, string filename, int repetitions, bool fileSaving = true);

    ~Statistics();

    void start(const char* label);
    void stop();
    //void push_back(float B, float M, float loss, int iteration, float preprocessingTime, float optimizationTime);
    void push_back(float Bstar, float B, float M, float beta, float loss, int iteration, float preprocessingTime, float optimizationTime);

    void close();

private:
    void initGlobalStats();
    float mean(vector<float> v);
    float variance(vector<float> v);
    float min(vector<float> v);
    float max(vector<float> v);
    float median(vector<float> v);

    float round(float v, int n);
};
} // namespace HTT
} // namespace Libraries
} // namespace Nidek
#endif //__STATISTICS_H__
