#ifndef LP8_MODBUS_H
#define LP8_MODBUS_H

// Documentation:
// https://rmtplusstoragesenseair.blob.core.windows.net/
//                                              docs/Dev/publicerat/TDE2712.pdf

/////////////////////////////////////////////////////////////
/// CRC (Cyclic Redundancy  Check) Calculation Parameters ///
/////////////////////////////////////////////////////////////

// ModBus CRC Polynom (x^16 + x^15 + x^2 + 1)
// The CRC16_MB_POLYNOM's LSB corresponds to the HIGHEST grade
// 0xA001 = 0b1010000000000001
// 0X8005 = 0b1000000000000101 (mirrored!)
const uint16_t CRC16_MB_POLYNOM = 0xA001;

// ModBus CRC initialization value
const uint16_t CRC16_MB_INIT = 0xFFFF;

////////////////////////
/// MODBUS addresses ///
////////////////////////

const uint8_t MODBUS_ADDRESS_LP8 = 0x68; // LP8 sensor MODBUS slave address;
const uint8_t MODBUS_ADDRESS_ALL = 0xFE; // MODBUS address 'all devices';


///////////////////////////////////////////////////////////
/// Funtion Codes for MODBUS ADU (Application Data Unit)///
///////////////////////////////////////////////////////////

enum class LP8_MODBUS_FUNC_CODE : uint8_t
{
  // LP8 MODBUS READ function code is strangely 0x44
  // (standard ANALOG INPUT REGISTER READ is 0x04?)
  READ = 0x44,
  // LP8 MODBUS WRITE function code is strangely 0x41
  // (standard ANALOG INPUT REGISTER WRITE SINGLE is 0x06?)
  WRITE = 0x41,
  // LP8 MODBUS WRITE Response Error code
  ERROR_WRITE = 0xC1,
  // LP8 MODBUS Response Error code
  ERROR_READ = 0xC4,
};

/////////////////////////
/// LP8 RAM Addresses ///
/////////////////////////

enum class LP8_RAM_ADDRESS : uint8_t
{
  // calculation control (1 byte)
  // Writing this parameter selects the operation mode and measurement function
  // for this measurement period for the LP8 sensor module.
  CALC_CONTROL = 0x80,

  // sensor state (23 bytes)
  // Proprietary structure for LP8 internal data; such as input to the
  // noise-suppression filter algorithm and calibration references. This has to
  // be passed and saved in the host retention memory during shutdown for use
  // in the next LP8 measurement.
  SENSOR_STATE_1  = 0x81,
  SENSOR_STATE_2  = 0x82,
  SENSOR_STATE_3  = 0x83,
  SENSOR_STATE_4  = 0x84,
  SENSOR_STATE_5  = 0x85,
  SENSOR_STATE_6  = 0x86,
  SENSOR_STATE_7  = 0x87,
  SENSOR_STATE_8  = 0x88,
  SENSOR_STATE_9  = 0x89,
  SENSOR_STATE_10 = 0x8a,
  SENSOR_STATE_11 = 0x8b,
  SENSOR_STATE_12 = 0x8c,
  SENSOR_STATE_13 = 0x8d,
  SENSOR_STATE_14 = 0x8e,
  SENSOR_STATE_15 = 0x8f,
  SENSOR_STATE_16 = 0x90,
  SENSOR_STATE_17 = 0x91,
  SENSOR_STATE_18 = 0x92,
  SENSOR_STATE_19 = 0x93,
  SENSOR_STATE_20 = 0x94,
  SENSOR_STATE_21 = 0x95,
  SENSOR_STATE_22 = 0x96,
  SENSOR_STATE_23 = 0x97,

  // host pressure (2 bytes) [dekaPascal]
  // If application host is measuring ambient pressure, writing this current
  // value to LP8 will update internal pressure correction algorithms and allow
  // reading out pressure corrected measurement values. If ambient pressure is
  // not measured by host, then this parameter can be skipped as LP8 will
  // assume normal sea level pressure of 10124 (1012.4 hPa).
  HOST_PRESSURE_HI = 0x98,
  HOST_PRESSURE_LO = 0x99,

  // raw CO2 concentration [ppm]
  // CO2 concentration without pressure-correction or any noise-suppression
  // filtering.
  CO2_CONC_HI = 0x9A,
  CO2_CONC_LO = 0x9B,

  // Pressure-corrected CO2 concentration value without noise-suppression
  // filtering. [ppm]
  CO2_CONC_PRES_HI = 0x9C,
  CO2_CONC_PRES_LO = 0x9D,

  // The measured LP8 sensor temperature from NTC.
  SPACE_TEMP_HI = 0x9E,
  SPACE_TEMP_LO = 0x9F,

  // VCAP voltage measured by sensor prior lamp pulse, indicator of battery
  // voltage [mV] (unsigned uint16_t)
  VCAP1_HI = 0xA0,
  VCAP1_LO = 0xA1,

  // VCAP voltage measured by sensor during lamp pulse, to ensure sufficient
  // energy [mV] (unsigned uint16_t)
  VCAP2_HI = 0xA2,
  VCAP2_LO = 0xA3,

  // Error status
  ERROR3 = 0xA4,
  ERROR2 = 0xA5,
  ERROR1 = 0xA6,
  ERROR0 = 0xA7,

  // Noise-filtered CO2 concentration without pressure-correction. [ppm]
  CO2_CONC_FILT_HI = 0xA8,
  CO2_CONC_FILT_LO = 0xA9,

  // Pressure-corrected filtered concentration value. [ppm]
  CO2_CONC_FILT_PRES_HI = 0xAA,
  CO2_CONC_FILT_PRES_LO = 0xAB,

  // Unused byte space
  UNUSED_1 = 0xAC,
  UNUSED_2 = 0xAD,
  UNUSED_3 = 0xAE,
  UNUSED_4 = 0xAF
};

const uint8_t LP8_RAM_SIZE = (1 + \
  (uint8_t)LP8_RAM_ADDRESS::UNUSED_4 - \
  (uint8_t)LP8_RAM_ADDRESS::CALC_CONTROL);

template<class TYPE>
inline uint8_t
LP8_REL_RAM_ADDRESS(const TYPE adr)
{
  return (uint8_t) adr - (uint8_t) LP8_RAM_ADDRESS::CALC_CONTROL;
}

////////////////////////////////////////////////
/// control bytes for LP8 RAM address 0x0080 ///
////////////////////////////////////////////////

enum class LP8_MODBUS_CONTROL_BYTE : uint8_t
{
  // initial measurement
  // Used only when a previous sensor state is unavailable; e.g. after a fresh
  // boot-up or after previous recalibrations where the noise-suppression
  // filters have been reset.
  INITIAL_MEASUREMENT = 0x10,

  // sequential measurement
  // The default operating mode, reusing previous sensor states for noise
  // suppression and passing background calibration references.
  SEQUENTIAL_MEASUREMENT = 0x20,

  // zero-calibration with noisy, non-filtered CO2
  // Requires a stable ambient environment free from any CO2, i.e.
  // concentration is 0 ppm. Makes a new measurement and sets a new baseline
  // offset from the noisy and non-filtered internal signal reference.
  ZERO_CALIB_NOISY_NONFILTERED = 0x40,

  // zero-calibration with noise-filtered CO2
  // Requires a stable ambient environment free from any CO2, i.e.
  // concentration is 0 ppm. Makes a new measurement and sets a new baseline
  // offset from the noise-filtered internal signal reference from the new and
  // old measurement.
  ZERO_CALIB_NOISE_FILTERED = 0x41,

  // zero-calibration with noisy, non-filtered CO2 + reset noise filter
  // Requires a stable ambient environment free from any CO2, i.e.
  // concentration is 0 ppm.  Makes a new measurement and sets a new baseline
  // offset from the noisy and non-filtered internal signal reference.  It then
  // overwrites the internal historic noise parameter references set after this
  // measurement to be the same as the current measured raw value.
  ZERO_CALIB_NOISY_NONFILTERED_RESET = 0x42,

  // zero-calibration with noise-filtered CO2 + reset noise filter
  // Requires a stable ambient environment free from any CO2, i.e.
  // concentration is 0 ppm. Makes a new measurement and sets a new baseline
  // offset from the noise-filtered internal signal reference. It then
  // overwrites the internal historic noise parameter references set after this
  // measurement to be the same as the current measured raw value.
  ZERO_CALIB_NOISE_FILTERED_RESET = 0x43,

  // background-calibration with noisy, non-filtered CO2
  // Requires a stable ambient environment in fresh air, i.e. concentration is
  // near-400 ppm. LP8 makes a new measurement and sets a new baseline offset
  // by the difference between the noisy and non- filtered (but
  // pressure-compensated) internal signal reference and the predefined fresh
  // air reference.
  BACK_CALIB_NOISY_NONFILTERED = 0x50,

  // background-calibration with noise-filtered CO2
  // Requires a stable ambient environment in fresh air, i.e. concentration is
  // near-400 ppm. LP8 makes a new measurement and sets a new baseline offset
  // from the difference between the noise-filtered (but pressure-compensated)
  // internal signal reference and the predefined fresh air reference.
  BACK_CALIB_NOISE_FILTERED = 0x51,

  // background-calibration with noisy, non-filtered CO2 + reset noise filter
  // Requires a stable ambient environment in fresh air, i.e. concentration is
  // near-400 ppm. LP8 makes a new measurement and sets a new baseline offset
  // from the difference between the noisy and non- filtered (but
  // pressure-compensated) internal signal reference and the predefined fresh
  // air reference.  It then overwrites the internal historic noise parameter
  // references set after this measurement to be the same as the current
  // measured raw value.
  BACK_CALIB_NOISY_NONFILTERED_RESET = 0x52,

  // background-calibration with noise-filtered CO2 + reset noise filter
  // Requires a stable ambient environment in fresh air, i.e. concentration is
  // near-400 ppm. LP8 makes a new measurement and sets a new baseline offset
  // by the difference between the noise-filtered (pressure-compensated)
  // internal signal reference and the predefined fresh air reference. It then
  // overwrites the internal historic noise parameter references set after this
  // measurement to be the same as the current measured raw value.
  BACK_CALIB_NOISE_FILTERED_RESET = 0x53,

  // automatic baseline correction with noise-filtered CO2
  // Informs the LP8 to perform an ABC calibration based on the stored baseline
  // reference continuously passed back and forth within the sensor states. A
  // new offset is set based from the stored reference value in Sensor State
  // and its assumed correlation to 400ppm.
  ABC_NOISE_FILTERED = 0x70,

  // automatic baseline correction with noise-filtered CO2
  // Informs the LP8 to perform an ABC calibration based on the stored baseline
  // reference continuously passed back and forth within the sensor states. A
  // new offset is set based from the stored reference value in Sensor State
  // and its assumed correlation to 400ppm. It then overwrites the internal
  // historic noise parameter reference set after this measurement to be the
  // same as the current measured raw value.
  ABC_NOISE_FILTERED_RESET = 0x71,
};

enum class LP8_ERROR0_BITS : uint8_t
{
  WarmUp = 7,
  Memory = 6,
  OutOfRange = 5,
  SelfDiag = 4,
  CalibrationError = 3,
  AlgError = 2,
  Reserved = 1,
  FatalError = 0,
};

enum class LP8_ERROR1_BITS : uint8_t
{
  ParameterOverride3 = 7,
  ParameterOverride2 = 6,
  ParameterOverride1 = 5,
  ParameterOverride0 = 4,
  DetectSig = 3,
  ADCError = 2,
  VCAP2low = 1,
  VCAP1low = 0,
};

enum class LP8_ERROR2_BITS : uint8_t
{
  Reserved3 = 7,
  Reserved2 = 6,
  Reserved1 = 5,
  Reserved0 = 4,
  UnfilteredPressureCorrectOOR = 3,
  UnfilteredTableEntryOOR = 2,
  UnfilteredTempCompOOR = 1,
  UnfilteredIRSignalOOR = 0,
};

enum class LP8_ERROR3_BITS : uint8_t
{
  Reserved3 = 7,
  Reserved2 = 6,
  Reserved1 = 5,
  Reserved0 = 4,
  FilteredPressureCorrectOOR = 3,
  FilteredTableEntryOOR = 2,
  FilteredTempCompOOR = 1,
  FilteredIRSignalOOR = 0,
};

#endif
