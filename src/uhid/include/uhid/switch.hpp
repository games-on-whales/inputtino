#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace uhid {

static constexpr uint8_t SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY = 0x21;
static constexpr uint8_t SWITCH_INPUT_REPORT_STANDARD_FULL = 0x30;
static constexpr uint8_t SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND = 0x01;
static constexpr uint8_t SWITCH_OUTPUT_REPORT_RUMBLE_ONLY = 0x10;

static constexpr uint8_t SWITCH_REPORT_MODE_STANDARD_FULL = 0x30;

static constexpr uint8_t SWITCH_SUBCMD_ENABLE_VIBRATION = 0x48;
static constexpr uint8_t SWITCH_SUBCMD_ENABLE_IMU = 0x40;
static constexpr uint8_t SWITCH_SUBCMD_SET_INPUT_REPORT_MODE = 0x03;
static constexpr uint8_t SWITCH_SUBCMD_REQ_DEV_INFO = 0x02;
static constexpr uint8_t SWITCH_SUBCMD_SPI_FLASH_READ = 0x10;
static constexpr uint8_t SWITCH_SUBCMD_SET_PLAYER_LIGHTS = 0x30;
static constexpr uint8_t SWITCH_SUBCMD_GET_PLAYER_LIGHTS = 0x31;
static constexpr uint8_t SWITCH_SUBCMD_SET_HOME_LIGHT = 0x38;

static constexpr uint8_t SWITCH_ACK = 0x80;
static constexpr uint8_t SWITCH_ACK_DEV_INFO = 0x82;
static constexpr uint8_t SWITCH_ACK_SPI_FLASH_READ = 0x90;

static constexpr uint8_t JOYCON_CTLR_TYPE_JCL = 0x01;
static constexpr uint8_t JOYCON_CTLR_TYPE_JCR = 0x02;
static constexpr uint8_t JOYCON_CTLR_TYPE_PRO = 0x03;

static constexpr uint16_t JOYCON_PID_LEFT = 0x2006;
static constexpr uint16_t JOYCON_PID_RIGHT = 0x2007;
static constexpr uint16_t JOYCON_PID_PRO = 0x2009;

static constexpr uint32_t JC_CAL_USR_LEFT_MAGIC_ADDR = 0x8010;
static constexpr uint32_t JC_CAL_USR_RIGHT_MAGIC_ADDR = 0x801B;
static constexpr uint32_t JC_CAL_FCT_DATA_LEFT_ADDR = 0x603D;
static constexpr uint32_t JC_CAL_FCT_DATA_RIGHT_ADDR = 0x6046;
static constexpr uint32_t JC_IMU_CAL_FCT_DATA_ADDR = 0x6020;
static constexpr uint32_t JC_IMU_CAL_USR_MAGIC_ADDR = 0x8026;

// 12-bit stick axis range: full range 0..4095, natural center at 2048.
static constexpr int SWITCH_AXIS_MAX = 4095;
static constexpr int SWITCH_AXIS_CENTER = 2048;
static constexpr float SWITCH_STANDARD_GRAVITY_CONST = 9.80665f;
static constexpr int16_t SWITCH_ACCEL_SCALE = 4096;
static constexpr int16_t SWITCH_GYRO_SCALE = 16;

// IMU calibration: identity offsets so hid-nintendo passes raw values through unchanged.
static constexpr int16_t SWITCH_IMU_ACCEL_OFFSET = 0;
static constexpr int16_t SWITCH_IMU_ACCEL_SCALE_CAL = 16384;
static constexpr int16_t SWITCH_IMU_GYRO_OFFSET = 0;
static constexpr int16_t SWITCH_IMU_GYRO_SCALE_CAL = 13371;

// Bluetooth report descriptor for Nintendo Switch Pro Controller.
static constexpr unsigned char switch_rdesc_bt[] = {
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x85, 0x30, 0x05, 0x01, 0x05, 0x09, 0x19, 0x01, 0x29, 0x0A, 0x15, 0x00, 0x25,
    0x01, 0x75, 0x01, 0x95, 0x0A, 0x55, 0x00, 0x65, 0x00, 0x81, 0x02, 0x05, 0x09, 0x19, 0x0B, 0x29, 0x0E, 0x15, 0x00,
    0x25, 0x01, 0x75, 0x01, 0x95, 0x04, 0x81, 0x02, 0x75, 0x01, 0x95, 0x02, 0x81, 0x03, 0x0B, 0x01, 0x00, 0x01, 0x00,
    0xA1, 0x00, 0x0B, 0x30, 0x00, 0x01, 0x00, 0x0B, 0x31, 0x00, 0x01, 0x00, 0x0B, 0x32, 0x00, 0x01, 0x00, 0x0B, 0x35,
    0x00, 0x01, 0x00, 0x15, 0x00, 0x27, 0xFF, 0x0F, 0x00, 0x00, 0x75, 0x10, 0x95, 0x04, 0x81, 0x02, 0xC0, 0x0B, 0x39,
    0x00, 0x01, 0x00, 0x15, 0x00, 0x25, 0x07, 0x35, 0x00, 0x46, 0x3B, 0x01, 0x65, 0x14, 0x75, 0x04, 0x95, 0x01, 0x81,
    0x42, 0x65, 0x00, 0x05, 0x09, 0x19, 0x0F, 0x29, 0x12, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x04, 0x81, 0x02,
    0x75, 0x08, 0x95, 0x34, 0x81, 0x03, 0x06, 0x00, 0xFF, 0x85, 0x21, 0x09, 0x01, 0x75, 0x08, 0x95, 0x3F, 0x81, 0x03,
    0x85, 0x81, 0x09, 0x02, 0x75, 0x08, 0x95, 0x3F, 0x81, 0x03, 0x85, 0x01, 0x09, 0x03, 0x75, 0x08, 0x95, 0x3F, 0x91,
    0x83, 0x85, 0x10, 0x09, 0x04, 0x75, 0x08, 0x95, 0x3F, 0x91, 0x83, 0x85, 0x80, 0x09, 0x05, 0x75, 0x08, 0x95, 0x3F,
    0x91, 0x83, 0x85, 0x82, 0x09, 0x06, 0x75, 0x08, 0x95, 0x3F, 0x91, 0x83, 0xC0};

#pragma pack(push, 1)

// Output report sent by the host to the controller
struct switch_rumble_data {
  uint8_t freq_high; // high-band frequency
  uint8_t amp_high;  // high-band amplitude, encoded in [0x00, 0xC8]
  uint8_t freq_low;  // low-band frequency; bit 7 is the low-amp half-step flag
  uint8_t amp_low;   // low-band amplitude (lower byte)
};

// Report 0x10: rumble data only (no subcommand)
struct switch_output_report_rumble_only {
  uint8_t report_id; // SWITCH_OUTPUT_REPORT_RUMBLE_ONLY (0x10)
  uint8_t packet_number;
  switch_rumble_data left;
  switch_rumble_data right;
};
static_assert(sizeof(switch_output_report_rumble_only) == 10);

// Report 0x01: rumble data + subcommand (subcmd_data follows immediately after this struct)
struct switch_output_report_rumble_subcmd {
  uint8_t report_id; // SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND (0x01)
  uint8_t packet_number;
  switch_rumble_data left;
  switch_rumble_data right;
  uint8_t subcmd_id;
};
static_assert(sizeof(switch_output_report_rumble_subcmd) == 11);

struct switch_standard_input_prefix {
  uint8_t report_id;
  uint8_t timer;
  uint8_t bat_con;
  uint8_t button_status[3];
  uint8_t left_stick[3];
  uint8_t right_stick[3];
  uint8_t vibrator_input;
};

struct switch_imu_sample {
  int16_t accel[3];
  int16_t gyro[3];
};

struct switch_input_report {
  switch_standard_input_prefix prefix;
  switch_imu_sample imu[3];
};

struct switch_subcmd_reply_report {
  switch_standard_input_prefix prefix;
  uint8_t ack;
  uint8_t subcmd_id;
  uint8_t data[35];
};
#pragma pack(pop)

} // namespace uhid
