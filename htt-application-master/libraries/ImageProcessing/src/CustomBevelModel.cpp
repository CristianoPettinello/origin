#include "CustomBevelModel.h"
#include <math.h>

using namespace Nidek::Libraries::HTT;

static const string tag = "[TBMO] ";

#define SHOULDER_LENGTH 80
#define CUSTOMBEVEL_POINTS 7
#define EXTRA_POINTS 2

CustomBevelModel::CustomBevelModel(shared_ptr<ScheimpflugTransform> st) : Model(st)
{
    m_A = m_st->pixelToMm(SHOULDER_LENGTH);
    m_B = 0.0;
    m_C = m_A;
    m_D = 0.0;
    m_E = 0.0;
    m_F = 0.0;
    m_G = 0.0;
    m_M = 0.0;
    m_S = 0.0;
    m_alpha = 0.0;

    // Back angle
    const double delta1 = 0.785398; // 45 / 180.0 * 3.141592
    // Front angle
    const double delta2 = 1.047197; // 60 / 180.0 * 3.141592

    m_tanDelta1 = tan(delta1);
    m_tanDelta2 = tan(delta2);

    m_objX.resize(CUSTOMBEVEL_POINTS);
    m_objY.resize(CUSTOMBEVEL_POINTS);

    m_imgX.resize(CUSTOMBEVEL_POINTS);
    m_imgY.resize(CUSTOMBEVEL_POINTS);

    m_distanceImgX.resize(EXTRA_POINTS);
    m_distanceImgY.resize(EXTRA_POINTS);
}

bool CustomBevelModel::setFreeParams(const float* v, const double* /* offset */)
{
    double* objXPtr = m_objX.data();
    double* objYPtr = m_objY.data();

    // Set the model's free parameters
    // params order: x1, y1, x5, y5, alpha, M, S, E
    objXPtr[1] = (double)v[0]; // x1
    objYPtr[1] = (double)v[1]; // y1
    objXPtr[5] = (double)v[2]; // x5
    objYPtr[5] = (double)v[3]; // y5
    m_alpha = (double)v[4];    // alpha
    m_M = (double)v[5];        // M
    m_S = (double)v[6];        // S
    m_E = (double)v[7];        // E

    // Compute other parameters
    computeOtherParams();

    return computeConstrainedBevelProfile();
}

shared_ptr<Model::Measures> CustomBevelModel::getMeasures()
{
    shared_ptr<Model::Measures> measures(new Model::Measures());
    measures->m.insert(make_pair("A", (float)m_A));
    measures->m.insert(make_pair("C", (float)m_C));
    measures->m.insert(make_pair("D", (float)m_D));
    measures->m.insert(make_pair("E", (float)m_E));
    measures->m.insert(make_pair("F", (float)m_F));
    measures->m.insert(make_pair("G", (float)m_G));

    return measures;
}

ModelType CustomBevelModel::modelType()
{
    return ModelType::CustomBevelModelType;
}

bool CustomBevelModel::finalizeModel(const float* v, const int* /* lastImgPointProfile */, shared_ptr<Profile<int>> profile)
{
    bool validSolution = setFreeParams(v);
    if (validSolution)
    {
        m_distanceImgX.clear();
        m_distanceImgY.clear();

        double srcObjPoint[] = {m_xm, m_ym, 0.0};
        double dstImgPoint[] = {0.0, 0.0, 0.0};
        m_st->objectToImage(srcObjPoint, dstImgPoint);

        m_distanceImgX.push_back((float)dstImgPoint[0]);
        m_distanceImgY.push_back((float)dstImgPoint[1]);

        m_G = m_E + m_B;
        m_F = m_B + m_S;
    }

    if (m_debug)
    {
        stringstream s;
        s << tag << "Intersection points: " << getIntersectionCounter();
        log->debug(s.str());
    }

    return validSolution;
}

void CustomBevelModel::printMeasures()
{
    stringstream s;
    s << tag << "Bevel Measures [mm]:";
    log->info(s.str());
    s.str("");
    s << tag << "   D: " << m_D << " E: " << m_E << " F: " << m_F << " G: " << m_G << " M: " << m_M;
    log->info(s.str());
}

void CustomBevelModel::computeOtherParams()
{
    double* objXPtr = m_objX.data();
    double* objYPtr = m_objY.data();

    double sin_alpha = sin(m_alpha);
    double cos_alpha = cos(m_alpha);

    double m_AsinAlpha = m_A * sin_alpha;
    double m_AcosAlpha = m_A * cos_alpha;

    objXPtr[0] = objXPtr[1] - m_AsinAlpha;
    objYPtr[0] = objYPtr[1] + m_AcosAlpha;

    objXPtr[6] = objXPtr[5] + m_AsinAlpha; // this is because m_A = m_C
    objYPtr[6] = objYPtr[5] - m_AcosAlpha; // this is because m_A = m_C

    objXPtr[2] = objXPtr[1] + m_S * cos_alpha;
    objYPtr[2] = objYPtr[1] + m_S * sin_alpha;

    double d_2_5 =
        sqrt((m_objX[2] - m_objX[5]) * (m_objX[2] - m_objX[5]) + (m_objY[2] - m_objY[5]) * (m_objY[2] - m_objY[5]));
    double beta = acos(m_M / d_2_5);
    double deltaH = d_2_5 * sin(beta);

    // Bevel height (front) [h2 = m_D]
    m_D = (m_M - m_E - deltaH) / (1 + m_tanDelta2);
    // b1 = h1 = h2+deltaH = m_B
    m_B = m_D + deltaH;

    // Projection of point 3 on line M
    m_xm = objXPtr[2] + m_B * sin_alpha;
    m_ym = objYPtr[2] - m_B * cos_alpha;

    objXPtr[3] = m_xm + m_B * cos_alpha;
    objYPtr[3] = m_ym + m_B * sin_alpha;

    objXPtr[4] = objXPtr[3] + m_E * sin_alpha;
    objYPtr[4] = objYPtr[3] - m_E * cos_alpha;

    // Compute the points in the image plane.
    computeImgPoints(m_objX, m_objY);
}
