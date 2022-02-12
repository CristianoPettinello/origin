//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __USB_RETURN_CODE_H__
#define __USB_RETURN_CODE_H__

namespace Nidek {
namespace Libraries {
namespace UsbDriver {

enum UsbReturnCode
{
    SUCCESS = 0,
    LIBUSB_INIT_FAILED,
    LIBUSB_GENERIC_ERROR,
    LIBUSB_DEVICE_NOT_FOUND,
    LIBUSB_INTERFACE_DOES_NOT_EXIST,
    LIBUSB_INTERFACE_BUSY,
    LIBUSB_CONTROL_TRANSFER_FAILED,
    LIBUSB_GET_ACTIVE_CONFIG_DESCRIPTOR_FAILED,
    LIBUSB_ERROR_NO_MEM,
    FW_CANNOT_ALLOCATE_MEMORY,
    FW_FAILED_STAT_FILE,
    FW_FILE_SIZE_EXCEEDS_MAXIMUM,
    FW_FILE_NOT_FOUND,
    FW_IMAGE_DOES_NOT_HAVE_CY,
    FW_IMAGE_DOES_NOT_CONTAIN_EXEC_CODE,
    FW_NOT_A_NORMAL_FW_BINARY,
    FW_WRITE_TO_FX3_RAM_FAILED,
    FW_CHECKSUM_ERROR,
    FW_PROTOCOL_ERROR,
    FW_BULK_TRANSFER_ERROR
};

} // namespace UsbDriver
} // namespace Libraries
} // namespace Nidek

#endif // __USB_RETURN_CODE_H__
