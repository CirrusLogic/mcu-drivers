# Bridge Instructions

The MCU Driver SDK now supports including an implementation of WISCE protocol over MCU UART, allowing control of supported Cirrus Logic devices from WISCE.  The Bridge implementation also includes enumerating of a virtual device ("vregmap") that can allow the developer to drive use-case transitions or other functionality from WISCE.  WISCE requires XML files copied to particular folders in the filesystem in order to correctly enumerate systems that include both a Cirrus Logic device and the virtual device.  The instructions included here outline where to copy the XML files in the filesystem.

# Using WISCE
For connecting to the Bridge implementation via WISCE, the following steps must be followed:

 1. Copy the file below:
 \<mcu-drivers repo\>/common/bridge/bridge_wisce_device.xml
 to
  \<WISCE Install\>/Devices/bridge_wisce_device.xml
 2. Build the desired example project with the make argument 'CONFIG_USE_VREGMAP=1'
 3. Flash and run the compiled binary
 4. In a terminal session, run \<mcu-drivers repo\>/tools/bridge_agent/run_bridge.py
 5. Open WISCE - if a bridge implementation is running, it will automatically connect
 6. Click on the device marked 'Unknown', and then click on 'Device' -> 'Properties...'
 7. Click on 'Load' and navigate to the bridge_wisce_device.xml in the WISCE install
 8. Click Accept
