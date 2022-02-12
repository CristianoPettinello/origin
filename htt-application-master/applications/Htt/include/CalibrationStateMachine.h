#ifndef __CALIBRATION_STATE_MACHINE_H__
#define __CALIBRATION_STATE_MACHINE_H__

#include <QAbstractTransition>
#include <QDebug>
#include <QEvent>
#include <QState>

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

struct CalibationResumeEvent : public QEvent
{
    CalibationResumeEvent(const QString& id) : QEvent(QEvent::Type(QEvent::User + 2)), m_id(id)
    {
    }

    QString m_id;
};

class CalibationResumeTransition : public QAbstractTransition
{
    Q_OBJECT

public:
    CalibationResumeTransition(const QString& id) : m_id(id)
    {
    }

protected:
    bool eventTest(QEvent* e) override
    {
        qDebug() << e->type() << " " << QEvent::Type(QEvent::User + 2);
        if (e->type() == QEvent::Type(QEvent::User + 2)) // UserEvents: CalibationResumeEvent
        {
            CalibationResumeEvent* event = static_cast<CalibationResumeEvent*>(e);
            qDebug() << m_id << " " << event->m_id;
            return (m_id == event->m_id);
        } else
            return false;
    }

    void onTransition(QEvent*) override
    {
    }

private:
    QString m_id;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __CALIBRATION_STATE_MACHINE_H__