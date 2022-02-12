//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __IMAGEPREPROCESSING_H__
#define __IMAGEPREPROCESSING_H__

#include <memory> // shared_ptr

#include "Image.h"
#include "Log.h"
#include "Profile.h"

using namespace std;
using namespace Nidek::Libraries::Utils;

namespace Nidek {
namespace Libraries {
namespace HTT {
///
/// \brief ImagePreprocessing class.
///
/// This class is in charge of applying all the preprocessing operations to the input image.
///
class ImagePreprocessing
{
public:
    template <typename T> struct ThresholdArgs
    {
        shared_ptr<Image<T>> lowPassImage;
        shared_ptr<Image<T>> sobelImage;
        T threshold;
        uint16_t searchWidth;
        uint16_t searchOffset;
        uint16_t leftBoundary;
        uint16_t rightBoundary;
        uint16_t upperBoundary;
        uint16_t lowerBoundary;
        uint8_t maxIterations;
        int profileOffset;

        // Stylus mask
        bool enableStylusMask;
        int stylusMaskX1;
        int stylusMaskY1;
        int stylusMaskX2;
        int stylusMaskY2;

        // Search offset from stylus tip.
        int referenceOffsetFromStylus;
        int referenceHalfHeight;
        int referenceWidth;
    };

private:
    shared_ptr<Log> m_log;
    bool m_debug;

public:
    ///
    /// Constructor
    ///
    ImagePreprocessing();

    ///
    /// Extract a given channel from an RGB image.
    /// In the case of a monochromatic image, the returned image will be itself.
    ///
    /// \tparam [in] srcImage			Shared pointer to input image
    /// \param  [in] selectedChannel	The channel to extract from image
    ///
    /// \return	Shared pointer to the monochromatic image selected by the given channel
    ///
    template <typename T> shared_ptr<Image<T>> getChannel(shared_ptr<Image<T>> srcImage, uint8_t selectedChannel);

    ///
    /// Normalizes between 0 and 1
    ///
    /// \param [in, out] srcDstImage    Image to be normalized.
    ///
    void normalize(shared_ptr<Image<float>> srcDstImage);

    ///
    /// Binarize an image using the given threshold
    ///
    /// \param [in, out] srcDstImage    Image to be binarized
    /// \param [in]      threshold      Binarization threshold
    ///
    void binarize(shared_ptr<Image<float>> srcDstImage, float threshold);

    ///
    /// Binarize an image using the given threshold
    ///
    /// \param [in, out] srcDstImage        Image to be binarized
    /// \param [in]      startingThreshold  Starting binarization threshold
    ///
    /// \return True on success, false otherwise.
    ///
    bool adaptiveBinarize(shared_ptr<Image<float>> srcDstImage, float startingThreshold);

    ///
    /// Implement the low-pass filter on 1-channel data
    ///
    /// \tparam [in] srcImage	Source image to apply the filter
    /// \param  [in] radius
    ///
    /// \return	Shared pointer to the filterd image
    ///
    template <typename T> shared_ptr<Image<T>> lowPassFilter1(shared_ptr<Image<T>> srcImage, uint8_t radius);

    ///
    /// Implement the Sobel filter on 1-channel data
    ///
    /// \tparam [in] srcImage	Source image to apply the filter
    ///
    /// \return	Shared pointer to the filterd image
    ///
    template <typename T> shared_ptr<Image<T>> sobelFilter(shared_ptr<Image<T>> srcImage);

    ///
    /// Implement the Gaussian filter on 1-channel data
    ///
    /// \tparam [in] srcImage	Source image to apply the filter
    /// \tparam [in] sigma1     Standard deviation of the first gaussian filter
    /// \tparam [in] sigma2     Standard deviation of the second gaussian filter
    ///
    /// \return Shared pointer to the filterd image
    ///
    template <typename T>
    shared_ptr<Image<T>> differenceOfGaussian(shared_ptr<Image<T>> srcImage, float sigma1, float sigma2);

    template <typename T>
    shared_ptr<Image<T>> filter(shared_ptr<Image<T>> srcImage, float* kernel, int nx, int ny, int roiX0 = -1,
                                int roiY0 = -1, int roiWidth = -1, int roiHeight = -1);

    ///
    /// Implement the Median Blur filter on 1-channel data
    ///
    /// \tparam [in] srcImage   Source image to apply the filter
    /// \tparam [in] radius
    ///
    /// \return Shared pointer to the filterd image
    ///
    template <typename T> shared_ptr<Image<T>> medianBlurFilter(shared_ptr<Image<T>> srcImage, int radius);

    ///
    /// Thresholding operation used to detect a profile on the image
    ///
    /// \tparam [in] lowPassImage	Source image filtered by low-pass filter
    /// \tparam [in] sobelImage		Source image filtered by Sobel filter
    /// \tparam [in] threshold		Threshold for detecting the profile
    /// \param  [in] searchWidth	Search width for determining the raw profile
    /// \param  [in] searchOffset	Offset for adjust the search interval
    /// \param  [in] leftBoundary	Boundary used to optimize the search
    ///
    /// \return	Shared pointer to the detected profile on the image
    ///
    template <typename T> shared_ptr<Profile<int>> threshold(const ThresholdArgs<T>& args);

    template <typename T> shared_ptr<Profile<int>> thresholdTest(shared_ptr<Image<T>> inputImage);

    ///
    /// This function cleans up the profile after the thresholding by removing small segments.
    ///
    /// \param [in] srcPath				Source image profile
    /// \param [in] distanceThreshold	Float value showing distance from the threshold
    /// \param [in] windowSize			Windows size showing the interval where search the elements
    /// \param [in] counterThreshold	Number of elements used as threshold
    ///
    /// \return	Shared pointer to the cleaned profile
    ///
    shared_ptr<Profile<int>> removeIsolatedPoints(shared_ptr<Profile<int>> srcPath, float distanceThreshold,
                                                  int windowSize, int counterThreshold);

    shared_ptr<Profile<int>> filterProfileWithInterpolation(shared_ptr<Profile<int>> profile, int W);
    shared_ptr<Profile<int>> filterProfile(shared_ptr<Profile<int>> profile, int W, const int& maxHeight);

private:
    ///
    /// Filter an image by row
    ///
    /// \tparam [in] srcImage	Source image to apply the filter
    /// \param	[in] kernel		Kernel matrix used by the filter
    ///
    /// \return 	Shared pointer to the filterd image
    ///
    template <typename T> shared_ptr<Image<T>> filterByRow(shared_ptr<Image<T>> srcImage, float kernel[3]);

    template <typename T> shared_ptr<Image<T>> filterByRowR(shared_ptr<Image<T>> srcImage, uint8_t radius);

    ///
    /// Filter an image by column
    ///
    /// \tparam [in] srcImage	Source image to apply the filter
    /// \param  [in] kernel		Kernel matrix used by the filter
    ///
    /// \return	Shared pointer to the filterd image
    ///
    template <typename T> shared_ptr<Image<T>> filterByColumn(shared_ptr<Image<T>> srcImage, float kernel[3]);

    template <typename T> shared_ptr<Image<T>> filterByColumnR(shared_ptr<Image<T>> srcImage, uint8_t radius);

    // FIXME: gaussianFilter
    template <typename T> shared_ptr<Image<T>> gaussianFilter(shared_ptr<Image<T>> srcImage, float sigma, int radius);

    template <typename T>
    shared_ptr<Image<T>> filterGaussianByRow(shared_ptr<Image<T>> srcImage, const float* kernel, int radius);

    template <typename T>
    shared_ptr<Image<T>> filterGaussianByColumn(shared_ptr<Image<T>> srcImage, const float* kernel, int radius);
};
} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __IMAGEPREPROCESSING_H__
