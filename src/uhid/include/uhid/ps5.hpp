#pragma once

#include <linux/uhid.h>
#include <vector>

namespace uhid {

/*
 * Ranges taken from https://github.com/nondebug/dualsense
 */
static constexpr int PS5_AXIS_MIN = 0;
static constexpr int PS5_AXIS_MAX = 0xFF;
static constexpr int PS5_AXIS_NEUTRAL = 0x80;

/*
 * DualSense hardware limits
 */
static constexpr int PS5_ACC_RES_PER_G = 8192;
static constexpr int PS5_ACC_RANGE = (4 * PS5_ACC_RES_PER_G);
static constexpr int PS5_GYRO_RES_PER_DEG_S = 1024;
static constexpr int PS5_GYRO_RANGE = (2048 * PS5_GYRO_RES_PER_DEG_S);
static constexpr int PS5_TOUCHPAD_WIDTH = 1920;
static constexpr int PS5_TOUCHPAD_HEIGHT = 1080;
static constexpr float SDL_STANDARD_GRAVITY = 9.80665f;

/**
 * Taken from: https://github.com/nondebug/dualsense/blob/main/report-descriptor-usb.txt
 * and verified manually using hid-decode
 */
static constexpr unsigned char ps5_rdesc[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,       // Usage (Game Pad)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       //   Report ID (1)
    0x09, 0x30,       //   Usage (X)
    0x09, 0x31,       //   Usage (Y)
    0x09, 0x32,       //   Usage (Z)
    0x09, 0x35,       //   Usage (Rz)
    0x09, 0x33,       //   Usage (Rx)
    0x09, 0x34,       //   Usage (Ry)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x06,       //   Report Count (6)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF, //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x20,       //   Usage (0x20)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,       //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x39,       //   Usage (Hat switch)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x07,       //   Logical Maximum (7)
    0x35, 0x00,       //   Physical Minimum (0)
    0x46, 0x3B, 0x01, //   Physical Maximum (315)
    0x65, 0x14,       //   Unit (System: English Rotation, Length: Centimeter)
    0x75, 0x04,       //   Report Size (4)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x42,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    0x65, 0x00,       //   Unit (None)
    0x05, 0x09,       //   Usage Page (Button)
    0x19, 0x01,       //   Usage Minimum (0x01)
    0x29, 0x0F,       //   Usage Maximum (0x0F)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x0F,       //   Report Count (15)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF, //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x21,       //   Usage (0x21)
    0x95, 0x0D,       //   Report Count (13)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF, //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x22,       //   Usage (0x22)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x34,       //   Report Count (52)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x02,       //   Report ID (2)
    0x09, 0x23,       //   Usage (0x23)
    0x95, 0x2F,       //   Report Count (47)
    0x91, 0x02,       //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x05,       //   Report ID (5)
    0x09, 0x33,       //   Usage (0x33)
    0x95, 0x28,       //   Report Count (40)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x08,       //   Report ID (8)
    0x09, 0x34,       //   Usage (0x34)
    0x95, 0x2F,       //   Report Count (47)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x09,       //   Report ID (9)
    0x09, 0x24,       //   Usage (0x24)
    0x95, 0x13,       //   Report Count (19)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x0A,       //   Report ID (10)
    0x09, 0x25,       //   Usage (0x25)
    0x95, 0x1A,       //   Report Count (26)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x20,       //   Report ID (32)
    0x09, 0x26,       //   Usage (0x26)
    0x95, 0x3F,       //   Report Count (63)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x21,       //   Report ID (33)
    0x09, 0x27,       //   Usage (0x27)
    0x95, 0x04,       //   Report Count (4)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x22,       //   Report ID (34)
    0x09, 0x40,       //   Usage (0x40)
    0x95, 0x3F,       //   Report Count (63)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x80,       //   Report ID (-128)
    0x09, 0x28,       //   Usage (0x28)
    0x95, 0x3F,       //   Report Count (63)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x81,       //   Report ID (-127)
    0x09, 0x29,       //   Usage (0x29)
    0x95, 0x3F,       //   Report Count (63)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x82,       //   Report ID (-126)
    0x09, 0x2A,       //   Usage (0x2A)
    0x95, 0x09,       //   Report Count (9)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x83,       //   Report ID (-125)
    0x09, 0x2B,       //   Usage (0x2B)
    0x95, 0x3F,       //   Report Count (63)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x84,       //   Report ID (-124)
    0x09, 0x2C,       //   Usage (0x2C)
    0x95, 0x3F,       //   Report Count (63)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x85,       //   Report ID (-123)
    0x09, 0x2D,       //   Usage (0x2D)
    0x95, 0x02,       //   Report Count (2)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xA0,       //   Report ID (-96)
    0x09, 0x2E,       //   Usage (0x2E)
    0x95, 0x01,       //   Report Count (1)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xE0,       //   Report ID (-32)
    0x09, 0x2F,       //   Usage (0x2F)
    0x95, 0x3F,       //   Report Count (63)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF0,       //   Report ID (-16)
    0x09, 0x30,       //   Usage (0x30)
    0x95, 0x3F,       //   Report Count (63)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF1,       //   Report ID (-15)
    0x09, 0x31,       //   Usage (0x31)
    0x95, 0x3F,       //   Report Count (63)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF2,       //   Report ID (-14)
    0x09, 0x32,       //   Usage (0x32)
    0x95, 0x0F,       //   Report Count (15)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF4,       //   Report ID (-12)
    0x09, 0x35,       //   Usage (0x35)
    0x95, 0x3F,       //   Report Count (63)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF5,       //   Report ID (-11)
    0x09, 0x36,       //   Usage (0x36)
    0x95, 0x03,       //   Report Count (3)
    0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,             // End Collection
};

enum PS5_REPORT_TYPES : unsigned int {
  CALIBRATION = 0x05,
  PAIRING_INFO = 0x09,
  FIRMWARE_INFO = 0x20
};

static constexpr unsigned char ps5_calibration_info[] = {
    0x05,
    0x00, // gyro_pitch_bias
    0x00,
    0x00, // gyro_yaw_bias
    0x00,
    0x00, // gyro_roll_bias
    0x00,
    0x10, // gyro_pitch_plus
    0x27,
    0xF0, // gyro_pitch_minus
    0xD8,
    0x10, // gyro_yaw_plus
    0x27,
    0xF0, // gyro_yaw_minus
    0xD8,
    0x10, // gyro_roll_plus
    0x27,
    0xF0, // gyro_roll_minus
    0xD8,
    0xF4, // gyro_speed_plus
    0x01,
    0xF4, // gyro_speed_minus
    0x01,
    0x10, // acc_x_plus
    0x27,
    0xF0, // acc_x_minus
    0xD8,
    0x10, // acc_y_plus
    0x27,
    0xF0, // acc_y_minus
    0xD8,
    0x10, // acc_z_plus
    0x27,
    0xF0, // acc_z_minus
    0xD8, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static constexpr int gyro_calib_bias = 0;

/**
 * see:
 * https://github.com/torvalds/linux/blob/fa4b851b4ad632dc673627f38a8a552547568a2c/drivers/hid/hid-playstation.c#L997
 * speed_2x = (gyro_speed_plus + gyro_speed_minus);
 * sens_numer = speed_2x * PS5_GYRO_RES_PER_DEG_S
 */
static constexpr int gyro_calib_sens_numer = (ps5_calibration_info[19] + ps5_calibration_info[21]) *
                                             PS5_GYRO_RES_PER_DEG_S;

/**
 * see:
 * https://github.com/torvalds/linux/blob/fa4b851b4ad632dc673627f38a8a552547568a2c/drivers/hid/hid-playstation.c#L998-L999
 * sens_denom = abs(gyro_pitch_plus - gyro_pitch_bias) + abs(gyro_pitch_minus - gyro_pitch_bias);
 */
static constexpr int gyro_calib_pitch_denom = ps5_calibration_info[7] - ps5_calibration_info[1] +
                                              ps5_calibration_info[9] - ps5_calibration_info[1];

/**
 * see:
 * https://github.com/torvalds/linux/blob/fa4b851b4ad632dc673627f38a8a552547568a2c/drivers/hid/hid-playstation.c#L1004-L1005
 * sens_denom = abs(gyro_yaw_plus - gyro_yaw_bias) + abs(gyro_yaw_minus - gyro_yaw_bias);
 */
static constexpr int gyro_calib_yaw_denom = ps5_calibration_info[11] - ps5_calibration_info[3] +
                                            ps5_calibration_info[13] - ps5_calibration_info[3];

/**
 * see:
 * https://github.com/torvalds/linux/blob/fa4b851b4ad632dc673627f38a8a552547568a2c/drivers/hid/hid-playstation.c#L1010-L1011
 * sens_denom = abs(gyro_roll_plus - gyro_roll_bias) + abs(gyro_roll_minus - gyro_roll_bias);
 */
static constexpr int gyro_calib_roll_denom = ps5_calibration_info[15] - ps5_calibration_info[5] +
                                             ps5_calibration_info[17] - ps5_calibration_info[5];

/**
 * see:
 * https://github.com/torvalds/linux/blob/fa4b851b4ad632dc673627f38a8a552547568a2c/drivers/hid/hid-playstation.c#L1036
 * range_2g = acc_x_plus - acc_x_minus;
 * sens_denom = range_2g;
 */
static constexpr int acc_calib_denom = ps5_calibration_info[23] - ps5_calibration_info[25];

/**
 * see:
 * https://github.com/torvalds/linux/blob/fa4b851b4ad632dc673627f38a8a552547568a2c/drivers/hid/hid-playstation.c#L1034
 * range_2g = acc_x_plus - acc_x_minus;
 * bias = acc_x_plus - range_2g / 2
 */
static constexpr int acc_calib_x_bias = ps5_calibration_info[23] - acc_calib_denom / 2;

/**
 * see:
 * https://github.com/torvalds/linux/blob/fa4b851b4ad632dc673627f38a8a552547568a2c/drivers/hid/hid-playstation.c#L1035
 * sens_numer = 2*DS_ACC_RES_PER_G;
 */
static constexpr int acc_calib_sens_numer = 2 * PS5_ACC_RES_PER_G;

/**
 * see:
 * https://github.com/torvalds/linux/blob/fa4b851b4ad632dc673627f38a8a552547568a2c/drivers/hid/hid-playstation.c#L1040
 * bias = acc_y_plus - range_2g / 2;
 */
static constexpr int acc_calib_y_bias = ps5_calibration_info[27] - acc_calib_denom / 2;

/**
 * see:
 * https://github.com/torvalds/linux/blob/fa4b851b4ad632dc673627f38a8a552547568a2c/drivers/hid/hid-playstation.c#L1046
 * bias = acc_z_plus - range_2g / 2;
 */
static constexpr int acc_calib_z_bias = ps5_calibration_info[31] - acc_calib_denom / 2;

static constexpr unsigned char ps5_firmware_info[] = {
    0x20, 0x4A, 0x75, 0x6E, 0x20, 0x31, 0x39, 0x20, 0x32, 0x30, 0x32, 0x33, 0x31, 0x34, 0x3A, 0x34,
    0x37, 0x3A, 0x33, 0x34, 0x03, 0x00, 0x44, 0x00, 0x08, 0x02, 0x00, 0x01, 0x36, 0x00, 0x00, 0x01,
    0xC1, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x01, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static constexpr unsigned char ps5_pairing_info[] = {
    0x09, 0x74, 0xE7, 0xD6, 0x3A, 0x53, 0x35, 0x08, 0x25, 0x00,
    0x1E, 0x00, 0xEE, 0x74, 0xD0, 0xBC, 0x00, 0x00, 0x00, 0x00,
};

struct dualsense_touch_point {
  /*
   * Status of a DualShock4 touch point contact.
   * Contact IDs, with highest bit set are 'inactive'
   * and any associated data is then invalid.
   */
  uint8_t contact;
  uint8_t x_lo;
  uint8_t x_hi : 4, y_lo : 4;
  uint8_t y_hi;
};

/*
 * Button masks for DualSense input report.
 * see:
 * https://github.com/torvalds/linux/blob/005f6f34bd47eaa61d939a2727fc648e687b84c1/drivers/hid/hid-playstation.c#L90-L106
 */
enum DS_BUTTONS0 : uint8_t {
  HAT_SWITCH = 0x07, // First 3 bits are HAT
  SQUARE = 0x10,
  CROSS = 0x20,
  CIRCLE = 0x40,
  TRIANGLE = 0x80
};

enum DS_BUTTONS1 : uint8_t {
  L1 = 0x01,
  R1 = 0x02,
  L2 = 0x04,
  R2 = 0x08,
  CREATE = 0x10,
  OPTIONS = 0x20,
  L3 = 0x40,
  R3 = 0x80
};

enum DS_BUTTONS2 : uint8_t {
  PS_HOME = 0x01,
  TOUCHPAD = 0x02,
  MIC_MUTE = 0x04
};

enum HAT_STATES : uint8_t {
  HAT_NEUTRAL = 0x8,
  HAT_N = 0x0,
  HAT_NE = 0x1,
  HAT_E = 0x2,
  HAT_SE = 0x3,
  HAT_S = 0x4,
  HAT_SW = 0x5,
  HAT_W = 0x6,
  HAT_NW = 0x7
};

struct dualsense_input_report_usb {
  uint8_t report_id = 0x01;
  uint8_t x, y = PS5_AXIS_NEUTRAL;   // LS
  uint8_t rx, ry = PS5_AXIS_NEUTRAL; // RS
  uint8_t z, rz = PS5_AXIS_NEUTRAL;  // L2, R2
  uint8_t seq_number = 0;
  // HAT_SWITCH is neutral when 0x8 is reported
  uint8_t buttons[4] = {HAT_NEUTRAL, 0, 0, 0};
  uint8_t reserved[4] = {0, 0, 0, 0};

  /* Motion sensors */
  __le16 gyro[3] = {0, 0, 0};  /* x, y, z */
  __le16 accel[3] = {0, 0, 0}; /* x, y, z */
  __le32 sensor_timestamp = 0;
  uint8_t reserved2 = 0;

  /* Touchpad */
  struct dualsense_touch_point points[2] = {};

  uint8_t reserved3 = 0;
  uint8_t r2_adaptive_trigger = 0;
  uint8_t l2_adaptive_trigger = 0;
  uint8_t reserved4[9] = {};

  uint8_t battery_charge : 4;
  uint8_t battery_status : 4;
  uint8_t battery2 = 0x0c;
  uint8_t reserved6[9] = {};
};

enum FLAG0 : uint8_t {
  MOTOR_OR_COMPATIBLE_VIBRATION = 0x01,
  LED_OR_HAPTIC_SELECT = 0x02,
  LED_BLINK = 0x04
};

enum FLAG1 : uint8_t {
  MIC_MUTE_LED_ENABLE = 0x01,
  POWER_SAVE_ENABLE = 0x02,
  LIGHTBAR_ENABLE = 0x04,
  RELEASE_LEDS = 0x08,
  PLAYER_INDICATOR_ENABLE = 0x10
};

enum FLAG2 : uint8_t {
  LIGHTBAR_SETUP_ENABLE = 0x02,
  COMPATIBLE_VIBRATION = 0x04
};

struct dualsense_output_report_usb {
  uint8_t report_id; // 0x02 for USB

  uint8_t valid_flag0; // see enum FLAG0
  uint8_t valid_flag1; // see enum FLAG1

  /* For DualShock 4 compatibility mode. */
  uint8_t motor_right;
  uint8_t motor_left;

  /* Audio controls */
  uint8_t reserved[4];
  uint8_t mute_button_led;

  uint8_t power_save_control;
  uint8_t reserved2[28];

  /* LEDs and lightbar */
  uint8_t valid_flag2; // see enum FLAG2
  uint8_t reserved3[2];
  uint8_t lightbar_setup;
  uint8_t led_brightness;
  uint8_t player_leds;
  uint8_t lightbar_red;
  uint8_t lightbar_green;
  uint8_t lightbar_blue;

  uint8_t reserved4[15];
};
} // namespace uhid