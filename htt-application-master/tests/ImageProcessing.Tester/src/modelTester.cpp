#include <iostream>
#include <memory>

#include "ScheimpflugTransform.h"
#include "Timer.h"
#include "MiniBevelModel.h"
#include "Image.h"
#include "Profile.h"

using namespace std;
using namespace Nidek::Libraries::HTT;


/* DEPRECATED */

void scheimpflugTransformationTest()
{
    cout << "------------- Scheimpflug Transformation Tester -------------- " << endl;

    // Define the R and H angles. These angles describe the orientation of the
    // HTT stylus point.
    double R = 0.0;
    double H = 0.0;

    Timer t;
    t.start();
    ScheimpflugTransform st(R, H);

    // Define a point in the object plane.
    // IMPORTANT: the third component of the vector must be always zero because
    // we are mapping points on planes.
    double pObj[3] = {1.0, 1.0, 0.0};
    double pImg[3] = {0.0, 0.0, 0.0};
    double pObjRec[3] = {0.0, 0.0, 0.0};


    // Apply the Scheimplfug Transform in order to find the point on the image plane.
    st.objectToImage(pObj, pImg);

    // Apply the inverse transformation
    st.imageToObject(pImg, pObjRec);

    // Print the results
    cout << "R: " << R << endl << "H: " << H << endl << endl;

    cout << "Object Point [mm]: " <<  endl;
    cout << " X: " << pObj[0] << endl;
    cout << " Y: " << pObj[1] << endl;
    cout << " Z: " << pObj[2] << endl << endl;

    cout << "Image Point [pixel]: " <<  endl;
    cout << " X: " << pImg[0] << endl;
    cout << " Y: " << pImg[1] << endl;
    cout << " Z: " << pImg[2] << endl << endl;

    cout << "Reconstructed Object Point [mm]: " <<  endl;
    cout << " X: " << pObjRec[0] << endl;
    cout << " Y: " << pObjRec[1] << endl;
    cout << " Z: " << pObjRec[2] << endl << endl;

    t.stop();

}

void miniBevelModelTest()
{
    cout << "------------- MiniBevelModel Tester -------------- " << endl;

    double R = 0.0;
    double H = 0.0;


    // Define the loss function
    shared_ptr<Image<float>> lossField(new Image<float>(400, 400, 1));
    shared_ptr<Profile<int>> trace(new Profile<int>());

    // Define a fake trace
    trace->reserve(400);
    for(int i = 0; i < 400; ++i)
    {
        trace->x[i] = 200;
        trace->y[i] = i;
    }

    float* pLossField = lossField->getData();
    for(int i = 0; i < 400*400; ++i)
        pLossField[i] = 1000;


    // Create the model and set the los field and trace
    MiniBevelModel model(R, H);
    model.setLossFieldAndProfile(lossField, trace, 1, false);


    // Set the free parameters in the model. This compute the model
    // points both in the object plane and in the image plane
    double v[] = {-0.2, 0.2, 0.0, 1.0};

    Timer t;
    t.start();
    double lossValue = 0.0;
    int REP = 200 * 1000;
    for(int rep = 0; rep < REP; ++rep)
    {
        model.setFreeParams(v);

        // From the image point we need to compute the associated profile
        model.computeBevelProfile();


        // After that we can compute the associated mse.
        lossValue += model.mse();
    }
    t.stop();

    cout << "Loss: " << lossValue / REP << endl;


    // Print the bevel points in the image plane
    model.printImgPoints();

    // Print the bevel points in the object plane
    model.printObjPoints();



}


int main(int argc, char* argv[])
{
    // Scheimpflug Trasnformation test
    //scheimpflugTransformationTest();

    // Mini Bevel Model Test
    miniBevelModelTest();


    return 0;
}
