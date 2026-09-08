#include "FreeRTOS.h"
#include "git_HSM_Operations.h"
#include "git_HSM_SPICommand.h"
#include "common.h"
#include "git_hsm.h"  // for setRSA_Encrypt
#include <string.h>
#include "ff.h"
#include "git_fsutil.h"
#include "firmware.h"

#define MAX_EDDSA_VERIFY_DATA_SIZE (MAX_DATA_PAYLOAD_SIZE + 64U)
//#define USE_FLASH_HSM_UPDATE

extern bool g_bHsmLogTxOn;
extern bool g_bHsmLogRxOn;
//U8 g_ucHsmDfuFailTest=0;//for test
extern uint32_t g_u32firmware_size;

/*
 * =================================================================
 * Module Variables
 * =================================================================
 */
extern U8 gHSM_Info_Name[18]; 

/** @brief Context ID for Mutual Authentication session (internal use only) */
static uint16_t g_auth_ctx_id = 0U;
extern SHSMUpdateAck *pHSMAck;

/*
 * =================================================================
 * Internal Helper Functions
 * =================================================================
 */

/**
 * @brief Build W_CMD packet parameters (6 bytes, Little Endian format)
 *
 * @details Constructs 6-byte parameter field from three 16-bit values.
 *          All values are packed in Little Endian format (LSB first).
 *
 * @note Parameter field structure (total 6 bytes):
 *       - Bytes 0-1: f1 (Little Endian)
 *       - Bytes 2-3: f2 (Little Endian)
 *       - Bytes 4-5: f3 (Little Endian)
 *
 * @example Common usage patterns:
 *          - Key Management: build_params(param, key_id, alg_type, data_len)
 *          - Multi-call: build_params(param, 0, context_id, data_len)
 *          - Random: build_params(param, 0, 0, rng_length)
 *
 * @param param_buffer Output buffer (must be at least 6 bytes)
 * @param f1 First 16-bit value
 * @param f2 Second 16-bit value
 * @param f3 Third 16-bit value
 */
static void build_params(uint8_t* param_buffer, uint16_t f1, uint16_t f2, uint16_t f3)
{
	param_buffer[0] = (uint8_t)(f1 & 0xFFU);
	param_buffer[1] = (uint8_t)((f1 >> 8U) & 0xFFU);
	param_buffer[2] = (uint8_t)(f2 & 0xFFU);
	param_buffer[3] = (uint8_t)((f2 >> 8U) & 0xFFU);
	param_buffer[4] = (uint8_t)(f3 & 0xFFU);
	param_buffer[5] = (uint8_t)((f3 >> 8U) & 0xFFU);
}

/**
 * @brief Validate HSM response code
 * @details Common helper function for consistent error handling across all HSM operations.
 *          Checks response length and response code, logs error if needed.
 *
 * @param rx Response buffer
 * @param rx_len Response length
 * @return HAL_OK if response is valid (AVO_RET_OK), HAL_ERROR otherwise
 */
static HAL_StatusTypeDef validate_response(const uint8_t* rx, uint16_t rx_len)
{
	if (rx_len == 0U)
	{
		hsm_print_error(AVO_ERR_INVALID_LENGTH);
		return HAL_ERROR;
	}

	if (rx[0] != AVO_RET_OK)
	{
		hsm_print_error(rx[0]);
		return HAL_ERROR;
	}

	return HAL_OK;
}

/**
 * @brief Execute simple 5-byte packet command (One-Shot)
 */
static HAL_StatusTypeDef exec_simple_cmd(uint8_t opcode)
{
	/* Wait for READY before W_CMD */
	HAL_StatusTypeDef status = wait_for_host_status(HOST_STATUS_READY, 5000U);
	if (status != HAL_OK)
	{
		return status;
	}

	status = spi_cmd_w_cmd_5byte(opcode);
	if (status != HAL_OK)
	{
		return status;
	}

	/* Wait for DONE after W_CMD */
	status = wait_for_host_status(HOST_STATUS_DONE, 5000U);
	if (status != HAL_OK)
	{
		return status;
	}

	uint8_t rx[64];
	uint16_t rx_len = 0U;
	status = spi_cmd_r_cmd(rx, (uint16_t)sizeof(rx), &rx_len, MODE_PLAINTEXT);

	return status;
}

/*
 * =================================================================
 * Phase 1: Basic Functions (4 OP Codes)
 * 0x00 Info | 0x01 Get HSN | 0x02 Cancel | 0x10 Random
 * =================================================================
 */

/**
 * @brief Get HSM version info (OP 0x00)
 */
HAL_StatusTypeDef hsm_get_version_info(HSM_VersionInfo_t* version_info)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_get_version_info] start\r\n");
#endif
	if (version_info == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status = wait_for_host_status(HOST_STATUS_READY, 5000U);
	if (status != HAL_OK)
	{
		GLogE("[W_CMD_5BYTE] HSM not ready, status check failed\r\n");
		return status;
	}
	
	status = spi_cmd_w_cmd_5byte(HSM_OPCODE_INFO);
	if (status != HAL_OK)
	{
		GLogE("[ERROR] W_CMD failed!\r\n");
		return status;
	}

	status = wait_for_host_status(HOST_STATUS_DONE, 5000U);
	if (status != HAL_OK)
	{
		GLogE("[W_CMD_5BYTE] HSM not ready, status check failed\r\n");
		return status;
	}

	uint8_t rx[100] = {0U};
	uint16_t rx_len = 0U;
	status = spi_cmd_r_cmd(rx, (uint16_t)sizeof(rx), &rx_len, MODE_PLAINTEXT);

	if (status != HAL_OK)
	{
		GLogE("[ERROR] spi_cmd_r_cmd failed with status: %d\r\n", status);
		return status;
	}

	/* Data starts at rx[0] (Response Code excluded by spi_cmd_r_cmd) */
	uint16_t i = 0U;
	version_info->host_major = (uint16_t)rx[i] | ((uint16_t)rx[i + 1U] << 8U); i += 2U;
	version_info->host_minor = (uint16_t)rx[i] | ((uint16_t)rx[i + 1U] << 8U); i += 2U;
	version_info->host_patch = (uint16_t)rx[i] | ((uint16_t)rx[i + 1U] << 8U); i += 2U;
	version_info->hse_major = (uint16_t)rx[i] | ((uint16_t)rx[i + 1U] << 8U); i += 2U;
	version_info->hse_minor = (uint16_t)rx[i] | ((uint16_t)rx[i + 1U] << 8U); i += 2U;
	version_info->hse_patch = (uint16_t)rx[i] | ((uint16_t)rx[i + 1U] << 8U); i += 2U;
	version_info->ask_vendor_id = (uint16_t)rx[i] | ((uint16_t)rx[i + 1U] << 8U); i += 2U;
	version_info->ask_module_id = (uint16_t)rx[i] | ((uint16_t)rx[i + 1U] << 8U); i += 2U;
	version_info->ask_major = rx[i]; i++;
	version_info->ask_minor = rx[i]; i++;
	version_info->ask_patch = rx[i]; i++;
	version_info->lot_number = (uint32_t)rx[i] | ((uint32_t)rx[i + 1U] << 8U) | ((uint32_t)rx[i + 2U] << 16U) | ((uint32_t)rx[i + 3U] << 24U); i += 4U;
	version_info->serial_number = (uint32_t)rx[i] | ((uint32_t)rx[i + 1U] << 8U) | ((uint32_t)rx[i + 2U] << 16U) | ((uint32_t)rx[i + 3U] << 24U); i += 4U;
	version_info->used_cert_slots = rx[i]; i++;
	version_info->release = (uint16_t)rx[i] | ((uint16_t)rx[i + 1U] << 8U);

	return HAL_OK;
}

/**
 * @brief Get HSM Serial Number - 8 bytes (OP 0x01)
 */
HAL_StatusTypeDef hsm_get_serial_number(uint8_t* serial_number)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_get_serial_number] start\r\n");
#endif
	if (serial_number == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status = spi_cmd_w_cmd_5byte(HSM_OPCODE_GET_HSN);
	if (status != HAL_OK)
	{
		return status;
	}

	status = wait_for_host_status(HOST_STATUS_DONE, 5000U);
	if (status != HAL_OK)
	{
		GLogE("[W_CMD_5BYTE] HSM not ready, status check failed\r\n");
		return status;
	}
	osDelay(5U);

	uint8_t rx[32];
	uint16_t rx_len = 0U;
	status = spi_cmd_r_cmd(rx, (uint16_t)sizeof(rx), &rx_len, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* Data starts at rx[0] (Response Code excluded by spi_cmd_r_cmd) */
	//(void)memcpy(serial_number, "11111111", 8U);
	return HAL_OK;
}

/**
 * @brief Cancel current job (OP 0x02)
 * Manual: Table 30 (Req), Table 31 (Resp)
 */
HAL_StatusTypeDef hsm_cancel_job(void)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_cancel_job] start\r\n");
#endif
	return exec_simple_cmd(HSM_OPCODE_CANCEL_JOB);
}

/**
 * @brief Get last error code (OP 0x0B)
 */
HAL_StatusTypeDef hsm_get_last_error(uint8_t* error_code)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_get_last_error] start\r\n");
#endif
	if (error_code == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	/* Wait for READY before W_CMD */
	HAL_StatusTypeDef status = wait_for_host_status(HOST_STATUS_READY, 5000U);
	if (status != HAL_OK)
	{
		return status;
	}

	status = spi_cmd_w_cmd_5byte(HSM_OPCODE_GET_ERROR);
	if (status != HAL_OK)
	{
		return status;
	}

	/* Wait for DONE after W_CMD */
	status = wait_for_host_status(HOST_STATUS_DONE, 5000U);
	if (status != HAL_OK)
	{
		return status;
	}

	uint8_t rx[8] = {0};
	uint16_t rx_len = 0U;
	status = spi_cmd_r_cmd(rx, (uint16_t)sizeof(rx), &rx_len, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* Data starts at rx[0] (Response Code excluded by spi_cmd_r_cmd) */
	/* Length=5: rx_len=0 (no error data), Length=6: rx_len=1 (has error code) */
	if (rx_len >= 1U)
	{
		*error_code = rx[0];
	}
	else
	{
		*error_code = 0U;  /* No error code data = no error */
	}
	return HAL_OK;
}

/**
 * @brief Generate random numbers (OP 0x10)
 * Manual: Table 48 (Req), Table 49 (Resp)
 */
HAL_StatusTypeDef hsm_generate_random(uint8_t* output, uint16_t length, bool use_trng)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_generate_random] start\r\n");
#endif
	if (output == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	if ((length == 0U) || (length > 512U))
	{
		hsm_print_error(AVO_ERR_OUT_OF_RANGE);
		return HAL_ERROR;
	}

	uint8_t param[LEN_PARAM];
	uint16_t mode;

	if (use_trng == true)
	{
		mode = HSM_RNG_TRNG;
	}
	else
	{
		mode = HSM_RNG_PRNG;
	}

	build_params(param, 0U, 0U, length);

	uint8_t rx[1024];
	uint16_t rx_len = 0U;

	HAL_StatusTypeDef status = process_host_communication(HSM_OPCODE_RANDOM, mode, param, NULL, 0U, rx, &rx_len, MODE_PLAINTEXT);

	if (status != HAL_OK)
	{
		return status;
	}

	/* Data starts at rx[0] (Response Code excluded by spi_cmd_r_cmd) */
	(void)memcpy(output, &rx[0], length);
	return HAL_OK;
}

/*
 * =================================================================
 * Phase 2-5: TO BE IMPLEMENTED (15 more OP Codes)
 * =================================================================
 */

/*
 * =================================================================
 * Phase 2: Key Management
 * OP Code 0x05: Key Mng - Manual Table 36, 40
 * =================================================================
 */

/**
 * @brief Import AES key (OP 0x05)
 * Manual: Table 36 (Req), Table 40 (Resp)
 *
 * When encrypted=true: key_data is RSA ciphertext (256 bytes for RSA-2048)
 * When encrypted=false: key_data is plaintext AES key (16/24/32 bytes)
 */
HAL_StatusTypeDef hsm_import_aes_key(uint16_t key_id, const uint8_t* key_data, uint8_t key_size, bool encrypted, bool lock_key)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_import_aes_key] start\r\n");
#endif
	if (key_data == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	uint16_t key_len;
	switch (key_size)
	{
		case HSM_AES_128:
			key_len = 16U;
			break;
		case HSM_AES_192:
			key_len = 24U;
			break;
		case HSM_AES_256:
			key_len = 32U;
			break;
		default:
			hsm_print_error(AVO_ERR_INVALID_PARAM);
			return HAL_ERROR;
	}

	// Data length: RSA ciphertext (256 bytes) if encrypted, else AES key size
	uint16_t data_len = (encrypted == true) ? 256U : key_len;

	uint8_t mode1 = HSM_KEYMNG_IMPORT | ((encrypted == true) ? 0x04U : 0x00U) | ((lock_key == true) ? 0x80U : 0x00U) | 0x08U;
	uint8_t mode2 = key_size;
	uint16_t mode = (uint16_t)mode1 | ((uint16_t)mode2 << 8U);

	uint8_t param[LEN_PARAM];
	build_params(param, key_id, HSM_ALG_AES, data_len);

	uint8_t rx[64];
	uint16_t rx_len = 0U;

	HAL_StatusTypeDef status = process_host_communication(HSM_OPCODE_KEY_MNG, mode, param, (uint8_t*)key_data, data_len, rx, &rx_len, MODE_PLAINTEXT);

	if ((status == HAL_OK) && (rx_len > 0U))
	{
		if (rx[0] != AVO_RET_OK)
		{
			hsm_print_error(rx[0]);
			return HAL_ERROR;
		}
	}

	return status;
}

/**
 * @brief Import HMAC key into HSM (OP 0x05)
 * @details HMAC key is always 256 bits (32 bytes). Used for HMAC-SHA256 operations.
 *          Key slots: #121-124 (UDK), #221-223 (TEMP)
 */
HAL_StatusTypeDef hsm_import_hmac_key(uint16_t key_id, const uint8_t* key_data, bool encrypted, bool lock_key)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_import_hmac_key] start\r\n");
#endif
	if (key_data == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	// HMAC key is always 32 bytes (256 bits)
	//uint16_t key_len = 32;
	uint16_t key_len = 16;

	// Data length: RSA ciphertext (256 bytes) if encrypted, else HMAC key size
	uint16_t data_len = (encrypted == true) ? 256U : key_len;

	uint8_t mode1 = HSM_KEYMNG_IMPORT | ((encrypted == true) ? 0x04U : 0x00U) | ((lock_key == true) ? 0x80U : 0x00U) | 0x08U;
	uint8_t mode2 = 0x00U;  // HMAC has no key size variations
	uint16_t mode = (uint16_t)mode1 | ((uint16_t)mode2 << 8U);

	uint8_t param[LEN_PARAM];
	build_params(param, key_id, HSM_ALG_HMAC, data_len);

	uint8_t rx[64];
	uint16_t rx_len = 0U;

	HAL_StatusTypeDef status = process_host_communication(HSM_OPCODE_KEY_MNG, mode, param, (uint8_t*)key_data, data_len, rx, &rx_len, MODE_PLAINTEXT);

	if ((status == HAL_OK) && (rx_len > 0U))
	{
		if (rx[0] != AVO_RET_OK)
		{
			hsm_print_error(rx[0]);
			return HAL_ERROR;
		}
	}

	return status;
}

/**
 * @brief Import Special Key (OP 0x05) - v5 Mode2 Bit2-5
 * @details 특수키 주입 시 Mode2 Bit2-5에 키 용도를 설정
 *          - HSM_SPECIAL_KEY_ECU_CODE (0x09): ECU-Code Key (#101) - AES-128
 *          - HSM_SPECIAL_KEY_KM (0x0A): Km (#102) - HMAC-256
 *          - HSM_SPECIAL_KEY_KAUTH (0x0B): Kauth (#103) - HMAC-256
 *          - HSM_SPECIAL_KEY_RSA_KEK (0x0C): RSA-KEK (#149) - RSA-2048
 *
 * @param special_key_type: HSM_SPECIAL_KEY_ECU_CODE / KM / KAUTH / RSA_KEK
 * @param key_data: 키 데이터 (암호화된 경우 RSA ciphertext 256 bytes)
 * @param key_len: 키 데이터 길이
 * @param encrypted: true = RSA로 암호화된 키
 * @param lock_key: true = 키 잠금
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_import_special_key(uint8_t special_key_type, const uint8_t* key_data, uint16_t key_len, bool encrypted, bool lock_key)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_import_special_key] start, type=0x%02X\r\n", special_key_type);
#endif
	if (key_data == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	uint16_t key_id;
	uint8_t alg_type;
	uint8_t key_size_bits = 0U;

	/* 특수키 타입에 따른 Key ID, Algorithm 설정 */
	switch (special_key_type)
	{
		case HSM_SPECIAL_KEY_ECU_CODE:  /* #101: ECU-Code Key - AES-128 */
			key_id = 101U;
			alg_type = HSM_ALG_AES;
			key_size_bits = HSM_AES_128;
			break;
		case HSM_SPECIAL_KEY_KM:        /* #102: Km - HMAC-256 */
			key_id = 102U;
			//alg_type = HSM_ALG_HMAC;
			alg_type = HSM_ALG_AES;
			break;
		case HSM_SPECIAL_KEY_KAUTH:     /* #103: Kauth - HMAC-256 */
			key_id = 103U;
			alg_type = HSM_ALG_HMAC;
			break;
		case HSM_SPECIAL_KEY_RSA_KEK:   /* #149: RSA-KEK - RSA-2048 */
			key_id = 149U;
			alg_type = HSM_ALG_RSA;
			key_size_bits = HSM_RSA_2048;
			break;
		default:
			hsm_print_error(AVO_ERR_INVALID_PARAM);
			return HAL_ERROR;
	}

	/* Data length: RSA ciphertext (256 bytes) if encrypted */
	uint16_t data_len = (encrypted == true) ? 256U : key_len;

	uint8_t mode1 = HSM_KEYMNG_IMPORT | ((encrypted == true) ? 0x04U : 0x00U) | ((lock_key == true) ? 0x80U : 0x00U) | 0x08U;
	/* v5: Mode2 Bit2-5 = special_key_type, Bit0-1 = key_size (AES only) */
	uint8_t mode2 = (uint8_t)(special_key_type << 2U) | key_size_bits;
	uint16_t mode = (uint16_t)mode1 | ((uint16_t)mode2 << 8U);

	uint8_t param[LEN_PARAM];
	build_params(param, key_id, alg_type, data_len);

	uint8_t rx[64];
	uint16_t rx_len = 0U;

	HAL_StatusTypeDef status = process_host_communication(HSM_OPCODE_KEY_MNG, mode, param, (uint8_t*)key_data, data_len, rx, &rx_len, MODE_PLAINTEXT);

	if ((status == HAL_OK) && (rx_len > 0U))
	{
		if (rx[0] != AVO_RET_OK)
		{
			hsm_print_error(rx[0]);
			return HAL_ERROR;
		}
	}

	return status;
}

/**
 * @brief Import RSA key (OP 0x05)
 */
HAL_StatusTypeDef hsm_import_rsa_key(uint16_t key_id, const HSM_RSAKey_t* rsa_key, bool public_only, bool encrypted, bool lock_key, uint8_t key_size_bits)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_import_rsa_key] start\r\n");
#endif
	if (rsa_key == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	uint8_t mode1 = HSM_KEYMNG_IMPORT | ((encrypted == true) ? 0x04U : 0x00U) | ((lock_key == true) ? 0x80U : 0x00U) | 0x08U;
	uint8_t mode2 = key_size_bits | ((public_only == true) ? 0x80U : 0x00U);  // Bit0-1: key_size, Bit6-7: b10=공개키 → Bit6=0,Bit7=1 = 0x80
	uint16_t mode = (uint16_t)mode1 | ((uint16_t)mode2 << 8U);

	uint8_t data[520];
	uint16_t offset = 0U;

	// Little Endian: Low byte first
	data[offset] = (uint8_t)(rsa_key->modulus_length & 0xFFU); offset++;
	data[offset] = (uint8_t)((rsa_key->modulus_length >> 8U) & 0xFFU); offset++;
	(void)memcpy(&data[offset], rsa_key->modulus, 256U);
	offset += 256U;
	data[offset] = (uint8_t)(rsa_key->public_exp_size & 0xFFU); offset++;
	data[offset] = (uint8_t)((rsa_key->public_exp_size >> 8U) & 0xFFU); offset++;
	(void)memcpy(&data[offset], rsa_key->public_exp, rsa_key->public_exp_size);
	offset += rsa_key->public_exp_size;

	if (public_only == false)
	{
		(void)memcpy(&data[offset], rsa_key->private_exp, 256U);
		offset += 256U;
	}

	uint8_t param[LEN_PARAM];
	build_params(param, key_id, HSM_ALG_RSA, offset);

	uint8_t rx[64];
	uint16_t rx_len = 0U;

	return process_host_communication(HSM_OPCODE_KEY_MNG, mode, param, data, offset, rx, &rx_len, MODE_PLAINTEXT);
}
HAL_StatusTypeDef hsm_import_rsa_key_en(uint16_t key_id, const HSM_RSAKeyEn_t* rsa_keyEn, uint8_t public_only, bool encrypted, bool lock_key, uint8_t key_size_bits)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_import_rsa_key_encrypt] start\r\n");
#endif
	if (rsa_keyEn == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	uint8_t mode1 = HSM_KEYMNG_IMPORT | ((encrypted == true) ? 0x04U : 0x00U) | ((lock_key == true) ? 0x80U : 0x00U) | 0x08U;
	uint8_t mode2 = key_size_bits;
	if(public_only == 0) mode2 |= 0;  // key pair
	else if(public_only == 1) mode2 |= 0x40;  // private key only
	else if(public_only == 2) mode2 |= 0x80;  // public key only
	uint16_t mode = (uint16_t)mode1 | ((uint16_t)mode2 << 8U);

	//uint8_t data[520];
	uint8_t data[1024];
	uint16_t offset = 0U;

	// Little Endian: Low byte first
	data[offset] = (uint8_t)(rsa_keyEn->modulus_length & 0xFFU); offset++;
	data[offset] = (uint8_t)((rsa_keyEn->modulus_length >> 8U) & 0xFFU); offset++;
	(void)memcpy(&data[offset], rsa_keyEn->modulus, 256U);
	offset += 256U;
	data[offset] = (uint8_t)(rsa_keyEn->public_exp_size & 0xFFU); offset++;
	data[offset] = (uint8_t)((rsa_keyEn->public_exp_size >> 8U) & 0xFFU); offset++;
	//(void)memcpy(&data[offset], rsa_keyEn->public_exp, rsa_keyEn->public_exp_size);
	//offset += rsa_keyEn->public_exp_size;

	if (public_only != 2)
	{
		(void)memcpy(&data[offset], rsa_keyEn->upper_private_exp, 256U);
		offset += 256U;
		(void)memcpy(&data[offset], rsa_keyEn->lower_private_exp, 256U);
		offset += 256U;
	}

	uint8_t param[LEN_PARAM];
	build_params(param, key_id, HSM_ALG_RSA, offset);

	uint8_t rx[64];
	uint16_t rx_len = 0U;

	return process_host_communication(HSM_OPCODE_KEY_MNG, mode, param, data, offset, rx, &rx_len, MODE_PLAINTEXT);
}

/**
 * @brief Import ECC key (OP 0x05)
 */
HAL_StatusTypeDef hsm_import_ecc_key(uint16_t key_id, const HSM_ECCKey_t* ecc_key, bool public_only, bool encrypted, bool lock_key)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_import_ecc_key] start\r\n");
#endif
	if (ecc_key == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	uint8_t mode1 = HSM_KEYMNG_IMPORT | ((encrypted == true) ? 0x04U : 0x00U) | ((lock_key == true) ? 0x80U : 0x00U) | 0x08U;
	uint8_t mode2 = ((public_only == true) ? 0x80U : 0x00U);
	uint16_t mode = (uint16_t)mode1 | ((uint16_t)mode2 << 8U);

	uint8_t data[96];
	uint16_t offset = 0U;

	(void)memcpy(&data[offset], ecc_key->public_key_x, 32U);
	offset += 32U;
	(void)memcpy(&data[offset], ecc_key->public_key_y, 32U);
	offset += 32U;

	if (public_only == false)
	{
		(void)memcpy(&data[offset], ecc_key->private_key, 32U);
		offset += 32U;
	}

	uint8_t param[LEN_PARAM];
	build_params(param, key_id, HSM_ALG_ECC, offset);

	uint8_t rx[64];
	uint16_t rx_len = 0U;

	return process_host_communication(HSM_OPCODE_KEY_MNG, mode, param, data, offset, rx, &rx_len, MODE_PLAINTEXT);
}

/**
 * @brief Import ED key (Ed25519) (OP 0x05)
 * @note When encrypted=true, ED Key data (64 bytes) is encrypted with RSA KEK (#149)
 *       using RSAES-PKCS1-v1.5, resulting in 256 bytes ciphertext.
 */
HAL_StatusTypeDef hsm_import_ed_key(uint16_t key_id, const HSM_EDKey_t* ed_key, uint8_t public_only, bool encrypted, bool lock_key)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_import_ed_key] start\r\n");
#endif
	if (ed_key == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	uint8_t mode1 = HSM_KEYMNG_IMPORT | ((encrypted == true) ? 0x04U : 0x00U) | ((lock_key == true) ? 0x80U : 0x00U) | 0x08U;
	uint8_t mode2 = 0;
	//mode2 = ((public_only == true) ? 0x80U : 0x00U);
	if(public_only == 0) mode2=0;  // key pair
	else if(public_only == 1) mode2=0x40;  // private key only
	else if(public_only == 2) mode2=0x80;  // public key only
	uint16_t mode = (uint16_t)mode1 | ((uint16_t)mode2 << 8U);

	// Prepare plaintext ED Key data (public + private = 64 bytes max)
	uint8_t plaintext[64];
	uint16_t plaintext_len = 0U;

	if (public_only == 0 || public_only == 2)
	{
		(void)memcpy(&plaintext[plaintext_len], ed_key->public_key, 32U);
		plaintext_len += 32U;
	}

	if (public_only == 0 || public_only == 1)
	{
		(void)memcpy(&plaintext[plaintext_len], ed_key->private_key, 32U);
		plaintext_len += 32U;
	}

	uint8_t tx_data[256];
	uint16_t tx_len = 0U;

	if (encrypted)
	{
		// Step 1: Export RSA KEK Public Key (#149)
		uint8_t rsa_pubkey[256];
		uint16_t rsa_pubkey_len = 0U;
		HAL_StatusTypeDef status = hsm_export_public_key(HSM_KEK_RSA_KEY_ID, HSM_ALG_RSA, rsa_pubkey, &rsa_pubkey_len);
		if (status != HAL_OK)
		{
			GLogE("[ED Import] RSA KEK Export failed\r\n");
			return HAL_ERROR;
		}

		// Step 2: Encrypt ED Key with RSA Public Key (RSAES-PKCS1-v1.5)
		// setRSA_Encrypt(plaintext, len, pubkey, pubkey_len, ciphertext)
		int32_t rsa_result = setRSA_Encrypt(plaintext, plaintext_len, rsa_pubkey, rsa_pubkey_len, tx_data);
		if (rsa_result != 0)  // 0 = CMOX_RSA_SUCCESS
		{
			GLogE("[ED Import] RSA Encrypt failed: %d\r\n", rsa_result);
			return HAL_ERROR;
		}
		tx_len = 256U;  // RSA-2048 ciphertext is always 256 bytes
		GLogN("[ED Import] ED Key encrypted with RSA KEK\r\n");
	}
	else
	{
		// Plaintext import
		(void)memcpy(tx_data, plaintext, plaintext_len);
		tx_len = plaintext_len;
	}

	uint8_t param[LEN_PARAM];
	build_params(param, key_id, HSM_ALG_ED, tx_len);

	uint8_t rx[64];
	uint16_t rx_len = 0U;

	return process_host_communication(HSM_OPCODE_KEY_MNG, mode, param, tx_data, tx_len, rx, &rx_len, MODE_PLAINTEXT);
}

HAL_StatusTypeDef hsm_import_ed_key_en(uint16_t key_id, const HSM_EDKey_enc_t* ed_key, uint8_t public_only, bool encrypted, bool lock_key)
//HAL_StatusTypeDef hsm_import_ed_key_en(uint16_t key_id, uint8_t public_only, bool encrypted, bool lock_key)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_import_ed_key] start\r\n");
#endif
	if (ed_key == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	uint8_t mode1 = HSM_KEYMNG_IMPORT | ((encrypted == true) ? 0x04U : 0x00U) | ((lock_key == true) ? 0x80U : 0x00U) | 0x08U;
	uint8_t mode2 = 0;
	//mode2 = ((public_only == true) ? 0x80U : 0x00U);
	if(public_only == 0) mode2=0;  // key pair
	else if(public_only == 1) mode2=0x40;  // private key only
	else if(public_only == 2) mode2=0x80;  // public key only
	uint16_t mode = (uint16_t)mode1 | ((uint16_t)mode2 << 8U);

	// Prepare plaintext ED Key data (public + private = 64 bytes max)
	uint8_t plaintext[256];
	uint16_t plaintext_len = 0U;

	(void)memcpy(&plaintext[plaintext_len], ed_key, 256U);
	plaintext_len += 256U;
#if 0

	if (public_only == 0)
	{
		(void)memcpy(&plaintext[plaintext_len], ed_key->private_key, 32U);
		plaintext_len += 32U;
	}
#endif

	uint8_t tx_data[256];
	uint16_t tx_len = 0U;
#if 0
	if (encrypted)
	{
		// Step 1: Export RSA KEK Public Key (#149)
		uint8_t rsa_pubkey[256];
		uint16_t rsa_pubkey_len = 0U;
		HAL_StatusTypeDef status = hsm_export_public_key(HSM_KEK_RSA_KEY_ID, HSM_ALG_RSA, rsa_pubkey, &rsa_pubkey_len);
		if (status != HAL_OK)
		{
			GLogE("[ED Import] RSA KEK Export failed\r\n");
			return HAL_ERROR;
		}

		// Step 2: Encrypt ED Key with RSA Public Key (RSAES-PKCS1-v1.5)
		// setRSA_Encrypt(plaintext, len, pubkey, pubkey_len, ciphertext)
		int32_t rsa_result = setRSA_Encrypt(plaintext, plaintext_len, rsa_pubkey, rsa_pubkey_len, tx_data);
		if (rsa_result != 0)  // 0 = CMOX_RSA_SUCCESS
		{
			GLogE("[ED Import] RSA Encrypt failed: %d\r\n", rsa_result);
			return HAL_ERROR;
		}
		tx_len = 256U;  // RSA-2048 ciphertext is always 256 bytes
		GLogN("[ED Import] ED Key encrypted with RSA KEK\r\n");
	}
	else
#endif
	{
		// Plaintext import
		(void)memcpy(tx_data, plaintext, plaintext_len);
		tx_len = plaintext_len;
	}

	uint8_t param[LEN_PARAM];
	build_params(param, key_id, HSM_ALG_ED, tx_len);

	uint8_t rx[64];
	uint16_t rx_len = 0U;

	return process_host_communication(HSM_OPCODE_KEY_MNG, mode, param, tx_data, tx_len, rx, &rx_len, MODE_PLAINTEXT);
}
HAL_StatusTypeDef hsm_import_KD_SHA256_key(uint16_t key_id, const uint8_t* key_data, bool encrypted, bool lock_key)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_import_ed_key] start\r\n");
#endif
	if (key_data == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	uint8_t mode1 = HSM_KEYMNG_IMPORT | ((encrypted == true) ? 0x04U : 0x00U) | ((lock_key == true) ? 0x80U : 0x00U) | 0x08U;
	uint8_t mode2 = 0;
	//mode2=0x40;  // private key only
	mode2=0x00;  // kd
	uint16_t mode = (uint16_t)mode1 | ((uint16_t)mode2 << 8U);

	// Prepare plaintext ED Key data (public + private = 64 bytes max)
	//uint8_t plaintext[32];
	//uint16_t plaintext_len = 0U;
	uint16_t tx_len = 0U;

	//uint8_t tx_data[256];
	

	if (encrypted)
	{
		// Encrypt import
		//(void)memcpy(&plaintext[plaintext_len], key_data, 256U);
		tx_len = 256U;
	}
	else
	{
		// Plaintext import
		//(void)memcpy(&plaintext[plaintext_len], key_data, 32U);
		tx_len = 32U;
	}

	uint8_t param[LEN_PARAM];
	build_params(param, key_id, HSM_ALG_CUSTOM, tx_len);

	uint8_t rx[64];
	uint16_t rx_len = 0U;

	return process_host_communication(HSM_OPCODE_KEY_MNG, mode, param, key_data, tx_len, rx, &rx_len, MODE_PLAINTEXT);
}

/**
 * @brief Export public key (OP 0x05)
 */
HAL_StatusTypeDef hsm_export_public_key(uint16_t key_id, uint8_t alg_type, uint8_t* output_buffer, uint16_t* output_length)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_export_public_key] start\r\n");
#endif
	if ((output_buffer == NULL) || (output_length == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	uint8_t mode1 = HSM_KEYMNG_EXPORT;
	uint8_t mode2 = 0x80U;
	uint16_t mode = (uint16_t)mode1 | ((uint16_t)mode2 << 8U);

	uint8_t param[LEN_PARAM];
	build_params(param, key_id, alg_type, 0U);

	uint8_t rx[512];
	uint16_t rx_len = 0U;

	HAL_StatusTypeDef status = process_host_communication(HSM_OPCODE_KEY_MNG, mode, param, NULL, 0U, rx, &rx_len, MODE_PLAINTEXT);

	if ((status == HAL_OK) && (rx_len > 0U))
	{
	  *output_length = rx_len;
	  (void)memcpy(output_buffer, rx, rx_len);
	}

	return status;
}

/**
 * @brief Generate key pair (OP 0x05)
 */
HAL_StatusTypeDef hsm_generate_key_pair(uint16_t key_id, uint8_t alg_type, uint8_t key_size, bool lock_key)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_generate_key_pair] start\r\n");
#endif
	uint8_t mode1 = HSM_KEYMNG_GENERATE | ((lock_key == true) ? 0x80U : 0x00U);
	uint8_t mode2 = (alg_type == HSM_ALG_AES) ? key_size : 0x00U;
	uint16_t mode = (uint16_t)mode1 | ((uint16_t)mode2 << 8U);

	uint8_t param[LEN_PARAM];
	build_params(param, key_id, alg_type, 0U);

	uint8_t rx[320];  // RSA public key can be ~260 bytes
	uint16_t rx_len = 0U;

	HAL_StatusTypeDef status = process_host_communication(HSM_OPCODE_KEY_MNG, mode, param, NULL, 0U, rx, &rx_len, MODE_PLAINTEXT);

	return status;
}

/**
 * @brief Store certificate (OP 0x06) - Multi-call
 */
HAL_StatusTypeDef hsm_store_certificate(uint16_t cert_id, uint8_t alg_type, const uint8_t* cert_data, uint16_t cert_length)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_store_certificate] start\r\n");
#endif
	if ((cert_data == NULL) || (cert_length == 0U) || (cert_length > 2048U))
	{
		hsm_print_error(AVO_ERR_INVALID_PARAM);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;
	uint16_t offset = 0U;
	uint16_t chunk_size;

	uint16_t mode_start = HSM_MODE_START | (0x00U << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, cert_id, alg_type, cert_length);

	uint8_t rx_start[32];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_CERT_MNG, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	if (rx_len_start >= 2U)
	{
		/* R_CMD response: [Context ID(2)] (Response Code excluded by spi_cmd_r_cmd) */
		ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);
	}

	while (offset < cert_length)
	{
		chunk_size = ((cert_length - offset) > 1024U) ? 1024U : (cert_length - offset);

		uint16_t mode_update = HSM_MODE_UPDATE | (0x00U << 8U);
		uint8_t param_update[LEN_PARAM];
		build_params(param_update, cert_id, alg_type, chunk_size);

		uint8_t rx_update[32];
		uint16_t rx_len_update = 0U;
		status = process_host_communication_m2(HSM_OPCODE_CERT_MNG, mode_update, param_update, (uint8_t*)&cert_data[offset], chunk_size, rx_update, &rx_len_update, MODE_PLAINTEXT);
		if (status != HAL_OK)
	{
		return status;
	}

		if ((rx_len_update > 0U) && (rx_update[0] != AVO_RET_OK))
		{
			hsm_print_error(rx_update[0]);
			return HAL_ERROR;
		}

		offset += chunk_size;
	}

	uint16_t mode_finish = HSM_MODE_FINISH | (0x00U << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, cert_id, alg_type, 0U);

	uint8_t rx_finish[32];
	uint16_t rx_len_finish = 0U;

	return process_host_communication_m2(HSM_OPCODE_CERT_MNG, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);
}

/**
 * @brief Read certificate (OP 0x06) - Multi-call
 */
HAL_StatusTypeDef hsm_read_certificate(uint16_t cert_id, uint8_t alg_type, uint8_t* cert_buffer, uint16_t* cert_length)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_read_certificate] start\r\n");
#endif
	if ((cert_buffer == NULL) || (cert_length == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;
	uint16_t total_len = 0U;
	uint16_t offset = 0U;

	uint16_t mode_start = HSM_MODE_START | (0x01U << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, cert_id, alg_type, 0U);

	uint8_t rx_start[32];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_CERT_MNG, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	if (rx_len_start >= 2U)
	{
		/* R_CMD response: [Total Length(2)] - CertId acts as CtxId, no separate CtxId returned */
		total_len = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);
	}
#if defined(NEW_HSM_LOG_ENABLE)
	/* Log Total Length from Start response */
	GLogN("[CERT_MNG] Export: Slot #%d total_len=%d (rx_len=%d)\r\n", cert_id, total_len, rx_len_start);
#endif

	/* Send at least one UPDATE to get actual data (do-while ensures at least one iteration) */
	do
	{
		uint16_t mode_update = HSM_MODE_UPDATE | (0x01U << 8U);
		uint8_t param_update[LEN_PARAM];
		build_params(param_update, cert_id, alg_type, 1024U);

		uint8_t rx_update[1024];
		uint16_t rx_len_update = 0U;

		status = process_host_communication_m2(HSM_OPCODE_CERT_MNG, mode_update, param_update, NULL, 0U, rx_update, &rx_len_update, MODE_PLAINTEXT);
		if (status != HAL_OK)
		{
			return status;
		}

		/* R_CMD response: [Message Length(2)] + [Message(N)] */
		uint16_t data_len = (uint16_t)rx_update[0] | ((uint16_t)rx_update[1] << 8U);
#if defined(NEW_HSM_LOG_ENABLE)
		GLogN("[CERT_MNG] Update: data_len=%d (rx_len=%d)\r\n", data_len, rx_len_update);
#endif

		if (data_len > 0U)
		{
			(void)memcpy(&cert_buffer[offset], &rx_update[2], data_len);
			offset += data_len;
		}
		else
		{
			/* No more data, exit loop */
			break;
		}
	} while (offset < 2048U);  /* Safety limit */

	*cert_length = offset;

	uint16_t mode_finish = HSM_MODE_FINISH | (0x01U << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, cert_id, alg_type, 0U);

	uint8_t rx_finish[32];
	uint16_t rx_len_finish = 0U;

	return process_host_communication_m2(HSM_OPCODE_CERT_MNG, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);
}

/**
 * @brief ECDH Key Exchange (OP 0x07) - Multi-call
 */
HAL_StatusTypeDef hsm_key_exchange_ecdh(uint16_t key_id, const uint8_t* peer_public_x, const uint8_t* peer_public_y, uint8_t* shared_secret)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_key_exchange_ecdh] start\r\n");
#endif
	if ((peer_public_x == NULL) || (peer_public_y == NULL) || (shared_secret == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;

	uint16_t mode_start = HSM_MODE_START | (0x00U << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, key_id, 0U, 0U);

	uint8_t rx_start[32];
	uint16_t rx_len_start = 0U;

	status = process_host_communication(HSM_OPCODE_KEY_EXCHANGE, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	if (rx_len_start >= 2U)
	{
		/* R_CMD response: [Context ID(2)] (Response Code excluded by spi_cmd_r_cmd) */
		ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);
	}

	uint16_t mode_update = HSM_MODE_UPDATE | (0x00U << 8U);
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, 0U, ctx_id, 0U);

	uint8_t peer_pubkey[64];
	(void)memcpy(&peer_pubkey[0], peer_public_x, 32U);
	(void)memcpy(&peer_pubkey[32U], peer_public_y, 32U);

	//uint8_t rx_update[32];
	//uint16_t rx_len_update = 0U;

	status = process_host_communication_m1_update(HSM_OPCODE_KEY_EXCHANGE, mode_update, param_update, peer_pubkey, 64U, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	uint16_t mode_finish = HSM_MODE_FINISH | (0x00U << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, 0U, ctx_id, 0U);

	uint8_t rx_finish[64];
	uint16_t rx_len_finish = 0U;

	status = process_host_communication(HSM_OPCODE_KEY_EXCHANGE, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Secret(32)] (Response Code excluded by spi_cmd_r_cmd) */
	if (rx_len_finish >= 32U)
	{
		(void)memcpy(shared_secret, &rx_finish[0], 32U);
	}

	return HAL_OK;
}

/**
 * @brief Calculate SHA hash (OP 0x11) - M1 Multi-call
 * @param sha_type: SHA algorithm (HSM_SHA_160/256/512)
 * @param use_kd: true = KD-SHA mode (Bit7=1), key_id 사용
 * @param key_id: KD-SHA일 때 Secret Key ID
 * @param input_data: 입력 데이터
 * @param input_length: 입력 길이
 * @param digest: 출력 해시값
 */
HAL_StatusTypeDef hsm_calculate_sha(uint8_t sha_type, bool use_kd, uint16_t key_id, const uint8_t* input_data, uint16_t input_length, uint8_t* digest)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_calculate_sha] start, type=0x%02X, kd=%d\r\n", sha_type, use_kd);
#endif
	if ((input_data == NULL) || (digest == NULL) || (input_length == 0U))
	{
		hsm_print_error(AVO_ERR_INVALID_PARAM);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;

	/* Mode2: Bit0-1=SHA Type, Bit7=KD-SHA flag */
	uint8_t mode2 = sha_type;
	if (use_kd == true)
	{
		mode2 |= 0x80U;  /* Bit7 = KD-SHA */
	}

	uint16_t mode_start = HSM_MODE_START | ((uint16_t)mode2 << 8U);
	uint8_t param_start[LEN_PARAM];
	uint16_t effective_key_id = (use_kd == true) ? key_id : 0U;
	build_params(param_start, effective_key_id, 0U, input_length);

	uint8_t rx_start[32];
	uint16_t rx_len_start = 0U;

	status = process_host_communication(HSM_OPCODE_SHA, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		GLogE("[SHA] Start FAIL\r\n");
		return status;
	}

	if (rx_len_start >= 2U)
	{
		/* R_CMD response: [Context ID(2)] (Response Code excluded by spi_cmd_r_cmd) */
		ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);
	}

	uint16_t offset = 0U;
	while (offset < input_length)
	{
		uint16_t chunk = ((input_length - offset) > 1024U) ? 1024U : (input_length - offset);

		uint16_t mode_update = HSM_MODE_UPDATE | ((uint16_t)mode2 << 8U);
		uint8_t param_update[LEN_PARAM];
		build_params(param_update, effective_key_id, ctx_id, chunk);

		//uint8_t rx_update[32];
		//uint16_t rx_len_update = 0U;

		status = process_host_communication_m1_update(HSM_OPCODE_SHA, mode_update, param_update, (uint8_t*)&input_data[offset], chunk, MODE_PLAINTEXT);
		if (status != HAL_OK)
		{
			GLogE("[SHA] Update FAIL\r\n");
			return status;
		}

		offset += chunk;
	}

	uint16_t mode_finish = HSM_MODE_FINISH | ((uint16_t)mode2 << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, effective_key_id, ctx_id, 0U);

	uint8_t rx_finish[64];
	uint16_t rx_len_finish = 0U;

	status = process_host_communication(HSM_OPCODE_SHA, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		GLogE("[SHA] Finish FAIL\r\n");
		return status;
	}

	/* v5: SHA512(64) 지원 추가 */
	uint8_t digest_len;
	if (sha_type == HSM_SHA_160) {
		digest_len = 20U;
	} else if (sha_type == HSM_SHA_512) {
		digest_len = 64U;
	} else {
		digest_len = 32U;  /* SHA256 */
	}
	/* R_CMD response: [Digest(N)] (Response Code excluded by spi_cmd_r_cmd) */
	if (rx_len_finish >= digest_len)
	{
		(void)memcpy(digest, &rx_finish[0], digest_len);
	}

	return HAL_OK;
}

/**
 * @brief AES encrypt (OP 0x12) - Multi-call
 */
HAL_StatusTypeDef hsm_aes_encrypt(uint16_t key_id, uint8_t aes_key_size, uint8_t aes_mode, const uint8_t* iv, const uint8_t* plaintext, uint16_t length, uint8_t* ciphertext)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_aes_encrypt] start\r\n");
#endif
	if ((plaintext == NULL) || (ciphertext == NULL) || (length == 0U) || (length > 512U))
	{
		hsm_print_error(AVO_ERR_INVALID_PARAM);
		return HAL_ERROR;
	}

	uint8_t padded_data[528];
	uint16_t padded_length = length;
	const uint8_t* data_to_encrypt = plaintext;

	if (((aes_mode == HSM_AES_ECB) || (aes_mode == HSM_AES_CBC)) && ((length % 16U) != 0U))
	{
		padded_length = ((length + 15U) / 16U) * 16U;
		if (padded_length > 512U)
		{
			hsm_print_error(AVO_ERR_INVALID_LENGTH);
			return HAL_ERROR;
		}
		(void)memcpy(padded_data, plaintext, length);
		(void)memset(padded_data + length, 0x00U, padded_length - length);
		data_to_encrypt = padded_data;
	}

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;

	uint8_t mode1 = HSM_MODE_START;
	uint8_t mode2 = aes_key_size | (uint8_t)(aes_mode << 3U);
	uint16_t mode_start = mode1 | ((uint16_t)mode2 << 8U);

	uint8_t param_start[LEN_PARAM];
	build_params(param_start, key_id, 0U, 0U);

	uint8_t tx_data[528];
	uint16_t tx_len = 0U;

	if (aes_mode != HSM_AES_ECB && iv != NULL)
	{
		(void)memcpy(tx_data, iv, 16U);
		tx_len = 16U;
	}

	uint8_t rx_start[32];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_AES, mode_start, param_start, (tx_len > 0U) ? tx_data : NULL, tx_len, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	if (rx_len_start >= 2U)
	{
		/* R_CMD response: [Context ID(2)] (Response Code excluded by spi_cmd_r_cmd) */
		ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);
	}

	uint16_t mode_update = HSM_MODE_UPDATE | ((uint16_t)mode2 << 8U);
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, key_id, ctx_id, padded_length);

	uint8_t rx_update[528];
	uint16_t rx_len_update = 0U;

	status = process_host_communication_m2(HSM_OPCODE_AES, mode_update, param_update, (uint8_t*)data_to_encrypt, padded_length, rx_update, &rx_len_update, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	if (rx_len_update >= 3U)
	{
		/* R_CMD response: [Message Length(2)] + [Message(N)] (Response Code excluded by spi_cmd_r_cmd) */
		uint16_t data_len = (uint16_t)rx_update[0] | ((uint16_t)rx_update[1] << 8U);
		(void)memcpy(ciphertext, &rx_update[2], data_len);
	}

	uint16_t mode_finish = HSM_MODE_FINISH | ((uint16_t)mode2 << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, key_id, ctx_id, 0U);

	uint8_t rx_finish[64];
	uint16_t rx_len_finish = 0U;

	return process_host_communication_m2(HSM_OPCODE_AES, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);
}

/**
 * @brief AES decrypt (OP 0x12) - Multi-call
 */
HAL_StatusTypeDef hsm_aes_decrypt(uint16_t key_id, uint8_t aes_key_size, uint8_t aes_mode, const uint8_t* iv, const uint8_t* ciphertext, uint16_t length, uint8_t* plaintext)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_aes_decrypt] start\r\n");
#endif
	if ((ciphertext == NULL) || (plaintext == NULL) || (length == 0U) || (length > 512U))
	{
		hsm_print_error(AVO_ERR_INVALID_PARAM);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;

	uint8_t mode1 = HSM_MODE_START | 0x80U;
	uint8_t mode2 = aes_key_size | (uint8_t)(aes_mode << 3U);
	uint16_t mode_start = mode1 | ((uint16_t)mode2 << 8U);

	uint8_t param_start[LEN_PARAM];
	build_params(param_start, key_id, 0U, 0U);

	uint8_t tx_data[528];
	uint16_t tx_len = 0U;

	if (aes_mode != HSM_AES_ECB && iv != NULL)
	{
		(void)memcpy(tx_data, iv, 16U);
		tx_len = 16U;
	}

	uint8_t rx_start[32];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_AES, mode_start, param_start, (tx_len > 0U) ? tx_data : NULL, tx_len, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	if (rx_len_start >= 2U)
	{
		/* R_CMD response: [Context ID(2)] (Response Code excluded by spi_cmd_r_cmd) */
		ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);
	}

	uint16_t mode_update = (HSM_MODE_UPDATE | 0x80U) | ((uint16_t)mode2 << 8U);
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, key_id, ctx_id, length);

	uint8_t rx_update[528];
	uint16_t rx_len_update = 0U;

	status = process_host_communication_m2(HSM_OPCODE_AES, mode_update, param_update, (uint8_t*)ciphertext, length, rx_update, &rx_len_update, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	if (rx_len_update >= 3U)
	{
		/* R_CMD response: [Message Length(2)] + [Message(N)] (Response Code excluded by spi_cmd_r_cmd) */
		uint16_t data_len = (uint16_t)rx_update[0] | ((uint16_t)rx_update[1] << 8U);
		(void)memcpy(plaintext, &rx_update[2], data_len);
	}

	uint16_t mode_finish = (HSM_MODE_FINISH | 0x80U) | ((uint16_t)mode2 << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, key_id, ctx_id, 0U);

	uint8_t rx_finish[64];
	uint16_t rx_len_finish = 0U;

	return process_host_communication_m2(HSM_OPCODE_AES, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);
}

/**
 * @brief Generate HMAC-SHA256 (OP 0x13) - Multi-call
 */
HAL_StatusTypeDef hsm_generate_hmac_sha256(uint16_t key_id, const uint8_t* input_data, uint16_t input_length, uint8_t* mac_output)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_generate_hmac_sha256] start\r\n");
#endif
	if ((input_data == NULL) || (mac_output == NULL) /*|| (input_length < 64U)*/ || (input_length > 512U))
	{
		hsm_print_error(AVO_ERR_INVALID_PARAM);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t context_id = 0U;

	uint16_t mode_start = HSM_MODE_HMAC_SHA256_START;
	uint8_t param_start[LEN_PARAM];
	//build_params(param_start, key_id, 0U, 0U);
	build_params(param_start, key_id, 0U, input_length);

	uint8_t rx_start[4];
	uint16_t rx_len_start = 0U;

	status = process_host_communication(HSM_OPCODE_MAC, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	context_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);

	uint16_t mode_update = HSM_MODE_HMAC_SHA256_UPDATE;
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, key_id, context_id, input_length);

	//uint8_t rx_update[8];
	//uint16_t rx_len_update = 0U;

	// Use full communication (W_CMD + R_CMD) for Update phase
	//status = process_host_communication(HSM_OPCODE_MAC, mode_update, param_update, input_data, input_length, rx_update, &rx_len_update, MODE_PLAINTEXT);
	//if (status != HAL_OK)
	//{
	//	return status;
	//}
	status = process_host_communication_m1_update(HSM_OPCODE_MAC, mode_update, param_update, input_data, input_length, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	// Check Update response code
	//if ((rx_len_update > 0U) && (rx_update[0] != AVO_RET_OK))
	//{
	//	hsm_print_error(rx_update[0]);
	//	return HAL_ERROR;
	//}

	uint16_t mode_finish = HSM_MODE_HMAC_SHA256_FINISH;
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, key_id, context_id, 0U);

	uint8_t rx_finish[32];
	uint16_t rx_len_finish = 0U;

	status = process_host_communication(HSM_OPCODE_MAC, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Digest(16)] (Response Code excluded by spi_cmd_r_cmd) */
	(void)memcpy(mac_output, &rx_finish[0], 32U);

	return HAL_OK;
}

/**
 * @brief Generate CMAC-AES (OP 0x13) - Multi-call
 */
HAL_StatusTypeDef hsm_generate_cmac_aes(uint16_t key_id, uint8_t aes_key_size, const uint8_t* input_data, uint16_t input_length, uint8_t* mac_output)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_generate_cmac_aes] start\r\n");
#endif
	if ((input_data == NULL) || (mac_output == NULL) || (input_length == 0U) || (input_length > 512U))
	{
		hsm_print_error(AVO_ERR_INVALID_PARAM);
		return HAL_ERROR;
	}

	uint8_t block_size = (aes_key_size == HSM_AES_128) ? 16U :
						 (aes_key_size == HSM_AES_192) ? 24U : 32U;
	if ((input_length % block_size) != 0U)
	{
		hsm_print_error(AVO_ERR_INVALID_LENGTH);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t context_id = 0U;

	uint16_t mode_start = HSM_MODE_CMAC_AES_START_BASE | ((uint16_t)aes_key_size << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, key_id, 0U, 0U);

	uint8_t rx_start[4];
	uint16_t rx_len_start = 0U;

	status = process_host_communication(HSM_OPCODE_MAC, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	context_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);

	uint16_t mode_update = HSM_MODE_CMAC_AES_UPDATE_BASE | ((uint16_t)aes_key_size << 8U);
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, key_id, context_id, input_length);

	//uint8_t rx_update[4];
	//uint16_t rx_len_update = 0U;

	status = process_host_communication_m1_update(HSM_OPCODE_MAC, mode_update, param_update, input_data, input_length, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	uint16_t mode_finish = HSM_MODE_CMAC_AES_FINISH_BASE | ((uint16_t)aes_key_size << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, key_id, context_id, 0U);

	uint8_t rx_finish[18];
	uint16_t rx_len_finish = 0U;

	status = process_host_communication(HSM_OPCODE_MAC, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Digest(16)] (Response Code excluded by spi_cmd_r_cmd) */
	(void)memcpy(mac_output, &rx_finish[0], 16U);

	return HAL_OK;
}

/**
 * @brief RSA Sign (OP 0x14) - Multi-call
 */
HAL_StatusTypeDef hsm_sign_rsa(uint16_t key_id, uint8_t sha_type, const uint8_t* digest, uint8_t* signature)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_sign_rsa] start\r\n");
#endif
	if ((digest == NULL) || (signature == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	uint8_t digest_len = (sha_type == HSM_SHA_160) ? 20U : 32U;
	uint8_t mode2 = (sha_type == HSM_SHA_160) ? 0x01U : 0x02U;

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;

	uint16_t mode_start = HSM_MODE_START | ((uint16_t)mode2 << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, key_id, 0U, 0U);

	uint8_t rx_start[4];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Response Code(1)] + [Context ID(2)] - Skip Response Code */
		ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);

	uint16_t mode_update = HSM_MODE_UPDATE | ((uint16_t)mode2 << 8U);
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, key_id, ctx_id, digest_len);

	uint8_t rx_update[4];
	uint16_t rx_len_update = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_update, param_update, digest, digest_len, rx_update, &rx_len_update, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	uint16_t mode_finish = HSM_MODE_FINISH | ((uint16_t)mode2 << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, key_id, ctx_id, 0U);

	uint8_t rx_finish[260];
	uint16_t rx_len_finish = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Signature(256)] (Response Code excluded by spi_cmd_r_cmd) */
	(void)memcpy(signature, &rx_finish[0], 256U);

	return HAL_OK;
}

/**
 * @brief RSA Verify Signature (OP 0x14) - Multi-call
 */
HAL_StatusTypeDef hsm_verify_rsa_signature(uint16_t key_id, uint8_t sha_type, const uint8_t* digest, const uint8_t* signature,
                                           bool* is_valid)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_verify_rsa_signature] start\r\n");
#endif
	if ((digest == NULL) || (signature == NULL) || (is_valid == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	uint8_t digest_len = (sha_type == HSM_SHA_160) ? 20U : 32U;
	uint8_t mode2 = (sha_type == HSM_SHA_160) ? 0x01U : 0x02U;

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;

	uint16_t mode_start = (HSM_MODE_START | 0x80U) | ((uint16_t)mode2 << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, key_id, 0U, 0U);

	uint8_t rx_start[4];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Response Code(1)] + [Context ID(2)] - Skip Response Code */
		ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);

	uint16_t mode_update = (HSM_MODE_UPDATE | 0x80U) | ((uint16_t)mode2 << 8U);
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, key_id, ctx_id, digest_len + 256U);

	uint8_t tx_data[288];
	(void)memcpy(tx_data, digest, digest_len);
	(void)memcpy(&tx_data[digest_len], signature, 256U);

	uint8_t rx_update[4];
	uint16_t rx_len_update = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_update, param_update, tx_data, digest_len + 256U, rx_update, &rx_len_update, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	uint16_t mode_finish = (HSM_MODE_FINISH | 0x80U) | ((uint16_t)mode2 << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, key_id, ctx_id, 0U);

	uint8_t rx_finish[4];
	uint16_t rx_len_finish = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);

	/* Verify result: HAL_OK = signature valid (0x00), HAL_ERROR = signature invalid (0x80) */
	*is_valid = (status == HAL_OK);

	return HAL_OK;
}

/**
 * @brief ECDSA Sign (OP 0x14) - Multi-call
 */
HAL_StatusTypeDef hsm_sign_ecdsa(uint16_t key_id, const uint8_t* digest, uint8_t* signature_r, uint8_t* signature_s)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_sign_ecdsa] start\r\n");
#endif
	if ((digest == NULL) || (signature_r == NULL) || (signature_s == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;

	uint16_t mode_start = HSM_MODE_START | (0x03U << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, key_id, 0U, 0U);

	uint8_t rx_start[4];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Response Code(1)] + [Context ID(2)] - Skip Response Code */
		ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);

	uint16_t mode_update = HSM_MODE_UPDATE | (0x03U << 8U);
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, key_id, ctx_id, 32U);

	uint8_t rx_update[4];
	uint16_t rx_len_update = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_update, param_update, digest, 32U, rx_update, &rx_len_update, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	uint16_t mode_finish = HSM_MODE_FINISH | (0x03U << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, key_id, ctx_id, 0U);

	uint8_t rx_finish[68];
	uint16_t rx_len_finish = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [R(32)] + [S(32)] (Response Code excluded by spi_cmd_r_cmd) */
	(void)memcpy(signature_r, &rx_finish[0], 32U);
	(void)memcpy(signature_s, &rx_finish[32U], 32U);

	return HAL_OK;
}

/**
 * @brief ECDSA Verify Signature (OP 0x14) - Multi-call
 */
HAL_StatusTypeDef hsm_verify_ecdsa_signature(uint16_t key_id, const uint8_t* digest, const uint8_t* signature_r, const uint8_t* signature_s, bool* is_valid)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_verify_ecdsa_signature] start\r\n");
#endif
	if ((digest == NULL) || (signature_r == NULL) || (signature_s == NULL) || (is_valid == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;

	uint16_t mode_start = (HSM_MODE_START | 0x80U) | (0x03U << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, key_id, 0U, 0U);

	uint8_t rx_start[4];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Response Code(1)] + [Context ID(2)] - Skip Response Code */
		ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);

	uint16_t mode_update = (HSM_MODE_UPDATE | 0x80U) | (0x03U << 8U);
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, key_id, ctx_id, 96U);

	uint8_t tx_data[96];
	(void)memcpy(tx_data, digest, 32U);
	(void)memcpy(&tx_data[32U], signature_r, 32U);
	(void)memcpy(&tx_data[64U], signature_s, 32U);

	uint8_t rx_update[4];
	uint16_t rx_len_update = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_update, param_update, tx_data, 96U, rx_update, &rx_len_update, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	uint16_t mode_finish = (HSM_MODE_FINISH | 0x80U) | (0x03U << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, key_id, ctx_id, 0U);

	uint8_t rx_finish[4];
	uint16_t rx_len_finish = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);

	/* Verify result: HAL_OK = signature valid (0x00), HAL_ERROR = signature invalid (0x80) */
	*is_valid = (status == HAL_OK);

	return HAL_OK;
}

/**
 * @brief EdDSA Sign (Ed25519) (OP 0x14) - Multi-call
 */
HAL_StatusTypeDef hsm_sign_eddsa(uint16_t key_id, const uint8_t* message, uint16_t message_length, uint8_t* signature)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_sign_eddsa] start\r\n");
#endif
	if ((message == NULL) || (signature == NULL) || (message_length == 0U))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;

	uint16_t mode_start = HSM_MODE_START | (0x04U << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, key_id, 0U, 0U);

	uint8_t rx_start[4];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Response Code(1)] + [Context ID(2)] - Skip Response Code */
		ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);

	uint16_t mode_update = HSM_MODE_UPDATE | (0x04U << 8U);
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, key_id, ctx_id, message_length);

	uint8_t rx_update[4];
	uint16_t rx_len_update = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_update, param_update, message, message_length, rx_update, &rx_len_update, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	uint16_t mode_finish = HSM_MODE_FINISH | (0x04U << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, key_id, ctx_id, 0U);

	uint8_t rx_finish[68];
	uint16_t rx_len_finish = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Signature(64)] (Response Code excluded by spi_cmd_r_cmd) */
	(void)memcpy(signature, &rx_finish[0], 64U);

	return HAL_OK;
}

/**
 * @brief EdDSA Verify Signature (Ed25519) (OP 0x14) - Multi-call
 */
HAL_StatusTypeDef hsm_verify_eddsa_signature(uint16_t key_id, const uint8_t* message, uint16_t message_length, const uint8_t* signature, bool* is_valid)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_verify_eddsa_signature] start\r\n");
#endif
	if ((message == NULL) || (signature == NULL) || (is_valid == NULL) || (message_length == 0U))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	if (message_length > MAX_DATA_PAYLOAD_SIZE)
	{
		hsm_print_error(AVO_ERR_OUT_OF_RANGE);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;

	uint16_t mode_start = (HSM_MODE_START | 0x80U) | (0x04U << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, key_id, 0U, 0U);

	uint8_t rx_start[4];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Response Code(1)] + [Context ID(2)] - Skip Response Code */
		ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);

	uint16_t mode_update = (HSM_MODE_UPDATE | 0x80U) | (0x04U << 8U);
	uint8_t param_update[LEN_PARAM];
	uint16_t total_len = message_length + 64U;
	build_params(param_update, key_id, ctx_id, total_len);

	uint8_t tx_data[MAX_EDDSA_VERIFY_DATA_SIZE];
	(void)memcpy(tx_data, message, message_length);
	(void)memcpy(&tx_data[message_length], signature, 64U);

	uint8_t rx_update[4];
	uint16_t rx_len_update = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_update, param_update, tx_data, total_len, rx_update, &rx_len_update, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	uint16_t mode_finish = (HSM_MODE_FINISH | 0x80U) | (0x04U << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, key_id, ctx_id, 0U);

	uint8_t rx_finish[4];
	uint16_t rx_len_finish = 0U;

	status = process_host_communication_m2(HSM_OPCODE_SIGN_AND_VERIFY, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);

	/* Verify result: HAL_OK = signature valid (0x00), HAL_ERROR = signature invalid (0x80) */
	*is_valid = (status == HAL_OK);

	return HAL_OK;
}

/**
 * @brief RSA Encrypt (OP 0x15) - Multi-call
 */
HAL_StatusTypeDef hsm_rsa_encrypt(uint16_t key_id, const uint8_t* plaintext, uint16_t plaintext_length, uint8_t* ciphertext)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_rsa_encrypt] start\r\n");
#endif
	if ((plaintext == NULL) || (ciphertext == NULL) || (plaintext_length == 0U) || (plaintext_length > 256U))
	{
		hsm_print_error(AVO_ERR_INVALID_PARAM);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;

	uint16_t mode_start = HSM_MODE_START | (0x01U << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, key_id, 0U, 0U);

	uint8_t rx_start[4];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_RSAES, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Response Code(1)] + [Context ID(2)] - Skip Response Code */
		ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);

	uint16_t mode_update = HSM_MODE_UPDATE | (0x01U << 8U);
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, key_id, ctx_id, plaintext_length);

	uint8_t rx_update[4];
	uint16_t rx_len_update = 0U;

	status = process_host_communication_m2(HSM_OPCODE_RSAES, mode_update, param_update, plaintext, plaintext_length, rx_update, &rx_len_update, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	uint16_t mode_finish = HSM_MODE_FINISH | (0x01U << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, key_id, ctx_id, 0U);

	uint8_t rx_finish[260];
	uint16_t rx_len_finish = 0U;

	status = process_host_communication_m2(HSM_OPCODE_RSAES, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Response Code(1)] + [Message Length(2)] + [Message(N)]
	 * Skip Response Code, read Message Length, then copy Message */
	uint16_t cipher_len = (uint16_t)rx_finish[0] | ((uint16_t)rx_finish[1] << 8U);
	(void)memcpy(ciphertext, &rx_finish[2], cipher_len);

	return HAL_OK;
}

/**
 * @brief RSA Decrypt (OP 0x15) - Multi-call
 */
HAL_StatusTypeDef hsm_rsa_decrypt(uint16_t key_id, const uint8_t* ciphertext, uint8_t* plaintext, uint16_t* plaintext_length)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_rsa_decrypt] start\r\n");
#endif
	if ((ciphertext == NULL) || (plaintext == NULL) || (plaintext_length == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;

	uint16_t mode_start = (HSM_MODE_START | 0x80U) | (0x01U << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, key_id, 0U, 0U);

	uint8_t rx_start[4];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_RSAES, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Response Code(1)] + [Context ID(2)] - Skip Response Code */
		ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);

	uint16_t mode_update = (HSM_MODE_UPDATE | 0x80U) | (0x01U << 8U);
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, key_id, ctx_id, 256U);

	uint8_t rx_update[4];
	uint16_t rx_len_update = 0U;

	status = process_host_communication_m2(HSM_OPCODE_RSAES, mode_update, param_update, ciphertext, 256U, rx_update, &rx_len_update, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	uint16_t mode_finish = (HSM_MODE_FINISH | 0x80U) | (0x01U << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, key_id, ctx_id, 0U);

	uint8_t rx_finish[260];
	uint16_t rx_len_finish = 0U;

	status = process_host_communication_m2(HSM_OPCODE_RSAES, mode_finish, param_finish, NULL, 0U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Response Code(1)] + [Message Length(2)] + [Message(N)]
	 * Skip Response Code, read Message Length, then copy Message */
	*plaintext_length = (uint16_t)rx_finish[0] | ((uint16_t)rx_finish[1] << 8U);
	(void)memcpy(plaintext, &rx_finish[2], *plaintext_length);

	return HAL_OK;
}

/**
 * @brief Derive key using HKDF (OP 0x16) - One-shot
 */
HAL_StatusTypeDef hsm_derive_key_hkdf(uint16_t source_key_id, const uint8_t* password, uint8_t password_length, const uint8_t* salt, uint8_t salt_length,
									  const uint8_t* info, uint16_t info_length, uint16_t output_key_length, uint8_t* derived_key)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_derive_key_hkdf] start\r\n");
#endif
	if (derived_key == NULL)
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	if ((password_length > 128U) || (salt_length > 128U) || (info_length > 256U) || (output_key_length > 512U))
	{
		hsm_print_error(AVO_ERR_OUT_OF_RANGE);
		return HAL_ERROR;
	}

	uint8_t mode2 = 0x05U;
	if ((password != NULL) && (password_length > 0U))
	{
		mode2 |= 0x80U;
	}
	uint16_t mode = ((uint16_t)mode2 << 8U);

	uint8_t tx_data[520];
	uint16_t offset = 0U;

	if ((password != NULL) && (password_length > 0U))
	{
		tx_data[offset] = password_length;
		offset++;
		(void)memcpy(&tx_data[offset], password, password_length);
		offset += password_length;
	}
	else
	{
		tx_data[offset] = 0U;
		offset++;
	}

	if ((salt != NULL) && (salt_length > 0U))
	{
		tx_data[offset] = salt_length;
		offset++;
		(void)memcpy(&tx_data[offset], salt, salt_length);
		offset += salt_length;
	}
	else
	{
		tx_data[offset] = 0U;
		offset++;
	}

	if ((info != NULL) && (info_length > 0U))
	{
		tx_data[offset] = (uint8_t)(info_length & 0xFFU);
		offset++;
		(void)memcpy(&tx_data[offset], info, info_length);
		offset += info_length;
	}
	else
	{
		tx_data[offset] = 0U;
		offset++;
	}

	tx_data[offset] = (uint8_t)(output_key_length & 0xFFU);
	offset++;
	tx_data[offset] = (uint8_t)((output_key_length >> 8U) & 0xFFU);
	offset++;

	uint8_t param[LEN_PARAM];
	build_params(param, source_key_id, 0U, offset);

	uint8_t rx_data[520];
	uint16_t rx_len = 0U;

	HAL_StatusTypeDef status = process_host_communication(HSM_OPCODE_KDF, mode, param, tx_data, offset, rx_data, &rx_len, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	uint16_t dk_len = (uint16_t)rx_data[0] | ((uint16_t)rx_data[1] << 8U);
	(void)memcpy(derived_key, &rx_data[2], dk_len);

	return HAL_OK;
}


/**
 * @brief Generate ASK key (OP 0x20) - One-shot
 * @param iv: IV pointer. NULL = use reserved IV (IV Length=0), non-NULL = use provided IV (IV Length=16)
 */
HAL_StatusTypeDef hsm_generate_ask_key(const uint8_t* seed, const uint8_t* encrypted_ecu_code, uint8_t master_id, const uint8_t* iv, uint8_t* ask_key)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_generate_ask_key] start\r\n");
#endif
	if ((seed == NULL) || (encrypted_ecu_code == NULL) || (ask_key == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	/* Step 1: Wait for READY */
	HAL_StatusTypeDef status = wait_for_host_status(HOST_STATUS_READY, 5000);
	if (status != HAL_OK)
	{
		return status;
	}

	/* Step 2: Prepare data buffer */
	uint8_t data[42];
	(void)memcpy(&data[0], seed, 8U);                /* Seed (8 bytes) */
	(void)memcpy(&data[8], encrypted_ecu_code, 16U); /* ECU Code (16 bytes) */
	data[24] = master_id;                             /* MasterID (1 byte) */

	if (iv == NULL)
	{
		/* Use reserved IV: IV Length = 0, IV filled with zeros */
		data[25] = 0x00U;                             /* IV Length = 0 */
		(void)memset(&data[26], 0, 16U);             /* IV = all zeros */
#if defined(NEW_HSM_LOG_ENABLE)
		GLogN("[hsm_generate_ask_key] IV Length=0 (reserved IV)\r\n");
#endif
	}
	else
	{
		/* Use provided IV: IV Length = 16, copy actual IV */
		data[25] = 0x10U;                             /* IV Length = 16 */
		(void)memcpy(&data[26], iv, 16U);            /* IV = provided value */
#if defined(NEW_HSM_LOG_ENABLE)
		GLogN("[hsm_generate_ask_key] IV Length=16 (user IV)\r\n");
#endif
	}

	/* Param - all 0x00 (Reserved) */
	uint8_t param[6] = {0};

	/* Step 3: Send W_CMD */
	status = spi_cmd_w_cmd(HSM_OPCODE_ASK_MNG, 0x0000U, param, data, 42U, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* Step 4: Wait for DONE */
	status = wait_for_host_status(HOST_STATUS_DONE, 10000);
	if (status != HAL_OK)
	{
		return status;
	}

	/* Step 5: Read response via R_CMD */
	uint8_t rx[16];
	uint16_t rx_len = 0U;
	status = spi_cmd_r_cmd(rx, 16U, &rx_len, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* ASK Key is 8 bytes */
	if (rx_len >= 8U)
	{
		(void)memcpy(ask_key, rx, 8U);
	}

	return HAL_OK;
}

/**
 * @brief Internal Authentication (OP 0x21) - Start + Update phases
 * @details Equivalent to OLD HSM's InternalAuthHSM()
 *          Performs Start (get Context ID) + Update (send SR, receive CR + Sign1)
 *          Context ID is stored internally in g_auth_ctx_id for hsm_external_auth()
 *
 * @param use_kauth: true to use Kauth key (#103), false to use Km key (#102)
 * @param random_sr: Pointer to 16-byte Server Random from VCI
 * @param random_cr: [OUT] Pointer to store 16-byte Card Random from HSM
 * @param signature_hsm: [OUT] Pointer to store 32-byte Sign1 from HSM
 * @param key_id: [OUT] Pointer to store key ID
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_internal_auth(bool use_kauth, const uint8_t* random_sr,
                                    uint8_t* random_cr, uint8_t* signature_hsm,
                                    uint8_t* key_id)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_internal_auth] start\r\n");
#endif
	if ((random_sr == NULL) || (random_cr == NULL) || (signature_hsm == NULL) || (key_id == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;

	uint8_t mode2 = (use_kauth == true) ? 0x02U : 0x01U;

	/* Step 1: Start - Get Context ID */
	uint16_t mode_start = HSM_MODE_START | ((uint16_t)mode2 << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, 0U, 0U, 0U);

	uint8_t rx_start[4];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_MUTUAL_AUTH, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Response Code(1)] + [Context ID(2)]
	 * Skip Response Code and store Context ID in global variable */
	g_auth_ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);

	/* Step 2: Update - Send SR, Receive CR + Sign1 */
	uint16_t mode_update = HSM_MODE_UPDATE | ((uint16_t)mode2 << 8U);
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, 0U, g_auth_ctx_id, 16U);

	uint8_t rx_update[520];
	uint16_t rx_len_update = 0U;

	status = process_host_communication_m2(HSM_OPCODE_MUTUAL_AUTH, mode_update, param_update, random_sr, 16U, rx_update, &rx_len_update, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	if (rx_len_update >= 49U)
	{
		/* R_CMD response: [CR(16)] + [Sign1(32)] + [KeyId(1)] (Response Code excluded by spi_cmd_r_cmd) */
		(void)memcpy(random_cr, &rx_update[0], 16U);
		(void)memcpy(signature_hsm, &rx_update[16U], 32U);
		*key_id = rx_update[48U];
	}

	return HAL_OK;
}

/**
 * @brief External Authentication (OP 0x21) - Finish phase
 * @details Equivalent to OLD HSM's ExternalAuthHSM()
 *          Performs Finish (send Sign2, receive auth result)
 *          Uses Context ID stored by hsm_internal_auth() in g_auth_ctx_id
 *
 * @param use_kauth: true to use Kauth key (#103), false to use Km key (#102)
 * @param signature_vci: Pointer to 32-byte Sign2 calculated by VCI
 * @param auth_success: [OUT] Pointer to store authentication result
 * @return HAL_StatusTypeDef
 * @note On success, session key Ks is generated and stored at slot #209
 */
HAL_StatusTypeDef hsm_external_auth(bool use_kauth, const uint8_t* signature_vci, bool* auth_success)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_external_auth] start\r\n");
#endif
	if ((signature_vci == NULL) || (auth_success == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	uint8_t mode2 = (use_kauth == true) ? 0x02U : 0x01U;

	/* Finish - Send Sign2, Receive auth result (using stored g_auth_ctx_id) */
	uint16_t mode_finish = HSM_MODE_FINISH | ((uint16_t)mode2 << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, 0U, g_auth_ctx_id, 32U);

	uint8_t rx_finish[4];
	uint16_t rx_len_finish = 0U;

	HAL_StatusTypeDef status = process_host_communication_m2(HSM_OPCODE_MUTUAL_AUTH, mode_finish, param_finish, signature_vci, 32U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);

	if (status != HAL_OK)
	{
		*auth_success = false;
		return status;
	}

	/* Finish 응답 Data에서 검증 결과 확인
	 * Manual 5.17.3: Finish Data = 검증결과 (성공: 0x00, 실패: 0x80) */
	if ((rx_len_finish > 0U) && (rx_finish[0] == 0x00U))
	{
		*auth_success = true;
#if defined(NEW_HSM_LOG_ENABLE)
		GLogN("[EXTERNAL_AUTH] Verification SUCCESS (0x00)\r\n");
#endif
	}
	else
	{
		*auth_success = false;
		GLogE("[EXTERNAL_AUTH] Verification FAILED (0x%02X) - Sign2 mismatch\r\n",
		      (rx_len_finish > 0U) ? rx_finish[0] : 0xFFU);
	}

	return HAL_OK;
}

/**
 * @brief Mutual Authentication (OP 0x21) - Full sequence (Legacy wrapper)
 * @warning This function requires Sign2 (signature_vci) to be known before calling.
 *          Since Sign2 depends on Sign1 from HSM, this creates a chicken-and-egg problem.
 *          For proper step-by-step authentication flow, use:
 *            1. hsm_internal_auth() -> Get CR + Sign1
 *            2. App verifies Sign1, calculates Sign2
 *            3. hsm_external_auth() -> Send Sign2, get result
 */
HAL_StatusTypeDef hsm_mutual_authentication(bool use_kauth, const uint8_t* random_sr, const uint8_t* signature_vci, uint8_t* random_cr,
                                           uint8_t* signature_hsm, uint8_t* key_id, bool* auth_success)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_mutual_authentication] start\r\n");
#endif
	if ((random_sr == NULL) || (signature_vci == NULL) || (random_cr == NULL) || (signature_hsm == NULL) || (key_id == NULL) || (auth_success == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;

	/* Step 1 & 2: Internal Authentication (Start + Update) */
	status = hsm_internal_auth(use_kauth, random_sr, random_cr, signature_hsm, key_id);
	if (status != HAL_OK)
	{
		return status;
	}

	/* Step 3: External Authentication (Finish) - uses g_auth_ctx_id internally */
	status = hsm_external_auth(use_kauth, signature_vci, auth_success);
	if (status != HAL_OK)
	{
		return status;
	}

	return HAL_OK;
}

/**
 * @brief Import Kauth key (OP 0x22) - One-shot
 * @note Key ID param = 0x00 (HSM internally uses slot #103)
 */
HAL_StatusTypeDef hsm_import_kauth_key(const uint8_t* encrypted_kauth, uint16_t kauth_length)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_import_kauth_key] start\r\n");
#endif
	if ((encrypted_kauth == NULL) || (kauth_length == 0U))
	{
		hsm_print_error(AVO_ERR_INVALID_PARAM);
		return HAL_ERROR;
	}

	uint8_t param[LEN_PARAM];
	build_params(param, 0U, 0U, kauth_length);  // Key ID = 0x00 (internal #103)

	uint8_t rx[8];
	uint16_t rx_len = 0U;

	return process_host_communication(HSM_OPCODE_KAUTH_IMPORT, 0U, param, encrypted_kauth, kauth_length, rx, &rx_len, MODE_PLAINTEXT);
}

/**
 * @brief Oneway Authentication (OP 0x23) - Multi-call
 */
HAL_StatusTypeDef hsm_oneway_authentication(const uint8_t* mac_value, uint8_t* random_cr, bool* auth_success)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_oneway_authentication] start\r\n");
#endif
	if ((mac_value == NULL) || (random_cr == NULL) || (auth_success == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint16_t ctx_id = 0U;

	uint16_t mode_start = HSM_MODE_START | (0x00U << 8U);
	uint8_t param_start[LEN_PARAM];
	build_params(param_start, 0U, 0U, 0U);

	uint8_t rx_start[4];
	uint16_t rx_len_start = 0U;

	status = process_host_communication_m2(HSM_OPCODE_ONEWAY_AUTH, mode_start, param_start, NULL, 0U, rx_start, &rx_len_start, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	/* R_CMD response: [Response Code(1)] + [Context ID(2)]
	 * Skip Response Code and read Context ID */
	ctx_id = (uint16_t)rx_start[0] | ((uint16_t)rx_start[1] << 8U);

	uint16_t mode_update = HSM_MODE_UPDATE | (0x00U << 8U);
	uint8_t param_update[LEN_PARAM];
	build_params(param_update, 0U, ctx_id, 0U);

	uint8_t rx_update[20];
	uint16_t rx_len_update = 0U;

	status = process_host_communication_m2(HSM_OPCODE_ONEWAY_AUTH, mode_update, param_update, NULL, 0U, rx_update, &rx_len_update, MODE_PLAINTEXT);
	if (status != HAL_OK)
	{
		return status;
	}

	if (rx_len_update >= 16U)
	{
		/* R_CMD response: [SR(16)] (Response Code excluded by spi_cmd_r_cmd) */
		(void)memcpy(random_cr, &rx_update[0], 16U);
	}

	uint16_t mode_finish = HSM_MODE_AUTH_FINISH | (0x00U << 8U);
	uint8_t param_finish[LEN_PARAM];
	build_params(param_finish, 0U, ctx_id, 16U);

	uint8_t rx_finish[4];
	uint16_t rx_len_finish = 0U;

	status = process_host_communication_m2(HSM_OPCODE_ONEWAY_AUTH, mode_finish, param_finish, mac_value, 16U, rx_finish, &rx_len_finish, MODE_PLAINTEXT);

	/* Auth result: HAL_OK = auth success (0x00), HAL_ERROR = auth failed (0x80) */
	*auth_success = (status == HAL_OK);

	return HAL_OK;
}
uint8_t dfu_data[520] = {0x00,};

/**
 * @brief DFU Start - Initialize firmware upgrade (OP 0x90, Mode 0x00)
 * @param[in] firmware_type  Firmware type (HSM_DFU_TYPE_HSE_FW or HSM_DFU_TYPE_APP_FW)
 * @param[in] firmware_size  Total firmware size in bytes
 * @return HAL_StatusTypeDef  HAL_OK on success, HAL_ERROR on failure
 */
static HAL_StatusTypeDef hsm_dfu_start(uint8_t firmware_type, uint32_t firmware_size)
{
	HAL_StatusTypeDef status = HAL_OK;
	uint16_t mode = 0U;
	uint8_t param[LEN_PARAM] = {0U};
	uint8_t tx_data[4] = {0U};
	uint8_t rx_data[8] = {0U};
	uint16_t rx_len = 0U;

#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_dfu_start] type=%d, size=%lu\r\n", firmware_type, firmware_size);
#endif

	mode = DFU_MODE_START | ((uint16_t)firmware_type << 8U);

	tx_data[0] = (uint8_t)(firmware_size & 0xFFU);
	tx_data[1] = (uint8_t)((firmware_size >> 8U) & 0xFFU);
	tx_data[2] = (uint8_t)((firmware_size >> 16U) & 0xFFU);
	tx_data[3] = (uint8_t)((firmware_size >> 24U) & 0xFFU);

	status = process_host_communication(HSM_OPCODE_DFU, mode, param,
	                                    tx_data, 4U, rx_data, &rx_len, MODE_PLAINTEXT);

	return status;
}

/**
 * @brief DFU Send Chunk - Send a single firmware chunk (OP 0x90, Mode 0x01)
 * @param[in] offset      Current offset in firmware
 * @param[in] chunk_size  Size of this chunk (max 512 bytes)
 * @param[in] chunk_data  Pointer to chunk data
 * @return HAL_StatusTypeDef  HAL_OK on success, HAL_ERROR on failure
 */
static HAL_StatusTypeDef hsm_dfu_send_chunk(uint32_t offset, uint16_t chunk_size, const uint8_t* chunk_data)
{
	HAL_StatusTypeDef status = HAL_OK;
	uint16_t mode = DFU_MODE_UPDATE;
	uint8_t param[LEN_PARAM] = {0U};

	param[0] = (uint8_t)(offset & 0xFFU);
	param[1] = (uint8_t)((offset >> 8U) & 0xFFU);
	param[2] = (uint8_t)((offset >> 16U) & 0xFFU);
	param[3] = (uint8_t)((offset >> 24U) & 0xFFU);
	param[4] = (uint8_t)(chunk_size & 0xFFU);
	param[5] = (uint8_t)((chunk_size >> 8U) & 0xFFU);

	status = process_host_communication_m1_update(HSM_OPCODE_DFU, mode, param, chunk_data, chunk_size, MODE_PLAINTEXT);

	return status;
}

/**
 * @brief DFU Finish - Complete firmware transfer (OP 0x90, Mode 0x02)
 * @return HAL_StatusTypeDef  HAL_OK on success, HAL_ERROR on failure
 */
static HAL_StatusTypeDef hsm_dfu_finish(void)
{
	HAL_StatusTypeDef status = HAL_OK;
	uint16_t mode = DFU_MODE_FINISH;
	uint8_t param[LEN_PARAM] = {0U};
	uint8_t rx_data[8] = {0U};
	uint16_t rx_len = 0U;

#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_dfu_finish] start\r\n");
#endif

	osDelay(DFU_FINISH_DELAY_MS);

	status = process_host_communication(HSM_OPCODE_DFU, mode, param, NULL, 0U, rx_data, &rx_len, MODE_PLAINTEXT);

	return status;
}

/**
 * @brief DFU Run - Execute firmware update (OP 0x90, Mode 0x04)
 * @param[in] firmware_type  Firmware type (HSM_DFU_TYPE_HSE_FW or HSM_DFU_TYPE_APP_FW)
 * @return HAL_StatusTypeDef  HAL_OK on success, HAL_ERROR on failure
 */
static HAL_StatusTypeDef hsm_dfu_run(uint8_t firmware_type)
{
	HAL_StatusTypeDef status = HAL_OK;
	uint16_t mode = 0U;
	uint8_t param[LEN_PARAM] = {0U};
	uint8_t rx_data[8] = {0U};
	uint16_t rx_len = 0U;

#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_dfu_run] type=%d\r\n", firmware_type);
#endif

	mode = DFU_MODE_RUN | ((uint16_t)firmware_type << 8U);
	//g_ucHsmDfuFailTest=1;//for test
	status = process_host_communication(HSM_OPCODE_DFU, mode, param, NULL, 0U, rx_data, &rx_len, MODE_PLAINTEXT);

	return status;
}

/**
 * @brief DFU Update - Transfer firmware chunks and verify completion (OP 0x90, Mode 0x01)
 * @param[in] firmware_type  Firmware type for flash read
 * @param[in] firmware_size  Total firmware size in bytes
 * @return HAL_StatusTypeDef  HAL_OK on success, HAL_ERROR on failure
 */
static HAL_StatusTypeDef hsm_dfu_update(uint8_t firmware_type, uint32_t firmware_size)
{
	HAL_StatusTypeDef status = HAL_OK;
	HAL_StatusTypeDef ret_status = HAL_OK;
	bool bContinue = true;
	bool bLogTxFlag = false;
	bool bLogRxFlag = false;

	uint32_t offset = 0U;
	uint32_t chunk_index = 0U;
	uint16_t chunk_size = 0U;
	uint8_t retry_count = 0U;
	//uint8_t last_progress = 0U;
	//uint8_t current_progress = 0U;

	/* Temporarily disable logs during transfer */
	if(g_bHsmLogTxOn == 1U)
	{
		bLogTxFlag = true;
		g_bHsmLogTxOn = 0U;
	}
	if(g_bHsmLogRxOn == 1U)
	{
		bLogRxFlag = true;
		g_bHsmLogRxOn = 0U;
	}

	while((offset < firmware_size) && (bContinue == true))
	{
		chunk_index++;

		/* Calculate chunk size */
		if((firmware_size - offset) > (uint32_t)DFU_CHUNK_SIZE)
		{
			chunk_size = DFU_CHUNK_SIZE;
		}
		else
		{
			chunk_size = (uint16_t)(firmware_size - offset);
		}

		/* Read file chunk with retry */
		retry_count = 0U;
#if defined(USE_FLASH_HSM_UPDATE)
		while(Send_HSM_File_Flash(dfu_data, chunk_index - 1U, DFU_CHUNK_SIZE, firmware_type) != 0)
#else
		(void)firmware_type;
		while(Send_HSM_File(dfu_data, chunk_index - 1U, DFU_CHUNK_SIZE) != 0)
#endif
		{
			retry_count++;
			if(retry_count > DFU_MAX_RETRY)
			{
				ret_status = HSM_UNKNOWN_ERROR;
				bContinue = false;
				break;
			}
		}

		/* Send chunk to HSM */
		if(bContinue == true)
		{
			status = hsm_dfu_send_chunk(offset, chunk_size, dfu_data);
			if(status != HAL_OK)
			{
				ret_status = status;
				bContinue = false;
			}
			else
			{
				offset += chunk_size;
				GLogN(".");
			}
		}
	}

	/* Restore log flags */
	if(bLogTxFlag == true)
	{
		g_bHsmLogTxOn = 1U;
	}
	if(bLogRxFlag == true)
	{
		g_bHsmLogRxOn = 1U;
	}

	return ret_status;
}

/**
 * @brief Firmware Upgrade (OP 0x90) - High-level wrapper
 * @param[in] firmware_type  Firmware type (HSM_DFU_TYPE_HSE_FW or HSM_DFU_TYPE_APP_FW)
 * @return HAL_StatusTypeDef  HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef hsm_firmware_upgrade(uint8_t firmware_type)
{
	FIL fpHsm;
	FRESULT res;
	HAL_StatusTypeDef status = HAL_OK;
	HAL_StatusTypeDef ret_status = HAL_OK;
	bool bFileOpened = false;
	bool bContinue = true;
	uint32_t firmware_size = 0U;

	pHSMAck->mResult = 0U;
	pHSMAck->mProgress = 0U;
	pHSMAck->mSelftestMode = 0U;

	/* ========== Step 1: Open Firmware File ========== */
#if defined(USE_FLASH_HSM_UPDATE)
	GLogN("USE_FLASH_HSM_UPDATE START\r\n", DIR_APP);
	if(firmware_type == HSM_DFU_TYPE_HSE_FW)
	{
		firmware_size = 184616U;
	}
	else
	{
		firmware_size = 123156U;
	}
	//firmware_size = g_u32firmware_size=133232;
	firmware_size = g_u32firmware_size;
	GLogN("firmware_size : %d \r\n", firmware_size);
#else
	if(firmware_type == HSM_DFU_TYPE_HSE_FW)
	{
		GLogN("HSE update start!! \r\n");
		(void)memcpy(gHSM_Info_Name, "AT_HSM_HSE.bin", (size_t)HSM_FW_NAME_LEN);
	}
	else
	{
		GLogN("HOST update start!! \r\n");
		(void)memcpy(gHSM_Info_Name, "AT_HSM_App.bin", (size_t)HSM_FW_NAME_LEN);
		//(void)memcpy(gHSM_Info_Name, "AT_HSM_App1.bin", 15);//for test
	}

	(void)f_chdir(DIR_ROOT);
	res = f_chdir(DIR_APP);
	if(res == FR_OK)
	{
		GLogN("Change Directory %s Ok\r\n", DIR_APP);
	}

	res = f_open(&fpHsm, gHSM_Info_Name, (BYTE)(FA_OPEN_EXISTING | FA_READ));
	if(res == FR_OK)
	{
		bFileOpened = true;
		firmware_size = (uint32_t)f_size(&fpHsm);
	}
	else
	{
		ret_status = HAL_ERROR;
		bContinue = false;
	}
#endif

	/* Validate firmware type */
	if(bContinue == true)
	{
		if((firmware_type != 0x01U) && (firmware_type != 0x02U))
		{
			hsm_print_error(AVO_ERR_INVALID_PARAM);
			ret_status = HAL_ERROR;
			bContinue = false;
		}
	}

	/* ========== Step 2: DFU Start ========== */
	if(bContinue == true)
	{
		status = hsm_dfu_start(firmware_type, firmware_size);
		if(status != HAL_OK)
		{
			ret_status = status;
			bContinue = false;
		}
	}

	/* ========== Step 3: DFU Update ========== */
	if(bContinue == true)
	{
		status = hsm_dfu_update(firmware_type, firmware_size);
		if(status != HAL_OK)
		{
			ret_status = status;
			bContinue = false;
		}
	}

	/* ========== Step 4: DFU Finish ========== */
	if(bContinue == true)
	{
		status = hsm_dfu_finish();
		if(status != HAL_OK)
		{
			ret_status = status;
			bContinue = false;
		}
	}

	/* ========== Step 5: DFU Run ========== */
	if(bContinue == true)
	{
		status = hsm_dfu_run(firmware_type);
		ret_status = status;
	}

	/* ========== Step 6: Cleanup ========== */
#if !defined(USE_FLASH_HSM_UPDATE)
	if(bFileOpened == true)
	{
		(void)f_close(&fpHsm);
	}
#endif

	/* Set final result */
	if(ret_status == HAL_OK)
	{
		GLogN("[hsm_firmware_upgrade] success!!\r\n");
		pHSMAck->mResult = 0U;
		pHSMAck->mProgress = 100U;
		HSM_Update_Ack(pHSMAck->mResult, pHSMAck->mProgress);
		DeleteHSMFile_AT(firmware_type);
	}
	else
	{
		GLogE("[hsm_firmware_upgrade] fail!!\r\n");
		pHSMAck->mResult = 1U;
		pHSMAck->mProgress = 0U;
		HSM_Update_Ack(pHSMAck->mResult, pHSMAck->mProgress);
	}

#ifdef FW_TEST_VERSION_OVERRIDE
	/* Persist test version regardless of success/fail */
	(void)Save_HSM_TestVersion(HSM_TEST_VERSION_AFTER_UPG);
#endif

	return ret_status;
}

#ifdef FW_TEST_VERSION_OVERRIDE
/* ============================================================
 * HSM Test Version persistence helpers (FW_TEST_VERSION_OVERRIDE 전용)
 *  - File: /01_Application/HSM_TEST_VER.bin (2-byte little-endian U16)
 * ============================================================ */
uint16_t Load_HSM_TestVersion(void)
{
	FIL      fpVer;
	FRESULT  res;
	UINT     uiBytesRead = 0U;
	uint16_t version     = HSM_TEST_VERSION_DEFAULT;

	(void)f_chdir(DIR_ROOT);
	res = f_chdir(DIR_APP);
	if(res != FR_OK)
	{
		GLogE("[Load_HSM_TestVersion] chdir fail (%d), use default %u\r\n", res, version);
		return version;
	}

	res = f_open(&fpVer, HSM_TEST_VERSION_FILE, (BYTE)(FA_OPEN_EXISTING | FA_READ));
	if(res != FR_OK)
	{
		GLogN("[Load_HSM_TestVersion] file not found, use default %u\r\n", version);
		return version;
	}

	res = f_read(&fpVer, &version, (UINT)sizeof(version), &uiBytesRead);
	(void)f_close(&fpVer);

	if((res != FR_OK) || (uiBytesRead != sizeof(version)))
	{
		GLogE("[Load_HSM_TestVersion] read fail (res=%d, n=%u), use default\r\n",
		      res, (unsigned)uiBytesRead);
		version = HSM_TEST_VERSION_DEFAULT;
	}

	GLogN("[Load_HSM_TestVersion] loaded = %u\r\n", version);
	return version;
}

HAL_StatusTypeDef Save_HSM_TestVersion(uint16_t version)
{
	FIL      fpVer;
	FRESULT  res;
	UINT     uiBytesWritten = 0U;

	(void)f_chdir(DIR_ROOT);
	res = f_chdir(DIR_APP);
	if(res != FR_OK)
	{
		GLogE("[Save_HSM_TestVersion] chdir fail (%d)\r\n", res);
		return HAL_ERROR;
	}

	res = f_open(&fpVer, HSM_TEST_VERSION_FILE, (BYTE)(FA_CREATE_ALWAYS | FA_WRITE));
	if(res != FR_OK)
	{
		GLogE("[Save_HSM_TestVersion] open fail (%d)\r\n", res);
		return HAL_ERROR;
	}

	res = f_write(&fpVer, &version, (UINT)sizeof(version), &uiBytesWritten);
	(void)f_close(&fpVer);

	if((res != FR_OK) || (uiBytesWritten != sizeof(version)))
	{
		GLogE("[Save_HSM_TestVersion] write fail (res=%d, n=%u)\r\n",
		      res, (unsigned)uiBytesWritten);
		return HAL_ERROR;
	}

	GLogN("[Save_HSM_TestVersion] saved = %u\r\n", version);
	return HAL_OK;
}
#endif /* FW_TEST_VERSION_OVERRIDE */

HAL_StatusTypeDef hsm_set_complete(void)
{
//#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_set_complete] start\r\n");
//#endif
	return exec_simple_cmd(HSM_OPCODE_SET_COMPLETE);
}

#define USE_OLD_HSM_SEED_SIGN  1  /* 1: OLD HSM compat (padding), 0: NEW HSM (doc 5.13.1) */

#if USE_OLD_HSM_SEED_SIGN  /* OLD HSM Compatible version */
/**
 * @brief RSA Sign with Seed using SEED_PADDING - OLD HSM SignPrivateHSMSHA256 compatible
 * @details This function mimics OLD HSM's SignPrivateHSMSHA256 behavior for SHA256:
 *          1. Calculate SHA256 hash of seed
 *          2. Create 255-byte PKCS#1 v1.5 padded data (SEED_PADDING_SHA256)
 *          3. Place hash at offset 223
 *          4. Calculate SHA256 hash of the 255-byte padded data
 *          5. RSA Sign with the hash
 */
HAL_StatusTypeDef hsm_sign_rsa_with_seed(const uint8_t* seed, uint16_t seed_len,
                                         uint8_t* signature, uint32_t* sign_len,
                                         uint8_t sha_type, uint16_t key_id)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_sign_rsa_with_seed] start\r\n");
#endif
	if ((seed == NULL) || (signature == NULL) || (sign_len == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;

	/* SEED_PADDING_SHA256 structure (255 bytes) - PKCS#1 v1.5 format for SHA256
	 * [0]: 0x01
	 * [1-202]: 0xFF (202 bytes)
	 * [203]: 0x00
	 * [204-222]: SHA256 DigestInfo (19 bytes)
	 * [223-254]: Hash position (32 bytes) - SHA256 hash goes here
	 */
	static const uint8_t SEED_PADDING_SHA256[255] = {
		0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
		/* SHA256 DigestInfo (19 bytes): 30 31 30 0D 06 09 60 86 48 01 65 03 04 02 01 05 00 04 20 */
		0x30, 0x31, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20,
		/* Hash position (32 bytes) - filled with SHA256 hash */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	uint8_t seed_hash[32];
	uint8_t padded_data[255];
	uint8_t final_hash[32];

	/* Step 1: Calculate SHA256 hash of seed (same as OLD HSM first step) */
	status = hsm_calculate_sha(HSM_SHA_256, false, 0, seed, seed_len, seed_hash);
	if (status != HAL_OK)
	{
#if defined(NEW_HSM_LOG_ENABLE)	
		GLogE("hsm_sign_rsa_with_seed: SHA256 seed hash failed\r\n");
#endif
		return status;
	}

	/* Step 2: Create padded data (SEED_PADDING_SHA256 + hash at offset 223) */
	(void)memcpy(padded_data, SEED_PADDING_SHA256, 255);
	(void)memcpy(&padded_data[223], seed_hash, 32);

	/* Step 3: Calculate SHA256 hash of padded_data (255 bytes) */
	status = hsm_calculate_sha(HSM_SHA_256, false, 0, padded_data, 255, final_hash);
	if (status != HAL_OK)
	{
		GLogE("hsm_sign_rsa_with_seed: SHA256 padded hash failed\r\n");
		return status;
	}

	/* Step 4: RSA Sign with the final hash */
	status = hsm_sign_rsa(key_id, HSM_SHA_256, final_hash, signature);
	if (status != HAL_OK)
	{
		GLogE("hsm_sign_rsa_with_seed: RSA sign failed\r\n");
		return status;
	}

	*sign_len = 256U;  /* RSA-2048 signature size */
	return HAL_OK;
}
#else  /* NEW HSM Document compliant version (5.13.1) */
/**
 * @brief RSA Sign with Seed - Document compliant (5.13.1)
 * @details Per HSM SPI Manual 5.13.1:
 *          - RSA Sign (0x14) takes Digest (hash) only
 *          - Mode2=0x02 (RSASSA-PKCS1-v1_5_SHA-256) means HSM handles PKCS#1 v1.5 padding internally
 *          Flow:
 *          1. Calculate SHA hash of seed
 *          2. RSA Sign with hash (HSM applies PKCS#1 v1.5 padding internally)
 */
HAL_StatusTypeDef hsm_sign_rsa_with_seed(const uint8_t* seed, uint16_t seed_len,
                                         uint8_t* signature, uint32_t* sign_len,
                                         uint8_t sha_type, uint16_t key_id)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_sign_rsa_with_seed] start\r\n");
#endif
	if ((seed == NULL) || (signature == NULL) || (sign_len == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;
	uint8_t seed_hash[32];  /* Max size for SHA256 */

	/* Step 1: Calculate SHA hash of seed */
	status = hsm_calculate_sha(sha_type, false, 0, seed, seed_len, seed_hash);
	if (status != HAL_OK)
	{
		GLogE("hsm_sign_rsa_with_seed: SHA hash failed\r\n");
		return status;
	}

	/* Step 2: RSA Sign with hash (HSM handles PKCS#1 v1.5 padding internally per 5.13.1) */
	status = hsm_sign_rsa(key_id, sha_type, seed_hash, signature);
	if (status != HAL_OK)
	{
		GLogE("hsm_sign_rsa_with_seed: RSA sign failed\r\n");
		return status;
	}

	*sign_len = 256U;  /* RSA-2048 signature size */
	return HAL_OK;
}
#endif  /* USE_OLD_HSM_SEED_SIGN */


/**
 * @brief Sign seed with RSA using SEED_PADDING - OLD HSM SignPrivateHSM compatible
 * @details This function mimics OLD HSM's SignPrivateHSM behavior for SHA1:
 *          1. Create 255-byte PKCS#1 v1.5 padded data (SEED_PADDING)
 *          2. Place seed at offset 235 (hash position)
 *          3. Calculate SHA1 hash of the 255-byte padded data
 *          4. RSA Sign with the hash
 *          This matches OLD HSM's non-standard SHA1 signing behavior.
 * @param seed: Pointer to seed data (typically 8 bytes)
 * @param seed_len: Length of seed (typically 8)
 * @param signature: Pointer to store signature (256 bytes)
 * @param sign_len: Pointer to store signature length
 * @param sha_type: SHA type (HSM_SHA_160 only - SHA1 specific function)
 * @param key_id: RSA key slot ID
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_sign_rsa_with_seed_direct(const uint8_t* seed, uint16_t seed_len,
                                                uint8_t* signature, uint32_t* sign_len,
                                                uint8_t sha_type, uint16_t key_id)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_sign_rsa_with_seed_direct] start\r\n");
#endif
	if ((seed == NULL) || (signature == NULL) || (sign_len == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;

	/* SEED_PADDING structure (255 bytes) - PKCS#1 v1.5 format for SHA1
	 * [0]: 0x01
	 * [1-218]: 0xFF (218 bytes)
	 * [219]: 0x00
	 * [220-234]: SHA1 DigestInfo (15 bytes)
	 * [235-254]: Hash position (20 bytes) - Seed goes here
	 */
	static const uint8_t SEED_PADDING[255] = {
		0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
		/* SHA1 DigestInfo (15 bytes): 30 21 30 09 06 05 2b 0e 03 02 1a 05 00 04 14 */
		0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14,
		/* Hash position (20 bytes) - filled with seed + zero padding */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	uint8_t padded_data[255];
	uint8_t sha1_hash[20];

	/* Step 1: Create padded data (SEED_PADDING + seed at offset 235) */
	(void)memcpy(padded_data, SEED_PADDING, 255);
	uint16_t copy_len = (seed_len > 20U) ? 20U : seed_len;
	(void)memcpy(&padded_data[235], seed, copy_len);

	/* Step 2: Calculate SHA1 hash of padded_data (255 bytes) */
	status = hsm_calculate_sha(HSM_SHA_160, false, 0, padded_data, 255, sha1_hash);
	if (status != HAL_OK)
	{
		GLogE("hsm_sign_rsa_with_seed_direct: SHA1 hash failed\r\n");
		return status;
	}

	/* Step 3: RSA Sign with the SHA1 hash */
	status = hsm_sign_rsa(key_id, HSM_SHA_160, sha1_hash, signature);
	if (status != HAL_OK)
	{
		GLogE("hsm_sign_rsa_with_seed_direct: RSA sign failed\r\n");
		return status;
	}

	*sign_len = 256U;  /* RSA-2048 signature size */
	return HAL_OK;
}

/**
 * @brief Sign with Seed using WBC/OLD HSM compatible method (SignPrivateHSM)
 * @details This function replicates OLD HSM's SignPrivateHSM behavior using SIGN command.
 *
 *          KEY INSIGHT: SIGN[0x14] internally creates PKCS#1 v1.5 padding:
 *
 *          SHA-1 mode:   [0x00][0x01][FF×218][0x00][DigestInfo:15][hash:20]
 *          SHA-256 mode: [0x00][0x01][FF×202][0x00][DigestInfo:19][hash:32]
 *
 *          By passing seed+zeros as "hash", we achieve WBC compatibility.
 *          The seed is placed at the start of the hash position.
 *
 * @param seed: Pointer to seed data (typically 8 bytes)
 * @param seed_len: Length of seed (max hash_len bytes)
 * @param signature: Output buffer for signature (256 bytes)
 * @param sign_len: Pointer to store signature length
 * @param key_id: RSA key slot ID
 * @param sha_type: HSM_SHA_160 (SHA-1, 20B hash) or HSM_SHA_256 (32B hash)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_sign_rsa_with_seed_new(const uint8_t* seed, uint16_t seed_len,
                                              uint8_t* signature, uint32_t* sign_len,
                                              uint16_t key_id, uint8_t sha_type)
{
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_sign_rsa_with_seed_new] start, key_id=%d, sha_type=%d\r\n", key_id, sha_type);
#endif
	if ((seed == NULL) || (signature == NULL) || (sign_len == NULL))
	{
		hsm_print_error(AVO_ERR_NULL_POINTER);
		return HAL_ERROR;
	}

	/* Determine hash length based on SHA type */
	uint8_t hash_len;
	const char* sha_name;
	if (sha_type == HSM_SHA_160)
	{
		hash_len = 20U;
		sha_name = "SHA-1";
	}
	else if (sha_type == HSM_SHA_256)
	{
		hash_len = 32U;
		sha_name = "SHA-256";
	}
	else
	{
		GLogE("hsm_sign_rsa_with_seed_new: unsupported sha_type %d\r\n", sha_type);
		return HAL_ERROR;
	}

	if (seed_len > hash_len)
	{
		GLogE("hsm_sign_rsa_with_seed_new: seed too long (max %d for %s)\r\n", hash_len, sha_name);
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status;

	/* Create hash for SIGN command based on OLD HSM behavior:
	 * SHA-1:   fake_hash = [seed:N][0x00:(20-N)] - NO hash calculation
	 * SHA-256: fake_hash = SHA256(seed) - actual hash calculation
	 *
	 * SIGN internally creates PKCS#1 v1.5 padding with DigestInfo
	 * and places our hash at the hash position.
	 */
	uint8_t fake_hash[32];  /* Max size for SHA-256 */
	(void)memset(fake_hash, 0, sizeof(fake_hash));

	if (sha_type == HSM_SHA_160)
	{
		/* SHA-1: Direct seed placement (no hash) - matches OLD HSM SignPrivateHSM() */
		(void)memcpy(fake_hash, seed, seed_len);
#if defined(NEW_HSM_LOG_ENABLE)
		GLogN("SHA-1 Mode: Direct seed placement (no hash)\r\n");
#endif
	}
	else
	{
		/* SHA-256: Calculate SHA256(seed) - matches OLD HSM SignPrivateHSMSHA256() */
		status = hsm_calculate_sha(HSM_SHA_256, false, 0, seed, seed_len, fake_hash);
		if (status != HAL_OK)
		{
			GLogE("hsm_sign_rsa_with_seed_new: SHA256 calculation failed\r\n");
			return status;
		}
#if defined(NEW_HSM_LOG_ENABLE)
		GLogN("SHA-256 Mode: SHA256(seed) calculated\r\n");
#endif
	}
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("WBC Method: Using SIGN command with %s\r\n", sha_name);
	GLogN("Hash for SIGN (%d bytes): ", hash_len);
	for (int i = 0; i < hash_len; i++)
	{
		GLogN("%02X ", fake_hash[i]);
	}
	GLogN("\r\n");
#endif
	/* Call SIGN with specified SHA mode
	 * HSM internally creates PKCS#1 v1.5 padding with appropriate DigestInfo
	 * and places our fake_hash at the hash position */
	status = hsm_sign_rsa(key_id, sha_type, fake_hash, signature);
	if (status != HAL_OK)
	{
		GLogE("hsm_sign_rsa_with_seed_new: SIGN failed\r\n");
		return status;
	}

	*sign_len = 256U;
#if defined(NEW_HSM_LOG_ENABLE)
	GLogN("[hsm_sign_rsa_with_seed_new] success with %s\r\n", sha_name);
#endif
	return HAL_OK;
}
