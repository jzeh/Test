/*
  ******************************************************************************
  * @file    GIT_SensorProc.c
  * @author
  * @version V1.0.0
  * @date    2017-02-01
  * @brief
  *
  *
  ******************************************************************************
*/
#include <math.h>

#include "GIT_SensorProc.h"
#include "GIT_OemInterface.h"
#include "HalHandler.h"

#include "MngSystem.h"

#include "HdDebug.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_SENSOR,__VA_ARGS__)

#define SENSOR_GET_TIME_OUT 1000

#define ENABLE_DEBUG_SENSOR					// Sensor Log
#define ENABLE_DEBUG_MPU6515				// Gyto Sensor
//#define ENABLE_MOVING_AVR

SENSOR_DATA SensorData;
extern unsigned char g_ucI2CFlag[2];

#ifdef				ENABLE_MOVING_AVR
int gnAcXBuffer[5] = {0};
int gnAcXPos = 0;
signed long gwAcXSum;
int gnAcXBufferLen = sizeof(gnAcXBuffer) / sizeof(int);

int gnAcYBuffer[5] = {0};
int gnAcYPos = 0;
signed long gwAcYSum;
int gnAcYBufferLen = sizeof(gnAcYBuffer) / sizeof(int);

int gnAcZBuffer[5] = {0};
int gnAcZPos = 0;
signed long gwAcZSum;
int gnAcZBufferLen = sizeof(gnAcZBuffer) / sizeof(int);
#endif

float gfdt;				// 시간 변화량(ms)
float compAngleX, compAngleY; // Calculate the angle using a complementary filter
float kalAngleX, kalAngleY; // Calculate the angle using a Kalman filter

int gnBiasXAngle;
int gnBiasYAngle;
int gnBiasZAngle;

float PI;
float GyroMeasError;     // gyroscope measurement error in rads/s (start at 60 deg/s), then reduce after ~10 s to 3
float beta;  // compute beta
float GyroMeasDrift;      // gyroscope measurement drift in rad/s/s (start at 0.0 deg/s/s)
float zeta;  // compute zeta, the other free parameter in the Madgwick scheme usually set to a small or zero value

/* Kalman filter variables */
float Kalman_Q_angle; // Process noise variance for the accelerometer
float Kalman_Q_bias; // Process noise variance for the gyro bias
float Kalman_R_measure; // Measurement noise variance - this is actually the variance of the measurement noise

float Kalman_X_angle; // The angle calculated by the Kalman filter - part of the 2x1 state matrix
float Kalman_X_bias; // The gyro bias calculated by the Kalman filter - part of the 2x1 state matrix
float Kalman_X_rate; // Unbiased rate calculated from the rate and the calculated bias - you have to call getAngle to update the rate
float Kalman_X_P[2][2]; // Error covariance matrix - This is a 2x2 matrix
float Kalman_X_K[2]; // Kalman gain - This is a 2x1 matrix
float Kalman_X_y; // Angle difference - 1x1 matrix
float Kalman_X_S; // Estimate error - 1x1 matrix

float Kalman_Y_angle; // The angle calculated by the Kalman filter - part of the 2x1 state matrix
float Kalman_Y_bias; // The gyro bias calculated by the Kalman filter - part of the 2x1 state matrix
float Kalman_Y_rate; // Unbiased rate calculated from the rate and the calculated bias - you have to call getAngle to update the rate
float Kalman_Y_P[2][2]; // Error covariance matrix - This is a 2x2 matrix
float Kalman_Y_K[2]; // Kalman gain - This is a 2x1 matrix
float Kalman_Y_y; // Angle difference - 1x1 matrix
float Kalman_Y_S; // Estimate error - 1x1 matrix

int movingAvg(int *ptrArrNumbers, signed long *ptrSum, int pos, int len, int nextNum);
float Kalman_getXAngle(float newAngle, float newRate, float dt);
float Kalman_getYAngle(float newAngle, float newRate, float dt);

void CalGyroAngle(int16_t *accelCount, float *gyroBias);
void CalAccelAngle(int16_t *accelCount, float *accelBias);
void CalComplimentValue(void);
void MPU6515_SetInterruptClearState2(bool bHelpInterrupSignal);


extern unsigned short Get_Speed();

void SystemDelay(uint16_t delay)
{
    Oem_GIT_mDelay(delay);
}

void MPU6515_initValue(void)
{
}

void MPU6515_writeByte(uint8_t address, uint8_t subAddress, uint8_t data)
{
	char data_write[2];
	u8 return_value,error_count;

	data_write[0] = data;
	return_value = 0;
	error_count = 0;

	while (!return_value && !(error_count == 3))
	{
		return_value = I2C_Write(address << 1, subAddress, (unsigned char*)data_write, 1);
		if(!return_value)
		{
			error_count++;
			Trace("\r\n [Error] MPU6515_writeByte Fail. error_count : %d. \r\n",error_count);
		}
	}
}

#define MAX_I2C_INIT_ERROR_COUNT 100
#define MAX_I2C_ERROR_COUNT 1000

int m_nI2CErrorCount = 0;
boolean_t m_bReqSystemPowerReset = false;

char MPU6515_readByte(uint8_t address, uint8_t subAddress)
{
	char data[1]={0,}; // `data` will store the register data
	u8 return_value,error_count;

	return_value = 0;
	error_count = 0;


	I2C_Read(address << 1, subAddress, (unsigned char*)data, 1);

	while (!return_value && !(error_count == 3))
	{
		return_value = I2C_Read(address << 1, subAddress, (unsigned char*)data, 1);
		if(!return_value)
		{
			error_count++;
			Trace("\r\n [Error] without count parameter\r\n",error_count);

            if( m_nI2CErrorCount++ > MAX_I2C_ERROR_COUNT )
            {
            	printf("#");
            	Trace("[Error]MPU6515_readByte Fail.max_error_count : %d \r\n",m_nI2CErrorCount);
                m_bReqSystemPowerReset = true;
			    SensorData.g_eSensorState = eSENSOR_STATE_IDLE;

				g_ucI2CFlag[0] = 1;
//				SystemForcelyReset();
//                HalGPIOSetVaule(GPIO_ETC_PWEN, eBIT_RESET);			// ETC_3V3 power OFF
//				//MONI 20190217 ME i2c couldn't initialize hardware because i2c power be always supplied.
//                HalGPIOSetVaule(GPIO_MA_PWEN, eBIT_RESET);          // main powre off
            }
		}
	}

	return data[0];
}

void MPU6515_readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest)
{
	char data[14];
	u8 return_value,error_count;

	return_value = 0;
	error_count = 0;

	if(count >14)
	{
		Trace("\r\n [Error] MPU6515_readBytes Fail. \r\n");
	}

	while (!return_value && !(error_count == 3))
	{


		return_value = I2C_Read(address << 1, subAddress, (unsigned char*)data, count);
		if(!return_value)
		{
			error_count++;
			Trace("\r\n [Error] MPU6515_readByte Fail. error_count : %d. \r\n",error_count);
		}
	}

	for(int ii = 0; ii < count; ii++) {
		dest[ii] = data[ii];
	}
}

void MPU6515_getGres(void)
{
  switch (SensorData.Gscale)
  {
		// Possible gyro scales (and their register bit settings) are:
		// 250 DPS (00), 500 DPS (01), 1000 DPS (10), and 2000 DPS  (11).
		// Here's a bit of an algorith to calculate DPS/(ADC tick) based on that 2-bit value:
		case GFS_250DPS:
			SensorData.gRes = 250.0/32768.0;
			break;

		case GFS_500DPS:
			SensorData.gRes = 500.0/32768.0;
			break;

		case GFS_1000DPS:
			SensorData.gRes = 1000.0/32768.0;
			break;

		case GFS_2000DPS:
			SensorData.gRes = 2000.0/32768.0;
			break;
  }
}

void MPU6515_getAres(void)
{
  switch (SensorData.Ascale)
  {
		// Possible accelerometer scales (and their register bit settings) are:
		// 2 Gs (00), 4 Gs (01), 8 Gs (10), and 16 Gs  (11).
		// Here's a bit of an algorith to calculate DPS/(ADC tick) based on that 2-bit value:
		case AFS_2G:
			SensorData.aRes = 2.0/32768.0;
			break;

		case AFS_4G:
			SensorData.aRes = 4.0/32768.0;
			break;

		case AFS_8G:
			SensorData.aRes = 8.0/32768.0;
			break;

		case AFS_16G:
			SensorData.aRes = 16.0/32768.0;
			break;
  }
}

void MPU6515_readAccelData(int16_t * destination)
{
  uint8_t rawData[6];  // x/y/z accel register data stored here

  MPU6515_readBytes(MPU6515_ADDRESS, ACCEL_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers into data array

  destination[0] = (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
  destination[1] = (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
  destination[2] = (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;
}

void MPU6515_readGyroData(int16_t * destination)
{
  uint8_t rawData[6];  // x/y/z gyro register data stored here

  MPU6515_readBytes(MPU6515_ADDRESS, GYRO_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers sequentially into data array

  destination[0] = (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
  destination[1] = (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
  destination[2] = (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;
}

void MPU6515_readMagData(int16_t * destination)
{
  uint8_t rawData[7];  // x/y/z gyro register data, ST2 register stored here, must read ST2 at end of data acquisition

  if(MPU6515_readByte(AK8963_ADDRESS, AK8963_ST1) & 0x01) { // wait for magnetometer data ready bit to be set
		MPU6515_readBytes(AK8963_ADDRESS, AK8963_XOUT_L, 7, &rawData[0]);  // Read the six raw data and ST2 registers sequentially into data array

		uint8_t c = rawData[6]; // End data read by reading ST2 register

		if(!(c & 0x08)) { // Check if magnetic sensor overflow set, if not then report data
			destination[0] = (int16_t)(((int16_t)rawData[1] << 8) | rawData[0]);  // Turn the MSB and LSB into a signed 16-bit value
			destination[1] = (int16_t)(((int16_t)rawData[3] << 8) | rawData[2]) ;  // Data stored as little Endian
			destination[2] = (int16_t)(((int16_t)rawData[5] << 8) | rawData[4]) ;
   }
  }
}

int16_t MPU6515_readTempData(void)
{
  uint8_t rawData[2];  // x/y/z gyro register data stored here

  MPU6515_readBytes(MPU6515_ADDRESS, TEMP_OUT_H, 2, &rawData[0]);  // Read the two raw data registers sequentially into data array

  return (int16_t)(((int16_t)rawData[0]) << 8 | rawData[1]) ;  // Turn the MSB and LSB into a 16-bit value
}

void MPU6515_resetMPU6515(void)
{
    // MONI 20190130
    // changed reset sequence.
#ifdef OLD_MPU6515_RESET
    // reset device
    MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_1, 0x80); // Write a one to bit 7 reset bit; toggle reset device
	//MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_2, 0x00);
    SystemDelay(100);
#else
    // reset device
    MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_1, 0x07); // Write a one to bit 7 reset bit; toggle reset device
    SystemDelay(10);
    MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_1, 0x80); // Write a one to bit 7 reset bit; toggle reset device
    //MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_2, 0x00);
    SystemDelay(100);

    //MPU6515_writeByte(MPU6515_ADDRESS, SIGNAL_PATH_RESET, 0x07);
    //MPU6515_writeByte(MPU6515_ADDRESS, USER_CTRL, 0x0F);

    //SystemDelay(100);

#endif
}

void MPU6515_initAK8963(float * destination)
{
  // First extract the factory calibration for each magnetometer axis
  uint8_t rawData[3];  // x/y/z gyro calibration data stored here

  MPU6515_writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x00); // Power down magnetometer

	SystemDelay(10);

  MPU6515_writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x0F); // Enter Fuse ROM access mode

	SystemDelay(10);

  MPU6515_readBytes(AK8963_ADDRESS, AK8963_ASAX, 3, &rawData[0]);  // Read the x-, y-, and z-axis calibration values

  destination[0] =  (float)(rawData[0] - 128)/256.0f + 1.0f;   // Return x-axis sensitivity adjustment values, etc.
  destination[1] =  (float)(rawData[1] - 128)/256.0f + 1.0f;
  destination[2] =  (float)(rawData[2] - 128)/256.0f + 1.0f;

  MPU6515_writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x00); // Power down magnetometer

	SystemDelay(10);

  // Configure the magnetometer for continuous read and highest resolution
  // set Mscale bit 4 to 1 (0) to enable 16 (14) bit resolution in CNTL register,
  // and enable continuous mode data acquisition Mmode (bits [3:0]), 0010 for 8 Hz and 0110 for 100 Hz sample rates
  MPU6515_writeByte(AK8963_ADDRESS, AK8963_CNTL, SensorData.Mscale << 4 | SensorData.Mmode); // Set magnetometer data resolution and sample ODR

	SystemDelay(10);

	return;
}

void MPU6515_initMPU6515(void)
{
 // Initialize MPU6515 device
 // wake up device
  MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_1, 0x00); // Clear sleep mode bit (6), enable all sensors

	SystemDelay(100);

 // get stable time source
  MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_1, 0x01);  // Set clock source to be PLL with x-axis gyroscope reference, bits 2:0 = 001
  MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_2, 0x00);  // Set clock source to be PLL with x-axis gyroscope reference, bits 2:0 = 001
 //MPU6515_writeByte(MPU6515_ADDRESS, FIFO_EN, 0x70);

 // Configure Gyro and Accelerometer
 // Disable FSYNC and set accelerometer and gyro bandwidth to 44 and 42 Hz, respectively;
 // DLPF_CFG = bits 2:0 = 010; this sets the sample rate at 1 kHz for both
 // Maximum delay is 4.9 ms which is just over a 200 Hz maximum rate
  MPU6515_writeByte(MPU6515_ADDRESS, CONFIG, 0x03);

 // Set sample rate = gyroscope output rate/(1 + SMPLRT_DIV)
  MPU6515_writeByte(MPU6515_ADDRESS, SMPLRT_DIV, 0x04);  // Use a 200 Hz rate; the same rate set in CONFIG above

 // Set gyroscope full scale range
 // Range selects FS_SEL and AFS_SEL are 0 - 3, so 2-bit values are left-shifted into positions 4:3
  uint8_t c = MPU6515_readByte(MPU6515_ADDRESS, GYRO_CONFIG); // get current GYRO_CONFIG register value
 // c = c & ~0xE0; // Clear self-test bits [7:5]
  c = c & ~0x02; // Clear Fchoice bits [1:0]
  c = c & ~0x18; // Clear AFS bits [4:3]
  c = c | SensorData.Gscale << 3; // Set full scale range for the gyro
 // c =| 0x00; // Set Fchoice for the gyro to 11 by writing its inverse to bits 1:0 of GYRO_CONFIG
  MPU6515_writeByte(MPU6515_ADDRESS, GYRO_CONFIG, c ); // Write new GYRO_CONFIG value to register

 // Set accelerometer full-scale range configuration
  c = MPU6515_readByte(MPU6515_ADDRESS, ACCEL_CONFIG); // get current ACCEL_CONFIG register value
 // c = c & ~0xE0; // Clear self-test bits [7:5]
  c = c & ~0x18;  // Clear AFS bits [4:3]
  c = c | SensorData.Ascale << 3; // Set full scale range for the accelerometer
  MPU6515_writeByte(MPU6515_ADDRESS, ACCEL_CONFIG, c); // Write new ACCEL_CONFIG register value

 // Set accelerometer sample rate configuration
 // It is possible to get a 4 kHz sample rate from the accelerometer by choosing 1 for
 // accel_fchoice_b bit [3]; in this case the bandwidth is 1.13 kHz
  c = MPU6515_readByte(MPU6515_ADDRESS, ACCEL_CONFIG2); // get current ACCEL_CONFIG2 register value
  c = c & ~0x0F; // Clear accel_fchoice_b (bit 3) and A_DLPFG (bits [2:0])
  c = c | 0x03;  // Set accelerometer rate to 1 kHz and bandwidth to 41 Hz
  MPU6515_writeByte(MPU6515_ADDRESS, ACCEL_CONFIG2, c); // Write new ACCEL_CONFIG2 register value

	// The accelerometer, gyro, and thermometer are set to 1 kHz sample rates,
	// but all these rates are further reduced by a factor of 5 to 200 Hz because of the SMPLRT_DIV setting

  // Configure Interrupts and Bypass Enable
  // Set interrupt pin active high, push-pull, and clear on read of INT_STATUS, enable I2C_BYPASS_EN so additional chips
  // can join the I2C bus and all can be controlled by the Arduino as master
   MPU6515_writeByte(MPU6515_ADDRESS, INT_PIN_CFG, 0x00);
   MPU6515_writeByte(MPU6515_ADDRESS, INT_ENABLE, 0x00);  // Enable data ready (bit 0) interrupt
}

// Function which accumulates gyro and accelerometer data after device initialization. It calculates the average
// of the at-rest readings and then loads the resulting offsets into accelerometer and gyro bias registers.
void MPU6515_calibrateMPU6515(float * dest1, float * dest2)
{
  uint8_t data[12]; // data array to hold accelerometer and gyro x, y, z, data
  uint16_t ii, packet_count, fifo_count;
  int32_t gyro_bias[3] = {0, 0, 0}, accel_bias[3] = {0, 0, 0};

#if 1  /* same */
  MPU6515_resetMPU6515();
#else
// reset device, reset all registers, clear gyro and accelerometer bias registers
  MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_1, 0x80); // Write a one to bit 7 reset bit; toggle reset device
  SystemDelay(100);
#endif

// get stable time source
// Set clock source to be PLL with x-axis gyroscope reference, bits 2:0 = 001
  MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_1, 0x01);
  MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_2, 0x00);

	SystemDelay(200);

// Configure device for bias calculation
  MPU6515_writeByte(MPU6515_ADDRESS, INT_ENABLE, 0x00);   // Disable all interrupts
  MPU6515_writeByte(MPU6515_ADDRESS, FIFO_EN, 0x00);      // Disable FIFO
  MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_1, 0x00);   // Turn on internal clock source
  MPU6515_writeByte(MPU6515_ADDRESS, I2C_MST_CTRL, 0x00); // Disable I2C master
  MPU6515_writeByte(MPU6515_ADDRESS, USER_CTRL, 0x00);    // Disable FIFO and I2C master modes
  MPU6515_writeByte(MPU6515_ADDRESS, USER_CTRL, 0x0C);    // Reset FIFO and DMP

	SystemDelay(20);

// Configure MPU6515 gyro and accelerometer for bias calculation
  MPU6515_writeByte(MPU6515_ADDRESS, CONFIG, 0x01);      // Set low-pass filter to 188 Hz
  MPU6515_writeByte(MPU6515_ADDRESS, SMPLRT_DIV, 0x00);  // Set sample rate to 1 kHz
  MPU6515_writeByte(MPU6515_ADDRESS, GYRO_CONFIG, 0x00);  // Set gyro full-scale to 250 degrees per second, maximum sensitivity
  MPU6515_writeByte(MPU6515_ADDRESS, ACCEL_CONFIG, 0x00); // Set accelerometer full-scale to 2 g, maximum sensitivity

  uint16_t  gyrosensitivity  = 131;   // = 131 LSB/degrees/sec
  uint16_t  accelsensitivity = 16384;  // = 16384 LSB/g

// Configure FIFO to capture accelerometer and gyro data for bias calculation
  MPU6515_writeByte(MPU6515_ADDRESS, USER_CTRL, 0x40);   // Enable FIFO
  MPU6515_writeByte(MPU6515_ADDRESS, FIFO_EN, 0x78);     // Enable gyro and accelerometer sensors for FIFO (max size 512 bytes in MPU-6515)

	SystemDelay(40);

// At end of sample accumulation, turn off FIFO sensor read
  MPU6515_writeByte(MPU6515_ADDRESS, FIFO_EN, 0x00);        // Disable gyro and accelerometer sensors for FIFO
  MPU6515_readBytes(MPU6515_ADDRESS, FIFO_COUNTH, 2, &data[0]); // read FIFO sample count

  fifo_count = ((uint16_t)data[0] << 8) | data[1];
  packet_count = fifo_count/12;// How many sets of full gyro and accelerometer data for averaging

  for (ii = 0; ii < packet_count; ii++) {
    int16_t accel_temp[3] = {0, 0, 0}, gyro_temp[3] = {0, 0, 0};

    MPU6515_readBytes(MPU6515_ADDRESS, FIFO_R_W, 12, &data[0]); // read data for averaging

    accel_temp[0] = (int16_t) (((int16_t)data[0] << 8) | data[1]  ) ;  // Form signed 16-bit integer for each sample in FIFO
    accel_temp[1] = (int16_t) (((int16_t)data[2] << 8) | data[3]  ) ;
    accel_temp[2] = (int16_t) (((int16_t)data[4] << 8) | data[5]  ) ;
    gyro_temp[0]  = (int16_t) (((int16_t)data[6] << 8) | data[7]  ) ;
    gyro_temp[1]  = (int16_t) (((int16_t)data[8] << 8) | data[9]  ) ;
    gyro_temp[2]  = (int16_t) (((int16_t)data[10] << 8) | data[11]) ;

    accel_bias[0] += (int32_t) accel_temp[0]; // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
    accel_bias[1] += (int32_t) accel_temp[1];
    accel_bias[2] += (int32_t) accel_temp[2];
    gyro_bias[0]  += (int32_t) gyro_temp[0];
    gyro_bias[1]  += (int32_t) gyro_temp[1];
    gyro_bias[2]  += (int32_t) gyro_temp[2];

#if defined(ENABLE_DEBUG_SENSOR)
  Trace("ii: %d\n\r", ii);
  Trace("x accel temp = %d\n\r", accel_temp[0]);
  Trace("y accel temp = %d\n\r", accel_temp[1]);
  Trace("z accel temp = %d\n\r", accel_temp[2]);
  Trace("x gyro temp = %d\n\r", gyro_temp[0]);
  Trace("y gyro temp = %d\n\r", gyro_temp[1]);
  Trace("z gyro temp = %d\n\r", gyro_temp[2]);
  Trace("\n\r");
#endif
	}

  accel_bias[0] /= (int32_t) packet_count; // Normalize sums to get average count biases
  accel_bias[1] /= (int32_t) packet_count;
  accel_bias[2] /= (int32_t) packet_count;
  gyro_bias[0]  /= (int32_t) packet_count;
  gyro_bias[1]  /= (int32_t) packet_count;
  gyro_bias[2]  /= (int32_t) packet_count;

#if defined(ENABLE_DEBUG_SENSOR)
  Trace("x accel bias = %d\n\r", accel_bias[0]);
  Trace("y accel bias = %d\n\r", accel_bias[1]);
  Trace("z accel bias = %d\n\r", accel_bias[2]);
  Trace("x gyro bias = %d\n\r", gyro_bias[0]);
  Trace("y gyro bias = %d\n\r", gyro_bias[1]);
  Trace("z gyro bias = %d\n\r", gyro_bias[2]);
  Trace("\n\r");
#endif

//  if(accel_bias[2] > 0L) {
//  	accel_bias[2] -= (int32_t) accelsensitivity;
//	}  // Remove gravity from the z-axis accelerometer bias calculation
//  else {
//  	accel_bias[2] += (int32_t) accelsensitivity;
//	}

// Construct the gyro biases for push to the hardware gyro bias registers, which are reset to zero upon device startup
  data[0] = (-gyro_bias[0]/4  >> 8) & 0xFF; // Divide by 4 to get 32.9 LSB per deg/s to conform to expected bias input format
  data[1] = (-gyro_bias[0]/4)       & 0xFF; // Biases are additive, so change sign on calculated average gyro biases
  data[2] = (-gyro_bias[1]/4  >> 8) & 0xFF;
  data[3] = (-gyro_bias[1]/4)       & 0xFF;
  data[4] = (-gyro_bias[2]/4  >> 8) & 0xFF;
  data[5] = (-gyro_bias[2]/4)       & 0xFF;

/// Push gyro biases to hardware registers
//  MPU6515_writeByte(MPU6515_ADDRESS, XG_OFFSET_H, data[0]);
//  MPU6515_writeByte(MPU6515_ADDRESS, XG_OFFSET_L, data[1]);
//  MPU6515_writeByte(MPU6515_ADDRESS, YG_OFFSET_H, data[2]);
//  MPU6515_writeByte(MPU6515_ADDRESS, YG_OFFSET_L, data[3]);
//  MPU6515_writeByte(MPU6515_ADDRESS, ZG_OFFSET_H, data[4]);
//  MPU6515_writeByte(MPU6515_ADDRESS, ZG_OFFSET_L, data[5]);

  dest1[0] = (float) gyro_bias[0]/(float) gyrosensitivity; // construct gyro bias in deg/s for later manual subtraction
  dest1[1] = (float) gyro_bias[1]/(float) gyrosensitivity;
  dest1[2] = (float) gyro_bias[2]/(float) gyrosensitivity;

// Construct the accelerometer biases for push to the hardware accelerometer bias registers. These registers contain
// factory trim values which must be added to the calculated accelerometer biases; on boot up these registers will hold
// non-zero values. In addition, bit 0 of the lower byte must be preserved since it is used for temperature
// compensation calculations. Accelerometer bias registers expect bias input as 2048 LSB per g, so that
// the accelerometer biases calculated above must be divided by 8.

  int32_t accel_bias_reg[3] = {0, 0, 0}; // A place to hold the factory accelerometer trim biases

  MPU6515_readBytes(MPU6515_ADDRESS, XA_OFFSET_H, 2, &data[0]); // Read factory accelerometer trim values

  accel_bias_reg[0] = (int16_t) ((int16_t)data[0] << 8) | data[1];

  MPU6515_readBytes(MPU6515_ADDRESS, YA_OFFSET_H, 2, &data[0]);

  accel_bias_reg[1] = (int16_t) ((int16_t)data[0] << 8) | data[1];

  MPU6515_readBytes(MPU6515_ADDRESS, ZA_OFFSET_H, 2, &data[0]);

  accel_bias_reg[2] = (int16_t) ((int16_t)data[0] << 8) | data[1];

  uint32_t mask = 1uL; // Define mask for temperature compensation bit 0 of lower byte of accelerometer bias registers
  uint8_t mask_bit[3] = {0, 0, 0}; // Define array to hold mask bit for each accelerometer bias axis

  for(ii = 0; ii < 3; ii++) {
    if(accel_bias_reg[ii] & mask)
    	mask_bit[ii] = 0x01; // If temperature compensation bit is set, record that fact in mask_bit
  }

  // Construct total accelerometer bias, including calculated average accelerometer bias from above
  accel_bias_reg[0] -= (accel_bias[0]/8); // Subtract calculated averaged accelerometer bias scaled to 2048 LSB/g (16 g full scale)
  accel_bias_reg[1] -= (accel_bias[1]/8);
  accel_bias_reg[2] -= (accel_bias[2]/8);

  data[0] = (accel_bias_reg[0] >> 8) & 0xFF;
  data[1] = (accel_bias_reg[0])      & 0xFF;
  data[1] = data[1] | mask_bit[0]; // preserve temperature compensation bit when writing back to accelerometer bias registers
  data[2] = (accel_bias_reg[1] >> 8) & 0xFF;
  data[3] = (accel_bias_reg[1])      & 0xFF;
  data[3] = data[3] | mask_bit[1]; // preserve temperature compensation bit when writing back to accelerometer bias registers
  data[4] = (accel_bias_reg[2] >> 8) & 0xFF;
  data[5] = (accel_bias_reg[2])      & 0xFF;
  data[5] = data[5] | mask_bit[2]; // preserve temperature compensation bit when writing back to accelerometer bias registers

// Apparently this is not working for the acceleration biases in the MPU-6515
// Are we handling the temperature correction bit properly?
// Push accelerometer biases to hardware registers
//  MPU6515_writeByte(MPU6515_ADDRESS, XA_OFFSET_H, data[0]);
//  MPU6515_writeByte(MPU6515_ADDRESS, XA_OFFSET_L, data[1]);
//  MPU6515_writeByte(MPU6515_ADDRESS, YA_OFFSET_H, data[2]);
//  MPU6515_writeByte(MPU6515_ADDRESS, YA_OFFSET_L, data[3]);
//  MPU6515_writeByte(MPU6515_ADDRESS, ZA_OFFSET_H, data[4]);
//  MPU6515_writeByte(MPU6515_ADDRESS, ZA_OFFSET_L, data[5]);

// Output scaled accelerometer biases for manual subtraction in the main program
	dest2[0] = (float)accel_bias[0]/(float)accelsensitivity;
	dest2[1] = (float)accel_bias[1]/(float)accelsensitivity;
	dest2[2] = (float)accel_bias[2]/(float)accelsensitivity;
}

/* If 'INT_ANYRD_2CLEAR' is 0, Interrupt status is cleared only by reading INT_STATUS register */
char MPU6515_Read_IntStatusRegister()
{
	return MPU6515_readByte(MPU6515_ADDRESS, INT_STATUS);
}

/* Get WOM_INT_Register */
bool MPU6515_CheckInterrupt()
{
	char temp_register;

	temp_register = MPU6515_Read_IntStatusRegister();

	Trace("\r\n  INT_STATUS: 0x%02x\r\n\r\n", temp_register);//for test

	if (temp_register & 0x40) /* WOM_INT */
		return TRUE;

	return FALSE;
}

void MPU6515_MakeAccelRunning(boolean_t bGyroActive)
{
#if 1
    // disable temperature sensor
	MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_1, 0x08);//set defualt clock
    if( bGyroActive == true )
        MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_2, 0x00);					// Enable Accel X/Y/Z, Gyro X/Y/Z
    else
	    MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_2, 0x07);				// Disable Gyro X/Y/Z

#else

	char temp_register;

	temp_register = MPU6515_readByte(MPU6515_ADDRESS, PWR_MGMT_1);
	temp_register &= ~(0x70);
	MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_1, temp_register);

	temp_register = MPU6515_readByte(MPU6515_ADDRESS, PWR_MGMT_2);
	temp_register &= ~(0x38);
	temp_register |= 0x07;
	MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_2, temp_register);
#endif
}

/* set 184 Hz Bandwidth */
void MPU6515_SetAccelLPF()
{
#if 1
	MPU6515_writeByte(MPU6515_ADDRESS, ACCEL_CONFIG2, 0x01);

#else
	char temp_register;

	temp_register = MPU6515_readByte(MPU6515_ADDRESS, ACCEL_CONFIG2);
	temp_register &= ~(0x0E);
	temp_register |= 0x01;
	MPU6515_writeByte(MPU6515_ADDRESS, ACCEL_CONFIG2, temp_register);
#endif
}

/* Add 0x37, INT_ANYRD_2CLEAR */
void MPU6515_SetInterruptClearState(bool interruptClearStatus)
{
#if 1
	//MPU6515_writeByte(MPU6515_ADDRESS, INT_PIN_CFG, 0x70);
	//MPU6515_writeByte(MPU6515_ADDRESS, INT_PIN_CFG, 0x60);
	MPU6515_writeByte(MPU6515_ADDRESS, INT_PIN_CFG, 0x60);
	//MPU6515_writeByte(MPU6515_ADDRESS, INT_PIN_CFG, 0x20);

#else

	char temp_register;

	temp_register = MPU6515_readByte(MPU6515_ADDRESS, INT_PIN_CFG);
	if (interruptClearStatus)
		temp_register |= 0x10;
	else
		temp_register &= ~(0x10);


	//temp_register |= 0x68;//for test

	temp_register = 0x20;//for test

	MPU6515_writeByte(MPU6515_ADDRESS, INT_PIN_CFG, temp_register);
#endif
}

void MPU6515_EnableMotionInterrupt(bool bActiveWom)
{
    if( bActiveWom == true )
	    MPU6515_writeByte(MPU6515_ADDRESS, INT_ENABLE, 0x40);				// enable WON interrupt
	else
    	MPU6515_writeByte(MPU6515_ADDRESS, INT_ENABLE, 0x00);				// disable WON interrupt
}

void MPU6515_EnableAccelHardwareIntelligence()
{
#if 1
	MPU6515_writeByte(MPU6515_ADDRESS, MOT_DETECT_CTRL, 0xC0);
    /*
    BIT     NAME                FUNCTION
    [7]     ACCEL_INTEL_EN      This bit enables the Wake-on-Motion detection logic.
    [6]     ACCEL_INTEL_MODE    This bit defines
                                1 = Compare the current sample with the previous sample.
                                0 = Not used. */

#else

	char temp_register;

	temp_register = MPU6515_readByte(MPU6515_ADDRESS, MOT_DETECT_CTRL);
	temp_register |= 0xC0;
	MPU6515_writeByte(MPU6515_ADDRESS, MOT_DETECT_CTRL, temp_register);
#endif
}

void MPU6515_SetMotionThreshold(char threshold)
{
	MPU6515_writeByte(MPU6515_ADDRESS, WOM_THR, threshold);
}

void MPU6515_SetWakeupFrequency()
{
#if 1
	MPU6515_writeByte(MPU6515_ADDRESS, LP_ACCEL_ODR, 0x00);//0.24 Hz
	//MPU6515_writeByte(MPU6515_ADDRESS, LP_ACCEL_ODR, 0x01);//0.49 Hz
	//MPU6515_writeByte(MPU6515_ADDRESS, LP_ACCEL_ODR, 0x03);//1.95 Hz
	//MPU6515_writeByte(MPU6515_ADDRESS, LP_ACCEL_ODR, 0x04);//3.91 Hz
	//MPU6515_writeByte(MPU6515_ADDRESS, LP_ACCEL_ODR, 0x05);//7.81 Hz
	//MPU6515_writeByte(MPU6515_ADDRESS, LP_ACCEL_ODR, 0x06);//15.63 Hz
	//MPU6515_writeByte(MPU6515_ADDRESS, LP_ACCEL_ODR, 0x0B);//500 Hz
#else
	char temp_register;

	temp_register = MPU6515_readByte(MPU6515_ADDRESS, LP_ACCEL_ODR);
	temp_register &= ~(0x0F);
	temp_register |= 0x01;
	MPU6515_writeByte(MPU6515_ADDRESS, LP_ACCEL_ODR, temp_register);
#endif
}

void MPU6515_EnableAccelLowPowerMode()
{
#if 1
    // In order to prevent deep sleep a MPU-6515, do not set cicle bit.
    // it cause MPU-6515 to go sleep a few minutes later.
	MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_1, 0x08);
#else
	char temp_register;

	temp_register = MPU6515_readByte(MPU6515_ADDRESS, PWR_MGMT_1);
	temp_register |= 0x20;
	//temp_register &= ~(0x20);
	//temp_register = 0;
	MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_1, temp_register);
#endif
}

void MPU6515_ConfigurationWakeUpOnMotionInterrupt(boolean_t bWomActive, boolean_t bGyroActive, unsigned char ucValue)
{
	//char temp_register;
	//Trace("\r\n MPU6515_ConfigurationWakeUpOnMotionInterrupt. \r\n");
	//MPU6515_writeByte(MPU6515_ADDRESS, I2C_MST_CTRL, 0x00); // Disable I2C master

	MPU6515_MakeAccelRunning(bGyroActive);

	//temp_register = MPU6515_readByte(MPU6515_ADDRESS, PWR_MGMT_1);
	//Trace("\r\n  0x%02x: 0x%02x\r\n\r\n",PWR_MGMT_1 ,temp_register);//for test
	//temp_register = MPU6515_readByte(MPU6515_ADDRESS, PWR_MGMT_2);
	//Trace("\r\n  0x%02x: 0x%02x\r\n\r\n",PWR_MGMT_2 ,temp_register);//for test

    //MPU6515_writeByte(MPU6515_ADDRESS, SIGNAL_PATH_RESET, 0x07);

	MPU6515_SetAccelLPF();

	//temp_register = MPU6515_readByte(MPU6515_ADDRESS, ACCEL_CONFIG2);
	//Trace("\r\n  0x%02x: 0x%02x\r\n\r\n", ACCEL_CONFIG2,temp_register);//for test

	/* Add 0x37, Clear 'INT_ANYRD_2CLEAR' */
	//MPU6515_SetInterruptClearState(FALSE);
	//temp_register = MPU6515_readByte(MPU6515_ADDRESS, INT_PIN_CFG);
	//Trace("\r\n  0x%02x: 0x%02x\r\n\r\n", INT_PIN_CFG,temp_register);//for test

	//MPU6515_SetMotionThreshold(0xAF);/* Need turnning. */
	MPU6515_SetMotionThreshold(ucValue);/* Need turnning. */
	//temp_register = MPU6515_readByte(MPU6515_ADDRESS, WOM_THR);
	//Trace("\r\n  0x%02x: 0x%02x\r\n\r\n", WOM_THR,temp_register);//for test

	MPU6515_SetWakeupFrequency();
	//temp_register = MPU6515_readByte(MPU6515_ADDRESS, LP_ACCEL_ODR);
	//Trace("\r\n  0x%02x: 0x%02x\r\n\r\n", LP_ACCEL_ODR,temp_register);//for test

	MPU6515_EnableAccelHardwareIntelligence();
	//temp_register = MPU6515_readByte(MPU6515_ADDRESS, MOT_DETECT_CTRL);
	//Trace("\r\n  0x%02x: 0x%02x\r\n\r\n", MOT_DETECT_CTRL,temp_register);//for test

	MPU6515_EnableMotionInterrupt(bWomActive);
	//temp_register = MPU6515_readByte(MPU6515_ADDRESS, INT_ENABLE);
	//Trace("\r\n  0x%02x: 0x%02x\r\n\r\n", INT_ENABLE,temp_register);//for test

	//MPU6515_EnableAccelLowPowerMode();
	//temp_register = MPU6515_readByte(MPU6515_ADDRESS, PWR_MGMT_1);
	//Trace("\r\n  0x%02x: 0x%02x\r\n\r\n", PWR_MGMT_1,temp_register);//for test
	//temp_register = MPU6515_readByte(MPU6515_ADDRESS, I2C_MST_CTRL);//0x24 address
	//Trace("\r\n  0x%02x: 0x%02x\r\n\r\n",I2C_MST_CTRL ,temp_register);//for test
}

//************************************************************************************************************************************
// 센서 체크 manager 함수
//************************************************************************************************************************************
void InitializeSensorManager(void)
{
    SensorData.g_eSensorState = eSENSOR_STATE_INIT;
    SensorData.ucVehicleMode = eSysModeNone;

    SensorData.g_bEnableSensor_MPU6515 = false;

    SensorData.Ascale = AFS_2G;     // AFS_2G, AFS_4G, AFS_8G, AFS_16G
    SensorData.Gscale = GFS_250DPS; // GFS_250DPS, GFS_500DPS, GFS_1000DPS, GFS_2000DPS
    SensorData.Mscale = MFS_14BITS; // MFS_14BITS or MFS_16BITS, 14-bit or 16-bit magnetometer resolution
    SensorData.Mmode = 0x06;        // Either 8 Hz 0x02) or 100 Hz (0x06) magnetometer data ODR

    Kalman_Q_angle = 0.001;
    Kalman_Q_bias = 0.003;
    Kalman_R_measure = 0.03;

    Kalman_X_bias = 0; // Reset bias
    Kalman_X_P[0][0] = 0; // Since we assume tha the bias is 0 and we know the starting angle (use setAngle), the error covariance matrix is set like so - see: http://en.wikipedia.org/wiki/Kalman_filter#Example_application.2C_technical
    Kalman_X_P[0][1] = 0;
    Kalman_X_P[1][0] = 0;
    Kalman_X_P[1][1] = 0;

    Kalman_Y_bias = 0; // Reset bias
    Kalman_Y_P[0][0] = 0; // Since we assume tha the bias is 0 and we know the starting angle (use setAngle), the error covariance matrix is set like so - see: http://en.wikipedia.org/wiki/Kalman_filter#Example_application.2C_technical
    Kalman_Y_P[0][1] = 0;
    Kalman_Y_P[1][0] = 0;
    Kalman_Y_P[1][1] = 0;

    GetGyroInitializeAngle(&SensorData.stGyroAngleDefault);

    Trace("Gyro Default X:%d, Y:%d, Z:%d\n",SensorData.stGyroAngleDefault.nX,
        SensorData.stGyroAngleDefault.nY,
        SensorData.stGyroAngleDefault.nZ);

	return;
}

void SetSensorState(eSENSOR_STATE state)
{
	char *strState;

	switch ( state ) {
		case eSENSOR_STATE_INIT:
			strState = "eSENSOR_STATE_INIT";
			break;

		case eSENSOR_STATE_WAIT:
			strState = "eSENSOR_STATE_WAIT";
			break;

		case eSENSOR_STATE_WHO_AM_I_MPU6515:
			strState = "eSENSOR_STATE_WHO_AM_I_MPU6515";
			break;

		case eSENSOR_STATE_RESET_MPU6515:
			strState = "eSENSOR_STATE_RESET_MPU6515";
			break;

		case eSENSOR_STATE_INIT_MPU6515:
			strState = "eSENSOR_STATE_INIT_MPU6515";
			break;

		case eSENSOR_STATE_GET_SENSITIVITY_MPU6515:
			strState = "eSENSOR_STATE_GET_SENSITIVITY_MPU6515";
			break;

		case eSENSOR_STATE_GET_DATA_MPU6515:
			strState = "eSENSOR_STATE_GET_DATA_MPU6515";
			break;

		case eSENSOR_STATE_PAUSE:
			strState = "eSENSOR_STATE_PAUSE";
			break;

		case eSENSOR_STATE_SLEEP_ALL_SENSOR:
			strState = "eSENSOR_STATE_SLEEP_ALL_SENSOR";
			break;

		case eSENSOR_STATE_IDLE:
			strState = "eSENSOR_STATE_IDLE";
			break;

		default:
			strState = "Unknown State";
			break;
	}

	SensorData.g_eSensorState = state;
	Trace("*%s : %s \r\n", __FUNCTION__, strState);

	return;
}

eSENSOR_STATE GetSensorState()
{
	return SensorData.g_eSensorState;
}

///////////////////////////////////////////////////////////////////////////////
#include "MngSystem.h"
#include "MngQueue.h"


void HandleMngSensorExternalEvent(stMsgSensor* pstSensor);
void SetupForInterruptforImpulse(bool bWomActive,bool bGyroActive, unsigned char ncValue,bool bHelpInterrupSignal);

void GIT_SensorManager(void)
{
	stMsgSensor tmpSensorData;
	uint8_t whoami;
	int16_t accelCount[3];  // Stores the 16-bit signed accelerometer sensor output
	int16_t gyroCount[3];   // Stores the 16-bit signed gyro sensor output
//	int16_t tempCount;   // Stores the real internal chip temperature in degrees Celsius

	static eSENSOR_STATE eBackupSensorState;
	static float gyroBias[3] = {0, 0, 0}; // Bias corrections for gyro and accelerometer
	static float accelBias[3] = {0, 0, 0}; // Bias corrections for gyro and accelerometer
	float fTempX, fTempY, fTempZ;
	int nTempX, nTempY, nTempZ;

	switch ( SensorData.g_eSensorState ) {
		case eSENSOR_STATE_INIT:

			SensorData.g_iTimerDataRateMPU6515 = HalTimerSetSWTimer(SENSOR_GET_TIME_OUT, eSWTimer_ONESHOT, NULL, TRUE);
			eBackupSensorState = eSENSOR_STATE_WHO_AM_I_MPU6515;
			SensorData.g_eSensorState = eSENSOR_STATE_WAIT;
			break;

		case eSENSOR_STATE_WAIT:
			if(HalTimerGetSwTimerCount(SensorData.g_iTimerDataRateMPU6515) == 0) {
				SensorData.g_eSensorState = eBackupSensorState;

				HalTimerChangeSWTimer(SensorData.g_iTimerDataRateMPU6515, SENSOR_GET_TIME_OUT, eSWTimer_ONESHOT, NULL, FALSE);
			}
			break;

		//************************************************************************************************************************************
		// MPU-6515: 자이로 센서 초기화
		//************************************************************************************************************************************
		case eSENSOR_STATE_WHO_AM_I_MPU6515:
            MPU6515_initValue();

            // Read WHO_AM_I register for MPU-6515
            // Read the WHO_AM_I register, this is a good test of communication
            whoami = MPU6515_readByte(MPU6515_ADDRESS, WHO_AM_I_MPU6515);

#if defined(ENABLE_DEBUG_SENSOR)
            Trace("I SHOULD BE 0x74, I AM 0x%x\r\n", whoami);
#endif
            // WHO_AM_I should always be 0x74  // MPU6515_ADDRESS /* Need to check. */
            if (whoami == 0x74)
            {
                if( SensorData.ucVehicleMode == eSysModeParking )
                {
                    // Reset registers to default in preparation for device calibration
                    MPU6515_resetMPU6515();

                    // disalbe all interrupt of sensor
                    MPU6515_writeByte(MPU6515_ADDRESS, INT_ENABLE, 0x00);
                    // disable gyro and accelerometer
                    MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_2, 0x3F);

                    eBackupSensorState = eSENSOR_STATE_IDLE;
                    SensorData.g_eSensorState = eSENSOR_STATE_WAIT;
                }
                else
                {
                    eBackupSensorState = eSENSOR_STATE_RESET_MPU6515;
                    SensorData.g_eSensorState = eSENSOR_STATE_WAIT;
                }
            }
            else
            {
                Trace("\r\n [Error] Could not connect to MPU6515: 0x%x\n\r", whoami);
                SensorData.g_bEnableSensor_MPU6515 = false;
                SensorData.g_eSensorState = eSENSOR_STATE_WHO_AM_I_MPU6515;
            }
            break;

		case eSENSOR_STATE_RESET_MPU6515:
	    	MPU6515_resetMPU6515(); 		// Reset registers to default in preparation for device calibration

			//MPU6515_calibrateMPU6515(gyroBias, accelBias); 	// Calibrate gyro and accelerometers, load biases in bias registers

#if defined(ENABLE_DEBUG_SENSOR)
            Trace("x gyro bias = %f\n\r", gyroBias[0]);
            Trace("y gyro bias = %f\n\r", gyroBias[1]);
            Trace("z gyro bias = %f\n\r", gyroBias[2]);
            Trace("x accel bias = %f\n\r", accelBias[0]);
            Trace("y accel bias = %f\n\r", accelBias[1]);
            Trace("z accel bias = %f\n\r", accelBias[2]);
#endif
			SystemDelay(10);


			MPU6515_writeByte(MPU6515_ADDRESS, SIGNAL_PATH_RESET, 0x0D);
			MPU6515_writeByte(MPU6515_ADDRESS, USER_CTRL, 0x4F);//

			//MPU6515_writeByte(MPU6515_ADDRESS, USER_CTRL, 0x80);//DMP On


			MPU6515_ConfigurationWakeUpOnMotionInterrupt(false, false, 0xAF);
			MPU6515_SetInterruptClearState(FALSE);

			/* for interrupt clear */

			SystemDelay(10);
			MPU6515_CheckInterrupt();
			SystemDelay(10);
			MPU6515_CheckInterrupt();
			SystemDelay(10);

			eBackupSensorState = eSENSOR_STATE_INIT_MPU6515;
			SensorData.g_eSensorState = eSENSOR_STATE_WAIT;
			break;

		case eSENSOR_STATE_INIT_MPU6515:
	    	MPU6515_initMPU6515();

#if defined(ENABLE_DEBUG_SENSOR)
	    	Trace("MPU6515 initialized for active data mode....\n\r"); // Initialize device for active mode read of acclerometer, gyroscope, and temperature
	    	Trace("Accelerometer full-scale range = %f  g\n\r", 2.0f * (float)(1 << SensorData.Ascale));
	    	Trace("Gyroscope full-scale range = %f  deg/s\n\r", 250.0f * (float)(1 << SensorData.Gscale));
#endif
			eBackupSensorState = eSENSOR_STATE_GET_SENSITIVITY_MPU6515;
			SensorData.g_eSensorState = eSENSOR_STATE_WAIT;
			break;

		case eSENSOR_STATE_GET_SENSITIVITY_MPU6515:
			MPU6515_getAres(); // Get accelerometer sensitivity
			MPU6515_getGres(); // Get gyro sensitivity

#if defined(ENABLE_DEBUG_SENSOR)
			Trace("Accelerometer sensitivity is %f LSB/g \n\r", 1.0f / SensorData.aRes);
			Trace("Gyroscope sensitivity is %f LSB/deg/s \n\r", 1.0f / SensorData.gRes);
#endif

			SensorData.g_bEnableSensor_MPU6515 = true;
			HalTimerStartSWTimer(SensorData.g_iTimerDataRateMPU6515, eSWTimer_ONESHOT);

			SensorData.g_eSensorState = eSENSOR_STATE_IDLE;
			break;

		case eSENSOR_STATE_DEFINE_DEFAULT_ANGLE:
			if(HalTimerGetSwTimerCount(SensorData.g_iTimerDataRateMPU6515) != 0) {
				break;
			}

			if(SensorData.g_bEnableSensor_MPU6515 == true) {
				long int wSumXAngle = 0;
				long int wSumYAngle = 0;
				long int wSumZAngle = 0;
				int x;

				for(x = 0; x < 10; x++) {
					MPU6515_readAccelData(accelCount);  // Read the x/y/z adc values
					// Now we'll calculate the accleration value into actual g's

//					Trace("acc-> x: %d, y: %d, z: %d\n", accelCount[0], accelCount[1], accelCount[2]);
					// 보드를 수평으로 정지한 상태에서 평균을 계산하여 보정을 수행한다. 보정값은 accelBias[3]에 보관한다.
//					fTempX = (float)(accelCount[0] * SensorData.aRes - accelBias[0]);  // get actual g value, this depends on scale being set
//					fTempY = (float)(accelCount[1] * SensorData.aRes - accelBias[1]);
//					fTempZ = (float)(accelCount[2] * SensorData.aRes - accelBias[2]);

					// 정차한 상태에서 계산된 평균값(보정값)을 적용하지 않은 경우
					fTempX = (float)(accelCount[0] * SensorData.aRes);  // get actual g value, this depends on scale being set
					fTempY = (float)(accelCount[1] * SensorData.aRes);
					fTempZ = (float)(accelCount[2] * SensorData.aRes);

					nTempX = (int)(atan(fTempX / sqrt(pow(fTempZ, 2) + pow(fTempY, 2))) * 180 / 3.14);
					nTempY = (int)(atan(fTempY / sqrt(pow(fTempZ, 2) + pow(fTempX, 2))) * 180 / 3.14);
					nTempZ = (int)(atan(fTempZ / sqrt(pow(fTempX, 2) + pow(fTempY, 2))) * 180 / 3.14);


#if defined(ENABLE_DEBUG_MPU6515)
					Trace("GYRO X: %d, Y: %d, Z: %d\n", nTempX, nTempY, nTempZ);
#endif
					wSumXAngle += nTempX;
					wSumYAngle += nTempY;
					wSumZAngle += nTempZ;

					SystemDelay(10);
				}

				gnBiasXAngle = wSumXAngle / 10;
				gnBiasYAngle = wSumYAngle / 10;
				gnBiasZAngle = wSumZAngle / 10;

#if defined(ENABLE_DEBUG_MPU6515)
				Trace("gnBiasXAngle: %d, gnBiasYAngle: %d, gnBiasZAngle: %d\n", gnBiasXAngle, gnBiasYAngle, gnBiasZAngle);
#endif

//				nTempX = (nTempX > 0) ? (nTempX - gnBiasXAngle) : (nTempX + gnBiasXAngle);
//				nTempY = (nTempY > 0) ? (nTempY - gnBiasYAngle) : (nTempY + gnBiasYAngle);
//				nTempZ = (nTempZ > 0) ? (nTempZ - gnBiasZAngle) : (nTempZ + gnBiasZAngle);

				nTempX = nTempX - gnBiasXAngle;
				nTempY = nTempY - gnBiasYAngle;
				nTempZ = nTempZ - gnBiasZAngle;

				// 상보필터 값의 초기값을 정한다.
				compAngleX = nTempX;
				compAngleY = nTempY;
			}

			HalTimerStartSWTimer(SensorData.g_iTimerDataRateMPU6515, eSWTimer_ONESHOT);

			SensorData.g_eSensorState = eSENSOR_STATE_GET_DATA_MPU6515;
			break;

		//************************************************************************************************************************************
		// Read Sensor Data
		//************************************************************************************************************************************
#if true
		case eSENSOR_STATE_GET_DATA_MPU6515:
			SensorData.g_eSensorState = eSENSOR_STATE_GET_DATA_MPU6515;

            // wait time out
			if(HalTimerGetSwTimerCount(SensorData.g_iTimerDataRateMPU6515) != 0) {
				break;
			}

#if true
#if 1 //for test, interrupt

			if(SensorData.g_bEnableSensor_MPU6515 == true) {

			  //if(MPU6515_readByte(MPU6515_ADDRESS, INT_STATUS) & 0x01) {  // On interrupt, check if data ready interrupt

				  	MPU6515_readAccelData(accelCount);  // Read the x/y/z adc values
					MPU6515_readGyroData(gyroCount);

					fTempX = (float)(accelCount[0] * SensorData.aRes);  // get actual g value, this depends on scale being set
					fTempY = (float)(accelCount[1] * SensorData.aRes);
					fTempZ = (float)(accelCount[2] * SensorData.aRes);

                    nTempX = (int)(atan(fTempY / sqrt(pow(fTempX, 2) + pow(fTempZ, 2))) * 180 / 3.14);
                    nTempY = (int)(atan(fTempX / sqrt(pow(fTempY, 2) + pow(fTempZ, 2))) * 180 / 3.14);
                    nTempZ = (int)(atan(fTempY / sqrt(pow(fTempX, 2) + pow(fTempY, 2))) * 180 / 3.14);

#if defined(ENABLE_DEBUG_MPU6515)
					//Trace("accel X: %d, Y: %d, Z: %d\n", nTempX, nTempY, nTempZ);
#endif
					nTempX = nTempX - gnBiasXAngle;
					nTempY = nTempY - gnBiasYAngle;
					nTempZ = nTempZ - gnBiasZAngle;

#if defined(ENABLE_DEBUG_MPU6515)
                    Trace("X: %.0f, Y: %.0f, Z: %.0f\n", fTempX, fTempY, fTempZ);
					Trace("BiasX: %d, BiasY: %d, BiasZ: %d\n", gnBiasXAngle, gnBiasYAngle, gnBiasZAngle);
					Trace("X: %d, Y: %d, Z: %d\n", nTempX, nTempY, nTempZ);
					Trace("Cx: %d, Cy: %d\n", (int)compAngleX, (int)compAngleY);				// 상보 필터 적용한 X, Y 각도는 따로 계산한다.
					Trace("Kx: %d, Ky: %d\n\n", (int)kalAngleX, (int)kalAngleY);				// 칼만 필터 적용한 X, Y 각도는 따로 계산한다.
#endif

					SensorData.nAngleX = nTempX;
					SensorData.nAngleY = nTempY;
					SensorData.nAngleZ = nTempZ;

					CalGyroAngle(gyroCount, gyroBias);
					CalAccelAngle(accelCount, accelBias);
					CalComplimentValue();
#endif
			  	//}
			}
#endif
			HalTimerStartSWTimer(SensorData.g_iTimerDataRateMPU6515, eSWTimer_ONESHOT);
			break;
#endif
		case eSENSOR_STATE_PAUSE:
			break;

		case eSENSOR_STATE_SLEEP_ALL_SENSOR:
			break;

		case eSENSOR_STATE_IDLE:
			break;

		default:
			break;
	}

#if false
	if(SensorData.g_eSensorState == eSENSOR_STATE_GET_DATA_MPU6515)
    {
        static int nCurrentTmr = 0, nPreTmr = 0;
        int nDeltaTmr;
        double gyroXrate;
        double gyroYrate;

        nCurrentTmr = Get_Tmr();
        nDeltaTmr = Get_TmrDelta(nCurrentTmr, nPreTmr);

        gfdt = (float)(nDeltaTmr);
        gfdt = gfdt / 1000.0;

        nPreTmr = nCurrentTmr;

        MPU6515_readGyroData(gyroCount);  // Read the x/y/z adc values

        // Calculate the gyro value into actual degrees per second
        gyroXrate = (float)(gyroCount[0] * SensorData.gRes - gyroBias[0]);  // get actual gyro value, this depends on scale being set
        gyroYrate = (float)(gyroCount[1] * SensorData.gRes - gyroBias[1]);

        // 상보 필터
        compAngleX = (0.93 * (compAngleX + (gyroXrate * gfdt))) + (0.07 * SensorData.fAccX); // Calculate the angle using a Complimentary filter
        compAngleY = (0.93 * (compAngleY + (gyroYrate * gfdt))) + (0.07 * SensorData.fAccY);

        // 칼만 필터
      //  kalAngleX = Kalman_getXAngle(SensorData.fAccX, gyroXrate, gfdt); // Calculate the angle using a Kalman filter
      //  kalAngleY = Kalman_getYAngle(SensorData.fAccY, gyroYrate, gfdt);

		//printf("kalman %lf %lf \r\n",kalAngleX, kalAngleY);
	}
#endif
    // check external event
#ifndef GLOBAL_SHARE_QUEUE //Get, check
    if( MngQueueGetMessage(ID_MNG_QUEUE_SENSOR, (int8_t*)&tmpSensorData, sizeof(tmpSensorData)) == true )
#else
	if( GetSysHdShareQueueMessage(ID_MNG_QUEUE_SENSOR, (uint8_t*)&tmpSensorData.header, sizeof(stMsgHeader), (uint8_t*)&tmpSensorData.carReport, sizeof(stCarReport)) == true )
#endif
    {
#if defined(MNG_QUEUE_DEBUG)
        Trace("%s]Get] buffer[0]:%d,buffer[1]:%d\n",__FUNCTION__,buffer[0],buffer[1]);
#endif

        // external event process
        HandleMngSensorExternalEvent(&tmpSensorData);
    }
}

void MeasureGyroAngle()
{
	int16_t accelCount[3];	// Stores the 16-bit signed accelerometer sensor output
	int16_t gyroCount[3];	// Stores the 16-bit signed gyro sensor output
	float gyroBias[3] = {0, 0, 0}; // Bias corrections for gyro and accelerometer
	float accelBias[3] = {0, 0, 0}; // Bias corrections for gyro and accelerometer
	float fTempX, fTempY, fTempZ;
	int nTempX, nTempY, nTempZ;

	MPU6515_readGyroData(gyroCount);
	MPU6515_readAccelData(accelCount);  // Read the x/y/z adc values


	fTempX = (float)(accelCount[0] * SensorData.aRes);  // get actual g value, this depends on scale being set
	fTempY = (float)(accelCount[1] * SensorData.aRes);
	fTempZ = (float)(accelCount[2] * SensorData.aRes);

    nTempX = (int)(atan(fTempY / sqrt(pow(fTempX, 2) + pow(fTempZ, 2))) * 180 / 3.14);
    nTempY = (int)(atan(fTempX / sqrt(pow(fTempY, 2) + pow(fTempZ, 2))) * 180 / 3.14);
    nTempZ = (int)(atan(fTempY / sqrt(pow(fTempX, 2) + pow(fTempY, 2))) * 180 / 3.14);

#if defined(ENABLE_DEBUG_MPU6515)
		//Trace("accel X: %d, Y: %d, Z: %d\n", nTempX, nTempY, nTempZ);
	Trace("accel X: %d, Y: %d, Z: %d\n", nTempX, nTempY, nTempZ);
#endif

	nTempX = nTempX - gnBiasXAngle;
	nTempY = nTempY - gnBiasYAngle;
	nTempZ = nTempZ - gnBiasZAngle;

#if defined(ENABLE_DEBUG_MPU6515)
    Trace("X: %.0f, Y: %.0f, Z: %.0f\n", fTempX, fTempY, fTempZ);
	Trace("BiasX: %d, BiasY: %d, BiasZ: %d\n", gnBiasXAngle, gnBiasYAngle, gnBiasZAngle);
	Trace("X: %d, Y: %d, Z: %d\n", nTempX, nTempY, nTempZ);
	Trace("Cx: %d, Cy: %d\n", (int)compAngleX, (int)compAngleY);				// 상보 필터 적용한 X, Y 각도는 따로 계산한다.
	Trace("Kx: %d, Ky: %d\n\n", (int)kalAngleX, (int)kalAngleY);				// 칼만 필터 적용한 X, Y 각도는 따로 계산한다.
	Trace("=== MeasureGyroAngle Original X: %f, Y: %f, Z: %f\n", fTempX, fTempY, fTempZ);
#endif

	SensorData.nAngleX = nTempX;
	SensorData.nAngleY = nTempY;
	SensorData.nAngleZ = nTempZ;

	CalGyroAngle(gyroCount, gyroBias);
	CalAccelAngle(accelCount, accelBias);
	CalComplimentValue();

	/*
	pstGyroAngle->nX = nTempX;
	pstGyroAngle->nY = nTempY;
	pstGyroAngle->nZ = nTempZ;
	*/

	//Trace(" === MeasureGyroAngle X: %d, Y: %d, Z: %d === \n", pstGyroAngle->nX, pstGyroAngle->nY, pstGyroAngle->nZ);
}

void CalGyroAngle(int16_t *gyroCount, float *gyroBias)
{
	static int uiNowTime = 0, uiPreTime, uiDeltaTime;
	float	fDt=0;

	float fGyroAngleX = 0;
	float fGyroAngleY = 0;
	float fGyroAngleZ = 0;

	uiNowTime = Get_Tmr();
	uiDeltaTime = Get_TmrDelta(uiNowTime, uiPreTime);
	fDt = (float)uiDeltaTime/1000.0;
	uiPreTime = uiNowTime;

	fGyroAngleX = ((float)gyroCount[0] * SensorData.gRes-gyroBias[0]) * fDt; // 각도 // ((float)gyroCount[0] * SensorData.gRes-gyroBias[0]) -> 각속도
	fGyroAngleY = ((float)gyroCount[1] * SensorData.gRes-gyroBias[1]) * fDt;
	fGyroAngleZ = ((float)gyroCount[2] * SensorData.gRes-gyroBias[2]) * fDt;

	SensorData.fGyroX = fGyroAngleX;
	SensorData.fGyroY = fGyroAngleY;
	SensorData.fGyroZ = fGyroAngleZ;


}
void CalAccelAngle(int16_t *accelCount, float *accelBias)
{

	//float alpha = 0.93;
	float radians_to_degrees = (180/3.141592);
	float fAccelX = 0;
	float fAccelY = 0;
	float fAccelZ = 0;

	float fAccelAngleX = 0;
	float fAccelAngleY = 0;
	float fAccelAngleZ = 0;

	fAccelX = ((float)accelCount[0] * SensorData.aRes - accelBias[0]);
	fAccelY = ((float)accelCount[1] * SensorData.aRes - accelBias[1]);
	fAccelZ = ((float)accelCount[2] * SensorData.aRes - accelBias[2]);

	fAccelAngleX = atan(fAccelY / sqrt(pow(fAccelX,2) + pow(fAccelZ,2))) * radians_to_degrees;
	fAccelAngleY = atan(-1 * fAccelX / sqrt(pow(fAccelY,2) + pow(fAccelZ,2))) * radians_to_degrees;
	fAccelAngleZ = 0;

	SensorData.fAccX = fAccelAngleX;
	SensorData.fAccY = fAccelAngleY;
	SensorData.fAccZ = fAccelAngleZ;

}

void CalComplimentValue(void)
{

	static float s_fAngleX = 0;
	static float s_fAngleY = 0;
	static float s_fAngleZ = 0;

	float alpha = 0.93;

	s_fAngleX = (alpha * SensorData.fGyroX) + ((1.0 - alpha) * SensorData.fAccX);
	s_fAngleY = (alpha * SensorData.fGyroY) + ((1.0 - alpha) * SensorData.fAccY);
	s_fAngleZ = SensorData.fGyroZ;

	SensorData.nAngleX += (int)s_fAngleX;
	SensorData.nAngleY += (int)s_fAngleY;
	SensorData.nAngleZ += (int)s_fAngleZ;

	//printf("------------\r\nAngle] %d %d %d\r\n------------\r\n",SensorData.nAngleX,SensorData.nAngleY,SensorData.nAngleZ); // woong bae
}

float Kalman_getXAngle(float newAngle, float newRate, float dt)
{
  // KasBot V2 - Kalman filter module - http://www.x-firm.com/?page_id=145
  // Modified by Kristian Lauszus
  // See my blog post for more information: http://blog.tkjelectronics.dk/2012/09/a-practical-approach-to-kalman-filter-and-how-to-implement-it

  // Discrete Kalman filter time update equations - Time Update ("Predict")
  // Update xhat - Project the state ahead
  /* Step 1 */
  Kalman_X_rate = newRate - Kalman_X_bias;
  Kalman_X_angle += dt * Kalman_X_rate;

  // Update estimation error covariance - Project the error covariance ahead
  /* Step 2 */
  Kalman_X_P[0][0] += dt * (dt*Kalman_X_P[1][1] - Kalman_X_P[0][1] - Kalman_X_P[1][0] + Kalman_Q_angle);
  Kalman_X_P[0][1] -= dt * Kalman_X_P[1][1];
  Kalman_X_P[1][0] -= dt * Kalman_X_P[1][1];
  Kalman_X_P[1][1] += Kalman_Q_bias * dt;

  // Discrete Kalman filter measurement update equations - Measurement Update ("Correct")
  // Calculate Kalman gain - Compute the Kalman gain
  /* Step 4 */
  Kalman_X_S = Kalman_X_P[0][0] + Kalman_R_measure;
  /* Step 5 */
  Kalman_X_K[0] = Kalman_X_P[0][0] / Kalman_X_S;
  Kalman_X_K[1] = Kalman_X_P[1][0] / Kalman_X_S;

  // Calculate angle and bias - Update estimate with measurement zk (newAngle)
  /* Step 3 */
  Kalman_X_y = newAngle - Kalman_X_angle;
  /* Step 6 */
  Kalman_X_angle += Kalman_X_K[0] * Kalman_X_y;
  Kalman_X_bias += Kalman_X_K[1] * Kalman_X_y;

  // Calculate estimation error covariance - Update the error covariance
  /* Step 7 */
  Kalman_X_P[0][0] -= Kalman_X_K[0] * Kalman_X_P[0][0];
  Kalman_X_P[0][1] -= Kalman_X_K[0] * Kalman_X_P[0][1];
  Kalman_X_P[1][0] -= Kalman_X_K[1] * Kalman_X_P[0][0];
  Kalman_X_P[1][1] -= Kalman_X_K[1] * Kalman_X_P[0][1];

  return Kalman_X_angle;
};

float Kalman_getYAngle(float newAngle, float newRate, float dt)
{
  // KasBot V2 - Kalman filter module - http://www.x-firm.com/?page_id=145
  // Modified by Kristian Lauszus
  // See my blog post for more information: http://blog.tkjelectronics.dk/2012/09/a-practical-approach-to-kalman-filter-and-how-to-implement-it

  // Discrete Kalman filter time update equations - Time Update ("Predict")
  // Update xhat - Project the state ahead
  /* Step 1 */
  Kalman_Y_rate = newRate - Kalman_Y_bias;
  Kalman_Y_angle += dt * Kalman_Y_rate;

  // Update estimation error covariance - Project the error covariance ahead
  /* Step 2 */
  Kalman_Y_P[0][0] += dt * (dt*Kalman_Y_P[1][1] - Kalman_Y_P[0][1] - Kalman_Y_P[1][0] + Kalman_Q_angle);
  Kalman_Y_P[0][1] -= dt * Kalman_Y_P[1][1];
  Kalman_Y_P[1][0] -= dt * Kalman_Y_P[1][1];
  Kalman_Y_P[1][1] += Kalman_Q_bias * dt;

  // Discrete Kalman filter measurement update equations - Measurement Update ("Correct")
  // Calculate Kalman gain - Compute the Kalman gain
  /* Step 4 */
  Kalman_Y_S = Kalman_Y_P[0][0] + Kalman_R_measure;
  /* Step 5 */
  Kalman_Y_K[0] = Kalman_Y_P[0][0] / Kalman_Y_S;
  Kalman_Y_K[1] = Kalman_Y_P[1][0] / Kalman_Y_S;

  // Calculate angle and bias - Update estimate with measurement zk (newAngle)
  /* Step 3 */
  Kalman_Y_y = newAngle - Kalman_Y_angle;
  /* Step 6 */
  Kalman_Y_angle += Kalman_Y_K[0] * Kalman_Y_y;
  Kalman_Y_bias += Kalman_Y_K[1] * Kalman_Y_y;

  // Calculate estimation error covariance - Update the error covariance
  /* Step 7 */
  Kalman_Y_P[0][0] -= Kalman_Y_K[0] * Kalman_Y_P[0][0];
  Kalman_Y_P[0][1] -= Kalman_Y_K[0] * Kalman_Y_P[0][1];
  Kalman_Y_P[1][0] -= Kalman_Y_K[1] * Kalman_Y_P[0][0];
  Kalman_Y_P[1][1] -= Kalman_Y_K[1] * Kalman_Y_P[0][1];

  return Kalman_Y_angle;
};

void MPU6515_SetInterruptClearState2(bool bHelpInterrupSignal)
{
	//MPU6515_writeByte(MPU6515_ADDRESS, INT_PIN_CFG, 0x70);
	//MPU6515_writeByte(MPU6515_ADDRESS, INT_PIN_CFG, 0x60);
	//MPU6515_writeByte(MPU6515_ADDRESS, INT_PIN_CFG, 0x20);
	if( bHelpInterrupSignal == true )
	    MPU6515_writeByte(MPU6515_ADDRESS, INT_PIN_CFG, 0x20);
    else
        MPU6515_writeByte(MPU6515_ADDRESS, INT_PIN_CFG, 0x00);
}

void SetupForInterruptforImpulse(bool bWomActive,bool bGyroActive, unsigned char ncValue,bool bHelpInterrupSignal)
{
    MPU6515_resetMPU6515();

    //MONI 20190130 block this code not used.
    //MPU6515_writeByte(MPU6515_ADDRESS, SIGNAL_PATH_RESET, 0x0D);
    //MPU6515_writeByte(MPU6515_ADDRESS, USER_CTRL, 0x4F);

    //MPU6515_writeByte(MPU6515_ADDRESS, USER_CTRL, 0x80);//DMP On
    //MONI 20190130 check setting sequence.

    MPU6515_SetInterruptClearState2(bHelpInterrupSignal);
    MPU6515_ConfigurationWakeUpOnMotionInterrupt(bWomActive, bGyroActive, ncValue);
    //MPU6515_SetInterruptClearState2(false);

    /* for interrupt clear */
    ///SystemDelay(100);
    //MPU6515_CheckInterrupt();
    //SystemDelay(10);
    //MPU6515_CheckInterrupt();
    MPU6515_readByte(MPU6515_ADDRESS, INT_STATUS);
}

/**************************************************************************/
void CheckInterrtupStatus()
{
    unsigned char uchInterruptStatus = MPU6515_readByte(MPU6515_ADDRESS, INT_STATUS);

    if( (uchInterruptStatus & 0x40) == 0x40 )
    {
        Trace("%s] Sensor Interrupt occurred : %x\r\n",__FUNCTION__,uchInterruptStatus);
    }
    Trace("%s] Sensor Interrupt not occurred : %x\r\n",__FUNCTION__,uchInterruptStatus);
}

bool ReadInterruptStatus()
{
    unsigned char uchInterruptStatus = MPU6515_readByte(MPU6515_ADDRESS, INT_STATUS);


    if( (uchInterruptStatus & 0x40) == 0x40 )
    {
        printf("%s] Sensor Interrupt occurred : %x\r\n",__FUNCTION__,uchInterruptStatus);
        return true;
    }

    printf("%s] Sensor Interrupt not occurred : %x\r\n",__FUNCTION__,uchInterruptStatus);
    return false;
}

bool CheckGPIOStatusofSensor()
{
    for(int i=0;i<3;i++)
    {
        if( HalGPIOGetStatus(GPIO_SENSOR_INT) == false)
        {
            return false;
        }

        SystemDelay(10);
    }

    return true;
}

#define TIME_REPORT_IMPULSE 15000

void CallBackSensorReportTimeout()
{
    Trace("Impulse interrupt report/n");
    Send2MngSysMsg(eMngSensor,eReqReport,eR_ImpulseAlram, (stCarReport *)NULL,0);
}

void HandleMngSensorExternalEvent(stMsgSensor* pstSensor)
{
    if( pstSensor->header.id == eMngSys )
    {
        if( pstSensor->header.event == eReqWakeupInterruptInfo )
        {
            if( CheckGPIOStatusofSensor() == true )
            {
                Trace("%s] GPIO true\n",__FUNCTION__);

                SensorData.g_iTimerReportDly = -1;
                EnableSystemMessageTimer((int32_t*)&SensorData.g_iTimerReportDly, TIME_REPORT_IMPULSE, eSWTimer_ONESHOT, CallBackSensorReportTimeout);
                //Send2MngSysMsg(eMngSensor,eReqReport, eR_ImpulseAlram, (stCarReport *)NULL,0);
                MPU6515_Read_IntStatusRegister();

                //if( ReadInterruptStatus() == true )
                //{
                //    Trace("%s] Register true\n",__FUNCTION__);
                //    Send2MngSysMsg(eMngSensor,eReqReport, eR_ImpulseAlram, (stCarReport *)NULL);
                //}
            }
        }
    }
    else if( pstSensor->header.id == eMngSysSensor )
    {
        if( pstSensor->header.event == eReqSleep || pstSensor->header.event == eReqWom )
        {
            // send this message to sensor handler of manager system.
            // set sensor state
            SensorData.g_eSensorState = eSENSOR_STATE_IDLE;

            // enalbe wom interrupt
            // SetupForInterruptforImpulse(false, false, 0xAF,false);

            // notify wom interrupt
            Send2MngSysSensor(eMngSensor,eRspSleep,0,(stCarReport*)NULL,0);
        }
        else
        {
            GIT_Assert(false,eErrorCodeSns|eEventUnknown);
        }
    }
    else if( pstSensor->header.id == eMngSensor )
    {
        if( pstSensor->header.event == eReqReport )
        {
            if( pstSensor->header.subEvent == eR_ImpulseAlram )
            {
                // clear mpu-6515 interrupt
                // if read status register, then clear interrupt register,
                if( ReadInterruptStatus() == true )
                {
                    SensorData.g_iTimerReportDly = -1;
                    EnableSystemMessageTimer((int32_t*)&SensorData.g_iTimerReportDly, TIME_REPORT_IMPULSE, eSWTimer_ONESHOT, CallBackSensorReportTimeout);
                    //Send2MngSysMsg(eMngSensor,eReqReport,eR_ImpulseAlram, (stCarReport *)NULL,0);
                    // clear interrupt pin
                    HalDrvExIntIOCtrl(eEXTI_IO_ClearItStatus, HAL_EXTI_Line5, NULL, 0, 0);
                }
            }
        }
        else
        {
            GIT_Assert(false,eErrorCodeSns|eEventUnknown);
        }
    }
    else if( pstSensor->header.id == eMngSysMsg )
    {
        if( pstSensor->header.event == eReqSuspend )
        {
            SensorData.g_eSensorState = eSENSOR_STATE_IDLE;
        }
        else if( pstSensor->header.event == eReqChangeMode )
        {
            SensorData.ucVehicleMode = pstSensor->header.subEvent;

            printf("Sensor Get Mode Info : %d\r\n",SensorData.ucVehicleMode);
            if( pstSensor->header.subEvent == eSysModeParking )
            {
				// Reset registers to default in preparation for device calibration
                //MPU6515_resetMPU6515();

                // stop gethering the sensor data
                SensorData.g_eSensorState = eSENSOR_STATE_IDLE;

                // disalbe all interrupt of sensor
                MPU6515_writeByte(MPU6515_ADDRESS, INT_ENABLE, 0x00);
                // disable gyro and accelerometer
                MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_2, 0x3F);
            }
            else
            {
                // start gethering the sensor data
                //SensorData.g_eSensorState = eSENSOR_STATE_INIT_MPU6515;
            }
        }
    }
    else
    {
        // not defined
        // error
        GIT_Assert(false,eErrorCodeSns|eUnknownId);
    }
}


uint8_t MPU6515_GetName(void)
{
    uint8_t whoami = 0;

    // reset device
    MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_1, 0x80); // Write a one to bit 7 reset bit; toggle reset device

    SystemDelay(100);

    // Read the WHO_AM_I register, this is a good test of communication
    whoami = MPU6515_readByte(MPU6515_ADDRESS, WHO_AM_I_MPU6515);  // Read WHO_AM_I register for MPU-6515
    printf("[%s] I AM 0x%x, I SHOULD BE 0x74\n\r", __FUNCTION__, whoami);


    SystemDelay(100);

    return whoami;
}

void GetGyroAngle(stSensorInfo* pstGyroAngle)
{
    if( SensorData.stGyroAngleDefault.nX > 0 )
        pstGyroAngle->nX = (SensorData.nAngleX-SensorData.stGyroAngleDefault.nX);
    else
        pstGyroAngle->nX = (SensorData.nAngleX+(SensorData.stGyroAngleDefault.nX*(-1)));

    if( SensorData.stGyroAngleDefault.nY > 0 )
        pstGyroAngle->nY = (SensorData.nAngleY-SensorData.stGyroAngleDefault.nY);
    else
        pstGyroAngle->nY = (SensorData.nAngleY+(SensorData.stGyroAngleDefault.nY*(-1)));

    if( SensorData.stGyroAngleDefault.nZ > 0 )
        pstGyroAngle->nZ = (SensorData.nAngleZ-SensorData.stGyroAngleDefault.nZ);
    else
        pstGyroAngle->nZ = (SensorData.nAngleZ+(SensorData.stGyroAngleDefault.nZ*(-1)));

	pstGyroAngle->bCalibration = SensorData.stGyroAngleDefault.bCalibration;

    //Trace("GetGyroX: %d, Y : %d, Z : %d\n",SensorData.nAngleX,SensorData.nAngleY,SensorData.nAngleZ);
}

void GetGyroNavieAngle(stSensorInfo* pstGyroAngle)
{
    pstGyroAngle->nX = SensorData.nAngleX;
    pstGyroAngle->nY = SensorData.nAngleY;
    pstGyroAngle->nZ = SensorData.nAngleZ;
    pstGyroAngle->bCalibration = SensorData.stGyroAngleDefault.bCalibration;
    Trace("GetGyroNavieX: %d, Y : %d, Z : %d\n",pstGyroAngle->nX,pstGyroAngle->nY,pstGyroAngle->nZ);
}

void UpdataGyroDefualtAngle()
{
	GetGyroInitializeAngle(&SensorData.stGyroAngleDefault);

	//Trace("Default Value GetGyroX Cal: %d, Y : %d, Z : %d\n",SensorData.stGyroAngleDefault.nX,SensorData.stGyroAngleDefault.nY,SensorData.stGyroAngleDefault.nZ);
}


