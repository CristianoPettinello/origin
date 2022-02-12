//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __RETURN_CODES_H__
#define __RETURN_CODES_H__

namespace Nidek {
namespace Libraries {
namespace HTT {
enum ReturnCode
{
    UNINITIALIZED = 0,
    PREPROCESS_SUCCESSFUL = 100,
    PREPROCESS_FAILED_IMAGE_NOT_AVAILABLE,
    PREPROCESS_FAILED_WRONG_IMAGE_SIZE,
    PREPROCESS_FAILED_WRONG_IMAGE_CHANNEL,
    PREPROCESS_FAILED_PROFILE_DETECTION,
    CALIBRATION_SUCCESSFUL = 200,
    CALIBRATION_FAILED_CONVERGENCE,
    CALIBRATION_FAILED_GRID,
    CALIBRATION_FAILED_LINE_DETECTION,
    CALIBRATION_FAILED_SEARCH_SPACE,
    CALIBRATION_FAILED_PSO_INIT,
    LENSFITTING_SUCCESSFUL = 300,
    LENSFITTING_FAILED_MODEL_NOT_INITIALIZED,
    LENSFITTING_FAILED_MAX_RETRIES_REACHED,
    LENSFITTING_FAILED_BEVELFRAME_INTERSECTION,
    LENSFITTING_FAILED_PSO_BOUNDARY_REACHED,
};
} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __RETURN_CODES_H__
