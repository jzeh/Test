/*----------------------------------------------------------------------
 *   Ethernet Control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_SENSOR_H__
#define	__GIT_SENSOR_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "i2c.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
typedef enum {
    GYRO_SENSOR_UNKNOWN,
    GYRO_SENSOR_MPU6515,
    GYRO_SENSOR_IIM42652
} SensorType;

/* ------------------------------ MPU6515 Defines ------------------------------ */

// MPU-6515 Register
#define MPU6515_ID_ADDRESS								0x68

// Magnetometer Registers
#define	MPU_6515_SELF_TEST_X_GYRO						0x00
#define	MPU_6515_SELF_TEST_Y_GYRO						0x01
#define	MPU_6515_SELF_TEST_Z_GYRO						0x02
/*
#define	X_FINE_GAIN								0x03
#define	Y_FINE_GAIN								0x04
#define	Z_FINE_GAIN								0x05
#define	XA_OFFSET_H								0x06
#define	XA_OFFSET_L_TC							0x07
#define	YA_OFFSET_H								0x08
#define	YA_OFFSET_L_TC							0x09
#define	ZA_OFFSET_H								0x0A
#define	ZA_OFFSET_L_TC							0x0B
*/
#define	MPU6515_SELF_TEST_X_ACCEL						0x0D
#define	MPU6515_SELF_TEST_Y_ACCEL						0x0E
#define	MPU6515_SELF_TEST_Z_ACCEL						0x0F

#define	MPU6515_SELF_TEST_A								0x10

#define	MPU6515_XG_OFFSET_H								0x13
#define	MPU6515_XG_OFFSET_L								0x14
#define	MPU6515_YG_OFFSET_H								0x15
#define	MPU6515_YG_OFFSET_L								0x16
#define	MPU6515_ZG_OFFSET_H								0x17
#define	MPU6515_ZG_OFFSET_L								0x18
#define	MPU6515_SMPLRT_DIV								0x19
#define	MPU6515_CONFIG									0x1A
#define	MPU6515_GYRO_CONFIG								0x1B
#define	MPU6515_ACCEL_CONFIG							0x1C
#define	MPU6515_ACCEL_CONFIG2							0x1D
#define	MPU6515_LP_ACCEL_ODR							0x1E
#define	MPU6515_WOM_THR									0x1F

#define	MPU6515_MOT_DUR									0x20
#define	MPU6515_ZMOT_THR								0x21
#define	MPU6515_ZRMOT_DUR								0x22

#define	MPU6515_FIFO_EN									0x23
#define	MPU6515_I2C_MST_CTRL							0x24
#define	MPU6515_I2C_SLV0_ADDR							0x25
#define	MPU6515_I2C_SLV0_REG							0x26
#define	MPU6515_I2C_SLV0_CTRL							0x27
#define	MPU6515_I2C_SLV1_ADDR							0x28
#define	MPU6515_I2C_SLV1_REG							0x29
#define	MPU6515_I2C_SLV1_CTRL							0x2A
#define	MPU6515_I2C_SLV2_ADDR							0x2B
#define	MPU6515_I2C_SLV2_REG							0x2C
#define	MPU6515_I2C_SLV2_CTRL							0x2D
#define	MPU6515_I2C_SLV3_ADDR							0x2E
#define	MPU6515_I2C_SLV3_REG							0x2F
#define	MPU6515_I2C_SLV3_CTRL							0x30
#define	MPU6515_I2C_SLV4_ADDR							0x31
#define	MPU6515_I2C_SLV4_REG							0x32
#define	MPU6515_I2C_SLV4_DO								0x33
#define	MPU6515_I2C_SLV4_CTRL							0x34
#define	MPU6515_I2C_SLV4_DI								0x35
#define	MPU6515_I2C_MST_STATUS							0x36
#define	MPU6515_INT_PIN_CFG								0x37
#define	MPU6515_INT_ENABLE								0x38
#define	MPU6515_DMP_INT_STATUS							0x39
#define	MPU6515_INT_STATUS								0x3A
#define	MPU6515_ACCEL_XOUT_H							0x3B
#define	MPU6515_ACCEL_XOUT_L							0x3C
#define	MPU6515_ACCEL_YOUT_H							0x3D
#define	MPU6515_ACCEL_YOUT_L							0x3E
#define	MPU6515_ACCEL_ZOUT_H							0x3F
#define	MPU6515_ACCEL_ZOUT_L							0x40
#define	MPU6515_TEMP_OUT_H								0x41
#define	MPU6515_TEMP_OUT_L								0x42
#define	MPU6515_GYRO_XOUT_H								0x43
#define	MPU6515_GYRO_XOUT_L								0x44
#define	MPU6515_GYRO_YOUT_H								0x45
#define	MPU6515_GYRO_YOUT_L								0x46
#define	MPU6515_GYRO_ZOUT_H								0x47
#define	MPU6515_GYRO_ZOUT_L								0x48
#define	MPU6515_EXT_SENS_DATA_00						0x49
#define	MPU6515_EXT_SENS_DATA_01						0x4A
#define	MPU6515_EXT_SENS_DATA_02						0x4B
#define	MPU6515_EXT_SENS_DATA_03						0x4C
#define	MPU6515_EXT_SENS_DATA_04						0x4D
#define	MPU6515_EXT_SENS_DATA_05						0x4E
#define	MPU6515_EXT_SENS_DATA_06						0x4F
#define	MPU6515_EXT_SENS_DATA_07						0x50
#define	MPU6515_EXT_SENS_DATA_08						0x51
#define	MPU6515_EXT_SENS_DATA_09						0x52
#define	MPU6515_EXT_SENS_DATA_10						0x53
#define	MPU6515_EXT_SENS_DATA_11						0x54
#define	MPU6515_EXT_SENS_DATA_12						0x55
#define	MPU6515_EXT_SENS_DATA_13						0x56
#define	MPU6515_EXT_SENS_DATA_14						0x57
#define	MPU6515_EXT_SENS_DATA_15						0x58
#define	MPU6515_EXT_SENS_DATA_16						0x59
#define	MPU6515_EXT_SENS_DATA_17						0x5A
#define	MPU6515_EXT_SENS_DATA_18						0x5B
#define	MPU6515_EXT_SENS_DATA_19						0x5C
#define	MPU6515_EXT_SENS_DATA_20						0x5D
#define	MPU6515_EXT_SENS_DATA_21						0x5E
#define	MPU6515_EXT_SENS_DATA_22						0x5F
#define	MPU6515_EXT_SENS_DATA_23						0x60
#define	MPU6515_MOT_DETECT_STATUS						0x61
#define	MPU6515_I2C_SLV0_DO								0x63
#define	MPU6515_I2C_SLV1_DO								0x64
#define	MPU6515_I2C_SLV2_DO								0x65
#define	MPU6515_I2C_SLV3_DO								0x66
#define	MPU6515_I2C_MST_DELAY_CTRL						0x67
#define	MPU6515_SIGNAL_PATH_RESET						0x68
#define	MPU6515_MOT_DETECT_CTRL							0x69
#define	MPU6515_USER_CTRL								0x6A
#define	MPU6515_PWR_MGMT_1								0x6B
#define	MPU6515_PWR_MGMT_2								0x6C
#define	MPU6515_DMP_BANK								0x6D
#define	MPU6515_DMP_RW_PNT								0x6E
#define	MPU6515_DMP_REG									0x6F
#define	MPU6515_DMP_REG_1								0x70
#define	MPU6515_DMP_REG_2								0x71
#define	MPU6515_FIFO_COUNTH								0x72
#define	MPU6515_FIFO_COUNTL								0x73
#define	MPU6515_FIFO_R_W								0x74
#define	MPU6515_WHO_AM_I								0x75	
#define	MPU6515_XA_OFFSET_H								0x77
#define	MPU6515_XA_OFFSET_L								0x78
#define	MPU6515_YA_OFFSET_H								0x7A
#define	MPU6515_YA_OFFSET_L								0x7B
#define	MPU6515_ZA_OFFSET_H								0x7D
#define	MPU6515_ZA_OFFSET_L								0x7E


/* ------------------------------ IIM42652 Defines ------------------------------ */

/*Device address*/
#define IIM42652_ID_ADDRESS 0x68 << 1

/*Registers addresses*/
/*User bank 0*/
#define IIM42652_DEVICE_CONFIG						 0x11
#define IIM42652_DRIVE_CONFIG						 0x13
#define IIM42652_INT_CONFIG							 0x14
#define IIM42652_FIFO_CONFIG						 0x16
#define IIM42652_TEMP_DATA1_UI						 0x1D
#define IIM42652_TEMP_DATA0_UI						 0x1E
#define IIM42652_ACCEL_DATA_X1_UI 					 0x1F
#define IIM42652_ACCEL_DATA_X0_UI 					 0x20
#define IIM42652_ACCEL_DATA_Y1_UI 					 0x21
#define IIM42652_ACCEL_DATA_Y0_UI 					 0x22
#define IIM42652_ACCEL_DATA_Z1_UI 					 0x23
#define IIM42652_ACCEL_DATA_Z0_UI 					 0x24
#define IIM42652_GYRO_DATA_X1_UI 					 0x25
#define IIM42652_GYRO_DATA_X0_UI 					 0x26
#define IIM42652_GYRO_DATA_Y1_UI 					 0x27
#define IIM42652_GYRO_DATA_Y0_UI 					 0x28
#define IIM42652_GYRO_DATA_Z1_UI 					 0x29
#define IIM42652_GYRO_DATA_Z0_UI 					 0x2A
#define IIM42652_TMST_FSYNCH						 0x2B
#define IIM42652_TMST_FSYNCL						 0x2C
#define IIM42652_INT_STATUS							 0x2D
#define IIM42652_FIFO_COUNTH						 0x2E
#define IIM42652_FIFO_COUNTL						 0x2F
#define IIM42652_FIFO_DATA							 0x30
#define IIM42652_APEX_DATA0 						 0x31
#define IIM42652_APEX_DATA1 						 0x32
#define IIM42652_APEX_DATA2 						 0x33
#define IIM42652_APEX_DATA3 						 0x34
#define IIM42652_APEX_DATA4 						 0x35
#define IIM42652_APEX_DATA5 						 0x36
#define IIM42652_INT_STATUS2						 0x37
#define IIM42652_INT_STATUS3 						 0x38
#define IIM42652_SIGNAL_PATH_RESET					 0x4B
#define IIM42652_INTF_CONFIG0						 0x4C
#define IIM42652_INTF_CONFIG1 						 0x4D
#define IIM42652_PWR_MGMT0 							 0x4E
#define IIM42652_GYRO_CONFIG0 						 0x4F
#define IIM42652_ACCEL_CONFIG0 						 0x50
#define IIM42652_GYRO_CONFIG1 						 0x51
#define IIM42652_GYRO_ACCEL_CONFIG0 				 0x52
#define IIM42652_ACCEL_CONFIG1 						 0x53
#define IIM42652_TMST_CONFIG 						 0x54
#define IIM42652_APEX_CONFIG0 						 0x56
#define IIM42652_SMD_CONFIG 						 0x57
#define IIM42652_FIFO_CONFIG1						 0x5F
#define IIM42652_FIFO_CONFIG2						 0x60
#define IIM42652_FIFO_CONFIG3						 0x61
#define IIM42652_FSYNC_CONFIG						 0x62
#define IIM42652_INT_CONFIG0						 0x63
#define IIM42652_INT_CONFIG1						 0x64
#define IIM42652_INT_SOURCE0						 0x65
#define IIM42652_INT_SOURCE1						 0x66
#define IIM42652_INT_SOURCE3						 0x68
#define IIM42652_INT_SOURCE4						 0x69
#define IIM42652_FIFO_LOST_PKT0						 0x6C
#define IIM42652_FIFO_LOST_PKT1						 0x6D
#define IIM42652_SELF_TEST_CONFIG					 0x70
#define IIM42652_WHO_AM_I							 0x75
#define IIM42652_BANK_SEL 							 0x76
#define IIM42652_CHIP_ID 							 0x6F

/*User bank 1*/
#define IIM42652_SENSOR_CONFIG0 					 0x03
#define IIM42652_GYRO_CONFIG_STATIC2 				 0x0B
#define IIM42652_GYRO_CONFIG_STATIC3 				 0x0C
#define IIM42652_GYRO_CONFIG_STATIC4 				 0x0D
#define IIM42652_GYRO_CONFIG_STATIC5 				 0x0E
#define IIM42652_GYRO_CONFIG_STATIC6 				 0x0F
#define IIM42652_GYRO_CONFIG_STATIC7 				 0x10
#define IIM42652_GYRO_CONFIG_STATIC8 				 0x11
#define IIM42652_GYRO_CONFIG_STATIC9 				 0x12
#define IIM42652_GYRO_CONFIG_STATIC10				 0x13
#define IIM42652_XG_ST_DATA 						 0x5F
#define IIM42652_YG_ST_DATA 						 0x60
#define IIM42652_ZG_ST_DATA 						 0x61
#define IIM42652_TMSTVAL0 							 0x62
#define IIM42652_TMSTVAL1 							 0x63
#define IIM42652_TMSTVAL2 							 0x64
#define IIM42652_INTF_CONFIG4 						 0x7A
#define IIM42652_INTF_CONFIG5 						 0x7B
#define IIM42652_INTF_CONFIG6 						 0x7C

/*User bank 2*/
#define IIM42652_ACCEL_CONFIG_STATIC2 				 0x03
#define IIM42652_ACCEL_CONFIG_STATIC3 				 0x04
#define IIM42652_ACCEL_CONFIG_STATIC4 				 0x05
#define IIM42652_XA_ST_DATA							 0x3B
#define IIM42652_YA_ST_DATA 						 0x3C
#define IIM42652_ZA_ST_DATA 						 0x3D

/*User bank 3*/
#define IIM42652_PU_PD_CONFIG1						 0x06
#define IIM42652_PU_PD_CONFIG2						 0x0E

/*User bank 4*/
#define IIM42652_FDR_CONFIG                          0x09
#define IIM42652_APEX_CONFIG1                        0x40
#define IIM42652_APEX_CONFIG2                        0x41
#define IIM42652_APEX_CONFIG3                        0x42
#define IIM42652_APEX_CONFIG4                        0x43
#define IIM42652_APEX_CONFIG5                        0x44
#define IIM42652_APEX_CONFIG6                        0x45
#define IIM42652_APEX_CONFIG7                        0x46
#define IIM42652_APEX_CONFIG8                        0x47
#define IIM42652_APEX_CONFIG9                        0x48
#define IIM42652_APEX_CONFIG10                       0x49
#define IIM42652_ACCEL_WOM_X_THR                     0x4A
#define IIM42652_ACCEL_WOM_Y_THR                     0x4B
#define IIM42652_ACCEL_WOM_Z_THR                     0x4C
#define IIM42652_INT_SOURCE6                         0x4D
#define IIM42652_INT_SOURCE7                         0x4E
#define IIM42652_INT_SOURCE8                         0x4F
#define IIM42652_INT_SOURCE9                         0x50
#define IIM42652_INT_SOURCE10                        0x51
#define IIM42652_OFFSET_USER0                        0x77
#define IIM42652_OFFSET_USER1                        0x78
#define IIM42652_OFFSET_USER2                        0x79
#define IIM42652_OFFSET_USER3                        0x7A
#define IIM42652_OFFSET_USER4                        0x7B
#define IIM42652_OFFSET_USER5                     	 0x7C
#define IIM42652_OFFSET_USER6                   	 0x7D
#define IIM42652_OFFSET_USER7						 0x7E
#define IIM42652_OFFSET_USER8						 0x7F
/**
 * @brief Command macros
 * 
 */

#define IIM42652_SET_GYRO_FS_SEL_2000_dps                0x00
#define IIM42652_SET_GYRO_FS_SEL_1000_dps                0x01
#define IIM42652_SET_GYRO_FS_SEL_500_dps                 0x02
#define IIM42652_SET_GYRO_FS_SEL_250_dps                 0x03
#define IIM42652_SET_GYRO_FS_SEL_125_dps                 0x04
#define IIM42652_SET_GYRO_FS_SEL_62_5_dps                0x05
#define IIM42652_SET_GYRO_FS_SEL_31_25_dps               0x06
#define IIM42652_SET_GYRO_FS_SEL_16_625_dps              0x07
    
#define IIM42652_SET_GYRO_ODR_32kHz                      0x01
#define IIM42652_SET_GYRO_ODR_16kHz                      0x02
#define IIM42652_SET_GYRO_ODR_8kHz                       0x03
#define IIM42652_SET_GYRO_ODR_4kHz                       0x04
#define IIM42652_SET_GYRO_ODR_2kHz                       0x05
#define IIM42652_SET_GYRO_ODR_1kHz                       0x06
#define IIM42652_SET_GYRO_ODR_200Hz                      0x07
#define IIM42652_SET_GYRO_ODR_100Hz                      0x08
#define IIM42652_SET_GYRO_ODR_50Hz                       0x09
#define IIM42652_SET_GYRO_ODR_25Hz                       0x0A
#define IIM42652_SET_GYRO_ODR_12_5Hz                     0x0B

#define IIM42652_SET_ACCEL_FS_SEL_16g                    0x00
#define IIM42652_SET_ACCEL_FS_SEL_8g                     0x01
#define IIM42652_SET_ACCEL_FS_SEL_4g                     0x02
#define IIM42652_SET_ACCEL_FS_SEL_2g                     0x03
    
#define IIM42652_SET_ACCEL_ODR_32kHz                     0x01
#define IIM42652_SET_ACCEL_ODR_16kHz                     0x02
#define IIM42652_SET_ACCEL_ODR_8kHz                      0x03
#define IIM42652_SET_ACCEL_ODR_4kHz                      0x04
#define IIM42652_SET_ACCEL_ODR_2kHz                      0x05
#define IIM42652_SET_ACCEL_ODR_1kHz                      0x06
#define IIM42652_SET_ACCEL_ODR_200Hz                     0x07
#define IIM42652_SET_ACCEL_ODR_100Hz                     0x08
#define IIM42652_SET_ACCEL_ODR_50Hz                      0x09
#define IIM42652_SET_ACCEL_ODR_25Hz                      0x0A
#define IIM42652_SET_ACCEL_ODR_12_5Hz                    0x0B

/*----------------------------------------------------------------------
 *   Typedef
 *--------------------------------------------------------------------*/
enum Ascale		// accelerometer scale
{
	AFS_2G	= 0,
	AFS_4G	= 1,
	AFS_8G	= 2,
	AFS_16G	= 3
};

enum Gscale		// gyro scale
{
	GFS_250DPS	= 0,
	GFS_500DPS	= 1,
	GFS_1000DPS	= 2,
	GFS_2000DPS	= 3
};

/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
extern int8_t InitGyroSensor(void);
extern void MPU6515_SetupForInterruptforImpulse(bool bWomActive,bool bGyroActive, uint8_t ncValue,bool bHelpInterrupSignal);
extern void IIM42652_SetupForInterruptforImpulse(bool bWomActive,bool bGyroActive, uint8_t ncValue,bool bHelpInterrupSignal);
/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
extern SensorType g_GyroSensorType;

#endif // __GIT_SENSOR_H__