#ifndef __LED_DRIVER_H__
#define __LED_DRIVER_H__


#include <QObject>


namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {


class LedDriver : public QObject
{

    Q_OBJECT

public:

    explicit LedDriver(QObject* parent = nullptr);
    ~LedDriver();


    virtual double power() = 0;
    virtual bool setPower(double power) = 0;

    // Return info about the driver
    virtual QStringList driverInfo() = 0;

private:

    double m_power;

};

}}}}

#endif // __LED_DRIVER_H__
