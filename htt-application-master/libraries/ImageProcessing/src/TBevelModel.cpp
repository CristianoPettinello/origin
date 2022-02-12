#include "TBevelModel.h"
#include "Settings.h"
#include <math.h>

using namespace Nidek::Libraries::HTT;

static const string tag = "[TBMO] ";

#define SHOULDER_LENGTH 80
#define TBEVEL_POINTS 6
#define EXTRA_POINTS 2

TBevelModel::TBevelModel(shared_ptr<ScheimpflugTransform> st) : Model(st)
{
    m_A = m_st->pixelToMm(SHOULDER_LENGTH);
    m_B = 0.0;
    m_C = m_A;
    m_D = 0.0;
    m_M = 0.0;
    m_alpha = 0.0;

    m_objX.resize(TBEVEL_POINTS);
    m_objY.resize(TBEVEL_POINTS);

    m_imgX.resize(TBEVEL_POINTS);
    m_imgY.resize(TBEVEL_POINTS);

    m_distanceImgX.resize(EXTRA_POINTS);
    m_distanceImgY.resize(EXTRA_POINTS);
}

bool TBevelModel::setFreeParams(const float* v, const double* offset)
{
    double* objXPtr = m_objX.data();
    double* objYPtr = m_objY.data();

    // Set the model's free parameters
    // params order: x1, y1, alpha, M, B
    objXPtr[1] = (double)v[0];
    objYPtr[1] = (double)v[1];
    m_alpha = (double)v[2];
    m_M = (double)v[3];
    m_B = (double)v[4];

    // Compute other parameters
    computeOtherParams(offset);

    return computeConstrainedTBevelProfile();
}

shared_ptr<Model::Measures> TBevelModel::getMeasures()
{
    shared_ptr<Model::Measures> measures(new Model::Measures());
    measures->m.insert(make_pair("A", (float)m_A));
    measures->m.insert(make_pair("B", (float)m_B));
    measures->m.insert(make_pair("C", (float)m_C));
    measures->m.insert(make_pair("D", (float)m_D));
    measures->m.insert(make_pair("M", (float)m_M));

    return measures;
}

ModelType TBevelModel::modelType()
{
    return ModelType::TBevelModelType;
}

bool TBevelModel::finalizeModel(const float* v, const int* lastImgPointProfile, shared_ptr<Profile<int>> profile)
{
    // FIXME
    shared_ptr<Settings> settings = Settings::getInstance();
    double offset[2] = {settings->getSettings()["search_space"][1]["shrinkOffsetM"].asDouble(),
                        settings->getSettings()["search_space"][1]["shrinkOffsetB"].asDouble()};

    bool validSolution = setFreeParams(v, offset);

    if (validSolution)
    {
        m_distanceImgX.clear();
        m_distanceImgY.clear();

        double srcObjPoint[] = {m_xm, m_ym, 0.0};
        double dstImgPoint[] = {0.0, 0.0, 0.0};
        m_st->objectToImage(srcObjPoint, dstImgPoint);

        m_distanceImgX.push_back((float)dstImgPoint[0]);
        m_distanceImgY.push_back((float)dstImgPoint[1]);

        double srcImgPoint[] = {(double)lastImgPointProfile[0], (double)lastImgPointProfile[1], 0.0};
        double dstObjPoint[] = {0.0, 0.0, 0.0};
        m_st->imageToObject(srcImgPoint, dstObjPoint);

        double X4_5 = m_objX[4] - m_objX[5];
        double Y4_5 = m_objY[4] - m_objY[5];
        double X4Y5 = m_objX[4] * m_objY[5];
        double X5Y4 = m_objX[5] * m_objY[4];
        double m1 = Y4_5 / X4_5;
        double m2 = -1.0 / m1;
        double q1 = ((X4Y5 - X5Y4) / X4_5);
        double q2 = dstObjPoint[1] - (m2 * dstObjPoint[0]);

        double Xb, Yb;
        if (fabs(X4_5) > 1e-6)
        {
            // log->debug(tag + "solution y=mx+q");
            Xb = (q2 - q1) / (m1 - m2);
            Yb = (m1 * Xb) + q1;
        } else
        {
            // log->debug(tag + "solution x=my+q");
            Yb = (dstObjPoint[0] + (dstObjPoint[1] * m1) - (q1 * m2)) / (m1 - m2);
            Xb = m2 * (q1 - Yb);
        }

        // d = sqrt( (Xb - xm)^2 + (Yb - ym)^2 )
        float Xb_xm = (float)(Xb - m_xm);
        float Yb_ym = (float)(Yb - m_ym);
        m_D = sqrt((Xb_xm * Xb_xm) + (Yb_ym * Yb_ym));

        // stream.str("");
        // stream << "m:(" << m_xm << "," << m_ym << ") b:(" << Xb << "," << Yb << ") d:" << m_D << "mm";
        // log->debug(tag + stream.str());

        double srcObjPointB[] = {Xb, Yb, 0.0};
        double dstImgPointB[] = {0.0, 0.0, 0.0};
        m_st->objectToImage(srcObjPointB, dstImgPointB);

        m_distanceImgX.push_back((float)dstImgPointB[0]);
        m_distanceImgY.push_back((float)dstImgPointB[1]);
    }

    if (m_debug)
    {
        stringstream s;
        s << tag << "Intersection points: " << getIntersectionCounter();
        log->debug(s.str());
    }

    return validSolution;
}

void TBevelModel::printMeasures()
{
    stringstream s;
    s << tag << "Bevel Measures [mm]:";
    log->info(s.str());
    s.str("");
    s << tag << "   B: " << m_B << " M: " << m_M << " D: " << m_D;
    log->info(s.str());

    cout << "B: " << m_st->mmToPixel(m_B) << " pixel" << endl;
}

void TBevelModel::computeOtherParams(const double* offset)
{
    double* objXPtr = m_objX.data();
    double* objYPtr = m_objY.data();

    double sin_alpha = sin(m_alpha);
    double cos_alpha = cos(m_alpha);

    // Shrink the bevel using different offset values
    if (offset != nullptr)
    {
        objXPtr[1] += offset[0] * sin_alpha;
        objYPtr[1] -= offset[0] * cos_alpha;
        m_M -= 2 * offset[0];
        m_B -= offset[1];
    }

    double m_AsinAlpha = m_A * sin_alpha;
    double m_AcosAlpha = m_A * cos_alpha;
    double m_BsinAlpha = m_B * sin_alpha;
    double m_BcosAlpha = m_B * cos_alpha;
    double m_MsinAlpha = m_M * sin_alpha;
    double m_McosAlpha = m_M * cos_alpha;

    objXPtr[0] = objXPtr[1] - m_AsinAlpha;
    objYPtr[0] = objYPtr[1] + m_AcosAlpha;

    objXPtr[2] = objXPtr[1] + m_BcosAlpha;
    objYPtr[2] = objYPtr[1] + m_BsinAlpha;

    objXPtr[3] = objXPtr[2] + m_MsinAlpha;
    objYPtr[3] = objYPtr[2] - m_McosAlpha;

    objXPtr[4] = objXPtr[1] + m_MsinAlpha;
    objYPtr[4] = objYPtr[1] - m_McosAlpha;

    objXPtr[5] = objXPtr[4] + m_AsinAlpha; // this is because m_A = m_C
    objYPtr[5] = objYPtr[4] - m_AcosAlpha; // this is because m_A = m_C

    // Get the middle point
    m_xm = (m_objX[1] + m_objX[4]) * 0.5;
    m_ym = (m_objY[1] + m_objY[4]) * 0.5;

    // Compute the points in the image plane.
    computeImgPoints(m_objX, m_objY);
}
