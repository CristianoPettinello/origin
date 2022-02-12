#include "Drivers/DummyLed.h"

using namespace Nidek::Solution::HTT::UserApplication;

DummyLed::DummyLed(QObject* parent /* = nullptr */) :
    LedDriver(parent),
    m_power(0.0)
{

}

DummyLed::~DummyLed()
{

}

double DummyLed::power()
{
    return m_power;
}

bool DummyLed::setPower(double power)
{
    // check if power is in the [0,100] range.
    if(power >= 0.0 && power <= 100.0)
    {
        m_power = power;
        return true;
    }
    else
        return false;
}

QStringList DummyLed::driverInfo()
{
    QStringList info;
    info << "Dummy Led v1.0";
    return info;
}
