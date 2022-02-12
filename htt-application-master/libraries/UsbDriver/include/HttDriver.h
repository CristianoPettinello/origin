//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __HTT_DRIVER_H__
#define __HTT_DRIVER_H__

#include "HttDriverGlobal.h"
#include "Image.h"
#include "Log.h"
#include "UsbDriver.h"
#include <memory>
#include <mutex>
#include <thread> // std::thread

using namespace std;
using namespace Nidek::Libraries::Utils;

// #define DEBUG

// Enable std thread instead of windows thread
// #define STDTHREAD

namespace Nidek {
namespace Libraries {
namespace UsbDriver {

class HttDriver
{
public:
    enum HttReturnCode
    {
        HTT_USB_FAIL,
        HTT_USB_PASS,
        HTT_USB_ERROR
    };

    typedef struct
    {
        int G;
        int dG;
        int aGa;
        int aGc;
    } gainFactors;

public:
    USB_DRIVER_LIBRARYSHARED_EXPORT
    HttDriver(const int& vid, const int& pid, const int& pidCamera, const int& endpoint, const string& filename);
    USB_DRIVER_LIBRARYSHARED_EXPORT ~HttDriver();
    USB_DRIVER_LIBRARYSHARED_EXPORT HttReturnCode systemReset();
    USB_DRIVER_LIBRARYSHARED_EXPORT HttReturnCode streamStart(bool restart = false);
    USB_DRIVER_LIBRARYSHARED_EXPORT HttReturnCode streamStop();
    USB_DRIVER_LIBRARYSHARED_EXPORT HttReturnCode cmosRegSet(int address, int value);
    USB_DRIVER_LIBRARYSHARED_EXPORT HttReturnCode cmosRegGet(int address, int& value);
    USB_DRIVER_LIBRARYSHARED_EXPORT HttReturnCode ledSet(int power);
    USB_DRIVER_LIBRARYSHARED_EXPORT HttReturnCode ledGet(int& power);
    USB_DRIVER_LIBRARYSHARED_EXPORT int getRawStreamData(unsigned char* buffer, int length);
    USB_DRIVER_LIBRARYSHARED_EXPORT HttReturnCode getFrame(shared_ptr<Image<uint8_t>> image,
                                                           bool horizontalFlipping = false);
    USB_DRIVER_LIBRARYSHARED_EXPORT HttReturnCode gainGet(int& gain);
    USB_DRIVER_LIBRARYSHARED_EXPORT HttReturnCode gainSet(int gain);
    USB_DRIVER_LIBRARYSHARED_EXPORT HttReturnCode exposuretTimeGet(int& time);
    USB_DRIVER_LIBRARYSHARED_EXPORT HttReturnCode exposuretTimeSet(int time);

private:
    bool checkSyncSymbols(unsigned char* ptr);
    unsigned char* findFooter(unsigned char* streamTail, unsigned char* footerPayload, int footerPayloadLen);
#ifdef STDTHREAD
    void threadLoop();
#else
    static DWORD WINAPI threadLoop(LPVOID in);
    DWORD threadStart();
#endif

private:
    shared_ptr<Log> m_log;
    shared_ptr<UsbDriver> m_driver;
    mutex m_usbBuffer1Mutex;
    mutex m_usbBuffer2Mutex;
#ifdef STDTHREAD
    thread m_thread;
#else
    HANDLE m_thread;
#endif

    unsigned char* m_streamBuffer; // Pointer to buffer used to copy stream data
    unsigned char* m_usbBuffer;    // Pointer to a buffer ready to be read
    unsigned char* m_usbBuffer1;   // Buffer 1 for usb data
    unsigned char* m_usbBuffer2;   // Buffer 2 for usb data
    int m_streamBufferLen;
    int m_usbBufferLen;
    int m_actualLength1;
    int m_actualLength2;

    int m_expectedUsbLen;
    int m_expectedStreamLen;

    bool m_isStarted;

#ifdef DEBUG
    FILE* m_fp;
#endif
};

} // namespace UsbDriver
} // namespace Libraries
} // namespace Nidek

#endif // __HTT_DRIVER_H__
