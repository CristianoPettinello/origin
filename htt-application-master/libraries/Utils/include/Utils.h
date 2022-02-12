//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __UTILS_H__
#define __UTILS_H__

#include <memory> // shared_ptr
#include <vector>

#include "Image.h"
#include "UtilsGlobal.h"

using namespace std;

#ifndef NDEBUG
#define M_Assert(Expr, Msg) __M_Assert(#Expr, Expr, __FILE__, __LINE__, Msg)
#else
#define M_Assert(Expr, Msg) ;
#endif
extern UTILS_LIBRARYSHARED_EXPORT void __M_Assert(const char* expr_str, bool expr, const char* file, int line,
                                                  const char* msg);

namespace Nidek {
namespace Libraries {
namespace Utils {

///
/// \brief Auxiliary functions for manage images in PNG format
///

///
/// Function used for reading an image in PNG format
///
/// \param [in] filePath	Char pointer to a file path
///
/// \return Shared pointer to a loaded image
UTILS_LIBRARYSHARED_EXPORT shared_ptr<Image<uint8_t>> loadImage(const char* filePath);

///
/// Function used for saving an image in PNG format
///
/// \param   [in] filePath	Char pointer to a destination file path
/// \param   [in] fileName	Char pointer to a destination file name
/// \tparam  [in] srcImage	Shared pointer to a source image
///
/// \return Flag used to indicate if the save was successful or not
///
template <class T>
UTILS_LIBRARYSHARED_EXPORT bool saveImage(const string& filePath, const string& fileName,
                                          shared_ptr<Image<T>> srcImage);

template <class T>
UTILS_LIBRARYSHARED_EXPORT void saveRawImage(const string& filePath, const string& fileName,
                                             shared_ptr<Image<T>> srcImage);
///
/// Convert image float data representation to uint8
///
/// \param [in] srcImage	Float data representation source image
///
/// \return Shared pointer to the uint8_t data rapresentation image
///
UTILS_LIBRARYSHARED_EXPORT shared_ptr<Image<uint8_t>> imageToUint8(shared_ptr<Image<float>> srcImage);

///
/// Convert uint8 data representation to float image
///
/// \param [in] srcImage	uint8_t data representation source image
/// \param [in] scaling	    (optional) Flag used to scaling or not the output values. Enabled by default
///
/// \return Shared pointer to the float data rapresentation image
///
UTILS_LIBRARYSHARED_EXPORT shared_ptr<Image<float>> imageToFloat(shared_ptr<Image<uint8_t>> srcImage,
                                                                 bool scaling = true);

UTILS_LIBRARYSHARED_EXPORT shared_ptr<Image<float>> imageNormalize(shared_ptr<Image<float>> srcImage);

///
/// Function used to Split string using char delimiter
///
/// \param [in] s			Input string to splip
/// \param [in] delimiter	Delimiter character
///
/// \return Vector containing the string divided into tokens
///
UTILS_LIBRARYSHARED_EXPORT vector<string> split(const string& s, char delimiter);

UTILS_LIBRARYSHARED_EXPORT void traceCalibrationGrid(shared_ptr<Image<float>> dstImage,
                                                     vector<pair<pair<int, int>, pair<int, int>>> lines);

UTILS_LIBRARYSHARED_EXPORT void parseFilename(const string& fileName, double& R, double& H);

UTILS_LIBRARYSHARED_EXPORT void horizontalFlip(shared_ptr<Image<uint8_t>> image);

} // namespace Utils
} // namespace Libraries
} // namespace Nidek

#endif // __UTILS_H__
