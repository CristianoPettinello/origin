#ifndef __MEASURES_STATE_MACHINE_H__
#define __MEASURES_STATE_MACHINE_H__

#include <QAbstractTransition>
#include <QDebug>
#include <QEvent>
#include <QPointF>
#include <QState>

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

struct MeasuresEvent : public QEvent
{
    MeasuresEvent(const QString& id, const QPointF& p) : QEvent(QEvent::Type(QEvent::User + 1)), value(id), point(p)
    {
    }

    QString value;
    QPointF point;
};

class MeasuresTransition : public QAbstractTransition
{
    Q_OBJECT

public:
    MeasuresTransition(const QString& id) : m_id(id), m_point(QPointF())
    {
    }

protected:
    bool eventTest(QEvent* e) override
    {
        if (e->type() == QEvent::Type(QEvent::User + 1)) // UserEvents: MeasuresEvent
        {
            MeasuresEvent* me = static_cast<MeasuresEvent*>(e);
            return (m_id == me->value);
        } else
            return false;
    }

    void onTransition(QEvent*) override
    {
    }

private:
    QString m_id;
    QPointF m_point;
};

class MeasuresState : public QState
{
    Q_OBJECT

public:
    MeasuresState(QState* parent = 0) : QState(parent)
    {
    }

protected:
    virtual void onEntry(QEvent* e) override
    {

        MeasuresEvent* ce = (MeasuresEvent*)e;
        point = ce->point;
        QState::onEntry(e);
    }

public:
    QPointF point;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __MEASURES_STATE_MACHINE_H__
