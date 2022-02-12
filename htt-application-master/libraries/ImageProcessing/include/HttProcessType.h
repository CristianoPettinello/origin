//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __HTT_PROCESS_TYPE_H__
#define __HTT_PROCESS_TYPE_H__

namespace Nidek {
namespace Libraries {
namespace HTT {

enum HttProcessType
{
    lensFitting,
    calibration,
    validation,
    testing,
    measure
};

enum HttCalibrationType
{
    Grid,
    Frame,
    MultiFrames
};

} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __HTT_PROCESS_TYPE_H__
