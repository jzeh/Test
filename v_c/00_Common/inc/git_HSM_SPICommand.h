#ifndef INC_SPI_COMMAND_H_
#define INC_SPI_COMMAND_H_

#include "stm32h7xx_hal.h"
#include <stdint.h>

#define MAX_SPI_PACKET_SIZE				1039 * 2 + 30// Maximum size of a Full Packet
#define MAX_DATA_PAYLOAD_SIZE			1024 // Maximum size of the Data field
#define SPI_DMA_TIMEOUT_MS				1000
#define HSM_RESPONSE_WINDOW_SIZE    	10
#define HSM_RX_SEARCH_WINDOW			30  // Max bytes to search for valid RX response start (HSM can delay up to 24 bytes)
// --- Security Mode Enum ---
typedef enum {
    MODE_PLAINTEXT = 0,
    MODE_SECURE    = 1
} HSM_SecurityMode_t;

// --- SPI Operation Codes ---
#define OPCODE_R_CS						0x01 // Read Command Status
#define OPCODE_W_CC						0x02 // Write Clear Command
#define OPCODE_R_CMD					0x03 // Read Command
#define OPCODE_W_CMD					0x04 // Write Command

// --- Host Status Codes (from R_CS Response) - Big Endian Format ---
#define HOST_STATUS_READY				0x0B00
#define HOST_STATUS_WAIT				0x0B01
#define HOST_STATUS_DONE				0x0B02
#define HOST_STATUS_ERROR				0x0B03

// --- Host Response Codes (ACK/NAK) - Big Endian Format ---
#define HOST_RESPONSE_ACK				0x0B11
#define HOST_RESPONSE_NAK				0x0B22

// --- Field Lengths ---
#define LEN_TRANSACTION_CODE			2
#define LEN_LENGTH						2
#define LEN_OP_CODE_PARAM				1
#define LEN_MODE						2
#define LEN_PARAM						6
#define LEN_CRC							2

// --- W_CMD OP Codes (Sub-commands) ---
#define HSM_OPCODE_INFO					0x00 // Get HSM version and certificate info
#define HSM_OPCODE_GET_HSN				0x01 // Get HSM Serial Number (UID 8 bytes)
#define HSM_OPCODE_CANCEL_JOB			0x02 // Cancel current job
#define HSM_OPCODE_GET_ERROR			0x0B // Get last error code (when AVO_RET_NOK received)
#define HSM_OPCODE_KEY_MNG				0x05 // Key Management (Import/Export/Generate)
#define HSM_OPCODE_CERT_MNG				0x06 // Certificate Management (Store/Read)
#define HSM_OPCODE_KEY_EXCHANGE			0x07 // Key Exchange (ECDH)
#define HSM_OPCODE_RANDOM				0x10 // Random number generation (TRNG/PRNG)
#define HSM_OPCODE_SHA					0x11 // SHA hashing (SHA1/SHA256/SHA512)
#define HSM_OPCODE_AES					0x12 // AES encryption/decryption
#define HSM_OPCODE_MAC					0x13 // MAC generation/verification (CMAC/HMAC)
#define HSM_OPCODE_SIGN_AND_VERIFY		0x14 // Digital signature and verification
#define HSM_OPCODE_RSAES				0x15 // RSAES encryption/decryption
#define HSM_OPCODE_KDF					0x16 // Key Derivation Function (HKDF)
#define HSM_OPCODE_ASK_MNG				0x20 // Advanced Seed Key management
#define HSM_OPCODE_MUTUAL_AUTH			0x21 // Mutual authentication
#define HSM_OPCODE_KAUTH_IMPORT			0x22 // Kauth key import
#define HSM_OPCODE_ONEWAY_AUTH			0x23 // One-way authentication
#define HSM_OPCODE_DFU					0x90 // Device Firmware Upgrade
#define HSM_OPCODE_SET_COMPLETE			0x91 // Life cycle transition (IN_FIELD → ACTIVE)

// --- Response Code ---
#define AVO_RET_OK						0x00
#define AVO_RET_NOK						0x80
#define AVO_RET_WAIT					0x81

// --- Mode Definitions for Multi-call APIs (Start/Update/Finish) ---
#define HSM_MODE_START					0x00 // Start operation (creates context)
#define HSM_MODE_UPDATE					0x01 // Update operation (continue processing)
#define HSM_MODE_FINISH					0x02 // Finish operation (get final result)

// --- Key Management Mode (OP Code 0x05) ---
#define HSM_KEYMNG_IMPORT				0x01 // Import key
#define HSM_KEYMNG_EXPORT				0x02 // Export key (public key only)
#define HSM_KEYMNG_GENERATE				0x03 // Generate key pair

// --- Algorithm Type Definitions ---
#define HSM_ALG_AES						0x01 // AES algorithm
#define HSM_ALG_ECC						0x02 // ECC algorithm
#define HSM_ALG_RSA						0x03 // RSA algorithm
#define HSM_ALG_HMAC					0x04 // HMAC algorithm
#define HSM_ALG_ED						0x05 // EdDSA algorithm
#define HSM_ALG_CUSTOM					0xA0 // KD SHA256 algorithm

// --- Special Key Type (v5: Mode2 Bit2-5) ---
#define HSM_SPECIAL_KEY_ECU_CODE		0x09 // b1001: ECU-Code Key (#101) - AES
#define HSM_SPECIAL_KEY_KM				0x0A // b1010: Km (#102) - HMAC
#define HSM_SPECIAL_KEY_KAUTH			0x0B // b1011: Kauth (#103) - HMAC
#define HSM_SPECIAL_KEY_RSA_KEK			0x0C // b1100: RSA-KEK (#149) - RSA ⚠️ v5에서 #149 결번처리됨!

// --- AES Key Size ---
#define HSM_AES_128						0x01 // AES-128
#define HSM_AES_192						0x02 // AES-192
#define HSM_AES_256						0x03 // AES-256

// --- RSA Key Size ---
#define HSM_RSA_1024					0x01 // RSA1024
#define HSM_RSA_2048					0x02 // RSA2048

// --- AES Mode of Operation ---
#define HSM_AES_ECB						0x00 // Electronic Codebook
#define HSM_AES_CBC						0x01 // Cipher Block Chaining
#define HSM_AES_CTR						0x02 // Counter mode
#define HSM_AES_OFB						0x03 // Output Feedback

// --- SHA Algorithm Type (Mode2 Bit0-1) ---
#define HSM_SHA_160						0x00 // b00: SHA-160 (SHA-1)
#define HSM_SHA_256						0x01 // b01: SHA-256
#define HSM_SHA_512						0x03 // b11: SHA-512 (v5 추가)

// --- RNG Mode ---
#define HSM_RNG_TRNG					0x00 // True Random Number Generator
#define HSM_RNG_PRNG					0x01 // Pseudo Random Number Generator

// --- DFU Mode ---
#define HSM_DFU_START_DOWNLOAD			0x00 // Start firmware download
#define HSM_DFU_UPDATE_DOWNLOAD			0x01 // Update firmware download (send data chunks)
#define HSM_DFU_FINISH_DOWNLOAD			0x02 // Finish firmware download
#define HSM_DFU_RUN						0x04 // Run DFU (apply firmware)

// --- DFU Type ---
#define HSM_DFU_TYPE_HSE_FW				0x01 // HSE Firmware
#define HSM_DFU_TYPE_APP_FW				0x02 // Application Firmware

// --- MAC Mode Constants ---
// Fixed: mode = mode1 | (mode2 << 8), Mode_1=low byte, Mode_2=high byte
// HMAC: Mode_1 Bit7=0, Mode_2 Bit6-7=01(SHA256)=0x40
#define HSM_MODE_HMAC_SHA256_START		0x4000 // HMAC-SHA256 Start (Mode_1=0x00, Mode_2=0x40)
#define HSM_MODE_HMAC_SHA256_UPDATE		0x4001 // HMAC-SHA256 Update (Mode_1=0x01, Mode_2=0x40)
#define HSM_MODE_HMAC_SHA256_FINISH		0x4002 // HMAC-SHA256 Finish (Mode_1=0x02, Mode_2=0x40)

// CMAC: Mode_1 Bit7=1(0x80), Mode_2=AES key size (OR separately)
#define HSM_MODE_CMAC_AES_BASE			0x0080 // CMAC Base (Mode_1=0x80), OR with (aes_size << 8)
#define HSM_MODE_CMAC_AES_START_BASE	0x0080 // CMAC Start base (Mode_1=0x80)
#define HSM_MODE_CMAC_AES_UPDATE_BASE	0x0081 // CMAC Update base (Mode_1=0x81)
#define HSM_MODE_CMAC_AES_FINISH_BASE	0x0082 // CMAC Finish base (Mode_1=0x82)

// --- Authentication Mode Constants ---
#define HSM_MODE_AUTH_FINISH			0x03 // Finish mode for Mutual/Oneway Auth (b11)

// --- Detailed Error Codes (v3 - 2025.11.14) ---
// Response Code
#define AVO_RET_OK                      0x00 // Success
#define AVO_RET_NOK                     0x80 // Internal Error (Get Error for details)

// General Error
#define AVO_ERR_NO_ERROR                0x00 // No Error

// HSM Operation Error
#define AVO_ERR_SECURE_BOOT_FAIL        0x10 // Host Secure Boot Fail
#define AVO_INVALID_SESSION_KEY_GEN     0x11 // Session key generation failed
#define AVO_ENC_FAIL                    0x12 // Encryption failed
#define AVO_DEC_FAIL                    0x13 // Decryption failed

// Type Check Error
#define AVO_ERR_NULL_POINTER            0x20 // Null pointer passed in arguments
#define AVO_ERR_INVALID_PARAM           0x21 // Invalid parameter delivered
#define AVO_ERR_OUT_OF_RANGE            0x22 // Parameter Out of Range
#define AVO_ERR_INVALID_LENGTH          0x23 // Invalid data length

// SPI Code Dispatcher Error
#define AVO_ERR_BAD_STATE               0x30 // Bad State
#define AVO_ERR_UNKNOWN_SPICODE         0x31 // Undefined SPI Code
#define AVO_ERR_BAD_MAGIC_NUMER         0x32 // Magic Number error
#define AVO_ERR_BAD_FLAG                0x33 // Invalid request buffer access
#define AVO_ERR_GEN_RESP_DATA_FAIL      0x34 // Response message generation error

// OP-Code Dispatcher Error
#define AVO_ERR_UNKNOWN_OPCODE          0x40 // Undefined OP Code
#define AVO_ERR_CMD_LENGTH_ERROR        0x41 // CMD Packet length error
#define AVO_ERR_EXEC_ERROR              0x42 // CMD execution error
#define AVO_ERR_CRC_VERIFY_FAIL         0x43 // CRC verification failed

// Crypto Dispatcher Error
#define AVO_ERR_BAD_CTX_ID              0x50 // Bad CTX ID (not from start mode)
#define AVO_ERR_UNKNOWN_KEY_ID          0x51 // Unidentified Key ID
#define AVO_ERR_BAD_MODE                0x52 // Incorrect Mode used
#define AVO_ERR_UNSUPPORT_ALG_TYPE      0x53 // Unsupported encryption algorithm requested
#define AVO_ERR_KEY_INSERT_FAIL         0x54 // HSE Key injection failed
#define AVO_ERR_KEY_EMPTY               0x55 // Key is empty
#define AVO_ERR_INVALID_KEY_ID          0x56 // Invalid Key ID or algorithm type

// Flash Read/Write Error
#define AVO_ERR_INVALID_SECTOR          0x60 // Invalid Sector ID
#define AVO_ERR_FLASH_READ_FAIL         0x61 // Flash Read failed
#define AVO_ERR_FLASH_WRITE_FAIL        0x62 // Flash Write failed
#define AVO_ERR_FLASH_ERASE_FAIL        0x63 // Flash Erase failed
#define AVO_ERR_FLASH_LOCK_FAIL         0x64 // Flash Lock failed

// DFU Dispatcher Error
#define AVO_ERR_DISCONTINUOUS_DATA      0x70 // Firmware data not continuous
#define AVO_ERR_INVALID_DATA            0x71 // Downloaded firmware data invalid
#define AVO_ERR_FAIL_SET_FIRMWARE       0x72 // Firmware update setting error
#define AVO_ERR_LEGACY_VERSION          0x73 // Firmware version invalid (same or older)
#define AVO_ERR_INVALID_DFU_TYPE        0x74 // Invalid DFU Type

// Legacy (deprecated - use AVO_ERR_INVALID_SECTOR instead)
#define AVO_ERR_SPI_DATA_MISMATCH       0x60 // SPI data mismatch (deprecated)

/*
 * =================================================================
 * Structure Definitions
 * =================================================================
 */

#pragma pack(push, 1) // Prevent byte padding

// Master -> Host Request Packet (Full Packet)
typedef struct {
    uint16_t transaction_code; // 0x5A | Opcode
    uint16_t length;           // Total length of fields: Length, Opcode, Mode, Param, Data, CRC
    uint8_t  op_code_param;    // The specific sub-command opcode within a W_CMD
    uint16_t mode;
    uint8_t  param[LEN_PARAM]; // Use defined constant
    uint8_t  data[MAX_DATA_PAYLOAD_SIZE];
    // CRC is dynamically appended after the data
} W_CMD_Packet_t;

#pragma pack(pop)

extern uint8_t g_spi1_dma_completed;
extern uint32_t error_i;
/*
 * =================================================================
 * Function Prototypes
 * =================================================================
 */

/**
 * @brief R_CS [0x01]: Queries the current status of the Host.
 * @param out_status: Pointer to store the queried status code (e.g., HOST_STATUS_READY).
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef spi_cmd_r_cs(uint16_t* out_status);

/**
 * @brief W_CC [0x02]: Transmits a Clear command to the Host.
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef spi_cmd_w_cc(void);

/**
 * @brief W_CMD [0x03]: Transmits a command to write data to the Host.
 * @param w_cmd_opcode: The specific sub-command opcode for the W_CMD packet.
 * @param w_cmd_mode: The Mode for the W_CMD packet.
 * @param w_cmd_param: The Param (6 bytes) for the W_CMD packet.
 * @param write_data: Buffer containing the data to be written to the Host.
 * @param write_len: Length of the data to be written.
 * @param mode: The security mode for this transaction (MODE_PLAINTEXT or MODE_SECURE).
 * @return HAL_StatusTypeDef (Verifies Host's ACK response on success).
 */
HAL_StatusTypeDef spi_cmd_w_cmd( uint8_t w_cmd_opcode,
								 uint16_t w_cmd_mode,
								 const uint8_t* w_cmd_param,
								 const uint8_t* write_data,
								 uint16_t write_len,
								 HSM_SecurityMode_t mode);

/**
 * @brief W_CMD with 5-byte OP-Code packet: Transmits simple commands (Info, Get HSN, Cancel, Get Error).
 * @note Manual Table 4: "5바이트로 구성된 OP-Code Packet은 secure Mode를 지원하지 않는다"
 * @param opcode: The OP-Code (0x00=Info, 0x01=Get HSN, 0x02=Cancel, 0x0B=Get Error)
 * @return HAL_StatusTypeDef (Verifies Host's ACK response on success).
 */
HAL_StatusTypeDef spi_cmd_w_cmd_5byte(uint8_t opcode);

/**
 * @brief R_CMD [0x04]: Reads the result data from the Host.
 * @param read_buffer: Buffer to store the read data.
 * @param buffer_size: Maximum size of the read_buffer.
 * @param read_len_out: Pointer to store the actual length of the data read.
 * @param mode: The security mode for this transaction (MODE_PLAINTEXT or MODE_SECURE).
 * @return HAL_StatusTypeDef: HAL_OK on success, HAL_BUSY if host is busy, HAL_ERROR on failure.
 */
HAL_StatusTypeDef spi_cmd_r_cmd( uint8_t* read_buffer,
								 uint16_t buffer_size,
								 uint16_t* read_len_out,
								 HSM_SecurityMode_t mode);

/**
 * @brief Wait for HSM to reach a specific status (READY, DONE, etc.)
 * @param target_status: Target status to wait for (e.g., HOST_STATUS_DONE)
 * @param timeout: Timeout in milliseconds
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef wait_for_host_status(uint16_t target_status, uint32_t timeout);

/**
 * @brief [Example] Executes the full communication sequence as per the initial request.
 */
HAL_StatusTypeDef process_host_communication( uint8_t w_cmd_opcode,
											  uint16_t w_cmd_mode,
											  const uint8_t* w_cmd_param,
											  const uint8_t* write_data,
											  uint16_t write_len,
											  uint8_t* read_buffer,
											  uint16_t* read_len_out,
											  HSM_SecurityMode_t mode);

/**
 * @brief M1 Update communication (Update returns READY immediately).
 * @note Use for M1 Update operations that accumulate data and return READY immediately.
 *       Skips DONE wait and R_CMD.
 *       Used by: Key Exchange, SHA, MAC, DFU.
 *       M2 Update operations should use process_host_communication_m2() instead.
 */
HAL_StatusTypeDef process_host_communication_m1_update( uint8_t w_cmd_opcode,
														uint16_t w_cmd_mode,
														const uint8_t* w_cmd_param,
														const uint8_t* write_data,
														uint16_t write_len,
														HSM_SecurityMode_t mode);
HAL_StatusTypeDef process_host_communication_m1_update_with_rcmd( uint8_t w_cmd_opcode,
														uint16_t w_cmd_mode,
														const uint8_t* w_cmd_param,
														const uint8_t* write_data,
														uint16_t write_len,
														uint8_t*			read_buffer,
											  			uint16_t*			read_len_out,
														HSM_SecurityMode_t mode);

/**
 * @brief M2 Multi-call communication (all steps wait for DONE).
 * @note Use for M2 operations where Start/Update/Finish all wait for DONE status.
 *       Used by: CERT_MNG, AES, SIGN_AND_VERIFY, RSAES, MUTUAL_AUTH, ONEWAY_AUTH.
 *       Internally calls process_host_communication().
 */
#define process_host_communication_m2 process_host_communication

/**
 * @brief Prints a human-readable string for a given HSM error code.
 * @param error_code: The error code to print (e.g., AVO_ERR_INVALID_PARAM).
 */
void hsm_print_error(uint8_t error_code);
void generate_prbs14(uint8_t* buffer, uint16_t length, uint16_t* p_seed);
#endif /* INC_SPI_COMMAND_H_ */