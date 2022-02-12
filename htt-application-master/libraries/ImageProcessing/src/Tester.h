//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __TESTER_H__
#define __TESTER_H__

#include "Settings.h"

namespace Nidek {
namespace Libraries {
namespace HTT {

class Tester
{
public:
    enum TestType
    {
        Cross,
        Square,
        Repetitions,
        GridCalibration,
        Validations,
        Comparative,
        SyntheticImagesGeneration,
        InversTransform
    };
    Tester(string settingsFile);
    void testScheimpflug(TestType type, double R = 0, double H = 0);
    void repetitionTests(string path, string folder, string modelType = "MINI_BEVEL");
    void validationTests(string path, string folder, string frame, int repetitions);
    void calibrationGrid();
    void comparativeTest();
    void generateSyntheticImages();
    void inverseTransform(string inputFile);

private:
    string m_settingsFile;
    shared_ptr<Settings> m_settings;
    shared_ptr<Log> m_log;
};

} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __TESTER_H__