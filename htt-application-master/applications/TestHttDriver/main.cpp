#include "HttDriver.h"
#include "Settings.h"
#include "Utils.h"
#include "wingetopt.h"

#include <iostream> /* cout */
#include <sstream>  /* stringstream */

#include <iomanip> // std::setfill, std::setw
#include <thread>  // std::thread

#ifdef _WIN32
#include "windows.h"
#define sleep(x) Sleep(x)
#endif

using namespace std;
using namespace Nidek::Libraries::UsbDriver;
using namespace Nidek::Libraries::Utils;

static const string tag = "[MAIN] ";

void doHelp(char* app)
{
    stringstream s;
    s << "\nHTT Image Processing Library\n"
      << "Usage: " << app << " [OPTION] ...\n"
      << "Example: " << app << " -h\n"
      << "\n"
      << "  -h                  Print out this help\n"
      << "\n"
      << "  -c 1                  Resets the camera at default settings.\n"
      << "  -c 2                  Starts the stream with the camera current settings.\n"
      << "  -c 3                  Stop the stream.\n"
      << "  -c 4 0xFFFF 0x1234    Set a CMOS register (address - value).\n"
      << "  -c 5 0xFFFF           Get a CMOS register (address).\n"
      << "  -c 6 100              Sets power for LED in the range [0:1000].\n"
      << "\n"
      << "  -r                    Sets a list of registries with the values in the json file"
      << "\n";
    cout << s.str() << endl;
}

static void test(shared_ptr<HttDriver> driver)
{
    driver->ledSet(700);
}

int main(int argc, char** argv)
{
    shared_ptr<Log> log = Log::getInstance();

    log->info(tag + "Load json file");
    shared_ptr<Settings> settings = Settings::getInstance();
    settings->loadHwBoard("hwBoard.json");

    Json::Value hw = settings->getHwBoard();
    int vid = stoul(hw["firmware"]["vid"].asString(), nullptr, 16);
    int pid = stoul(hw["firmware"]["pid"].asString(), nullptr, 16);
    string filename = hw["firmware"]["filename"].asString();
    int pidBoard = stoul(hw["board"]["pid"].asString(), nullptr, 16);
    int endpoint = stoul(hw["board"]["streaming_endpoint"].asString(), nullptr, 16);
    shared_ptr<HttDriver> driver = shared_ptr<HttDriver>(new HttDriver(vid, pid, pidBoard, endpoint, filename));

    int c, cmd, address, value;
    int ret = -1;

    while ((c = getopt(argc, argv, (char*)"c:frsh")) != -1)
    {
        switch (c)
        {
        case 'c':
        {
            cmd = stoi(optarg);
            HttDriver::HttReturnCode r;
            switch (cmd)
            {
            case UsbDriver::UsbCommandId::SYTEM_RESET:
                driver->systemReset();
                break;
            case UsbDriver::UsbCommandId::STREAM_START:
                r = driver->streamStart();
                log->info(tag + "STREAM_START reply " + to_string(r));
                break;
            case UsbDriver::UsbCommandId::STREAM_STOP:
                r = driver->streamStop();
                log->info(tag + "STREAM_STOP reply " + to_string(r));
                break;
            case UsbDriver::UsbCommandId::CMOS_REG_SET:
            {
                if (!argv[optind])
                {
                    log->error(__FILE__, __LINE__, tag + "Missing value");
                    doHelp(argv[0]);
                    break;
                }
                address = strtol(argv[optind], NULL, 16) & 0xffff;

                if (!argv[optind + 1])
                {
                    log->error(__FILE__, __LINE__, tag + "Missing value");
                    doHelp(argv[0]);
                    break;
                }
                value = strtol(argv[optind + 1], NULL, 16) & 0xffff;

                stringstream ss;
                ss << "address : 0x" << setfill('0') << setw(4) << hex << address << " value 0x" << setfill('0')
                   << setw(4) << hex << value << endl;
                log->info(tag + ss.str());
                r = driver->cmosRegSet(address, value);
                log->info(tag + "CMOS_REG_SET reply " + to_string(r));
                break;
            }
            case UsbDriver::UsbCommandId::CMOS_REG_GET:
            {
                if (!argv[optind])
                {
                    log->error(__FILE__, __LINE__, tag + "Missing value");
                    doHelp(argv[0]);
                    break;
                }

                address = strtol(argv[optind], NULL, 16) & 0xffff;
                r = driver->cmosRegGet(address, value);

                stringstream ss;
                ss << "REG_GET reply " << r << " 0x" << setfill('0') << setw(4) << hex << value << endl;
                log->info(tag + ss.str());
                break;
            }
            case UsbDriver::UsbCommandId::LED_SET:
            {
                if (!argv[optind])
                {
                    log->error(__FILE__, __LINE__, tag + "Missing value");
                    doHelp(argv[0]);
                    break;
                }

                value = stoi(argv[optind]);
                r = driver->ledSet(value);
                log->info(tag + "LED_SET reply " + to_string(r));
                break;
            }
            default:
                log->warn(tag + "Command " + to_string(cmd) + " not supported");
                break;
            }
            break;
        }
        case 'f':
        {
            log->info(tag + "GET FRAME");
            shared_ptr<Image<uint8_t>> frame(new Image<uint8_t>(560, 560, 1));
            // Start Stream
            HttDriver::HttReturnCode r = driver->streamStart();
            if (r == HttDriver::HttReturnCode::HTT_USB_PASS)
            {
                log->debug(tag + "driver->getFrame()");
                r = driver->getFrame(frame);
                if (r == HttDriver::HttReturnCode::HTT_USB_PASS)
                {
                    log->debug(tag + "saveImage()");
                    // bool ret = saveImage(".", "test.png", frame);
                    // log->debug(tag + to_string(ret));
                    saveRawImage(".", "test.data", frame);
                } else
                {
                    log->debug(tag + "getFrame fails");
                }
            }
            driver->streamStop();

            break;
        }
        case 'r':
        {
            log->warn(tag + "Restart with configured setup");

            Json::Value::Members keys = hw["reg_setup"].getMemberNames();
            HttDriver::HttReturnCode r;
            for (unsigned int i = 0; i < hw["reg_setup"].size(); ++i)
            {
                // cout << keys[i] << ": " << hw["reg_setup"][keys[i]] << endl;
                address = strtol(keys[i].c_str(), NULL, 16) & 0xffff;
                value = strtol(hw["reg_setup"][keys[i]].asCString(), NULL, 16) & 0xffff;
                r = driver->cmosRegSet(address, value);
                log->info(tag + "CMOS_REG_SET reply " + to_string(r));
                if (r == !HttDriver::HttReturnCode::HTT_USB_PASS)
                {
                    stringstream ss;
                    ss << "CMOS_REG_SET " << keys[i] << ": " << hw["reg_setup"][keys[i]].asString() << " fails";
                    log->error(__FILE__, __LINE__, tag + ss.str());
                }
                sleep(200);
            }
            break;
        }
        case 's':
        {

            log->info(tag + "stream: ");
            unsigned char buffer[1024];

            while (1)
            {
                int len = driver->getRawStreamData(buffer, 1024);
                sleep(20);

                {
                    stringstream ss;
                    for (int i = 0; i < len; ++i)
                    {
                        ss << " 0x" << setfill('0') << setw(2) << hex << static_cast<int>(buffer[i]);
                    }
                    log->info(tag + ss.str());
                }
            }
            break;
        }
        case '?': // error
        case 'h':
        {
            doHelp(argv[0]);
            break;
        }
        default:
            break;
        }
    }

    return ret;
}
