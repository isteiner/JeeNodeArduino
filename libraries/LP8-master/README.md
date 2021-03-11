# LP8 Arduino library

This library provides a class to interface the LP8 CO2 sensor.

## Capabilities

- communication with the LP8 sensor via any serial interface: `HardwareSerial`
  (i.e. `Serial`, `Serial1`, ...), `SoftwareSerial`, `AltSoftSerial`
  (untested), ...
- automatic initialisation and handling of the whole MODBUS communication
- convenient access to all the information the LP8 CO2 sensor can provide:
    - raw CO2 concentration
    - pressure-corrected CO2-concentration (if pressure is available)
    - noise-filtered CO2 concentration
    - pressure-corrected and noise-filtered CO2 concentration
    - LP8 sensor temperature
    - sensor diagnostics

## Example

See the example under the `examples` folder.

## Debugging

You can get the library to talk about every action it takes by setting the
compiler flag `LP8_DEBUG` (e.g. set `build_flags = -DLP8_DEBUG` if you are
using PlatformIO, I don't know how to do this in the Arduino IDE directly, but
you can always just do a `#define LP8_DEBUG` in the `LP8.h` header file to
accomplish this.)
