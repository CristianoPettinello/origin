//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include "Profile.h"

using namespace Nidek::Libraries::HTT;

template class Nidek::Libraries::HTT::Profile<int>;
template class Nidek::Libraries::HTT::Profile<float>;

template <class T> void Profile<T>::clear()
{
    x.clear();
    y.clear();
}

template <class T> void Profile<T>::reserve(int size)
{
    if (size > 0)
    {
        x.reserve(size);
        y.reserve(size);
    }
}
