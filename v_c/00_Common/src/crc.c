/* main.c */

/* Includes */
#include "crc.h"

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc1;

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
void MX_CRC_Init(void)
{
  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc1.Instance = CRC;
  hcrc1.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_DISABLE;
  hcrc1.Init.GeneratingPolynomial = 0x1021;								// 프로토콜 요구사항: Polynomial 0x1021
  hcrc1.Init.CRCLength = CRC_POLYLENGTH_16B;							// 프로토콜 요구사항: 16-bit CRC
  hcrc1.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_DISABLE;
  hcrc1.Init.InitValue = 0xFFFF;										// 프로토콜 요구사항: Seed 0xFFFF
  hcrc1.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;		// 프로토콜 요구사항: RefIn = 0
  hcrc1.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;// 프로토콜 요구사항: RefOut = 0
  hcrc1.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;					// 8비트(byte) 단위로 데이터를 처리하도록 설정
  if (HAL_CRC_Init(&hcrc1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */
}

void MX_CRC_Init_CMOX(void)
{
  hcrc1.Instance = CRC;
  hcrc1.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc1.Init.CRCLength = CRC_POLYLENGTH_32B;
  hcrc1.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc1.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc1.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc1.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;
  if (HAL_CRC_Init(&hcrc1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
* @brief CRC MSP Initialization
* This function configures the hardware resources needed for the CRC peripheral
* @param hcrc: CRC handle pointer
* @retval None
*/
void HAL_CRC_MspInit(CRC_HandleTypeDef* hcrc1)
{
  if(hcrc1->Instance==CRC)
  {
  /* USER CODE BEGIN CRC_MspInit 0 */

  /* USER CODE END CRC_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_CRC_CLK_ENABLE();
  /* USER CODE BEGIN CRC_MspInit 1 */

  /* USER CODE END CRC_MspInit 1 */
  }
}

/**
* @brief CRC MSP De-Initialization
* This function freeze the hardware resources needed for the CRC peripheral
* @param hcrc: CRC handle pointer
* @retval None
*/
void HAL_CRC_MspDeInit(CRC_HandleTypeDef* hcrc1)
{
  if(hcrc1->Instance==CRC)
  {
  /* USER CODE BEGIN CRC_MspDeInit 0 */

  /* USER CODE END CRC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CRC_CLK_DISABLE();
  /* USER CODE BEGIN CRC_MspDeInit 1 */

  /* USER CODE END CRC_MspDeInit 1 */
  }
}
