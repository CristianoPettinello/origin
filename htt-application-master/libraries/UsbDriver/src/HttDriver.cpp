#include "HttDriver.h"
#include "Utils.h"

#include < chrono>  // std::chrono::milliseconds
#include <future>   // std::async, std::future
#include <iomanip>  // std::setfill, std::setw
#include <iostream> // std::cout, std::endl
#include <sstream>

#ifdef UNIX
#include <cstring>
#inlcude < math.h>
#else
#include <windows.h>
#endif

using namespace std;
using namespace Nidek::Libraries::UsbDriver;
using namespace Nidek::Libraries::Utils;

// Format used to rapresent the pixel, 8 or 10 bit
#define RAW10

#define SYNC_LEN 5
#define FOOTER_PAYLOAD_LEN 64
#define FOOTER_LEN (SYNC_LEN + FOOTER_PAYLOAD_LEN + SYNC_LEN)

// ARX3A0 Image Sensor Register
#define REG_GLOBAL_GAIN 0x305e
#define REG_CIT 0x3012

// CIT is Coarse Integration Time. This parameter is controled through register 0x3012
// and can vary in the range [0:65535].
// In the current configuration its value is 1798 giving the maximum Et at 30 FPS. Setting values higher than
// 1798 will hang the sensor.
#define MAX_CIT_VALUE 1798
#define GAIN_AGC_MASK 0x000F
#define GAIN_AGA_MASK 0x0070
#define GAIN_DG_MASK 0xFF80

static const string tag = "[HTTD] ";

static string threadId()
{
    return "[" + to_string(GetCurrentThreadId()) + "] ";
}

const HttDriver::gainFactors gainLookupTable[64] = {
    {0, 0, 0, 0},    {1, 64, 0, 0},   {2, 64, 1, 0},   {3, 64, 1, 8},   {4, 64, 2, 0},   {5, 64, 2, 4},
    {6, 64, 2, 8},   {7, 64, 2, 12},  {8, 64, 3, 0},   {9, 72, 3, 0},   {10, 80, 3, 0},  {11, 88, 3, 0},
    {12, 96, 3, 0},  {13, 104, 3, 0}, {14, 112, 3, 0}, {15, 120, 3, 0}, {16, 128, 3, 0}, {17, 136, 3, 0},
    {18, 144, 3, 0}, {19, 152, 3, 0}, {20, 160, 3, 0}, {21, 168, 3, 0}, {22, 176, 3, 0}, {23, 184, 3, 0},
    {24, 192, 3, 0}, {25, 200, 3, 0}, {26, 208, 3, 0}, {27, 216, 3, 0}, {28, 224, 3, 0}, {29, 232, 3, 0},
    {30, 240, 3, 0}, {31, 248, 3, 0}, {32, 256, 3, 0}, {33, 264, 3, 0}, {34, 272, 3, 0}, {35, 280, 3, 0},
    {36, 288, 3, 0}, {37, 296, 3, 0}, {38, 304, 3, 0}, {39, 312, 3, 0}, {40, 320, 3, 0}, {41, 328, 3, 0},
    {42, 336, 3, 0}, {43, 344, 3, 0}, {44, 352, 3, 0}, {45, 360, 3, 0}, {46, 368, 3, 0}, {47, 376, 3, 0},
    {48, 384, 3, 0}, {49, 392, 3, 0}, {50, 400, 3, 0}, {51, 408, 3, 0}, {52, 416, 3, 0}, {53, 424, 3, 0},
    {54, 432, 3, 0}, {55, 440, 3, 0}, {56, 448, 3, 0}, {57, 456, 3, 0}, {58, 464, 3, 0}, {59, 472, 3, 0},
    {60, 480, 3, 0}, {61, 488, 3, 0}, {62, 496, 3, 0}, {63, 504, 3, 0}};

HttDriver::HttDriver(const int& vid, const int& pid, const int& pidCamera, const int& endpoint, const string& filename)
    : m_log(Log::getInstance()),
      m_streamBuffer(nullptr),
      m_usbBuffer(nullptr),
      m_streamBufferLen(0),
      m_usbBufferLen(0),
      m_actualLength1(0),
      m_actualLength2(0),
      m_expectedUsbLen(0),
      m_expectedStreamLen(0),
      m_isStarted(false)
{
    m_driver = shared_ptr<UsbDriver>(new UsbDriver(vid, pid, pidCamera, endpoint, filename));

    m_expectedUsbLen = 0x8000;              // Length of transmitted data in each usb frame.
    m_usbBufferLen = m_expectedUsbLen * 30; // Length of the temporary usb buffer

#ifdef RAW10
    m_expectedStreamLen = ((560 * 560) * 10 / 8) + FOOTER_LEN; // RAW10 -> 392072
#else
    m_expectedStreamLen = 560 * 560 + FOOTER_LEN; // RAW8 -> 313,674
#endif

    m_streamBufferLen = m_expectedStreamLen * 4;

    m_streamBuffer = new unsigned char[m_streamBufferLen];
    m_usbBuffer1 = new unsigned char[m_usbBufferLen];
    m_usbBuffer2 = new unsigned char[m_usbBufferLen];
    m_usbBuffer = m_usbBuffer1;

    m_thread = nullptr;
    // m_thread = CreateThread(NULL, 0, threadLoop, this, 0, NULL);
    // SetThreadPriority(m_thread, THREAD_PRIORITY_TIME_CRITICAL);

#ifdef DEBUG
    m_fp = fopen("stream.data", "wb");
    if (!m_fp)
        abort();
#endif
}

HttDriver::~HttDriver()
{
#ifdef DEBUG
    if (m_fp)
        fclose(m_fp);
#endif
    if (m_streamBuffer)
    {
        delete[] m_streamBuffer;
        m_streamBuffer = nullptr;
        m_streamBufferLen = 0;
    }

    if (m_usbBuffer1)
    {
        delete[] m_usbBuffer1;
        m_usbBuffer1 = nullptr;
        // m_usbBufferLen = 0;
    }

    if (m_usbBuffer2)
    {
        delete[] m_usbBuffer2;
        m_usbBuffer2 = nullptr;
        // m_usbBufferLen = 0;
    }
    m_usbBufferLen = 0;
}

HttDriver::HttReturnCode HttDriver::systemReset()
{
    // System reset command, vid=0x04B4 pid=0x00F1
    // Resets the camera at default settings.
    // No payload, no reply
    UsbReturnCode r = m_driver->sendControlCommands(UsbDriver::UsbCommandId::SYTEM_RESET, nullptr, 0);
    return r == UsbReturnCode::SUCCESS ? HttReturnCode::HTT_USB_PASS : HttReturnCode::HTT_USB_ERROR;
}
HttDriver::HttReturnCode HttDriver::streamStart(bool restart /* = false */)
{
    // Stream start command, vid=0x04B4 pid=0x00F1
    // Starts the stream with the camera current settings.
    // No payload
    // Reply:
    //  [0] Command ID
    //  [1] Result{0x00 = Fail, 0x01 = Pass}
    UsbReturnCode r = m_driver->sendControlCommands(UsbDriver::UsbCommandId::STREAM_START, nullptr, 0);
    if (r == UsbReturnCode::SUCCESS)
    {
        // Enable thread loop
        m_isStarted = true;
#ifdef STDTHREAD
        m_thread = thread(&HttDriver::threadLoop, this);
        // FIXME: set thread priority
#else
        // threadStart();

        if (!restart && m_thread == nullptr)
        {
            m_log->debug(tag + threadId() + "Created new thread");
            m_thread = CreateThread(NULL, 0, threadLoop, this, 0, NULL);
            SetThreadPriority(m_thread, THREAD_PRIORITY_TIME_CRITICAL);
            m_log->debug(tag + threadId() + "restart: " + to_string(restart) +
                         " m_thread: " + to_string(int(m_thread)));
        } else
        {
            m_log->debug(tag + threadId() + "Resumed previusly thread");
            m_log->debug(tag + threadId() + "restart: " + to_string(restart) +
                         " m_thread: " + to_string(int(m_thread)));
        }
#endif

        unsigned char buff[8];
        r = m_driver->readControlResponse(UsbDriver::UsbCommandId::STREAM_START, buff);
        if (r == SUCCESS)
        {
            return buff[1] == 0 ? HTT_USB_FAIL : HTT_USB_PASS;
        }
    }
    return HTT_USB_ERROR;
}
HttDriver::HttReturnCode HttDriver::streamStop()
{
    // Stream stop command, vid=0x04B4 pid=0x00F1
    // Stop the stream.
    // No payload
    // Reply:
    //  [0] Command ID
    //  [1] Result{0x00 = Fail, 0x01 = Pass}
    UsbReturnCode r = m_driver->sendControlCommands(UsbDriver::UsbCommandId::STREAM_STOP, nullptr, 0);
    if (r == UsbReturnCode::SUCCESS)
    {
        // Disable thread loop
        m_isStarted = false;
#ifdef STDTHREAD
        // Wait for thread stop
        if (m_thread.joinable())
            m_thread.join();
#else
        WaitForSingleObject(m_thread, 30);
        CloseHandle(m_thread);
        m_thread = nullptr;
#endif

        unsigned char buff[8];
        r = m_driver->readControlResponse(UsbDriver::UsbCommandId::STREAM_STOP, buff);
        if (r == SUCCESS)
        {
            return buff[1] == 0 ? HTT_USB_FAIL : HTT_USB_PASS;
        }
    }
    return HTT_USB_ERROR;
}
HttDriver::HttReturnCode HttDriver::cmosRegSet(int address, int value)
{
    // CMOS set register command, vid=0x04B4 pid=0x00F1
    // Set a CMOS register.
    // Payload:
    //  [0] Address MSB
    //  [1] Address LSB
    //  [2] Value MSB
    //  [3] Value LSB
    // Reply:
    //  [0] Command ID
    //  [1] Result{0x00 = Fail, 0x01 = Pass}
    unsigned char payload[4];
    payload[0] = (address & 0xFF00) >> 8;
    payload[1] = address & 0x00FF;
    payload[2] = (value & 0xFF00) >> 8;
    payload[3] = value & 0x00FF;
    UsbReturnCode r = m_driver->sendControlCommands(UsbDriver::UsbCommandId::CMOS_REG_SET, payload, 4);
    if (r == UsbReturnCode::SUCCESS)
    {
        unsigned char buff[8];
        r = m_driver->readControlResponse(UsbDriver::UsbCommandId::STREAM_STOP, buff);
        if (r == SUCCESS)
        {
            return buff[1] == 0 ? HTT_USB_FAIL : HTT_USB_PASS;
        }
    }
    return HTT_USB_ERROR;
}
HttDriver::HttReturnCode HttDriver::cmosRegGet(int address, int& value)
{
    // CMOS get register command, vid=0x04B4 pid=0x00F1
    // Get a CMOS register.
    // Payload:
    //  [0] Address MSB
    //  [1] Address LSB
    // Reply:
    //  [0] Command ID
    //  [1] Result { 0x00 = Fail, 0x01 = Pass }
    //  [2] Value MSB
    //  [3] Value LSB
    unsigned char payload[2];
    payload[0] = (address & 0xFF00) >> 8;
    payload[1] = address & 0x00FF;
    UsbReturnCode r = m_driver->sendControlCommands(UsbDriver::UsbCommandId::CMOS_REG_GET, payload, 2);
    if (r == UsbReturnCode::SUCCESS)
    {
        unsigned char buff[8];
        r = m_driver->readControlResponse(UsbDriver::UsbCommandId::CMOS_REG_GET, buff);

        if (r == SUCCESS)
        {
            value = 0;
            if (buff[1] == 0)
                return HTT_USB_FAIL;
            else
            {
                value = (((int)buff[2]) << 8) + (int)buff[3];
                return HTT_USB_PASS;
            }
        }
    }

    return HTT_USB_ERROR;
}
HttDriver::HttReturnCode HttDriver::ledSet(int power)
{
    // Set led power command, vid=0x04B4 pid=0x00F1
    // Sets power for LED in the range [0:1000]
    // Payload:
    //  [0] Power MSB
    //  [1] Power LSB
    unsigned char payload[2]; // = {0x0, 0x0};
    payload[0] = (power & 0xFF00) >> 8;
    payload[1] = power & 0x00FF;
    {
        stringstream ss;
        ss << "Set led power: " << power << " -> " << setfill('0') << setw(2) << hex << " 0x"
           << static_cast<int>(payload[0]) << " 0x" << static_cast<int>(payload[1]);
        m_log->debug(tag + ss.str());
    }
    UsbReturnCode r = m_driver->sendControlCommands(UsbDriver::UsbCommandId::LED_SET, payload, 2);
    if (r == UsbReturnCode::SUCCESS)
    {
        // call function asynchronously:
        // UsbDriver::ControlTransferReply reply;
        unsigned char buff[8];
        r = m_driver->readControlResponse(UsbDriver::UsbCommandId::LED_SET, buff);

        if (r == SUCCESS)
        {
            return buff[1] == 0 ? HTT_USB_FAIL : HTT_USB_PASS;
        } else
            return HTT_USB_ERROR;
    } else
    {
        return HTT_USB_ERROR;
    }
}
HttDriver::HttReturnCode HttDriver::ledGet(int& power)
{
    // Get led power command, vid=0x04B4 pid=0x00F1
    // Gets power for LED in the range [0:1000]
    // No payload
    // Reply:
    //  [0] Command ID
    //  [1] Result { 0x00 = Fail, 0x01 = Pass }
    //  [2] Value MSB
    //  [3] Value LSB
    UsbReturnCode r = m_driver->sendControlCommands(UsbDriver::UsbCommandId::LED_GET, nullptr, 0);
    if (r == UsbReturnCode::SUCCESS)
    {
        // call function asynchronously:
        // UsbDriver::ControlTransferReply reply;
        unsigned char buff[8];
        r = m_driver->readControlResponse(UsbDriver::UsbCommandId::LED_GET, buff);

        if (r == SUCCESS)
        {
            power = 0;
            if (buff[1] == 0)
                return HTT_USB_FAIL;
            else
            {
                power = (((int)buff[2]) << 8) + (int)buff[3];
                {
                    stringstream ss;
                    ss << "Led power: " << power; // << " -> " << time << " ms";
                    m_log->debug(tag + ss.str());
                }
                return HTT_USB_PASS;
            }
        }
    } else
    {
        return HTT_USB_ERROR;
    }
}

int HttDriver::getRawStreamData(unsigned char* buffer, int length)
{
    int actualLength = 0;
    m_driver->getStream(buffer, length, actualLength);
    return actualLength;
}

bool HttDriver::checkSyncSymbols(unsigned char* ptr)
{
    return (ptr[0] == 0xff) && (ptr[1] == 0x00) && (ptr[2] == 0xff) && (ptr[3] == 0x00) && (ptr[4] == 0xcc);
}

unsigned char* HttDriver::findFooter(unsigned char* streamTail, unsigned char* footerPayload, int footerPayloadLen)
{
    // tail points to the next available buffer position
    // tail - 5 is the beggining of the footer

    unsigned char* footerHead = streamTail - FOOTER_LEN;

    while (footerHead != m_streamBuffer /* + m_expectedStreamLen*/)
    {
        if (checkSyncSymbols(footerHead) & checkSyncSymbols(footerHead + SYNC_LEN + footerPayloadLen))
        // if (checkSyncSymbols(footerHead))
        {
            memcpy(footerPayload, footerHead + SYNC_LEN, footerPayloadLen);

            // Return the pointer to the beginning of the image data.
            // m_log->debug(tag + "Footer found");

            // FXIME: FOOTER
            // stringstream ss;
            // ss << "FOOTER: ";
            // for (int i = 0; i < FOOTER_LEN; ++i)
            // {
            //     ss << setfill('0') << setw(2) << hex << (int)footerHead[i] << " ";
            // }
            // m_log->debug(tag + ss.str());

            return footerHead - (m_expectedStreamLen - FOOTER_LEN);
            // return m_streamBuffer;
        } else
            // Sync symbols not found, tries with the previous byte in the stream.
            --footerHead;
    }

    // m_log->debug(tag + "No footer found");

    return nullptr;
}

#ifdef STDTHREAD
void HttDriver::threadLoop()
#else
DWORD WINAPI HttDriver::threadLoop(LPVOID in)
{
    HttDriver* htt = (HttDriver*)in;
    return htt->threadStart();
}
DWORD HttDriver::threadStart()
#endif
{
    // When a new thread is created it is necessary to restore the m_usbBuffer pointer to the m_usbBuffer1 value
    m_usbBuffer = m_usbBuffer1;
    unsigned char* currBuff = m_usbBuffer1;
    unsigned char* nextBuff = m_usbBuffer2;

    m_log->info(tag + threadId() + "Enter threadStart loop - m_isStarted=" + to_string(m_isStarted));
    while (m_isStarted)
    {
        // Protect buffer write
        if (currBuff == m_usbBuffer1)
        {
            lock_guard<mutex> lock(m_usbBuffer1Mutex);
            m_driver->getStream(currBuff, m_usbBufferLen, m_actualLength1);
            // m_log->info(tag + "buf1 len: " + to_string(m_actualLength1));
        } else
        {
            lock_guard<mutex> lock(m_usbBuffer2Mutex);
            m_driver->getStream(currBuff, m_usbBufferLen, m_actualLength2);
            // m_log->info(tag + "buf2 len: " + to_string(m_actualLength2));
        }

        // Update m_usbBuffer pointer
        m_usbBuffer = currBuff;

        // Swap the buffers
        currBuff = nextBuff;
        nextBuff = m_usbBuffer;
    }
    m_log->info(tag + threadId() + "Exit threadStart loop - m_isStarted=" + to_string(m_isStarted));
#ifndef UNIX
    return 0;
#endif
}

// FIXME: NBr new implementation of getFrame
HttDriver::HttReturnCode HttDriver::getFrame(shared_ptr<Image<uint8_t>> image, bool horizontalFlipping /*=false*/)
{
    // Check if working buffers are valid
    if (m_streamBuffer == nullptr || m_usbBuffer == nullptr)
    {
        m_log->error(__FILE__, __LINE__, tag + "Buffers not allocated");
        return HttDriver::HttReturnCode::HTT_USB_FAIL;
    }

    int transferred = 0;

    // Define two pointers to the stream buffer. The idea is to fill m_streamBuffer with data coming from the USB. When
    // enough data has arrived, we check the presence of the footer. At that point the streamHead points to the
    // beginning of the image.
    unsigned char* streamHead = nullptr;
    unsigned char* streamTail = m_streamBuffer;

    // Define the payload associated to the footer.
    unsigned char footerPayload[FOOTER_PAYLOAD_LEN];

    if (m_usbBuffer == m_usbBuffer1)
    {
        lock_guard<mutex> lock(m_usbBuffer1Mutex);

        // m_log->debug(tag + to_string(m_actualLength1));

        if (m_actualLength1 > 0)
        {
            // Copies the collected partial data into the stream data.
            memcpy(streamTail, m_usbBuffer, m_actualLength1);
            transferred += m_actualLength1;
            {
                stringstream ss;
                ss << "Copies partial stream data, actualLength: " << m_actualLength1
                   << " transferred: " << transferred;
                // m_log->debug(tag + ss.str());
            }

            if (transferred > m_streamBufferLen)
            {
                // the m_streamBuffer is full and we haven't found the footer. Data is discarded and the stream
                // starts from zero.
                streamTail = m_streamBuffer;
                transferred = 0;
                m_log->error(__FILE__, __LINE__, tag + "streamBuffer is full and we haven't found the footer.");
            } else
                streamTail += m_actualLength1;
        } else
        {
            m_log->debug(tag + "No data1, try with the other buffer");

            m_usbBuffer = m_usbBuffer2;

            lock_guard<mutex> lock2(m_usbBuffer2Mutex);

            // m_log->debug(tag + to_string(m_actualLength2));

            if (m_actualLength2 > 0)
            {
                // Copies the collected partial data into the stream data.
                memcpy(streamTail, m_usbBuffer, m_actualLength2);
                transferred += m_actualLength2;
                {
                    stringstream ss;
                    ss << "Copies partial stream data, actualLength: " << m_actualLength2
                       << " transferred: " << transferred;
                    // m_log->debug(tag + ss.str());
                }

                if (transferred > m_streamBufferLen)
                {
                    // the m_streamBuffer is full and we haven't found the footer. Data is discarded and the stream
                    // starts from zero.
                    streamTail = m_streamBuffer;
                    transferred = 0;
                    m_log->error(__FILE__, __LINE__, tag + "streamBuffer is full and we haven't found the footer.");
                } else
                    streamTail += m_actualLength2;
            } else
            {
                m_log->debug(tag + "No data, try to restart the camera");
                return HttDriver::HttReturnCode::HTT_USB_ERROR;
            }
        }
    } else
    {
        lock_guard<mutex> lock(m_usbBuffer2Mutex);

        // m_log->debug(tag + to_string(m_actualLength2));

        if (m_actualLength2 > 0)
        {
            // Copies the collected partial data into the stream data.
            memcpy(streamTail, m_usbBuffer, m_actualLength2);
            transferred += m_actualLength2;
            {
                stringstream ss;
                ss << "Copies partial stream data, actualLength: " << m_actualLength2
                   << " transferred: " << transferred;
                // m_log->debug(tag + ss.str());
            }

            if (transferred > m_streamBufferLen)
            {
                // the m_streamBuffer is full and we haven't found the footer. Data is discarded and the stream
                // starts from zero.
                streamTail = m_streamBuffer;
                transferred = 0;
                m_log->error(__FILE__, __LINE__, tag + "streamBuffer is full and we haven't found the footer.");
            } else
                streamTail += m_actualLength2;
        } else
        {
            m_log->debug(tag + "No data1, try with the other buffer");

            m_usbBuffer = m_usbBuffer1;

            lock_guard<mutex> lock2(m_usbBuffer1Mutex);

            if (m_actualLength1 > 0)
            {
                // Copies the collected partial data into the stream data.
                memcpy(streamTail, m_usbBuffer, m_actualLength1);
                transferred += m_actualLength1;
                {
                    stringstream ss;
                    ss << "Copies partial stream data, actualLength: " << m_actualLength1
                       << " transferred: " << transferred;
                    // m_log->debug(tag + ss.str());
                }

                if (transferred > m_streamBufferLen)
                {
                    // the m_streamBuffer is full and we haven't found the footer. Data is discarded and the stream
                    // starts from zero.
                    streamTail = m_streamBuffer;
                    transferred = 0;
                    m_log->error(__FILE__, __LINE__, tag + "streamBuffer is full and we haven't found the footer.");
                } else
                    streamTail += m_actualLength1;
            } else
            {
                m_log->debug(tag + "No data, try to restart the camera");
                return HttDriver::HttReturnCode::HTT_USB_ERROR;
            }
        }
    }

    // Check that the number of transferred bytes is > the expected stream length.
    if (transferred >= m_expectedStreamLen)
    {
        // m_log->debug(tag + "Find the footer signature starting from the end.");

        // Find the footer signature starting from the end.
        streamHead = findFooter(streamTail, footerPayload, FOOTER_PAYLOAD_LEN);
        // streamHead = m_streamBuffer;
    }

    if (!streamHead)
    {
        m_log->debug(tag + "No streamHead");
        return HttDriver::HttReturnCode::HTT_USB_ERROR;
    }

    uint8_t* imagePtr = image->getData();
    unsigned char* ptr = streamHead; // m_streamBuffer;

#ifdef RAW10
    for (int i = 0; i < (m_expectedStreamLen - FOOTER_LEN); i += 5)
    {
        *imagePtr++ = *ptr++;
        *imagePtr++ = *ptr++;
        *imagePtr++ = *ptr++;
        *imagePtr++ = *ptr++;
        ptr++;
    }
#else
    memcpy(imagePtr, ptr, m_expectedStreamLen - FOOTER_LEN);
#endif

    // Flipping image
    if (horizontalFlipping)
        Utils::horizontalFlip(image);

    return HttDriver::HttReturnCode::HTT_USB_PASS;
}

#if 0
HttDriver::HttReturnCode HttDriver::getFrame(shared_ptr<Image<uint8_t>> image)
{
    // m_log->info("getFrame");
    // Sleep(30);
    // return HttDriver::HttReturnCode::HTT_USB_PASS;

    // copia di m_usbBuffer in m_streamBuffer
    // inserire un mutex

    // Check if working buffers are valid
    if (m_streamBuffer == nullptr || m_usbBuffer1 == nullptr || m_usbBuffer2 == nullptr)
    {
        m_log->error(__FILE__, __LINE__, tag + "Buffers not allocated");
        return HttDriver::HttReturnCode::HTT_USB_FAIL;
    }

    int transferred = 0;

    // Define two pointers to the stream buffer. The idea is to fill m_streamBuffer with data coming from the USB. When
    // enough data has arrived, we check the presence of the footer. At that point the streamHead points to the
    // beginning of the image.
    unsigned char* streamHead = nullptr;
    unsigned char* streamTail = m_streamBuffer;

    // Define the payload associated to the footer.
    int footerPayloadLen = 64;
    unsigned char footerPayload[64];

    // Loops till the beginning of the image data has been found.
    // while (!streamHead)
    // while (true)
    {
        // Get the data from the USB
        int actualLength = 0;
        m_driver->getStream(m_usbBuffer, m_usbBufferLen, actualLength);

#if 1
#ifdef DEBUG
        fwrite(m_usbBuffer, sizeof(uint8_t), actualLength, m_fp);
        fflush(m_fp);
#endif

        // // Debug
        // if (actualLength > m_usbBufferLen)
        // {
        //     // FIXME log this condition
        //     m_log->error(__FILE__, __LINE__,
        //                  tag + "Data length (" + to_string(actualLength) + ") exceed the expected USB length(" +
        //                      to_string(m_expectedUsbLen) + ")");
        // }

        if (actualLength > 0)
        {
            // Copies the collected partial data into the stream data.
            memcpy(streamTail, m_usbBuffer, actualLength);
            transferred += actualLength;
            {
                stringstream ss;
                ss << "Copies partial stream data, actualLength: " << actualLength << " transferred: " << transferred;
                // m_log->debug(tag + ss.str());
            }

            if (transferred > m_streamBufferLen)
            {
                // the m_streamBuffer is full and we haven't found the footer. Data is discarded and the stream starts
                // from zero.
                streamTail = m_streamBuffer;
                transferred = 0;
                // m_log->error(__FILE__, __LINE__, tag + "streamBuffer is full and we haven't found the footer.");
            } else
                streamTail += actualLength;
        } else
        {
            // m_log->debug(tag + "No data, try to restart the camera");
            return HttDriver::HttReturnCode::HTT_USB_ERROR;
        }

        // Check that the number of transferred bytes is > the expected stream length.
        if (transferred >= m_expectedStreamLen)
        {
            // m_log->debug(tag + "Find the footer signature starting from the end.");

            // Find the footer signature starting from the end.
            streamHead = findFooter(streamTail, footerPayload, footerPayloadLen);
            // streamHead = m_streamBuffer;
        }
#else
        return HttDriver::HttReturnCode::HTT_USB_PASS;
#endif // FIXME
    }

    // streamHead punta all'inizio dei dati dell'immagine (a 10 bit)
    // ogni 4 byte si scarta il successivo

    if (!streamHead)
        return HttDriver::HttReturnCode::HTT_USB_ERROR;

    // FIXME: save stream data into a file
    // {
    //     FILE* fp = fopen("stream.data", "wb");
    //     if (!fp)
    //         abort();

    //     cout << "stride: " << srcImage->getStride() << " h: " << srcImage->getHeight()
    //          << " nch: " << (double)srcImage->getNChannels() << endl;

    //     fwrite(m_streamBuffer, sizeof(uint8_t), m_expectedStreamLen, m_fp);
    //     fflush(m_fp);

    //     fclose(fp);
    // }

    // FIXME Get the image buffer
    uint8_t* imagePtr = image->getData();

    // m_log->debug(tag + "Frame length: " + to_string(m_expectedStreamLen - FOOTER_LEN));

    unsigned char* ptr = streamHead; // m_streamBuffer;

    // cout << "imgSize: " << image->getStride() * image->getHeight() * image->getNChannels() << endl;
    // cout << "realSize: " << (m_expectedStreamLen - FOOTER_LEN) * 8 / 10 << endl;

#ifdef RAW10
    for (int i = 0; i < (m_expectedStreamLen - FOOTER_LEN); i += 5)
    {
        *imagePtr++ = *ptr++;
        *imagePtr++ = *ptr++;
        *imagePtr++ = *ptr++;
        *imagePtr++ = *ptr++;
        ptr++;
    }
#else
    memcpy(imagePtr, ptr, m_expectedStreamLen - FOOTER_LEN);
#endif

    return HttDriver::HttReturnCode::HTT_USB_PASS;
}
#endif

HttDriver::HttReturnCode HttDriver::gainGet(int& gain)
{
    int value;
    HttDriver::HttReturnCode r = cmosRegGet(REG_GLOBAL_GAIN, value);
    if (r == HttDriver::HttReturnCode::HTT_USB_PASS)
    {
        int aGc = value & GAIN_AGC_MASK;
        int aGa = (value & GAIN_AGA_MASK) >> 4;
        int dG = (value & GAIN_DG_MASK) >> 7;
        // G = (aGc/16 + 1) * (2^aGa) * (dG/64)
        gain = (int)((((double)aGc / 16.0) + 1.0) * (double)(1 << aGa) * ((double)dG / 64.0));

        {
            stringstream ss;
            ss << "cameraGain value: " << value << " aGc: " << aGc << " aGa: " << aGa << " dG: " << dG
               << " gain: " << gain;
            m_log->debug(tag + ss.str());
        }
    }
    return r;
}

HttDriver::HttReturnCode HttDriver::gainSet(int gain)
{
    if (gain < 1 || gain > 63)
        return HttReturnCode::HTT_USB_ERROR;

    // G = (aGc/16 + 1) * (2^aGa) * (dG/64)
    int dG = gainLookupTable[gain].dG;
    int aGa = gainLookupTable[gain].aGa;
    int aGc = gainLookupTable[gain].aGc;

    {
        stringstream ss;
        ss << "gain: " << gain << " aGa: " << aGa << " aGc: " << aGc << " dG: " << dG;
        m_log->debug(tag + ss.str());
    }

    int value = (dG << 7) + (aGa << 4) + aGc;
    {
        stringstream ss;
        ss << "setCameraGain: " << gain << " -> " << value;
        m_log->debug(tag + ss.str());
    }
    return cmosRegSet(REG_GLOBAL_GAIN, value);
}

HttDriver::HttReturnCode HttDriver::exposuretTimeGet(int& time)
{
    int value;
    HttDriver::HttReturnCode r = cmosRegGet(REG_CIT, value);
    if (r == HttDriver::HttReturnCode::HTT_USB_PASS)
    {
        // Et [ms] = CIT / 54
        time = value / 54;
        {
            stringstream ss;
            ss << "exposureTime value: " << value << " -> " << time << " ms";
            m_log->debug(tag + ss.str());
        }
    }
    return r;
}

HttDriver::HttReturnCode HttDriver::exposuretTimeSet(int time)
{
    // Et [ms] = CIT / 54
    int value = min(time * 54, MAX_CIT_VALUE);
    {
        stringstream ss;
        ss << "set exposure time: " << time << " ms -> " << value;
        m_log->debug(tag + ss.str());
    }
    return cmosRegSet(REG_CIT, value);
}
