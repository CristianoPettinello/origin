#include "HwData.h"

using namespace Nidek::Solution::HTT::UserApplication;

HwData::HwData()
    : image(nullptr),
      frameCounter(0),
      frameRate(0.0),
      exposureTime(0.0),
      ledPower(0.0),
      deltaN(0.0),
      H(0.0),
      modelType(ModelType::Undefined)
{
}
