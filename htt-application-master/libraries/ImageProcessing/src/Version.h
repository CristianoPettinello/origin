//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __VERSION_H__
#define __VERSION_H__

namespace Nidek {
namespace Libraries {
namespace HTT {

const char* str =
#include "../VERSION"
    ;

static string getVersion()
{
    return str;
}

} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __VERSION_H__