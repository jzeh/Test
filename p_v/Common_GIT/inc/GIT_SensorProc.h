#ifndef __GIT_SENSORPROC_H__
#define __GIT_SENSORPROC_H__

#include "GIT_Interprotocol.h"
#include "HalHandler.h"

//#define ENABLE_SENSOR_BMP_280

typedef __packed struct __stSensorInfo
{
	BOOL bCalibration;
    int32_t nX;
    int32_t nY;
    int32_t nZ;
}stSensorInfo;


// Error codes
typedef enum{
	NO_ERROR = 0x00, // no error
	ACK_ERROR = 0x01, // no acknowledgment error
	CHECKSUM_ERROR = 0x02, // checksum mismatch error
	TIMEOUT_ERROR = 0x04, // timeout error
	PARM_ERROR = 0x80, // parameter out of range error
} etError;

//************************************************************************************************************************************
// MPU-6515: 자이로 센서
//************************************************************************************************************************************
// See also MPU-6515 Register Map and Descriptions, Revision 4.0, RM-MPU-6515A-00, Rev. 1.4, 9/9/2013 for registers not listed in
// above document; the MPU6515 and MPU9150 are virtually identical but the latter has a different register map
//
//Magnetometer Registers
#define AK8963_ADDRESS   0x0C<<1
#define WHO_AM_I_AK8963  0x00 // should return 0x48
#define INFO             0x01
#define AK8963_ST1       0x02  // data ready status bit 0
#define AK8963_XOUT_L    0x03  // data
#define AK8963_XOUT_H    0x04
#define AK8963_YOUT_L    0x05
#define AK8963_YOUT_H    0x06
#define AK8963_ZOUT_L    0x07
#define AK8963_ZOUT_H    0x08
#define AK8963_ST2       0x09  // Data overflow bit 3 and data read error status bit 2
#define AK8963_CNTL      0x0A  // Power down (0000), single-measurement (0001), self-test (1000) and Fuse ROM (1111) modes on bits 3:0
#define AK8963_ASTC      0x0C  // Self test control
#define AK8963_I2CDIS    0x0F  // I2C disable
#define AK8963_ASAX      0x10  // Fuse ROM x-axis sensitivity adjustment value
#define AK8963_ASAY      0x11  // Fuse ROM y-axis sensitivity adjustment value
#define AK8963_ASAZ      0x12  // Fuse ROM z-axis sensitivity adjustment value

#define SELF_TEST_X_GYRO 0x00
#define SELF_TEST_Y_GYRO 0x01
#define SELF_TEST_Z_GYRO 0x02

/*
#define X_FINE_GAIN      0x03 // [7:0] fine gain
#define Y_FINE_GAIN      0x04
#define Z_FINE_GAIN      0x05
#define XA_OFFSET_H      0x06 // User-defined trim values for accelerometer
#define XA_OFFSET_L_TC   0x07
#define YA_OFFSET_H      0x08
#define YA_OFFSET_L_TC   0x09
#define ZA_OFFSET_H      0x0A
#define ZA_OFFSET_L_TC   0x0B
*/

#define SELF_TEST_X_ACCEL 0x0D
#define SELF_TEST_Y_ACCEL 0x0E
#define SELF_TEST_Z_ACCEL 0x0F

#define SELF_TEST_A      0x10

#define XG_OFFSET_H      0x13  // User-defined trim values for gyroscope
#define XG_OFFSET_L      0x14
#define YG_OFFSET_H      0x15
#define YG_OFFSET_L      0x16
#define ZG_OFFSET_H      0x17
#define ZG_OFFSET_L      0x18
#define SMPLRT_DIV       0x19
#define CONFIG           0x1A
#define GYRO_CONFIG      0x1B
#define ACCEL_CONFIG     0x1C
#define ACCEL_CONFIG2    0x1D
#define LP_ACCEL_ODR     0x1E
#define WOM_THR          0x1F

#define MOT_DUR          0x20  // Duration counter threshold for motion interrupt generation, 1 kHz rate, LSB = 1 ms
#define ZMOT_THR         0x21  // Zero-motion detection threshold bits [7:0]
#define ZRMOT_DUR        0x22  // Duration counter threshold for zero motion interrupt generation, 16 Hz rate, LSB = 64 ms

#define FIFO_EN          0x23
#define I2C_MST_CTRL     0x24
#define I2C_SLV0_ADDR    0x25
#define I2C_SLV0_REG     0x26
#define I2C_SLV0_CTRL    0x27
#define I2C_SLV1_ADDR    0x28
#define I2C_SLV1_REG     0x29
#define I2C_SLV1_CTRL    0x2A
#define I2C_SLV2_ADDR    0x2B
#define I2C_SLV2_REG     0x2C
#define I2C_SLV2_CTRL    0x2D
#define I2C_SLV3_ADDR    0x2E
#define I2C_SLV3_REG     0x2F
#define I2C_SLV3_CTRL    0x30
#define I2C_SLV4_ADDR    0x31
#define I2C_SLV4_REG     0x32
#define I2C_SLV4_DO      0x33
#define I2C_SLV4_CTRL    0x34
#define I2C_SLV4_DI      0x35
#define I2C_MST_STATUS   0x36
#define INT_PIN_CFG      0x37
#define INT_ENABLE       0x38
#define DMP_INT_STATUS   0x39  // Check DMP interrupt
#define INT_STATUS       0x3A
#define ACCEL_XOUT_H     0x3B
#define ACCEL_XOUT_L     0x3C
#define ACCEL_YOUT_H     0x3D
#define ACCEL_YOUT_L     0x3E
#define ACCEL_ZOUT_H     0x3F
#define ACCEL_ZOUT_L     0x40
#define TEMP_OUT_H       0x41
#define TEMP_OUT_L       0x42
#define GYRO_XOUT_H      0x43
#define GYRO_XOUT_L      0x44
#define GYRO_YOUT_H      0x45
#define GYRO_YOUT_L      0x46
#define GYRO_ZOUT_H      0x47
#define GYRO_ZOUT_L      0x48
#define EXT_SENS_DATA_00 0x49
#define EXT_SENS_DATA_01 0x4A
#define EXT_SENS_DATA_02 0x4B
#define EXT_SENS_DATA_03 0x4C
#define EXT_SENS_DATA_04 0x4D
#define EXT_SENS_DATA_05 0x4E
#define EXT_SENS_DATA_06 0x4F
#define EXT_SENS_DATA_07 0x50
#define EXT_SENS_DATA_08 0x51
#define EXT_SENS_DATA_09 0x52
#define EXT_SENS_DATA_10 0x53
#define EXT_SENS_DATA_11 0x54
#define EXT_SENS_DATA_12 0x55
#define EXT_SENS_DATA_13 0x56
#define EXT_SENS_DATA_14 0x57
#define EXT_SENS_DATA_15 0x58
#define EXT_SENS_DATA_16 0x59
#define EXT_SENS_DATA_17 0x5A
#define EXT_SENS_DATA_18 0x5B
#define EXT_SENS_DATA_19 0x5C
#define EXT_SENS_DATA_20 0x5D
#define EXT_SENS_DATA_21 0x5E
#define EXT_SENS_DATA_22 0x5F
#define EXT_SENS_DATA_23 0x60
#define MOT_DETECT_STATUS 0x61
#define I2C_SLV0_DO      0x63
#define I2C_SLV1_DO      0x64
#define I2C_SLV2_DO      0x65
#define I2C_SLV3_DO      0x66
#define I2C_MST_DELAY_CTRL 0x67
#define SIGNAL_PATH_RESET  0x68
#define MOT_DETECT_CTRL  0x69
#define USER_CTRL        0x6A  // Bit 7 enable DMP, bit 3 reset DMP
#define PWR_MGMT_1       0x6B // Device defaults to the SLEEP mode
#define PWR_MGMT_2       0x6C
#define DMP_BANK         0x6D  // Activates a specific bank in the DMP
#define DMP_RW_PNT       0x6E  // Set read/write pointer to a specific start address in specified DMP bank
#define DMP_REG          0x6F  // Register in DMP from which to read or to which to write
#define DMP_REG_1        0x70
#define DMP_REG_2        0x71
#define FIFO_COUNTH      0x72
#define FIFO_COUNTL      0x73
#define FIFO_R_W         0x74
#define WHO_AM_I_MPU6515 0x75 // Should return 0x74, MPU9250은 0x71을 반환함.
#define XA_OFFSET_H      0x77
#define XA_OFFSET_L      0x78
#define YA_OFFSET_H      0x7A
#define YA_OFFSET_L      0x7B
#define ZA_OFFSET_H      0x7D
#define ZA_OFFSET_L      0x7E

// Using the MSENSR-6515 breakout board, ADO is set to 0
// Seven-bit device address is 110100 for ADO = 0 and 110101 for ADO = 1
//mbed uses the eight-bit device address, so shift seven-bit addresses left by one!
#define ADO 0

#if ADO
	#define MPU6515_ADDRESS 0x69  // Device address when ADO = 1
#else
	#define MPU6515_ADDRESS 0x68  // Device address when ADO = 0
#endif

// Set initial input parameters
enum Ascale
{
  AFS_2G = 0,
  AFS_4G,
  AFS_8G,
  AFS_16G
};

enum Gscale
{
  GFS_250DPS = 0,
  GFS_500DPS,
  GFS_1000DPS,
  GFS_2000DPS
};

enum Mscale
{
  MFS_14BITS = 0, // 0.6 mG per LSB
  MFS_16BITS      // 0.15 mG per LSB
};

void MPU6515_initValue(void);
void MPU6515_writeByte(uint8_t address, uint8_t subAddress, uint8_t data);
char MPU6515_readByte(uint8_t address, uint8_t subAddress);
void MPU6515_readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest);
void MPU6515_getMres(void);
void MPU6515_getGres(void);
void MPU6515_getAres(void);
void MPU6515_readAccelData(int16_t * destination);
void MPU6515_readGyroData(int16_t * destination);
void MPU6515_readMagData(int16_t * destination);
int16_t MPU6515_readTempData(void);
void MPU6515_resetMPU6515(void);
void MPU6515_initAK8963(float * destination);
void MPU6515_initMPU6515(void);
void MPU6515_calibrateMPU6515(float * dest1, float * dest2);
void MPU6515_MPU6515SelfTest(float * destination);
void MPU6515_MadgwickQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);
void MPU6515_MahonyQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);

int MPU6515_Test(void);

void MPU6515_SetMotionThreshold(char threshold);
void MPU6515_ConfigurationWakeUpOnMotionInterrupt(boolean_t bWomActive, boolean_t bGyroActive, unsigned char ucValue);
bool MPU6515_CheckInterrupt();

//************************************************************************************************************************************
// 센서 체크 manager 함수 관련
//************************************************************************************************************************************

typedef enum _eSENSOR_STATE
{
	eSENSOR_STATE_INIT = 0,
	eSENSOR_STATE_WAIT,
	eSENSOR_STATE_WHO_AM_I_MPU6515,				// 자이로 센서
	eSENSOR_STATE_RESET_MPU6515,				// 자이로 센서
	eSENSOR_STATE_INIT_MPU6515,					// 자이로 센서
	eSENSOR_STATE_GET_SENSITIVITY_MPU6515,		// 자이로 센서
	eSENSOR_STATE_DEFINE_DEFAULT_ANGLE,
	eSENSOR_STATE_GET_DATA_MPU6515,
	eSENSOR_STATE_PAUSE,
	eSENSOR_STATE_SLEEP_ALL_SENSOR,
	eSENSOR_STATE_IDLE,
} eSENSOR_STATE;

typedef __packed struct _SENSOR_DATA {
	eSENSOR_STATE g_eSensorState;

	uint8_t g_bEnableSensor_MPU6515;
	uint8_t g_bEnableSensor_MXG2320;

#ifdef				ENABLE_SENSOR_BMP_280
	uint8_t g_bEnableSensor_BMP280;
#endif

	float g_fMXG2320CoefficientX;
	float g_fMXG2320CoefficientY;
	float g_fMXG2320CoefficientZ;

	float g_fMXG2320AdjustedMeasurementDataX;
	float g_fMXG2320AdjustedMeasurementDataY;
	float g_fMXG2320AdjustedMeasurementDataZ;

	int g_iTimerSensorDly;
	int g_iTimerDataRateMPU6515;
    int g_iTimerReportDly;

	float aRes;      // scale resolutions per LSB for the sensors
	float gRes;      // scale resolutions per LSB for the sensors

	uint8_t Ascale;     // AFS_2G, AFS_4G, AFS_8G, AFS_16G
	uint8_t Gscale; // GFS_250DPS, GFS_500DPS, GFS_1000DPS, GFS_2000DPS
	uint8_t Mscale; // MFS_14BITS or MFS_16BITS, 14-bit or 16-bit magnetometer resolution
	uint8_t Mmode;        // Either 8 Hz 0x02) or 100 Hz (0x06) magnetometer data ODR

	//*****************************************************************
	// 센서값을 저장해 둔다.
	//*****************************************************************
	float g_fMXG2320Adjusted_uTX;
	float g_fMXG2320Adjusted_uTY;
	float g_fMXG2320Adjusted_uTZ;

	float fAccX;
	float fAccY;
	float fAccZ;

	float fGyroX;
	float fGyroY;
	float fGyroZ;

	int32_t nAngleX;
	int32_t nAngleY;
	int32_t nAngleZ;

	float fTemperature_MPU6515;
	float fPressure;

    stSensorInfo stGyroAngleDefault;

    uint8_t ucVehicleMode;
#ifdef ENABLE_SENSOR_BMP_280
	float fTemperature_BMP280;
#endif
	//*****************************************************************
} SENSOR_DATA;

extern SENSOR_DATA SensorData;

void InitializeSensorManager(void);
void GIT_SensorManager(void);
extern volatile unsigned long g_ul16timer_ms;

// External Functions List
void SetEvt_MngSensor(int lparam, int rparam);
void GetEvt_MngSensor(int lparam, int rparam);

//export angle 
void GetGyroAngle(stSensorInfo* pstGyroAngle);
void GetGyroNavieAngle(stSensorInfo* pstGyroAngle);
uint8_t MPU6515_GetName(void);

#endif
