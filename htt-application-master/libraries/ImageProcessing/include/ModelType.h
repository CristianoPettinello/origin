//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __MODEL_TYPE_H__
#define __MODEL_TYPE_H__

#include <string>

using namespace std;

namespace Nidek {
namespace Libraries {
namespace HTT {

enum ModelType
{
    Undefined = 0,
    MiniBevelModelType = 0x01,    // 0001
    TBevelModelType = 0x02,       // 0010
    CustomBevelModelType = 0x04,  // 0100
    MiniBevelModelExtType = 0x08, // 1000
};

inline ModelType getModelType(const string& model)
{

    if (model == "T_BEVEL")
        return ModelType::TBevelModelType;
    else if (model == "CUSTOM_BEVEL")
        return ModelType::CustomBevelModelType;
    else if (model == "MINI_BEVEL_EXT")
        return ModelType::MiniBevelModelExtType;
    else
        // default value
        return ModelType::MiniBevelModelType;
}

} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __MODEL_TYPE_H__
