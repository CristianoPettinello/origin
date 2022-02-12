#include "Htt.h"
#include "Image.h"
#include "Utils.h"
#include "Version.h"
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>

using namespace std;
using namespace Nidek::Libraries::HTT;

namespace py = pybind11;

string defaultSettingsFile = "settings.json";
string defaultLogProperties = "log4cpp.properties";

static std::vector<float> measures;

// FIXME - remove commented source code

// wrap C++ function with NumPy array IO
// FIXME: check the deltaN sign! Should the sign inversion be performed in the application or here?
py::array pyProcess(py::array_t<uint8_t, py::array::c_style | py::array::forcecast> array, string settingsFile,
                    double deltaN, double H, bool validation, int modelType)
{
    // check input dimensions
    if (array.ndim() != 3)
        throw std::runtime_error("Input should be 3-D NumPy array");

    // Load log4cpp proprieties
    shared_ptr<Log> log = Log::getInstance();
    log->setProprieties(defaultLogProperties);

    // Load settings from file and create json object
    shared_ptr<Settings> settings = Settings::getInstance();
    settings->load(settingsFile);

    shared_ptr<Htt> htt(new Htt());

    // Create an empty image with the input array dimensions
    uint16_t height = (uint16_t)array.shape()[0];
    uint16_t width = (uint16_t)array.shape()[1];
    uint8_t nChannels = (uint8_t)array.shape()[2];
    shared_ptr<Image<uint8_t>> img(new Image<uint8_t>(width, height, nChannels));

    // Copy the input array into Image
    uint8_t* imgPtr = img->getData();
    int imgStride = img->getStride();
    const uint8_t* arrayPtr = array.data();
    int arrayStride = width * nChannels * sizeof(uint8_t);
    for (int i = 0; i < height; i++)
    {
        std::memcpy(&imgPtr[i * imgStride], &arrayPtr[i * arrayStride], arrayStride);
    }

    // call pure C++ function

    measures.clear();

    Htt::processArgs args;
    args.R = deltaN;
    args.H = H;
    args.inputImage = img;
    args.repetitions = 1;
    // args.processType = HttProcessType::calibration;
    // args.imageLabel = filename.substr(0, filename.find_last_of('.'));
    args.modelType = (ModelType)(1 << modelType); // ModelType::MiniBevelModelType;

    if (validation)
        args.processType = HttProcessType::validation;
    shared_ptr<Htt::ProcessResults> result = htt->process(args);

    py::array returnArray;

    if (result->valid && result->bevelImage)
    {
        // Save measures into a static vector
        switch (args.modelType)
        {
        case ModelType::MiniBevelModelType:
        case ModelType::TBevelModelType:
        {
            measures.push_back(result->optimizationOutput->measures->m["A"]);
            measures.push_back(result->optimizationOutput->measures->m["C"]);
            measures.push_back(result->optimizationOutput->measures->m["B"]);
            measures.push_back(result->optimizationOutput->measures->m["M"]);
            measures.push_back(result->optimizationOutput->measures->m["D"]);
        }
        break;
        case ModelType::CustomBevelModelType:
        {
            measures.push_back(result->optimizationOutput->measures->m["A"]);
            measures.push_back(result->optimizationOutput->measures->m["C"]);
            measures.push_back(result->optimizationOutput->measures->m["D"]);
            measures.push_back(result->optimizationOutput->measures->m["E"]);
            measures.push_back(result->optimizationOutput->measures->m["F"]);
            measures.push_back(result->optimizationOutput->measures->m["G"]);
        }
        break;

        default:
            break;
        }

        shared_ptr<Image<uint8_t>> processedImage = result->bevelImage;

        int resultHeight = processedImage->getHeight();
        int resultWidth = processedImage->getWidth();
        int resultNChannels = processedImage->getNChannels();

        ssize_t ndim = 3;
        std::vector<ssize_t> shape = {resultHeight, resultWidth, resultNChannels};
        unsigned int offsetInBytes = resultNChannels * sizeof(uint8_t);
        std::vector<ssize_t> strides = {resultWidth * offsetInBytes, offsetInBytes, sizeof(uint8_t)};

        // return 3-D NumPy array
        returnArray = py::array(py::buffer_info(processedImage->getData(), /* data as contiguous array  */
                                                sizeof(uint8_t),           /* size of one scalar        */
                                                py::format_descriptor<uint8_t>::format(), /* data type */
                                                ndim,   /* number of dimensions      */
                                                shape,  /* shape of the matrix       */
                                                strides /* strides for each axis     */
                                                ));
    }

    htt->printReturnCode();
    return returnArray;
}

py::array_t<float> pyGetMeasures()
{
    // allocate py::array (to pass the result of the C++ function to Python)
    auto result = py::array_t<float>(measures.size());
    auto result_buffer = result.request();
    int* result_ptr = (int*)result_buffer.ptr;
    // copy std::vector -> py::array
    std::memcpy(result_ptr, measures.data(), measures.size() * sizeof(float));
    return result;
}

// FIXME: check the deltaN sign! Should the sign inversion be performed in the application or here?
bool pyCalibrate(py::array_t<uint8_t, py::array::c_style | py::array::forcecast> array, string settingsFile,
                 double deltaN, double H)
{
    // check input dimensions
    if (array.ndim() != 3)
        throw std::runtime_error("Input should be 3-D NumPy array");

    // Load log4cpp proprieties
    shared_ptr<Log> log = Log::getInstance();
    log->setProprieties(defaultLogProperties);

    // Load settings from file and create json object
    shared_ptr<Settings> settings = Settings::getInstance();
    settings->load(settingsFile);

    shared_ptr<Htt> htt(new Htt());

    // Create an empty image with the input array dimensions
    uint16_t height = (uint16_t)array.shape()[0];
    uint16_t width = (uint16_t)array.shape()[1];
    uint8_t nChannels = (uint8_t)array.shape()[2];
    shared_ptr<Image<uint8_t>> img(new Image<uint8_t>(width, height, nChannels));

    // Copy the input array into Image
    uint8_t* imgPtr = img->getData();
    int imgStride = img->getStride();
    const uint8_t* arrayPtr = array.data();
    int arrayStride = width * nChannels * sizeof(uint8_t);
    for (int i = 0; i < height; i++)
    {
        std::memcpy(&imgPtr[i * imgStride], &arrayPtr[i * arrayStride], arrayStride);
    }

    Htt::processArgs args;
    args.R = deltaN;
    args.H = H;
    args.inputImage = img;
    args.repetitions = 1;
    args.processType = HttProcessType::calibration;
    // args.imageLabel = filename.substr(0, filename.find_last_of('.'));
    args.modelType = ModelType::MiniBevelModelType;
    shared_ptr<Htt::ProcessResults> resProcess = htt->process(args);

    if (!resProcess->valid)
    {
        htt->printReturnCode();
        return false;
    }

    vector<float> imgX;
    vector<float> imgY;
    for (auto i = 0; i < resProcess->optimizationOutput->bevel->imgX.size(); ++i)
    {
        imgX.push_back((float)resProcess->optimizationOutput->bevel->imgX[i]);
        imgY.push_back((float)resProcess->optimizationOutput->bevel->imgY[i]);
    }

    Htt::OptimizeScheimpflugTransformResults res = htt->optimizeScheimpflugTransform(img, imgX, imgY, deltaN, H);
    if (!res.valid)
    {
        htt->printReturnCode();
        return false;
    }

    // Save the Scheimpflug Transforms' parameters into a new json file
    // string newSettingFile = settingsFile.substr(0, settingsFile.find(".json"));
    // newSettingFile = newSettingFile + "_calibrated.json";
    Settings::getInstance()->save(settingsFile);
    htt->printReturnCode();
    return true;
}

string pyGetVersion()
{
    return getVersion();
}

#ifdef UNIX
void pyProcessFolder(const string folderToProcess)
{
    Htt* htt = new Htt(defaultLogProperties, defaultSettingsFile);
    Log* log = Log::getInstance();
    // call pure C++ function
    htt->processFolder(folderToProcess, [htt, log](const string& filePath, const string& fileName) {
        // Get extension file
        string extension = fileName.substr(fileName.find_last_of('.') + 1);

        // Extract R and H from filename
        // i.e. Frame_15_LED=0600_R=-15_H=+15_05.bmp
        // R=-15 and H=15 (double values)
        double R = 0, H = 0;
        vector<string> tokens = Debug::split(fileName, '_');
        for (string token : tokens)
        {
            char* pToken = (char*)token.c_str();
            if (pToken[0] == 'R' && pToken[1] == '=')
            {
                string val = token.substr(token.find('=') + 1);
                R = stod(val);
            } else if (pToken[0] == 'H' && pToken[1] == '=')
            {
                string val = token.substr(token.find('=') + 1);
                H = stod(val);
            }
        }

        stringstream str;
        str << "PATH:" << filePath << " NAME: " << fileName << " EXT: " << extension << " R: " << R << " H: " << H;
        log->debug(str.str());

        string inputImage;
        bool continueToProcess = false;
        if (extension == "png")
        {
            inputImage = filePath + fileName;
            continueToProcess = true;
        } else
            log->debug(extension + "not supported");

        if (continueToProcess)
        {
            // Load png image from inputImage path
            log->info("Load image " + inputImage);
            shared_ptr<Image<uint8_t>> img = Debug::loadImage((char*)inputImage.c_str());
            // Process a single image
            // htt->process(img);
            if (Debug::saveImage((char*)"data.png", htt->process(img)) == false)
                log->error(__FILE__, __LINE__, "Error on saveImage");
        }
    });

    // FIXME: check how the delete of htt is handled.

    // if (htt)
    //     delete htt;
}
#endif

PYBIND11_MODULE(pyHtt, m)
{
    m.doc() = "pybind11 htt plugin"; // optional module docstring
    m.def("process", &pyProcess, "A function which process or validate a single image");
    m.def("getMeasures", &pyGetMeasures, "Returns optimization engine output mesures");
    m.def("calibrate", &pyCalibrate, "A function which calibrate a Scheimpflug Transformation");
    m.def("getVersion", &pyGetVersion, "A function which return the library version");
#ifdef UNIX
    m.def("processFolder", &pyProcessFolder, "A function which process an entire folder and its subfolders");
#endif
}
