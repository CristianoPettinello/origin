#include "UsbDriver.h"
#include <fcntl.h>
#include <iomanip>  // std::setfill, std::setw
#include <iostream> // std::cout, std::endl
#include <sstream>  // std::stringstream
#ifdef _WIN32
#include "windows.h"
#include <io.h>
#define usleep(x) Sleep(x / 1000)
#else
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <sys/stat.h>

using namespace std;
using namespace Nidek::Libraries::UsbDriver;

static const string tag = "[USBD] ";

#ifndef LIBUSB
string m_FWDWNLOAD_ERROR_MSG[] = {"SUCCESS",
                                  "FAILED",
                                  "INVALID_MEDIA_TYPE",
                                  "INVALID_FWSIGNATURE",
                                  "DEVICE_CREATE_FAILED",
                                  "INCORRECT_IMAGE_LENGTH",
                                  "INVALID_FILE",
                                  "SPI_FLASH_ERASE_FAILED",
                                  "CORRUPT_FIRMWARE_IMAGE_FILE",
                                  "I2C_EEPROM_UNKNOWN_SIZE"};
#endif

UsbDriver::UsbDriver(int vid, int pid, int pidCamera, int endpoint, const string& filename)
    : m_log(Log::getInstance()),
      m_isConnected(false),
      m_vid(vid),
      m_pid(pid),
      m_pidCamera(pidCamera),
      m_endpoint(endpoint),
      m_numDevicesDetected(0),
      //   m_timeout(35)
      m_timeout(100)

{
    m_filename = filename;
    connect();
}

UsbDriver::~UsbDriver()
{
    cout << "~UsbDriver()" << endl;
}

UsbReturnCode UsbDriver::connect()
{
    m_log->debug(tag + "UsbDriver::connect()");
    UsbReturnCode r = init(m_vid, m_pid);
    if (r == UsbReturnCode::SUCCESS)
    {
        r = loadFirwmare(m_filename.c_str());
        if (r == UsbReturnCode::SUCCESS)
        {
            usleep(REBOOT_DELAY_US);
#ifdef LIBUSB
            r = init(m_vid, m_pidCamera);
#else
            r = initCyUSBDevice(m_vid, m_pidCamera);
#endif
        } else
            m_log->error(__FILE__, __LINE__, tag + "Error: " + to_string(r));
    } else
    {
        if (r == UsbReturnCode::LIBUSB_DEVICE_NOT_FOUND)
#ifdef LIBUSB
            r = init(m_vid, m_pidCamera);
#else
            r = initCyUSBDevice(m_vid, m_pidCamera);
#endif
        else
            m_log->error(__FILE__, __LINE__, tag + "Error: " + to_string(r));
    }
    return r;
}

UsbReturnCode UsbDriver::disconnect()
{
    m_log->debug(tag + "UsbDriver::disconnect()");
#ifdef LIBUSB
// TODO: rilasciare tutte le interfacce
#else
    if (m_usbDevice)
    {
        m_usbDevice->Close();
        delete m_usbDevice;
    }

    if (m_selectedUSBDevice)
    {
        m_selectedUSBDevice->Close();
        delete m_selectedUSBDevice;
    }

#endif

    return UsbReturnCode::SUCCESS;
}

UsbReturnCode UsbDriver::sendControlCommands(UsbCommandId cid, unsigned char* data, uint16_t length)
{
    // TODO check length < 32
    stringstream ss;
    ss << "UsbDriver::sendControlCommands cid:" << cid << " size: " << length;
    if (length)
    {
        ss << " data:";
        for (int i = 0; i < length; ++i)
            ss << " 0x" << setfill('0') << setw(2) << hex << static_cast<int>(data[i]);
    }
    m_log->debug(tag + ss.str());

    // FIXME
    // list_endpoints(libusb_get_device(h));

#ifdef LIBUSB
    // bmRequestType [0x40]
    // Bits 0:4 determine recipient
    // Bits 5:6 determine type
    // Bit 7 determines data transfer direction
    int bmRequestType = libusb_endpoint_direction::LIBUSB_ENDPOINT_OUT |
                        libusb_request_type::LIBUSB_REQUEST_TYPE_VENDOR |
                        libusb_request_recipient::LIBUSB_RECIPIENT_DEVICE;

    int r = libusb_control_transfer(m_h, bmRequestType, cid, 0x0000, 0x0000, data, length, VENDORCMD_TIMEOUT);

    if (r == length)
        return SUCCESS;
    else
        return LIBUSB_GENERIC_ERROR; // FIXME
#else
    CCyControlEndPoint* ept = m_selectedUSBDevice->ControlEndPt;
    if (!ept)
        return UsbReturnCode::LIBUSB_DEVICE_NOT_FOUND;

    // Send a vendor request (bRequest = cid) to the device
    ept->Target = TGT_DEVICE;
    ept->ReqType = REQ_VENDOR;
    ept->Direction = DIR_TO_DEVICE;
    ept->ReqCode = cid;
    ept->Value = 0;
    ept->Index = 0;

    LONG l = length;
    ept->TimeOut = VENDORCMD_TIMEOUT;
    bool r = ept->XferData(data, l);

    return (r) ? UsbReturnCode::SUCCESS : UsbReturnCode::LIBUSB_GENERIC_ERROR;
#endif
}

UsbReturnCode UsbDriver::readControlResponse(UsbCommandId cid, unsigned char data[8])
{
#ifdef LIBUSB
    // bmRequestType [0xC0]
    // Bits 0:4 determine recipient
    // Bits 5:6 determine type
    // Bit 7 determines data transfer direction
    int bmRequestType = libusb_endpoint_direction::LIBUSB_ENDPOINT_IN |
                        libusb_request_type::LIBUSB_REQUEST_TYPE_VENDOR |
                        libusb_request_recipient::LIBUSB_RECIPIENT_DEVICE;

    int r = libusb_control_transfer(m_h, bmRequestType, cid, 0x0000, 0x0000, data, sizeof(data), VENDORCMD_TIMEOUT);

    if (r < 0)
        return LIBUSB_GENERIC_ERROR;
    if (r == 0)
        return FW_PROTOCOL_ERROR;
    else
    {
        stringstream ss;
        for (int i = 0; i < r; ++i)
        {
            ss << " 0x" << setfill('0') << setw(2) << hex << static_cast<int>(data[i]);
        }
        m_log->debug(tag + ss.str());
    }
    return SUCCESS;
#else
    CCyControlEndPoint* ept = m_selectedUSBDevice->ControlEndPt;
    if (!ept)
        return UsbReturnCode::LIBUSB_DEVICE_NOT_FOUND;

    // Send a vendor request (bRequest = cid) to the device
    ept->Target = TGT_DEVICE;
    ept->ReqType = REQ_VENDOR;
    ept->Direction = DIR_FROM_DEVICE;
    ept->ReqCode = cid;
    ept->Value = 0;
    ept->Index = 0;

    LONG l = sizeof(data);
    bool r = ept->XferData(data, l);

    if (r)
    {
        stringstream ss;
        for (int i = 0; i < l; ++i)
        {
            ss << " 0x" << setfill('0') << setw(2) << hex << static_cast<int>(data[i]);
        }
        m_log->debug(tag + ss.str());
        return UsbReturnCode::SUCCESS;
    } else
        return FW_PROTOCOL_ERROR;
#endif
}

UsbReturnCode UsbDriver::getStream(unsigned char* buffer, int length, int& actualLength)
{
#ifdef LIBUSB
    actualLength = 0;
    int r = libusb_bulk_transfer(m_h, m_endpoint, buffer, length, &actualLength, m_timeout /*VENDORCMD_TIMEOUT*/);
    // m_log->info(tag + libusb_error_name(r) + " len: " + to_string(actualLength));
    // if (r)
    // {
    //     // libusb_clear_halt(m_h, m_endpoint);
    //     // sendControlCommands(UsbDriver::UsbCommandId::STREAM_START, nullptr, 0);
    //     m_timeout = 100;
    // } else
    //     m_timeout = 35;
    return r == 0 ? SUCCESS : FW_BULK_TRANSFER_ERROR;
#else
    CCyUSBEndPoint* epBulkIn = m_selectedUSBDevice->EndPointOf(m_endpoint);
    // Update timeout, default value 10 sec
    epBulkIn->TimeOut = m_timeout;
    // m_log->info(to_string(epBulkIn->TimeOut));

    LONG l = length;
    bool r = epBulkIn->XferData((UCHAR*)buffer, l);
    actualLength = (int)l;
    // if (!r)
    // {
    //     m_log->debug(tag + ">>>>>>>>>>>>>>< RESET <<<<<<<<<<<<<<<<<");
    //     epBulkIn->Reset();
    // }

    return (r) ? UsbReturnCode::SUCCESS : FW_BULK_TRANSFER_ERROR;
#endif
}

UsbReturnCode UsbDriver::init(int vid, int pid)
{
    m_log->debug(tag + "UsbDriver::init()");
    UsbReturnCode r = openUsb(vid, pid);
#ifdef LIBUSB
    if (r == UsbReturnCode::SUCCESS)
    {
        int ret = libusb_claim_interface(m_h, 0);
        switch (ret)
        {
        case 0:
            r = UsbReturnCode::SUCCESS;
            break;
        case libusb_error::LIBUSB_ERROR_NOT_FOUND:
            r = UsbReturnCode::LIBUSB_INTERFACE_DOES_NOT_EXIST;
            break;
        case libusb_error::LIBUSB_ERROR_BUSY:
            r = UsbReturnCode::LIBUSB_INTERFACE_BUSY;
            break;
        case libusb_error::LIBUSB_ERROR_NO_DEVICE:
            r = UsbReturnCode::LIBUSB_DEVICE_NOT_FOUND;
            break;
        default:
            r = UsbReturnCode::LIBUSB_GENERIC_ERROR;
            break;
        }
    }
#endif

    return r;
}

UsbReturnCode UsbDriver::openUsb(int vid, int pid)
{
    {
        stringstream ss;
        ss << "UsbDriver::openUsb() ";
        ss << "VID=0x" << setfill('0') << setw(4) << hex << vid;
        ss << ",PID=0x" << setfill('0') << setw(4) << hex << pid;
        m_log->debug(tag + ss.str());
    }

#ifdef LIBUSB

    m_h = nullptr;

    int r = libusb_init(nullptr);
    if (r)
    {
        m_log->error(__FILE__, __LINE__, tag + "Error in initializing libusb library...");
        return LIBUSB_INIT_FAILED;
    }

    m_h = libusb_open_device_with_vid_pid(NULL, vid, pid);
    if (!m_h)
    {
        m_log->error(__FILE__, __LINE__, tag + "Device not found");
        return LIBUSB_DEVICE_NOT_FOUND;
    }

    libusb_device* dev = libusb_get_device(m_h);
    m_usbDev[0].dev = dev;
    m_usbDev[0].handle = m_h;
    m_usbDev[0].vid = getVendor(dev);
    m_usbDev[0].pid = getProduct(dev);
    m_usbDev[0].is_open = 1;
    m_usbDev[0].busnum = getBusnumber(dev);
    m_usbDev[0].devaddr = getDevaddr(dev);
    m_nid = 1;
    m_numDevicesDetected = 1;
    m_isConnected = true;
    return SUCCESS;
#else
    bool flag = false;
    m_usbDevice = new CCyFX3Device();
    if (m_usbDevice == NULL)
    {
        m_log->error(__FILE__, __LINE__, tag + "Failed to create USB Device");
        return LIBUSB_INIT_FAILED; // FIXME: update enum
    }

    for (int i = 0; i < m_usbDevice->DeviceCount(); i++)
    {
        if (m_usbDevice->Open((UCHAR)i))
        {
            if ((m_usbDevice->VendorID == vid) && (m_usbDevice->ProductID == pid))
            {
                /* We have to look for a device that supports the firmware download vendor commands. */
                if (m_usbDevice->IsBootLoaderRunning())
                {

                    m_log->info("Info : Found FX3 USB BootLoader");
                    flag = true;
                    break;
                }
            }

            m_usbDevice->Close();
        }
    }

    if (flag == false)
    {
        m_log->error(__FILE__, __LINE__, tag + "FX3 USB BootLoader Device not found");

        if (m_usbDevice)
            delete m_usbDevice;
        return LIBUSB_DEVICE_NOT_FOUND; // FIXME
    }

    return SUCCESS;
#endif
}

UsbReturnCode UsbDriver::loadFirwmare(const char* filename)
{
    m_log->debug(tag + "UsbDriver::loadFirwmare: " + filename);
    return fx3UsbbootDonwload(filename);
}

UsbReturnCode UsbDriver::fx3UsbbootDonwload(const char* filename)
{
#ifdef LIBUSB
    unsigned char* fwBuf;
    unsigned int* data_p;
    unsigned int i, checksum;
    int index;

    fwBuf = (unsigned char*)calloc(1, MAX_FWIMG_SIZE);
    if (fwBuf == 0)
    {
        m_log->error(__FILE__, __LINE__, tag + "Failed to allocate buffer to store firmware binary");
        return FW_CANNOT_ALLOCATE_MEMORY;
    }

    // Read the firmware image into the local RAM buffer.
    UsbReturnCode r = readFirmwareImage(filename, fwBuf, NULL);
    if (r != UsbReturnCode::SUCCESS)
    {
        m_log->error(__FILE__, __LINE__, tag + "Failed to read firmware file " + string(filename));
        free(fwBuf);
        return r;
    }

    // Run through each section of code, and use vendor commands to download them to RAM.
    index = 4;
    checksum = 0;
    while (index < m_filesize)
    {
        data_p = (unsigned int*)(fwBuf + index);
        int length = data_p[0];
        int address = data_p[1];
        if (length != 0)
        {
            for (i = 0; i < length; i++)
                checksum += data_p[2 + i];
            r = ramWrite(fwBuf + index + 8, address, length * 4);
            if (r != UsbReturnCode::SUCCESS)
            {
                m_log->error(__FILE__, __LINE__, tag + "Failed to download data to FX3 RAM");
                free(fwBuf);
                return r;
            }
        } else
        {
            if (checksum != data_p[2])
            {
                m_log->error(__FILE__, __LINE__, tag + "Checksum error in firmware binary");
                free(fwBuf);
                return UsbReturnCode::FW_CHECKSUM_ERROR;
            }

            int ret = libusb_control_transfer(m_h, 0x40, 0xA0, GET_LSW(address), GET_MSW(address), NULL, 0,
                                              VENDORCMD_TIMEOUT);
            if (ret != 0)
            {
                m_log->debug(tag + "Ignored error in control transfer: " + to_string(ret));
            }
            break;
        }

        index += (8 + length * 4);
    }

    free(fwBuf);
    return UsbReturnCode::SUCCESS;
#else
    char* filepath = NULL;

    if (filename == NULL)
    {
        m_log->error(__FILE__, __LINE__, tag + "Firmware binary not specified");
        return UsbReturnCode::FW_FILE_NOT_FOUND;
    }

    FILE* fw_img_p = NULL;
    fopen_s(&fw_img_p, filename, "rb");
    filepath = (char*)filename;

    if (fw_img_p == NULL)
    {
        m_log->error(__FILE__, __LINE__, tag + "Failed to open file");
        return UsbReturnCode::FW_NOT_A_NORMAL_FW_BINARY;
    } else
    {
        /* Find the length of the image */
        fseek(fw_img_p, 0, SEEK_END);
        m_filesize = ftell(fw_img_p);
    }

    if (m_filesize <= 0)
    {
        fclose(fw_img_p);
        m_log->error(__FILE__, __LINE__, tag + "Invalid file with length = ZERO");
        return UsbReturnCode::FW_FILE_SIZE_EXCEEDS_MAXIMUM; // FIXME
    } else
    {
        fclose(fw_img_p);
    }

    FX3_FWDWNLOAD_ERROR_CODE dwld_status = m_usbDevice->DownloadFw(filepath, RAM);

    if (dwld_status == SUCCESS)
    {
        m_log->info("Programming completed");
    } else
    {
        std::stringstream sstr;
        sstr.str("");
        sstr << "\nError: Firmware download failed unexpectedly with error code: "
             << m_FWDWNLOAD_ERROR_MSG[dwld_status];
        m_log->error(__FILE__, __LINE__, tag + sstr.str());
        return UsbReturnCode::FW_PROTOCOL_ERROR; // FIXME
    }
    return UsbReturnCode::SUCCESS;
#endif
}

#ifdef LIBUSB
int UsbDriver::getVendor(libusb_device* dev)
{
    struct libusb_device_descriptor d;
    libusb_get_device_descriptor(dev, &d);
    return d.idVendor;
}
int UsbDriver::getProduct(libusb_device* dev)
{
    struct libusb_device_descriptor d;
    libusb_get_device_descriptor(dev, &d);
    return d.idProduct;
}
int UsbDriver::getBusnumber(libusb_device* dev)
{
    return libusb_get_bus_number(dev);
}
int UsbDriver::getDevaddr(libusb_device* dev)
{
    return libusb_get_device_address(dev);
}
libusb_device_handle* UsbDriver::getHandle(int index)
{
    return m_usbDev[index].handle;
}

UsbReturnCode UsbDriver::updateDevlist()
{
    m_log->debug(tag + "UsbDriver::updateDevlist()");
    int i, index = 0;
    struct libusb_config_descriptor* config_desc = NULL;
    libusb_device_handle* h = nullptr;

    for (i = 0; i < m_numDevicesDetected; ++i)
    {
        // Get a handle to the USB device with specified index.
        h = getHandle(i);
        libusb_device* tdev = libusb_get_device(h);

        stringstream ss;
        ss << "VID=0x" << setfill('0') << setw(4) << hex << getVendor(tdev);
        ss << ",PID=0x" << setfill('0') << setw(4) << hex << getProduct(tdev);
        ss << ",BusNum=0x" << setfill('0') << setw(2) << hex << getBusnumber(tdev);
        ss << ",Addr=" << getDevaddr(tdev);
        m_log->debug(tag + ss.str());

        int r = libusb_get_active_config_descriptor(tdev, &config_desc);
        if (r)
        {
            m_log->error(__FILE__, __LINE__, tag + "Error in 'get_active_config_descriptor'");
            return UsbReturnCode::LIBUSB_GET_ACTIVE_CONFIG_DESCRIPTOR_FAILED;
        }
        int num_interfaces = config_desc->bNumInterfaces;
        while (num_interfaces)
        {
            if (libusb_kernel_driver_active(h, index))
            {
                libusb_detach_kernel_driver(h, index);
            }
            index++;
            num_interfaces--;
        }
        libusb_free_config_descriptor(config_desc);
    }
    return SUCCESS;
}

UsbReturnCode UsbDriver::readFirmwareImage(const char* filename, unsigned char* buf, int* romsize)
{
    m_log->debug(tag + "UsbDriver::readFirmwareImage");
    int fd;
    int nbr;
    struct stat filestat;

    // Verify that the file size does not exceed our limits.
    if (stat(filename, &filestat) != 0)
    {
        m_log->error(__FILE__, __LINE__, tag + "Failed to stat file " + string(filename));
        return FW_FAILED_STAT_FILE;
    }

    m_filesize = filestat.st_size;
    if (m_filesize > MAX_FWIMG_SIZE)
    {
        m_log->error(__FILE__, __LINE__, tag + "File size exceeds maximum firmware image size");
        return FW_FILE_SIZE_EXCEEDS_MAXIMUM;
    }

    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        m_log->error(__FILE__, __LINE__, tag + "File not found");
        return FW_FILE_NOT_FOUND;
    }
    nbr = read(fd, buf, 2); /* Read first 2 bytes, must be equal to 'CY'	*/
    if ((nbr != 2) || (strncmp((char*)buf, "CY", 2)))
    {
        m_log->error(__FILE__, __LINE__, tag + "Image does not have 'CY' at start. aborting");
        return FW_IMAGE_DOES_NOT_HAVE_CY;
    }
    nbr = read(fd, buf, 1); /* Read 1 byte. bImageCTL	*/
    if ((nbr != 1) || (buf[0] & 0x01))
    {
        m_log->error(__FILE__, __LINE__, tag + "Image does not contain executable code");
        return FW_IMAGE_DOES_NOT_CONTAIN_EXEC_CODE;
    }
    if (romsize != 0)
        *romsize = i2c_eeprom_size[(buf[0] >> 1) & 0x07];

    nbr = read(fd, buf, 1); /* Read 1 byte. bImageType	*/
    if ((nbr != 1) || !(buf[0] == 0xB0))
    {
        m_log->error(__FILE__, __LINE__, tag + "Not a normal FW binary with checksum");
        return FW_NOT_A_NORMAL_FW_BINARY;
    }

    // Read the complete firmware binary into a local buffer.
    lseek(fd, 0, SEEK_SET);
    /*nbr = */ read(fd, buf, m_filesize);

    close(fd);
    return SUCCESS;
}

UsbReturnCode UsbDriver::ramWrite(unsigned char* buf, unsigned int ramAddress, int len)
{
    m_log->debug(tag + "UsbDriver::ramWrite");
    int index = 0;
    while (len > 0)
    {
        int size = (len > MAX_WRITE_SIZE) ? MAX_WRITE_SIZE : len;
        int r = libusb_control_transfer(m_h, 0x40, 0xA0, GET_LSW(ramAddress), GET_MSW(ramAddress), &buf[index], size,
                                        VENDORCMD_TIMEOUT);
        if (r != size)
        {
            m_log->error(__FILE__, __LINE__, tag + "Vendor write to FX3 RAM failed");
            return FW_WRITE_TO_FX3_RAM_FAILED;
        }

        ramAddress += size;
        index += size;
        len -= size;
    }

    return SUCCESS;
}

#else
UsbReturnCode UsbDriver::initCyUSBDevice(int vid, int pid)
{
    {
        stringstream ss;
        ss << "UsbDriver::openUsb() ";
        ss << "VID=0x" << setfill('0') << setw(4) << hex << vid;
        ss << ",PID=0x" << setfill('0') << setw(4) << hex << pid;
        m_log->debug(tag + ss.str());
    }

    m_selectedUSBDevice = new CCyUSBDevice(nullptr);
    if (m_selectedUSBDevice == nullptr)
    {
        m_log->error(__FILE__, __LINE__, tag + "Failed to create USB Device");
        return UsbReturnCode::LIBUSB_INIT_FAILED; // FIXME: update enum
    }

    bool flag = false;
    for (int i = 0; i < m_selectedUSBDevice->DeviceCount(); i++)
    {
        if (m_selectedUSBDevice->Open((UCHAR)i))
        {
            if ((m_selectedUSBDevice->VendorID == vid) && (m_selectedUSBDevice->ProductID == pid))
            {
                m_log->info(tag + "CyUSB device found");
                flag = true;
                break;
            }

            m_selectedUSBDevice->Close();
        }
    }

    if (flag == false)
    {
        m_log->error(__FILE__, __LINE__, tag + "CyUSB Device not found");

        if (m_selectedUSBDevice)
            delete m_selectedUSBDevice;
        return UsbReturnCode::LIBUSB_DEVICE_NOT_FOUND; // FIXME
    }

    return UsbReturnCode::SUCCESS;
}
#endif
