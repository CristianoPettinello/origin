#ifndef __HW_DATA_H__
#define __HW_DATA_H__

#include "ModelType.h"
#include <QImage>
#include <QSharedPointer>

using namespace std;
using namespace Nidek::Libraries::HTT;

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

struct HwData
{
    HwData();

    // Image grabbed from the camera.
    QSharedPointer<QImage> image;

    // Frame counter
    int frameCounter;

    // Camera frame rate
    double frameRate;

    // Exposure time
    double exposureTime;

    // Led power when the frame was captured.
    double ledPower;

    double deltaN;
    double H;
    ModelType modelType;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __HW_DATA_H__
