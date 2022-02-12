import usb.core
import usb.util


# Sensor driver parameters
SENSOR_VID    = 0x5a9
SENSOR_PID    = 0x8066
SENSOR_IN_EP  = 0x81
SENSOR_OUT_EP = 0x02

SENSOR_CMD_RESET = 0x6a300005


# List USB devices
devices = usb.core.show_devices()
print("USB devices:")
print(devices)

# Find the sensor
dev = usb.core.find(idVendor=SENSOR_VID, idProduct=SENSOR_PID)
if not dev:
    print("ERROR: HTT sensor not connected.")
    exit(1)
else:
    print("HTT sensor has been found.")

# Get first configuration
cfg = dev.configurations()[0]

# Get first interface
itf = cfg.interfaces()[0]

# Get the available endpoints of first (unique) interface
endpoints = itf.endpoints()
# Find the expected endpoints
in_ep  = next(x for x in endpoints if x.bEndpointAddress == SENSOR_IN_EP)
out_ep = next(x for x in endpoints if x.bEndpointAddress == SENSOR_OUT_EP)

#



