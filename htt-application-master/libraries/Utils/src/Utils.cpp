//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include <stdlib.h> // abort, NULL
#ifdef UNIX
#include <png.h>
#endif
#include <sstream>
#include <cstring>

// #include "ImagePreprocessing.h"

#include "Utils.h"

#include <iostream> /* std::cerr */

// using namespace Nidek::Libraries::Utils;

void __M_Assert(const char* expr_str, bool expr, const char* file, int line, const char* msg)
{
    if (!expr)
    {
        cerr << "Assert failed:\t" << msg << "\n"
             << "Expected:\t" << expr_str << "\n"
             << "Source:\t\t" << file << ", line " << line << "\n";
        abort();
    }
}

namespace Nidek {
namespace Libraries {
namespace Utils {

shared_ptr<Image<uint8_t>> loadImage(const char* filePath)
{
#ifdef UNIX
    FILE* fp = fopen(filePath, "rb");
    if (!fp)
        return nullptr; // abort();

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
        return nullptr; // abort();

    png_infop info = png_create_info_struct(png);
    if (!info)
        return nullptr; // abort();

    if (setjmp(png_jmpbuf(png)))
        return nullptr; // abort();

    png_init_io(png, fp);

    png_read_info(png, info);

    uint16_t width = png_get_image_width(png, info);
    uint16_t height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);
    png_bytep* row_pointers = NULL;

    // Read any color_type into 8bit depth, RGBA format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if (bit_depth == 16)
        png_set_strip_16(png);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

        // Disable auto fill alpha channel
#if 0
		// These color_type don't have an alpha channel then fill it with 0xff.
		if (color_type == PNG_COLOR_TYPE_RGB ||
			color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
#endif

    // if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    //    png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    if (row_pointers)
        return nullptr; // abort();

    shared_ptr<Image<uint8_t>> img(new Image<uint8_t>(width, height, png_get_channels(png, info)));
    uint8_t* pImage = img->getData();

    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; ++y)
    {
        // row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
        row_pointers[y] = (png_byte*)&pImage[y * img->getStride()];
    }

    png_read_image(png, row_pointers);

    fclose(fp);

    png_destroy_read_struct(&png, &info, NULL);

    free(row_pointers);

    return img;
#else
    return nullptr;
#endif
}

template <>
UTILS_LIBRARYSHARED_EXPORT bool saveImage(const string& filePath, const string& fileName,
                                          shared_ptr<Image<uint8_t>> srcImage)
{
#ifdef UNIX
    // Make parent directories as needed
    stringstream s;
    s << "mkdir -p " << filePath;
    system(s.str().c_str());

    s.str("");
    s << filePath << "/" << fileName;
    FILE* fp = fopen(s.str().c_str(), "wb");
    if (!fp)
        abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
        abort();

    png_infop info = png_create_info_struct(png);
    if (!info)
        abort();

    if (setjmp(png_jmpbuf(png)))
        abort();

    png_init_io(png, fp);

    uint16_t width = srcImage->getWidth();
    uint16_t height = srcImage->getHeight();
    uint16_t stride = srcImage->getStride();
    uint8_t* pImage = srcImage->getData();

    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; ++y)
    {
        row_pointers[y] = (png_byte*)&pImage[y * stride];
    }

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(png, info, width, height, 8,
                 (srcImage->getNChannels() == 3) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
    // Use png_set_filler().
    // png_set_filler(png, 0, PNG_FILLER_AFTER);

    if (!row_pointers)
        return false; // abort();

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    free(row_pointers);

    fclose(fp);

    png_destroy_write_struct(&png, &info);
#endif
    return true;
}

template <>
UTILS_LIBRARYSHARED_EXPORT bool saveImage(const string& filePath, const string& fileName,
                                          shared_ptr<Image<float>> srcImage)
{
#ifdef UNIX
    // Make parent directories as needed
    stringstream s;
    s << "mkdir -p " << filePath;
    system(s.str().c_str());

    s.str("");
    s << filePath << "/" << fileName;
    FILE* fp = fopen(s.str().c_str(), "wb");
    if (!fp)
        abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
        abort();

    png_infop info = png_create_info_struct(png);
    if (!info)
        abort();

    if (setjmp(png_jmpbuf(png)))
        abort();

    png_init_io(png, fp);

    uint16_t width = srcImage->getWidth();
    uint16_t height = srcImage->getHeight();

    shared_ptr<Image<uint8_t>> newImage = imageToUint8(srcImage);
    uint8_t* pImage = newImage->getData();
    uint16_t stride = newImage->getStride();

    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; ++y)
    {
        row_pointers[y] = (png_byte*)&pImage[y * stride];
    }

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(png, info, width, height, 8,
                 (srcImage->getNChannels() == 3) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
    // Use png_set_filler().
    // png_set_filler(png, 0, PNG_FILLER_AFTER);

    if (!row_pointers)
        return false; // abort();

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    free(row_pointers);

    fclose(fp);

    png_destroy_write_struct(&png, &info);
#endif
    return true;
}

template <>
UTILS_LIBRARYSHARED_EXPORT void saveRawImage(const string& filePath, const string& fileName,
                                             shared_ptr<Image<uint8_t>> srcImage)
{
    stringstream s;
    s << filePath << "/" << fileName;

    FILE* fp = fopen(s.str().c_str(), "wb");
    if (!fp)
        abort();

    // cout << "stride: " << srcImage->getStride() << " h: " << srcImage->getHeight()
    //      << " nch: " << (double)srcImage->getNChannels() << endl;

    fwrite(srcImage->getData(), sizeof(uint8_t),
           srcImage->getStride() * srcImage->getHeight() * srcImage->getNChannels(), fp);

    fclose(fp);
}

shared_ptr<Image<uint8_t>> imageToUint8(shared_ptr<Image<float>> srcImage)
{
    float* pSrcImage = srcImage->getData();
    uint16_t width = srcImage->getWidth();
    uint16_t height = srcImage->getHeight();
    shared_ptr<Image<uint8_t>> dstImage(new Image<uint8_t>(width, height, srcImage->getNChannels()));
    uint8_t* pDstImage = dstImage->getData();

    uint16_t strideSrcImage = srcImage->getStride() / sizeof(float);
    uint16_t strideDstImage = dstImage->getStride();

    for (uint16_t y = 0; y < height; ++y)
    {
        uint32_t srcRowStartAddress = y * strideSrcImage;
        uint32_t dstRowStartAddress = y * strideDstImage;
        for (uint32_t x = 0; x < width; ++x)
            pDstImage[x + dstRowStartAddress] = (uint8_t)(pSrcImage[x + srcRowStartAddress] * 255.0f);
    }

    return dstImage;
}

shared_ptr<Image<float>> imageToFloat(shared_ptr<Image<uint8_t>> srcImage, bool scaling)
{
    const uint16_t width = srcImage->getWidth();
    const uint16_t height = srcImage->getHeight();
    uint8_t* pSrcImage = srcImage->getData();
    shared_ptr<Image<float>> dstImage(new Image<float>(width, height, srcImage->getNChannels()));
    float* pDstImage = dstImage->getData();

    const uint16_t strideSrcImage = srcImage->getStride();
    const uint16_t strideDstImage = dstImage->getStride() / sizeof(float);

    if (scaling)
    {
        const float iScale = 1.0f / 255.f;
        for (uint16_t y = 0; y < height; ++y)
        {
            // uint32_t srcRowStartAddress = y * strideSrcImage;
            // uint32_t dstRowStartAddress = y * strideDstImage;
            // for (uint32_t x = 0; x < strideSrcImage; ++x)
            //    pDstImage[x + dstRowStartAddress] = (float)pSrcImage[x + srcRowStartAddress] * iScale;

            uint8_t* pSrcImageRow = &pSrcImage[y * strideSrcImage];
            float* pDstImageRow = &pDstImage[y * strideDstImage];
            for (uint32_t x = 0; x < width; ++x)
                pDstImageRow[x] = (float)pSrcImageRow[x] * iScale;
        }
    } else
    {
        for (uint16_t y = 0; y < height; ++y)
        {
            // uint32_t srcRowStartAddress = y * strideSrcImage;
            // uint32_t dstRowStartAddress = y * strideDstImage;
            // for (uint32_t x = 0; x < strideSrcImage; ++x)
            //    pDstImage[x + dstRowStartAddress] = (float)pSrcImage[x + srcRowStartAddress];

            uint8_t* pSrcImageRow = &pSrcImage[y * strideSrcImage];
            float* pDstImageRow = &pDstImage[y * strideDstImage];
            for (uint32_t x = 0; x < width; ++x)
                pDstImageRow[x] = (float)pSrcImageRow[x];
        }
    }

    return dstImage;
}

vector<string> split(const string& s, char delimiter)
{
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

void traceCalibrationGrid(shared_ptr<Image<float>> dstImage, vector<pair<pair<int, int>, pair<int, int>>> lines)
{
    uint16_t width = dstImage->getWidth();
    uint16_t height = dstImage->getWidth();
    uint16_t stride = dstImage->getStride() / sizeof(float);

    vector<int> xG;
    vector<int> yG;

    if (!lines.empty())
    {
        for (auto it = lines.begin(); it != lines.end(); ++it)
        {
            const int xi = it->first.first;
            const int yi = it->first.second;

            const int xj = it->second.first;
            const int yj = it->second.second;

            const int L = xj - xi;
            const int K = yj - yi;
            // cout << "(" << xi << "," << yi << ") (" << xj << "," << yj << ") L: " << L << " K: " << K << endl;

            if (abs(L) >= abs(K))
            {
                // Excursion on the X axis is greater than the one on the Y axis. The inner
                // loop connects (xi, yi) to (xj, yj) with a segment iterating on x.
                const float m = (float)K / L;
                float q = (float)yi - m * xi;

                for (int x = xi; x <= xj; ++x)
                {
                    int y = (int)(m * x + q);

                    if (x >= 0 && x < width && y >= 0 && y < height)
                    {
                        // Store the current segment point in xG and yG
                        xG.push_back(x);
                        yG.push_back(y);
                    }
                }
            } else
            {
                // Excursion on the Y axis is greater than the one on the X axis. The inner
                // loop connects (xi, yi) to (xj, yj) with a segment iterating on y.
                const float m = (float)L / K;
                float q = (float)xi - m * yi;

                for (int y = yi; y >= yj; --y)
                {
                    int x = (int)(m * y + q);

                    if (x >= 0 && x < width && y >= 0 && y < height)
                    {
                        // Store the current segment point in xG and yG
                        xG.push_back(x);
                        yG.push_back(y);
                    }
                }
            }
        }
    }

    float* pImg = dstImage->getData();
    for (int i = 0; i < (int)xG.size(); ++i)
    {
        int index = xG[i] + yG[i] * stride;
        pImg[index] = 1.0;
    }
}

void parseFilename(const string& fileName, double& R, double& H)
{
    vector<string> tokens = split(fileName, '_');
    for (string token : tokens)
    {
        char* pToken = (char*)token.c_str();
        if ((pToken[0] == 'R' && pToken[1] == '=') ||
            (pToken[0] == 'd' && pToken[1] == 'e' && pToken[2] == 'l' && pToken[3] == 't' && pToken[4] == 'a' &&
             pToken[5] == 'N' && pToken[6] == '='))
        {
            string val = token.substr(token.find('=') + 1);
            R = stod(val);
        } else if (pToken[0] == 'H' && pToken[1] == '=')
        {
            string val = token.substr(token.find('=') + 1);
            H = stod(val);
        }
    }
}

void horizontalFlip(shared_ptr<Image<uint8_t>> image)
{
    M_Assert(image->getNChannels() == 1, "The number of channels of the input image is greather than one");

    const int height = image->getHeight();
    const int width = image->getWidth();
    const int stride = image->getStride();
    uint8_t* pImage = image->getData();
    uint8_t* tmpArray = new uint8_t[width];

    for (int y = 0; y < height; ++y)
    {
        uint8_t* rowPtr = &pImage[y * stride];
        for (int x = 0; x < width; ++x)
        {
            tmpArray[x] = rowPtr[width - x];
        }
        memcpy(rowPtr, tmpArray, stride);
    }

    if (tmpArray)
        delete[] tmpArray;
}

} // namespace Utils
} // namespace Libraries
} // namespace Nidek
