#ifndef __DUMMY_LED_H__
#define __DUMMY_LED_H__


#include "LedDriver.h"


namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {


class DummyLed : public LedDriver
{

    Q_OBJECT

public:

    explicit DummyLed(QObject* parent = nullptr);
    ~DummyLed();

    virtual double power();
    virtual bool setPower(double power);


    // Return info about the driver
    virtual QStringList driverInfo();

private:

    double m_power;

};

}}}}

#endif // __DUMMY_LED_H__
