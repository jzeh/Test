/*************************************************************
 * NOTE : git_sensor.c
 *      Sensor Control( MPU6515 )
 * Author : Lee junho
 * Since : 2021.02.01
**************************************************************/
#include "cmsis_os.h"

#include "common.h"
#include "typedef.h"

#include "git_sensor.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define MPU6515_I2C_PORT								hi2c2
#define IIM42652_I2C_PORT								hi2c2

#define SENSOR_GET_TIME_OUT								1000

#define ENABLE_DEBUG_SENSOR					// Sensor Log
#define ENABLE_DEBUG_MPU6515				// Gyto Sensor

#define MAX_I2C_INIT_ERROR_COUNT						100
#define MAX_I2C_ERROR_COUNT								1000

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
int8_t MPU6515_readByte( uint8_t reg_addr );
void IIM42652_CalGyroAngle(void);
float IIM42652_normalize_angle(float angle);
void IIM42652_SetDefaultGyroData();
void MPU6515_initMPU6515(void);
void IIM42652_initIIM42652(void);
/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
float DefaultGyroAngle[3];  // Default Gyro angle
float CurrentGyroAngle[3];  // Current Gyro angle
SensorType g_GyroSensorType = GYRO_SENSOR_UNKNOWN; // Gyro Sensor Type
/*----------------------------------------------------------------------
 *   Functions definition
 *--------------------------------------------------------------------*/
SensorType GetName_GyroSensor(void)
{
	g_GyroSensorType = MPU6515_readByte( MPU6515_WHO_AM_I );  // Read WHO_AM_I register of both Gyro sensor
	if (g_GyroSensorType == 0x74) 
	{
		printf("Get Name MPU-6515(0x%x)\n\r", g_GyroSensorType);
		g_GyroSensorType = GYRO_SENSOR_MPU6515;
	} 
//	else if (g_GyroSensorType == 0x6F) 
//	{
//		printf("Get Name IIM-42652(0x%x)\n\r", g_GyroSensorType);
//		g_GyroSensorType = GYRO_SENSOR_IIM42652;
//	} 
//	else 
//	{
//		printf("Unknown Name(0x%x)\n\r", g_GyroSensorType);
//		g_GyroSensorType = GYRO_SENSOR_UNKNOWN;
//	}
    
    /* sometime IIM-42652 return 0xFF */
    else
	{
		printf("Get Name IIM-42652(0x%x)\n\r", g_GyroSensorType);
		g_GyroSensorType = GYRO_SENSOR_IIM42652;
	} 

	return g_GyroSensorType;
}

int8_t InitGyroSensor(void)
{
	SensorType whoami = GetName_GyroSensor();
	if (whoami == GYRO_SENSOR_MPU6515) 
	{
		MPU6515_initMPU6515();
		return INIT_OK;
	}
	else if (whoami == GYRO_SENSOR_IIM42652) 
	{
		IIM42652_initIIM42652();
		return INIT_OK;
	}
	return INIT_FAIL;
}

/* ----------------------------------------------------------------------------------------- /
/																							 /
/	       						MPU6515 Function definition									 /
/																							 /										
/ ------------------------------------------------------------------------------------------*/
int8_t MPU6515_writeByte( uint8_t reg_addr, uint8_t data )
{
	int8_t	buf[2];

	buf[0]	= reg_addr;
	buf[1]	= data;

	if( HAL_I2C_Master_Transmit( &MPU6515_I2C_PORT, (MPU6515_ID_ADDRESS << 1), (uint8_t*)buf, 2, HAL_MAX_DELAY ) != HAL_OK )			return -1;

	return 0;
}

int8_t MPU6515_readByte( uint8_t reg_addr )
{
	int8_t buf;

	if( HAL_I2C_Master_Transmit( &MPU6515_I2C_PORT, (MPU6515_ID_ADDRESS << 1), &reg_addr, 1, 10) != HAL_OK )						return -1;
	if( HAL_I2C_Master_Receive( &MPU6515_I2C_PORT, (MPU6515_ID_ADDRESS << 1) | 0x01, (uint8_t *)&buf, 1, 10) != HAL_OK )			return -1;

	return buf;
}

int8_t MPU6515_readBytes( uint8_t reg_addr, uint16_t len, uint8_t *data )
{
	if( HAL_I2C_Master_Transmit( &MPU6515_I2C_PORT, (MPU6515_ID_ADDRESS << 1), &reg_addr, 1, 10) != HAL_OK )			return -1;
	if( HAL_I2C_Master_Receive( &MPU6515_I2C_PORT, (MPU6515_ID_ADDRESS << 1) | 0x01, data, len, 10) != HAL_OK )			return -1;

	return 0;
}

void MPU6515_readAccelData( int16_t *destination )
{
	uint8_t rawData[6];  // x/y/z accel register data stored here
	int8_t cRet=0;

	cRet = MPU6515_readBytes( MPU6515_ACCEL_XOUT_H, 6, &rawData[0] ); 							// Read the six raw data registers into data array

	if( cRet == 0 )
	{
		destination[0] = (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]);		// Turn the MSB and LSB into a signed 16-bit value
		destination[1] = (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]);
		destination[2] = (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]);
	}
	else
	{
		GLogN("[%s] fail:%d\r\n",__FUNCTION__, cRet);
	}
}

void MPU6515_readGyroData()
{
	uint8_t rawData[6];  // x/y/z gyro register data stored here
	int8_t cRet=0;
    int16_t destination[3];

	cRet = MPU6515_readBytes( MPU6515_GYRO_XOUT_H, 6, &rawData[0] );							// Read the six raw data registers sequentially into data array

	if( cRet == 0 )
	{
		destination[0] = (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]);		// Turn the MSB and LSB into a signed 16-bit value
		destination[1] = (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]);
		destination[2] = (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]);

		GLogN("X-angle : %d, Y-angle : %d, Z-angle : %d\r\n", destination[0], destination[1], destination[2]);  
	}
	else
	{
		GLogN("[%s] fail:%d\r\n",__FUNCTION__, cRet);
	}
}

void MPU6515_resetMPU6515(void)
{
	// reset device
	MPU6515_writeByte( MPU6515_PWR_MGMT_1, 0x07); // Write a one to bit 7 reset bit; toggle reset device
	HAL_Delay(10);
    //jkc osDelay(10);
	MPU6515_writeByte( MPU6515_PWR_MGMT_1, 0x80); // Write a one to bit 7 reset bit; toggle reset device
	HAL_Delay(100);
    //jkc osDelay(100);
}

void MPU6515_initMPU6515(void)
{
	uint8_t	c = 0;

	// Initialize MPU6515 device
	// wake up device
	MPU6515_writeByte( MPU6515_PWR_MGMT_1, 0x00 );	// Clear sleep mode bit (6), enable all sensors
    
	//jkc osDelay(100);
    HAL_Delay(100);
    
	// get stable time source
	MPU6515_writeByte( MPU6515_PWR_MGMT_1, 0x01 );  // Set clock source to be PLL with x-axis gyroscope reference, bits 2:0 = 001
	MPU6515_writeByte( MPU6515_PWR_MGMT_2, 0x00 );  // Set clock source to be PLL with x-axis gyroscope reference, bits 2:0 = 001

	// Configure Gyro and Accelerometer
	// Disable FSYNC and set accelerometer and gyro bandwidth to 44 and 42 Hz, respectively;
	// DLPF_CFG = bits 2:0 = 010; this sets the sample rate at 1 kHz for both
	// Maximum delay is 4.9 ms which is just over a 200 Hz maximum rate
	MPU6515_writeByte( MPU6515_CONFIG, 0x03 );

	// Set sample rate = gyroscope output rate/(1 + SMPLRT_DIV)
	MPU6515_writeByte( MPU6515_SMPLRT_DIV, 0x04 );  // Use a 200 Hz rate; the same rate set in CONFIG above

	// Set gyroscope full scale range
	// Range selects FS_SEL and AFS_SEL are 0 - 3, so 2-bit values are left-shifted into positions 4:3
	c = MPU6515_readByte( MPU6515_GYRO_CONFIG ); // get current GYRO_CONFIG register value
//	c = c & ~0xE0;						// Clear self-test bits [7:5]
	c = c & ~0x02;						// Clear Fchoice bits [1:0]
	c = c & ~0x18;						// Clear AFS bits [4:3]
	c = c | GFS_250DPS << 3;			// Set full scale range for the gyro
//	c =| 0x00;							// Set Fchoice for the gyro to 11 by writing its inverse to bits 1:0 of GYRO_CONFIG
	MPU6515_writeByte( MPU6515_GYRO_CONFIG, c ); // Write new GYRO_CONFIG value to register

	// Set accelerometer full-scale range configuration
	c = MPU6515_readByte( MPU6515_ACCEL_CONFIG ); // get current ACCEL_CONFIG register value
//	c = c & ~0xE0; // Clear self-test bits [7:5]
	c = c & ~0x18;  // Clear AFS bits [4:3]
	c = c | AFS_16G << 3; // Set full scale range for the accelerometer
	MPU6515_writeByte( MPU6515_ACCEL_CONFIG, c ); // Write new ACCEL_CONFIG register value

	// Set accelerometer sample rate configuration
	// It is possible to get a 4 kHz sample rate from the accelerometer by choosing 1 for
	// accel_fchoice_b bit [3]; in this case the bandwidth is 1.13 kHz
	c = MPU6515_readByte( MPU6515_ACCEL_CONFIG2 ); // get current ACCEL_CONFIG2 register value
	c = c & ~0x0F; // Clear accel_fchoice_b (bit 3) and A_DLPFG (bits [2:0])
	c = c | 0x03;  // Set accelerometer rate to 1 kHz and bandwidth to 41 Hz
	MPU6515_writeByte( MPU6515_ACCEL_CONFIG2, c ); // Write new ACCEL_CONFIG2 register value

	// The accelerometer, gyro, and thermometer are set to 1 kHz sample rates,
	// but all these rates are further reduced by a factor of 5 to 200 Hz because of the SMPLRT_DIV setting

	// Configure Interrupts and Bypass Enable
	// Set interrupt pin active high, push-pull, and clear on read of INT_STATUS, enable I2C_BYPASS_EN so additional chips
	// can join the I2C bus and all can be controlled by the Arduino as master
	MPU6515_writeByte( MPU6515_INT_PIN_CFG, 0x00 );
	MPU6515_writeByte( MPU6515_INT_ENABLE, 0x00 );  // Enable data ready (bit 0) interrupt
}

/* If 'INT_ANYRD_2CLEAR' is 0, Interrupt status is cleared only by reading INT_STATUS register */
char MPU6515_Read_IntStatusRegister()
{
	return MPU6515_readByte( MPU6515_INT_STATUS );
}

/* Get WOM_INT_Register */
bool MPU6515_CheckInterrupt()
{
	char temp_register;

	temp_register = MPU6515_Read_IntStatusRegister();

	GLogN("\r\n  INT_STATUS: 0x%02x\r\n\r\n", temp_register);		//for test

	if( temp_register & 0x40 )			return TRUE;
	else								return FALSE;
}

void MPU6515_MakeAccelRunning( uint8_t bGyroActive )
{
	// disable temperature sensor
	MPU6515_writeByte( MPU6515_PWR_MGMT_1, 0x08);//set defualt clock
	if( bGyroActive == 1 )			MPU6515_writeByte( MPU6515_PWR_MGMT_2, 0x00 );				// Enable Accel X/Y/Z, Gyro X/Y/Z
	else							MPU6515_writeByte( MPU6515_PWR_MGMT_2, 0x07 );				// Disable Gyro X/Y/Z
}

/* set 184 Hz Bandwidth */
void MPU6515_SetAccelLPF()
{
	MPU6515_writeByte( MPU6515_ACCEL_CONFIG2, 0x01 );
}

/* Add 0x37, INT_ANYRD_2CLEAR */
void MPU6515_SetInterruptClearState( uint8_t interruptClearStatus )
{
//	MPU6515_writeByte( MPU6515_INT_PIN_CFG, 0x70);
	MPU6515_writeByte( MPU6515_INT_PIN_CFG, 0x60);
//	MPU6515_writeByte( MPU6515_INT_PIN_CFG, 0x20);
}

void MPU6515_EnableMotionInterrupt( uint8_t bActiveWom )
{
	if( bActiveWom == 1 )				MPU6515_writeByte( MPU6515_INT_ENABLE, 0x40 );				// enable WON interrupt
	else								MPU6515_writeByte( MPU6515_INT_ENABLE, 0x00 );				// disable WON interrupt
}

void MPU6515_EnableAccelHardwareIntelligence()
{
	MPU6515_writeByte( MPU6515_MOT_DETECT_CTRL, 0xC0 );
}

void MPU6515_SetMotionThreshold(char threshold)
{
	MPU6515_writeByte( MPU6515_WOM_THR, threshold );
}

void MPU6515_SetWakeupFrequency()
{
	MPU6515_writeByte( MPU6515_LP_ACCEL_ODR, 0x00);//0.24 Hz
//	MPU6515_writeByte( MPU6515_LP_ACCEL_ODR, 0x01);//0.49 Hz
//	MPU6515_writeByte( MPU6515_LP_ACCEL_ODR, 0x03);//1.95 Hz
//	MPU6515_writeByte( MPU6515_LP_ACCEL_ODR, 0x04);//3.91 Hz
//	MPU6515_writeByte( MPU6515_LP_ACCEL_ODR, 0x05);//7.81 Hz
//	MPU6515_writeByte( MPU6515_LP_ACCEL_ODR, 0x06);//15.63 Hz
//	MPU6515_writeByte( MPU6515_LP_ACCEL_ODR, 0x0B);//500 Hz
}

void MPU6515_EnableAccelLowPowerMode()
{
	// In order to prevent deep sleep a MPU-6515, do not set cicle bit.
	// it cause MPU-6515 to go sleep a few minutes later.
	MPU6515_writeByte( MPU6515_PWR_MGMT_1, 0x08);
}

void MPU6515_ConfigurationWakeUpOnMotionInterrupt(uint8_t bWomActive, uint8_t bGyroActive, uint8_t ucValue)
{
	MPU6515_MakeAccelRunning( bGyroActive );

	MPU6515_SetAccelLPF();

	MPU6515_SetMotionThreshold(ucValue);/* Need turnning. */

	MPU6515_SetWakeupFrequency();

	MPU6515_EnableAccelHardwareIntelligence();

	MPU6515_EnableMotionInterrupt(bWomActive);
}

void MPU6515_SetInterruptClearState2(bool bHelpInterrupSignal)
{
	if( bHelpInterrupSignal == true )			MPU6515_writeByte( MPU6515_INT_PIN_CFG, 0x20 );
	else										MPU6515_writeByte( MPU6515_INT_PIN_CFG, 0x00 );
}

void MPU6515_SetupForInterruptforImpulse(bool bWomActive,bool bGyroActive, uint8_t ncValue,bool bHelpInterrupSignal)
{
	MPU6515_resetMPU6515();

	MPU6515_SetInterruptClearState2(bHelpInterrupSignal);
	MPU6515_ConfigurationWakeUpOnMotionInterrupt(bWomActive, bGyroActive, ncValue);

	MPU6515_readByte( MPU6515_INT_STATUS );
}

bool MPU6515_ReadInterruptStatus()
{
	uint8_t	uchInterruptStatus = MPU6515_readByte( MPU6515_INT_STATUS );

	if( (uchInterruptStatus & 0x40) == 0x40 )			return true;
	else												return false;
}

uint8_t MPU6515_GetName(void)
{
	uint8_t whoami;

	// reset device
	MPU6515_writeByte( MPU6515_PWR_MGMT_1, 0x80 ); // Write a one to bit 7 reset bit; toggle reset device

	osDelay(100);

	// Read the WHO_AM_I register, this is a good test of communication
	whoami = MPU6515_readByte( MPU6515_WHO_AM_I );  // Read WHO_AM_I register for MPU-6515
	printf("I AM 0x%x\n\r", whoami);
	printf("I SHOULD BE 0x74\n\r");

	osDelay(100);

	return whoami;
}

/* ----------------------------------------------------------------------------------------- /
/																							 /
/	       					 IIM42652 Function definition 									 /
/																							 /										
/ ------------------------------------------------------------------------------------------*/

int8_t IIM42652_writeByte( uint8_t reg_addr, uint8_t data )
{
  int8_t	buf[2];
  
  buf[0]	= reg_addr;
  buf[1]	= data;
  
  if( HAL_I2C_Master_Transmit( &IIM42652_I2C_PORT, (IIM42652_ID_ADDRESS), (uint8_t*)buf, 2, HAL_MAX_DELAY ) != HAL_OK )			return -1;
  
  return 0;
}

int8_t IIM42652_readByte( uint8_t reg_addr )
{
	int8_t buf;

	if( HAL_I2C_Master_Transmit( &IIM42652_I2C_PORT, (IIM42652_ID_ADDRESS), &reg_addr, 1, 10) != HAL_OK )						return -1;
	if( HAL_I2C_Master_Receive( &IIM42652_I2C_PORT, (IIM42652_ID_ADDRESS) | 0x01, (uint8_t *)&buf, 1, 10) != HAL_OK )			return -1;

	return buf;
}

int8_t IIM42652_readBytes( uint8_t reg_addr, uint16_t len, uint8_t *data )
{
	if( HAL_I2C_Master_Transmit( &IIM42652_I2C_PORT, (IIM42652_ID_ADDRESS), &reg_addr, 1, 10) != HAL_OK )			return -1;
	if( HAL_I2C_Master_Receive( &IIM42652_I2C_PORT, (IIM42652_ID_ADDRESS) | 0x01, data, len, 10) != HAL_OK )			return -1;

	return 0;
}

void IIM42652_readAccelData( int16_t *destination )
{
	uint8_t rawData[6];  // x/y/z gyro register data stored here
	int8_t cRet=0;

	cRet = IIM42652_readBytes( IIM42652_ACCEL_DATA_X1_UI, 6, &rawData[0] );							// Read the six raw data registers sequentially into data array

	if( cRet == 0 )
	{
		destination[0] = (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]);		// Turn the MSB and LSB into a signed 16-bit value
		destination[1] = (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]);
		destination[2] = (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]);
	}
	else
	{
		GLogN("[%s] fail:%d\r\n",__FUNCTION__, cRet);
	}
}

int8_t IIM42652_readGyroData( int16_t *destination )
{
	uint8_t rawData[6];  // x/y/z gyro register data stored here
	int8_t cRet=0;

	cRet = IIM42652_readBytes( IIM42652_GYRO_DATA_X1_UI, 6, &rawData[0] );							// Read the six raw data registers sequentially into data array

	if( cRet == 0 )
	{
		destination[0] = (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]);		// Turn the MSB and LSB into a signed 16-bit value
		destination[1] = (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]);
		destination[2] = (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]);      
	}
	else
	{
		//GLogN("[%s] fail:%d\r\n",__FUNCTION__, cRet);
	}

	return cRet;
}

void IIM42652_CalGyroAngle(void)
{
	static int uiNowTime = 0, uiPreTime, uiDeltaTime;
	float fDt=0;
	float fGyroAngleX = 0;
	float fGyroAngleY = 0;
	float fGyroAngleZ = 0;
    int16_t gyro_tmp[3];       // gyro sensor OUTPUT
    float gyro_rate[3];       // gyro sensor OUTPUT
    int8_t cRet=0;

    cRet = IIM42652_readGyroData(gyro_tmp);

	if (cRet == 0)
	{
		gyro_rate[0] = ( (float)gyro_tmp[0] * 250.0 ) / 32768.0;
		gyro_rate[1] = ( (float)gyro_tmp[1] * 250.0 ) / 32768.0;
		gyro_rate[2] = ( (float)gyro_tmp[2] * 250.0 ) / 32768.0;
			
		CurrentGyroAngle[0] += (gyro_rate[0] - DefaultGyroAngle[0]);
		CurrentGyroAngle[1] += (gyro_rate[1] - DefaultGyroAngle[1]); 
		CurrentGyroAngle[2] += (gyro_rate[2] - DefaultGyroAngle[2]); 
		
		uiNowTime = Get_Tmr();
		uiDeltaTime = Get_TmrDelta(uiNowTime, uiPreTime);
		fDt = (float)uiDeltaTime/1000.0;
		uiPreTime = uiNowTime;
		
		fGyroAngleX = IIM42652_normalize_angle(CurrentGyroAngle[0] * fDt); // ¡Æ¡Ë¥ì¥ì
		fGyroAngleY = IIM42652_normalize_angle(CurrentGyroAngle[1] * fDt);
		fGyroAngleZ = IIM42652_normalize_angle(CurrentGyroAngle[2] * fDt);
		
		GLogN("X-angle : %f, Y-angle : %f, Z-angle : %f\r\n", fGyroAngleX, fGyroAngleY, fGyroAngleZ);  
	}
	else
	{
		GLogN("[%s] fail:%d\r\n",__FUNCTION__, cRet);
	}
}

float IIM42652_normalize_angle(float angle) {
    // normalize between -180 ~ 180
    angle = fmod(angle, 360.0f);
    return angle;
}

void IIM42652_SetDefaultGyroData()
{
	uint16_t ii;
	float cnt = 0;
	int8_t cRet = 0;
	
	for (ii = 0; ii <= 1000; ii++) 
	{
		int16_t gyro_temp[3] = {0, 0, 0};
		
		cRet = IIM42652_readGyroData(gyro_temp);
		
		if ((gyro_temp[0] == -32768) || (cRet != 0)) continue;
		DefaultGyroAngle[0]  += (float)gyro_temp[0];
		DefaultGyroAngle[1]  += (float)gyro_temp[1];
		DefaultGyroAngle[2]  += (float)gyro_temp[2];
		cnt += 1;
	}

	if (cnt != 0)
	{
		DefaultGyroAngle[0] = DefaultGyroAngle[0] / cnt;
		DefaultGyroAngle[1] = DefaultGyroAngle[1] / cnt;
		DefaultGyroAngle[2] = DefaultGyroAngle[2] / cnt;
		
		DefaultGyroAngle[0] = ( (float)DefaultGyroAngle[0] * 250.0 ) / 32768.0;
		DefaultGyroAngle[1] = ( (float)DefaultGyroAngle[1] * 250.0 ) / 32768.0;
		DefaultGyroAngle[2] = ( (float)DefaultGyroAngle[2] * 250.0 ) / 32768.0;
		
		//  CurrentGyroAngle[0] = DefaultGyroAngle[0];
		//  CurrentGyroAngle[1] = DefaultGyroAngle[1];
		//  CurrentGyroAngle[2] = DefaultGyroAngle[2];
	}
	else
	{
		GLogN("[%s] fail:%d\r\n",__FUNCTION__, cRet);
	}
}

void IIM42652_resetIIM42652(void)
{
	// reset device
	IIM42652_writeByte( IIM42652_BANK_SEL, 0x00);      // Register Bank Select 0
	IIM42652_writeByte( IIM42652_DEVICE_CONFIG, 0x01); // SOFT_RESET_CONFIG (Enable reset)
	HAL_Delay(2);
}

void IIM42652_initIIM42652(void)
{
	uint8_t	c = 0;

    IIM42652_resetIIM42652();                   // SW Reset and Set register Bank to 0
    
    IIM42652_writeByte( IIM42652_PWR_MGMT0, 0x1E );      // GYRO_MODE(LN mode), ACCEL_MODE(LP mode), RC_OSC_EN

	// Configure Gyro
    c = IIM42652_readByte( IIM42652_GYRO_CONFIG0 );      // get current GYRO_CONFIG register value
    c = c & ~0xE0;                                 // Clear FS bits [7:5]
    c = c | (GFS_250DPS << 5);                     // Set full scale range for the gyro
	IIM42652_writeByte( IIM42652_GYRO_CONFIG0, c );      
    
    // Configure Accelerometer
    c = (AFS_16G << 5) | 0x09;                        // Set full scale range and 50hz(ACCEL_ODR) for the accel
	IIM42652_writeByte( IIM42652_ACCEL_CONFIG0, c ); 
    
    c = IIM42652_readByte( IIM42652_INTF_CONFIG1 );
    c = c & ~0x04;
    IIM42652_writeByte( IIM42652_INTF_CONFIG1, c ); 
    
    HAL_Delay(2);

    c = IIM42652_readByte( IIM42652_INT_SOURCE1 );
	c = c | 0x07;
	IIM42652_writeByte( IIM42652_INT_SOURCE1, c );
 
    IIM42652_writeByte( IIM42652_INT_CONFIG, 0x07 ); 
	
	IIM42652_readByte( IIM42652_INT_STATUS );
   
    IIM42652_SetDefaultGyroData(); // setDefault Gyro Data
}

char IIM42652_Read_IntStatusRegister()
{
	return IIM42652_readByte( IIM42652_INT_STATUS );
}

/* Get WOM_INT_Register */
bool IIM42652_CheckInterrupt()
{
	char temp_register;

	temp_register = IIM42652_Read_IntStatusRegister();

	GLogN("\r\n  INT_STATUS: 0x%02x\r\n\r\n", temp_register);		//for test

	if( temp_register & 0x40 )			return TRUE;
	else								return FALSE;
}

void IIM42652_MakeAccelRunning( uint8_t bGyroActive )
{
    uint8_t	c = 0;
    c = IIM42652_readByte( IIM42652_PWR_MGMT0 );
    c = c | 0x20;                                                              // disable temperature sensor
	IIM42652_writeByte( IIM42652_PWR_MGMT0, c );      
    
    IIM42652_writeByte( IIM42652_BANK_SEL, 0x01);      // Register Bank Select 1
	if( bGyroActive == 1 )      IIM42652_writeByte( IIM42652_SENSOR_CONFIG0, 0x00 );     // Enable Accel X/Y/Z, Gyro X/Y/Z
	else						IIM42652_writeByte( IIM42652_SENSOR_CONFIG0, 0x38 );     // Disable Gyro X/Y/Z
    IIM42652_writeByte( IIM42652_BANK_SEL, 0x00);      // Register Bank Select 0
}

/* set Bandwidth */
void IIM42652_SetAccelLPF()
{
    uint8_t	c = 0;
    c = IIM42652_readByte( IIM42652_GYRO_ACCEL_CONFIG0 );
    c = c | 0x10;
	IIM42652_writeByte( IIM42652_GYRO_ACCEL_CONFIG0, c );    // set Bandwith = 50Hz x 1 = 50Hz(LP Mode)
}

void IIM42652_EnableMotionInterrupt( uint8_t bActiveWom )
{
	if( bActiveWom == 1 )				IIM42652_writeByte( IIM42652_SMD_CONFIG, 0x05 );				// enable WON interrupt
	else								IIM42652_writeByte( IIM42652_SMD_CONFIG, 0x00 );				// disable WON interrupt
}

void IIM42652_EnableAccelHardwareIntelligence()
{
    uint8_t	c = 0;
    c = IIM42652_readByte( IIM42652_SMD_CONFIG );
    c = c | 0x04;
	IIM42652_writeByte( IIM42652_SMD_CONFIG, c );            // Compare current sample to prev sample
}

void IIM42652_SetMotionThreshold(char threshold)
{
    IIM42652_writeByte( IIM42652_BANK_SEL, 0x04);                // Register Bank Select 4
	IIM42652_writeByte( IIM42652_ACCEL_WOM_X_THR, threshold );   // set threshold on x-axis
    IIM42652_writeByte( IIM42652_ACCEL_WOM_Y_THR, threshold );   // set threshold on y-axis
    IIM42652_writeByte( IIM42652_ACCEL_WOM_Z_THR, threshold );   // set threshold on z-axis
    IIM42652_writeByte( IIM42652_BANK_SEL, 0x00);                // Register Bank Select 0
    HAL_Delay(2);
}

void IIM42652_SetWakeupFrequency()
{
    uint8_t	c = 0;

    //c = IIM42652_readByte( ACCEL_CONFIG0 );
    //c = 0x0E;                      
	//IIM42652_writeByte( ACCEL_CONFIG0, c );
    //c = 0x0B; 
    //IIM42652_writeByte( GYRO_CONFIG0, c );

  
//	    c = IIM42652_readByte( INT_SOURCE1 );
//		c = c | 0x07;                        // Set full scale range and 50hz(ACCEL_ODR) for the accel
//		IIM42652_writeByte( INT_SOURCE1, c );
    
    HAL_Delay(100);
}

void IIM42652_ConfigurationWakeUpOnMotionInterrupt(uint8_t bWomActive, uint8_t bGyroActive, uint8_t ucValue)
{
	IIM42652_MakeAccelRunning( bGyroActive );

	IIM42652_SetAccelLPF();

	IIM42652_SetMotionThreshold(ucValue);/* Need turnning. */

	IIM42652_SetWakeupFrequency();

//	IIM42652_EnableAccelHardwareIntelligence();

	IIM42652_EnableMotionInterrupt(bWomActive);
}

void IIM42652_SetInterruptClearState2(bool bHelpInterrupSignal)
{
	if( bHelpInterrupSignal == true )			IIM42652_writeByte( IIM42652_INT_CONFIG, 0x00 ); 
	else										IIM42652_writeByte( IIM42652_INT_CONFIG, 0x07 ); 
}

void IIM42652_SetupForInterruptforImpulse(bool bWomActive,bool bGyroActive, uint8_t ncValue,bool bHelpInterrupSignal)
{
	// IIM42652_resetIIM42652();

	IIM42652_SetInterruptClearState2(bHelpInterrupSignal);
	IIM42652_ConfigurationWakeUpOnMotionInterrupt(bWomActive, bGyroActive, ncValue);

	IIM42652_writeByte( IIM42652_INT_SOURCE1, 0x07 );

	IIM42652_readByte( IIM42652_INT_STATUS );
}


bool IIM42652_ReadInterruptStatus()
{
	uint8_t	uchInterruptStatus = IIM42652_readByte( IIM42652_INT_STATUS2 );

	if( (uchInterruptStatus & 0x0F) != 0x00 )			return true;        // Interrupt on SMD, WOM_X, WOM_Y, WOM_Z 
	else											    return false;
}

uint8_t IIM42652_GetName(void)
{
	uint8_t whoami;

	// reset device
	IIM42652_resetIIM42652();

	osDelay(100);

	// Read the WHO_AM_I register, this is a good test of communication
	whoami = IIM42652_readByte( IIM42652_WHO_AM_I );  // Read WHO_AM_I register for IIM42652
	printf("I AM 0x%x\n\r", whoami);
	printf("I SHOULD BE 0x6f\n\r");

	osDelay(100);

	return whoami;
}
