//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __USB_DRIVER_H__
#define __USB_DRIVER_H__

#include "Log.h"
#include "UsbReturnCode.h"
#include <memory>

#ifdef LIBUSB
#include <libusb-1.0/libusb.h>
#else
// "windows.h" MUST be included before the "CyAPI.h"
#include "windows.h"

#include "CyAPI.h"
// extern int sprintf(char str, const char format, ...);
#endif

using namespace std;
using namespace Nidek::Libraries::Utils;

namespace Nidek {
namespace Libraries {
namespace UsbDriver {

/* This is the maximum number of 'devices of interest' we are willing to store as default. */
/* These are the maximum number of devices we will communicate with simultaneously */
#define MAXDEVICES 10
#define MAX_FWIMG_SIZE (512 * 1024) // Maximum size of the firmware binary.
#define MAX_WRITE_SIZE (2 * 1024)   // Max. size of data that can be written through one vendor command.
#define GET_LSW(v) ((unsigned short)((v)&0xFFFF))
#define GET_MSW(v) ((unsigned short)((v) >> 16))
#define VENDORCMD_TIMEOUT (5000) // Timeout for each vendor command is set to 5 seconds.
#define REBOOT_DELAY_US 500000   // Reboot time in us

const int i2c_eeprom_size[] = {1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};

class UsbDriver
{
private:
#ifdef LIBUSB
    struct usbDev
    {
        libusb_device* dev;           /* as above ... */
        libusb_device_handle* handle; /* as above ... */
        unsigned short vid;           /* Vendor ID */
        unsigned short pid;           /* Product ID */
        unsigned char is_open;        /* When device is opened, val = 1 */
        unsigned char busnum;         /* The bus number of this device */
        unsigned char devaddr;        /* The device address*/
        unsigned char filler;         /* Padding to make struct = 16 bytes */
    };
#endif

public:
    enum UsbCommandId
    {
        SYTEM_RESET = 0x01,
        STREAM_START,
        STREAM_STOP,
        CMOS_REG_SET,
        CMOS_REG_GET,
        LED_SET,
        LED_GET
    };

    struct ControlTransferReply
    {
        UsbCommandId id;
        union {
            bool result;
            unsigned short int value;
        } data;
    };

public:
    UsbDriver(int vid, int pid, int pidCamera, int endpoint, const string& filename);
    ~UsbDriver();
    UsbReturnCode connect();
    UsbReturnCode disconnect();
    UsbReturnCode sendControlCommands(UsbCommandId cid, unsigned char* data, uint16_t length);
    UsbReturnCode readControlResponse(UsbCommandId cid, unsigned char data[8]);
    UsbReturnCode getStream(unsigned char* buffer, int length, int& actualLength);

private:
    shared_ptr<Log> m_log;
    // Initialize the USB device with specified vid/pid.
    UsbReturnCode init(int vid, int pid);
    // Open a handle to the USB device with specified vid/pid.
    UsbReturnCode openUsb(int vid, int pid);
    UsbReturnCode loadFirwmare(const char* filename);
    UsbReturnCode fx3UsbbootDonwload(const char* filename);
#ifdef LIBUSB
    // Get the Vendor ID for the current USB device.
    int getVendor(libusb_device* dev);
    // Get the Product ID for the current USB device.
    int getProduct(libusb_device* dev);
    // Get USB bus number on which the specified device is connected.
    int getBusnumber(libusb_device* dev);
    // Get USB device address assigned to the specified device.
    int getDevaddr(libusb_device* dev);
    // Get a handle to the USB device with specified index.
    libusb_device_handle* getHandle(int index);
    UsbReturnCode updateDevlist();
    UsbReturnCode readFirmwareImage(const char* filename, unsigned char* buf, int* romsize);
    UsbReturnCode ramWrite(unsigned char* buf, unsigned int ramAddress, int len);
#else
    // Initialize CCyUSBDevice
    UsbReturnCode initCyUSBDevice(int vid, int pid);

#endif

private:
    bool m_isConnected;
    int m_vid;
    int m_pid;
    int m_pidCamera;
    int m_endpoint;
    int m_numDevicesDetected;
    int m_timeout;
    string m_filename; /* Firmaware filename */
    int m_filesize;    /* Firwmare image size */
#ifdef LIBUSB
    libusb_device_handle* m_h;
    struct usbDev m_usbDev[MAXDEVICES]; /* List of devices of interest that are connected. */
    int m_nid;                          /* Number of Interesting Devices. */
#else
    CCyFX3Device* m_usbDevice;
    CCyUSBDevice* m_selectedUSBDevice;
#endif
};

} // namespace UsbDriver
} // namespace Libraries
} // namespace Nidek

#endif // __USB_DRIVER_H__
