//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __PROFILE_H__
#define __PROFILE_H__

#include <vector>

using namespace std;

namespace Nidek {
namespace Libraries {
namespace HTT {
///
/// \brief Profile class.
///
/// A profile represents a sequence of points on the image.
/// As instance, this can be the profile extracted by thresholding or
/// the representation of the bevel model on the image.
///
template <class T> class Profile
{
public:
    /// Clear vectors
    ///
    void clear();

    /// Reserve space for vector elements, if size <= 0 nothing is done.
    ///
    /// \param [in] size    Amount of items to be contained by vector
    ///
    void reserve(int size);

public:
    vector<T> x;            ///< X coordinates of a profile on the image.
    vector<T> y;            ///< Y coordinates of a profile on the image.
    vector<T> imgX;         ///< X coordinates of a model points onto the image plane
    vector<T> imgY;         ///< Y coordinates of a model points onto the image plane
    vector<T> distanceImgX; ///< X coordinates of the points used to compute the distance D onto the image plane
    vector<T> distanceImgY; ///< Y coordinates of the points used to compute the distance D onto the image plane
    vector<T> ancorImgX;    ///< X coordinates of the upper and lower maxima found on the profile onto the image plane
    vector<T> ancorImgY;    ///< Y coordinates of the upper and lower maxima found on the profile onto the image plane
};

} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __PROFILE_H__
