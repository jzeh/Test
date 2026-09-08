#ifndef GIT_HSM_API_H_
#define GIT_HSM_API_H_

#include "git_HSM_SPICommand.h"
#include <stdint.h>
#include <stdbool.h>

/*
 * =================================================================
 * NEW HSM Key Slot Definitions (SPI HSM Manual v3)
 * =================================================================
 */
//#define NEW_HSM_LOG_ENABLE

/* UDK (User Defined Key) - AES #101-108 */
#define HSM_KEY_ECU_CODE                101     /* ECU-Code key */
#define HSM_KEY_KM                      102     /* Km - Master Key for mutual auth */
#define HSM_KEY_KAUTH                   103     /* Kauth - Authentication Key */
#define HSM_KEY_AES_104                 104
#define HSM_KEY_AES_105                 105
#define HSM_KEY_AES_106                 106
#define HSM_KEY_AES_107                 107
#define HSM_KEY_AES_108                 108

/* UDK - HMAC #121-124 */
#define HSM_KEY_HMAC_121                121
#define HSM_KEY_HMAC_122                122
#define HSM_KEY_HMAC_123                123
#define HSM_KEY_HMAC_124                124

/* UDK - RSA #141-150 */
#define HSM_KEY_RSA_CERT_141            141     /* RSA cert private key */
#define HSM_KEY_RSA_CERT_142            142
#define HSM_KEY_RSA_CERT_143            143
#define HSM_KEY_RSA_CERT_144            144
#define HSM_KEY_RSA_CERT_145            145
#define HSM_KEY_RSA_CERT_146            146
#define HSM_KEY_RSA_CERT_147            147
#define HSM_KEY_RSA_CERT_148            148
#define HSM_KEY_KEK_RSA                 149     /* RSA key pair for key enc/dec */
#define HSM_KEY_RSA_150                 150

/* CSAC Key Slot Mapping (Old HSM key_num → New HSM key_id) */
#define HSM_KEY_CSAC_PA_SHA1            HSM_KEY_RSA_CERT_141  /* Passenger SHA1 */
#define HSM_KEY_CSAC_PA_SHA2            HSM_KEY_RSA_CERT_142  /* Passenger SHA256 */
#define HSM_KEY_CSAC_CO_SHA1            HSM_KEY_RSA_CERT_143  /* Commercial SHA1 */
#define HSM_KEY_CSAC_CO_SHA2            HSM_KEY_RSA_CERT_144  /* Commercial SHA256 */

/* TEMP - Session Key (RAM) */
#define HSM_KEY_SESSION                 209     /* Ks - Session Key (generated after mutual auth) */

/* TEMP - AES #201-204 */
#define HSM_KEY_TEMP_AES_201            201
#define HSM_KEY_TEMP_AES_202            202
#define HSM_KEY_TEMP_AES_203            203
#define HSM_KEY_TEMP_AES_204            204

/* TEMP - HMAC #221-223 */
#define HSM_KEY_TEMP_HMAC_221           221
#define HSM_KEY_TEMP_HMAC_222           222
#define HSM_KEY_TEMP_HMAC_223           223

/* TEMP - RSA Public #241-242 */
#define HSM_KEY_TEMP_RSA_PUB_241        241
#define HSM_KEY_TEMP_RSA_PUB_242        242

/* UDK - ECC(ED) #161-166 (Private keys for signing) */
/* NOTE: ECC and ED share the same key slots per HSM SPI Command Manual v3 */
#define HSM_KEY_ED_161                  161
#define HSM_KEY_ED_162                  162
#define HSM_KEY_ED_163                  163
#define HSM_KEY_ED_164                  164
#define HSM_KEY_ED_165                  165
#define HSM_KEY_ED_166                  166

/* ECC key aliases (same slots as ED) */
#define HSM_KEY_ECC_161                 161
#define HSM_KEY_ECC_162                 162
#define HSM_KEY_ECC_163                 163
#define HSM_KEY_ECC_164                 164
#define HSM_KEY_ECC_165                 165
#define HSM_KEY_ECC_166                 166

/* TEMP - ECC(ED) #261-263 (Temp private keys) */
#define HSM_KEY_TEMP_ED_261             261
#define HSM_KEY_TEMP_ED_262             262
#define HSM_KEY_TEMP_ED_263             263

/* ECC TEMP key aliases (same slots as ED TEMP) */
#define HSM_KEY_TEMP_ECC_261            261
#define HSM_KEY_TEMP_ECC_262            262
#define HSM_KEY_TEMP_ECC_263            263

/* TEMP - ECC(ED) Public #271-272 (Temp public keys for verify) */
#define HSM_KEY_TEMP_ED_PUB_271         271
#define HSM_KEY_TEMP_ED_PUB_272         272

/* ECC Public TEMP key aliases */
#define HSM_KEY_TEMP_ECC_PUB_271        271
#define HSM_KEY_TEMP_ECC_PUB_272        272

/* Alias for backward compatibility */
#define HSM_KEK_RSA_KEY_ID              HSM_KEY_KEK_RSA  /* Key ID for KEK RSA key pair (149) */

/* DFU Constants */
#define DFU_CHUNK_SIZE					(512U)
#define DFU_MAX_RETRY					(3U)
#define DFU_PROGRESS_STEP				(10U)
#define DFU_MODE_START					(0x00U)
#define DFU_MODE_UPDATE					(0x01U)
#define DFU_MODE_FINISH					(0x02U)
#define DFU_MODE_RUN					(0x04U)
#define DFU_FINISH_DELAY_MS				(10U)
#define HSM_FW_NAME_LEN					(14U)

/*
 * =================================================================
 * Data Structures
 * =================================================================
 */

// HSM Version Information (OP Code 0x00)
typedef struct {
    uint16_t host_major;
    uint16_t host_minor;
    uint16_t host_patch;
    uint16_t hse_major;
    uint16_t hse_minor;
    uint16_t hse_patch;
    uint16_t ask_vendor_id;
    uint16_t ask_module_id;
    uint8_t  ask_major;
    uint8_t  ask_minor;
    uint8_t  ask_patch;
    uint32_t lot_number;          // Year + Week + Lot + Factory
    uint32_t serial_number;       // Sequential number starting from 1
    uint8_t  used_cert_slots;     // Number of stored certificates
    uint16_t release;
} HSM_VersionInfo_t;

// RSA Key Structure
typedef struct {
    uint16_t modulus_length;      // RSA modulus byte length
    uint8_t  modulus[256];        // RSA modulus
    uint16_t public_exp_size;     // Actual size of exponent in bytes
    uint8_t  public_exp[256];     // RSA public exponent
    uint8_t  private_exp[256];    // RSA private exponent (only for import)
} HSM_RSAKey_t;
// RSA Key Structure
typedef struct {
    uint16_t modulus_length;      // RSA modulus byte length
    uint8_t  modulus[256];        // RSA modulus
    uint16_t public_exp_size;     // Actual size of exponent in bytes
    //uint8_t  public_exp[256];     // RSA public exponent
    uint8_t  upper_private_exp[256];    // RSA private exponent (only for import)
    uint8_t  lower_private_exp[256];    // RSA private exponent (only for import)
} HSM_RSAKeyEn_t;


// ECC Key Structure
typedef struct {
    uint8_t public_key_x[32];     // Public key X coordinate
    uint8_t public_key_y[32];     // Public key Y coordinate
    uint8_t private_key[32];      // Private key (only for import, 0 for public only)
} HSM_ECCKey_t;

// ED Key Structure (Ed25519)
typedef struct {
    uint8_t private_key[32];      // Private key (32 bytes)
    uint8_t public_key[32];       // Public key (32 bytes)
} HSM_EDKey_t;
// ED Key Structure (Ed25519)
typedef struct {
    uint8_t key[256];      // Encrypt key (Private or Public) (256 bytes)
} HSM_EDKey_enc_t;


// Context ID for Multi-call operations
typedef uint16_t HSM_ContextID_t;

/*
 * =================================================================
 * Phase 1: Basic Functions (OP Codes 0x00, 0x01, 0x02, 0x10)
 * =================================================================
 */

/**
 * @brief Get HSM version and certificate storage information
 * @param version_info: Pointer to store version information
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_get_version_info(HSM_VersionInfo_t* version_info);

/**
 * @brief Get HSM Serial Number (UID 8 bytes)
 * @param serial_number: Pointer to 8-byte buffer to store HSN
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_get_serial_number(uint8_t* serial_number);

/**
 * @brief Cancel current HSM job
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_cancel_job(void);

/**
 * @brief Get last error code from HSM (OP 0x0B)
 * @details Use this function after receiving AVO_RET_NOK (0x80) to get detailed error information.
 * @param error_code: Pointer to store error code (1 byte)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_get_last_error(uint8_t* error_code);

/**
 * @brief Generate random numbers
 * @param output: Pointer to buffer to store random data
 * @param length: Length of random data to generate (max 1024 bytes)
 * @param use_trng: true for TRNG, false for PRNG
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_generate_random(uint8_t* output,
									  uint16_t length,
									  bool use_trng);

/*
 * =================================================================
 * Phase 2: Key Management (OP Codes 0x05, 0x06, 0x07)
 * =================================================================
 */

/**
 * @brief Import AES key into HSM
 * @param key_id: Key slot ID (see Key Table in manual)
 * @param key_data: Pointer to AES key data
 * @param key_size: AES key size (HSM_AES_128/192/256)
 * @param encrypted: true if key is encrypted, false for plaintext
 * @param lock_key: true to lock key after import
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_import_aes_key(uint16_t key_id,
									 const uint8_t* key_data,
									 uint8_t key_size,
									 bool encrypted,
									 bool lock_key);

/**
 * @brief Import HMAC key into HSM
 * @param key_id: Key slot ID (HMAC slots: #121-124 UDK, #221-223 TEMP)
 * @param key_data: Pointer to HMAC key data (32 bytes, 256 bits)
 * @param encrypted: true if key is encrypted, false for plaintext
 * @param lock_key: true to lock key after import
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_import_hmac_key(uint16_t key_id,
									  const uint8_t* key_data,
									  bool encrypted,
									  bool lock_key);

/**
 * @brief Import Special Key (v5 Mode2 Bit2-5)
 * @details 특수키 주입 시 Mode2 Bit2-5에 키 용도를 설정
 * @param special_key_type: HSM_SPECIAL_KEY_ECU_CODE (#101) / KM (#102) / KAUTH (#103) / RSA_KEK (#149)
 * @param key_data: 키 데이터 (암호화된 경우 RSA ciphertext 256 bytes)
 * @param key_len: 키 데이터 길이
 * @param encrypted: true = RSA로 암호화된 키
 * @param lock_key: true = 키 잠금
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_import_special_key(uint8_t special_key_type,
										 const uint8_t* key_data,
										 uint16_t key_len,
										 bool encrypted,
										 bool lock_key);

/**
 * @brief Import RSA key pair into HSM
 * @param key_id: Key slot ID
 * @param rsa_key: Pointer to RSA key structure
 * @param public_only: true to import only public key
 * @param encrypted: true if key is encrypted
 * @param lock_key: true to lock key after import
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_import_rsa_key(uint16_t key_id,
									 const HSM_RSAKey_t* rsa_key,
									 bool public_only,
									 bool encrypted,
									 bool lock_key,
									 uint8_t key_size_bits);
HAL_StatusTypeDef hsm_import_rsa_key_en(uint16_t key_id,
									 const HSM_RSAKeyEn_t* rsa_keyEn,
									 uint8_t public_only,
									 bool encrypted,
									 bool lock_key,
									 uint8_t key_size_bits);


/**
 * @brief Import ECC key pair into HSM
 * @param key_id: Key slot ID
 * @param ecc_key: Pointer to ECC key structure
 * @param public_only: true to import only public key
 * @param encrypted: true if key is encrypted
 * @param lock_key: true to lock key after import
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_import_ecc_key(uint16_t key_id,
									 const HSM_ECCKey_t* ecc_key,
									 bool public_only,
									 bool encrypted,
									 bool lock_key);

/**
 * @brief Import ED key (Ed25519) into HSM
 * @param key_id: Key slot ID
 * @param ed_key: Pointer to ED key structure
 * @param public_only: true to import only public key
 * @param encrypted: true if key is encrypted
 * @param lock_key: true to lock key after import
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_import_ed_key(uint16_t key_id,
									const HSM_EDKey_t* ed_key,
									uint8_t public_only,
									bool encrypted,
									bool lock_key);
HAL_StatusTypeDef hsm_import_ed_key_en(uint16_t key_id,
									const HSM_EDKey_enc_t* ed_key,
									uint8_t public_only,
									bool encrypted,
									bool lock_key);


/**
 * @brief Export public key from HSM (RSA/ECC/ED)
 * @param key_id: Key slot ID
 * @param alg_type: Algorithm type (HSM_ALG_RSA/ECC/ED)
 * @param output_buffer: Pointer to buffer to store exported key
 * @param output_length: Pointer to store actual output length
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_export_public_key(uint16_t key_id,
										uint8_t alg_type,
										uint8_t* output_buffer,
										uint16_t* output_length);

/**
 * @brief Generate key pair in HSM (AES/RSA/ECC)
 * @param key_id: Key slot ID to store generated key
 * @param alg_type: Algorithm type (HSM_ALG_AES/RSA/ECC/ED)
 * @param key_size: For AES: HSM_AES_128/192/256, ignored for others
 * @param lock_key: true to lock key after generation
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_generate_key_pair(uint16_t key_id,
										uint8_t alg_type,
										uint8_t key_size,
										bool lock_key);

/**
 * @brief Store certificate in HSM (Multi-call: Start/Update/Finish)
 * @param cert_id: Certificate slot ID (1-28)
 * @param alg_type: Algorithm type (HSM_ALG_RSA/ECC/ED)
 * @param cert_data: Pointer to certificate data
 * @param cert_length: Total length of certificate
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_store_certificate(uint16_t cert_id,
										uint8_t alg_type,
										const uint8_t* cert_data,
										uint16_t cert_length);

/**
 * @brief Read certificate from HSM (Multi-call: Start/Update/Finish)
 * @param cert_id: Certificate slot ID (1-28)
 * @param alg_type: Algorithm type (HSM_ALG_RSA/ECC/ED)
 * @param cert_buffer: Pointer to buffer to store certificate
 * @param cert_length: Pointer to store actual certificate length
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_read_certificate(uint16_t cert_id,
									   uint8_t alg_type,
									   uint8_t* cert_buffer,
									   uint16_t* cert_length);

/**
 * @brief Perform ECDH key exchange
 * @param key_id: Local private key ID
 * @param peer_public_x: Peer's public key X coordinate (32 bytes)
 * @param peer_public_y: Peer's public key Y coordinate (32 bytes)
 * @param shared_secret: Pointer to store shared secret (32 bytes)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_key_exchange_ecdh(uint16_t key_id,
                                        const uint8_t* peer_public_x,
                                        const uint8_t* peer_public_y,
                                        uint8_t* shared_secret);

/*
 * =================================================================
 * Phase 3: Cryptographic Operations (OP Codes 0x11-0x16)
 * =================================================================
 */

/**
 * @brief Calculate SHA hash (single-shot or streaming)
 * @param sha_type: SHA algorithm (HSM_SHA_160/256/512)
 * @param use_kd: true = KD-SHA mode (Bit7=1), uses key_id for Secret Key
 * @param key_id: Key index for KD-SHA mode (ignored when use_kd=false)
 * @param input_data: Pointer to input data
 * @param input_length: Length of input data
 * @param digest: Pointer to store digest (20/32/64 bytes)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_calculate_sha(uint8_t sha_type,
                                    bool use_kd,
                                    uint16_t key_id,
                                    const uint8_t* input_data,
                                    uint16_t input_length,
                                    uint8_t* digest);

/**
 * @brief AES encrypt data
 * @param key_id: AES key slot ID
 * @param aes_key_size: AES key size (HSM_AES_128/192/256)
 * @param mode: AES mode (HSM_AES_ECB/CBC/CTR/OFB)
 * @param iv: Initial vector (16 bytes, NULL for ECB)
 * @param plaintext: Pointer to plaintext data
 * @param length: Length of data (must be multiple of 16 for ECB/CBC)
 * @param ciphertext: Pointer to store encrypted data
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_aes_encrypt(uint16_t key_id,
								  uint8_t aes_key_size,
								  uint8_t mode,
								  const uint8_t* iv,
								  const uint8_t* plaintext,
								  uint16_t length,
								  uint8_t* ciphertext);

/**
 * @brief AES decrypt data
 * @param key_id: AES key slot ID
 * @param aes_key_size: AES key size (HSM_AES_128/192/256)
 * @param mode: AES mode (HSM_AES_ECB/CBC/CTR/OFB)
 * @param iv: Initial vector (16 bytes, NULL for ECB)
 * @param ciphertext: Pointer to ciphertext data
 * @param length: Length of data
 * @param plaintext: Pointer to store decrypted data
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_aes_decrypt(uint16_t key_id,
								  uint8_t aes_key_size,
								  uint8_t mode,
								  const uint8_t* iv,
								  const uint8_t* ciphertext,
								  uint16_t length,
								  uint8_t* plaintext);

/**
 * @brief Generate HMAC-SHA256
 * @param key_id: HMAC key slot ID
 * @param input_data: Pointer to input data
 * @param input_length: Length of input data (64-512 bytes)
 * @param mac_output: Pointer to store MAC (32 bytes for SHA256)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_generate_hmac_sha256(uint16_t key_id,
										   const uint8_t* input_data,
										   uint16_t input_length,
										   uint8_t* mac_output);

/**
 * @brief Generate CMAC-AES
 * @param key_id: AES key slot ID
 * @param aes_key_size: AES key size (HSM_AES_128/192/256)
 * @param input_data: Pointer to input data
 * @param input_length: Length of input data (max 512 bytes, must be multiple of block size)
 * @param mac_output: Pointer to store MAC (16 bytes)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_generate_cmac_aes(uint16_t key_id,
										uint8_t aes_key_size,
										const uint8_t* input_data,
										uint16_t input_length,
										uint8_t* mac_output);

/**
 * @brief Sign data with RSA
 * @param key_id: RSA private key slot ID
 * @param sha_type: SHA algorithm (1=SHA1, 2=SHA256)
 * @param digest: Pointer to digest to sign
 * @param signature: Pointer to store signature (256 bytes for RSA-2048)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_sign_rsa(uint16_t key_id,
							   uint8_t sha_type,
							   const uint8_t* digest,
							   uint8_t* signature);

/**
 * @brief Verify RSA signature
 * @param key_id: RSA public key slot ID
 * @param sha_type: SHA algorithm (1=SHA1, 2=SHA256)
 * @param digest: Pointer to digest
 * @param signature: Pointer to signature to verify
 * @param is_valid: Pointer to store verification result (true=valid)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_verify_rsa_signature(uint16_t key_id,
										   uint8_t sha_type,
										   const uint8_t* digest,
										   const uint8_t* signature,
										   bool* is_valid);

/**
 * @brief Sign data with ECDSA
 * @param key_id: ECC private key slot ID
 * @param digest: Pointer to digest (32 bytes)
 * @param signature_r: Pointer to store R component (32 bytes)
 * @param signature_s: Pointer to store S component (32 bytes)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_sign_ecdsa(uint16_t key_id,
								 const uint8_t* digest,
								 uint8_t* signature_r,
								 uint8_t* signature_s);

/**
 * @brief Verify ECDSA signature
 * @param key_id: ECC public key slot ID
 * @param digest: Pointer to digest (32 bytes)
 * @param signature_r: Pointer to R component (32 bytes)
 * @param signature_s: Pointer to S component (32 bytes)
 * @param is_valid: Pointer to store verification result
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_verify_ecdsa_signature(uint16_t key_id,
											 const uint8_t* digest,
											 const uint8_t* signature_r,
											 const uint8_t* signature_s,
											 bool* is_valid);

/**
 * @brief Sign data with EdDSA (Ed25519)
 * @param key_id: ED private key slot ID
 * @param message: Pointer to message data
 * @param message_length: Length of message
 * @param signature: Pointer to store signature (64 bytes)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_sign_eddsa(uint16_t key_id,
								 const uint8_t* message,
								 uint16_t message_length,
								 uint8_t* signature);

/**
 * @brief Verify EdDSA (Ed25519) signature
 * @param key_id: ED public key slot ID
 * @param message: Pointer to message data
 * @param message_length: Length of message
 * @param signature: Pointer to signature (64 bytes)
 * @param is_valid: Pointer to store verification result
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_verify_eddsa_signature(uint16_t key_id,
											 const uint8_t* message,
											 uint16_t message_length,
											 const uint8_t* signature,
											 bool* is_valid);

/**
 * @brief RSA encrypt data (RSAES-PKCS1-v1_5)
 * @param key_id: RSA public key slot ID
 * @param plaintext: Pointer to plaintext (max 256 bytes)
 * @param plaintext_length: Length of plaintext
 * @param ciphertext: Pointer to store ciphertext (256 bytes for RSA-2048)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_rsa_encrypt(uint16_t key_id,
								  const uint8_t* plaintext,
								  uint16_t plaintext_length,
								  uint8_t* ciphertext);

/**
 * @brief RSA decrypt data (RSAES-PKCS1-v1_5)
 * @param key_id: RSA private key slot ID
 * @param ciphertext: Pointer to ciphertext (256 bytes for RSA-2048)
 * @param plaintext: Pointer to store decrypted plaintext
 * @param plaintext_length: Pointer to store plaintext length
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_rsa_decrypt(uint16_t key_id,
								  const uint8_t* ciphertext,
								  uint8_t* plaintext,
								  uint16_t* plaintext_length);

/**
 * @brief Derive key using HKDF-SHA256
 * @param source_key_id: Source key ID (or 0 to use password)
 * @param password: Pointer to password (if source_key_id=0)
 * @param password_length: Password length
 * @param salt: Pointer to salt (max 128 bytes)
 * @param salt_length: Salt length
 * @param info: Pointer to info (max 256 bytes)
 * @param info_length: Info length
 * @param output_key_length: Desired output key length
 * @param derived_key: Pointer to store derived key
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_derive_key_hkdf(uint16_t source_key_id,
									  const uint8_t* password,
									  uint8_t password_length,
									  const uint8_t* salt,
									  uint8_t salt_length,
									  const uint8_t* info,
									  uint16_t info_length,
									  uint16_t output_key_length,
									  uint8_t* derived_key);

/*
 * =================================================================
 * Phase 4: Authentication (OP Codes 0x20-0x24)
 * =================================================================
 */

/**
 * @brief Advanced Seed Key (ASK) generation
 * @param seed: Pointer to 8-byte seed
 * @param encrypted_ecu_code: Pointer to 16-byte encrypted ECU code (AES-128-CTR)
 * @param master_id: Master ID (0=PV, 1=CV, 2=Rework)
 * @param iv: Pointer to 16-byte IV, or NULL to use reserved IV
 *            - NULL: IV Length=0, HSM uses internal reserved IV
 *            - non-NULL: IV Length=16, HSM uses provided IV
 * @param ask_key: Pointer to store generated ASK key (8 bytes)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_generate_ask_key(const uint8_t* seed,
									   const uint8_t* encrypted_ecu_code,
									   uint8_t master_id,
									   const uint8_t* iv,
									   uint8_t* ask_key);

/**
 * @brief Internal Authentication - Step 1 of mutual auth (Start + Update)
 * @details Equivalent to OLD HSM's InternalAuthHSM()
 *          Call this first, then verify Sign1, calculate Sign2, then call hsm_external_auth()
 *          Context ID is stored internally for hsm_external_auth()
 * @param use_kauth: true to use Kauth key (#103), false to use Km key (#102)
 * @param random_sr: Pointer to 16-byte Server Random from VCI
 * @param random_cr: [OUT] Pointer to store 16-byte Card Random from HSM
 * @param signature_hsm: [OUT] Pointer to store 32-byte Sign1 from HSM
 * @param key_id: [OUT] Pointer to store key ID
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_internal_auth(bool use_kauth,
                                    const uint8_t* random_sr,
                                    uint8_t* random_cr,
                                    uint8_t* signature_hsm,
                                    uint8_t* key_id);

/**
 * @brief External Authentication - Step 2 of mutual auth (Finish)
 * @details Equivalent to OLD HSM's ExternalAuthHSM()
 *          Call this after hsm_internal_auth() with Sign2 calculated by App
 *          Uses Context ID stored by hsm_internal_auth()
 * @param use_kauth: true to use Kauth key (#103), false to use Km key (#102)
 * @param signature_vci: Pointer to 32-byte Sign2 calculated by VCI
 * @param auth_success: [OUT] Pointer to store authentication result
 * @return HAL_StatusTypeDef
 * @note On success, session key Ks is generated and stored at slot #209
 */
HAL_StatusTypeDef hsm_external_auth(bool use_kauth,
                                    const uint8_t* signature_vci,
                                    bool* auth_success);

/**
 * @brief Perform mutual authentication with HSM (Legacy - full sequence)
 * @warning Requires Sign2 before calling. Use hsm_internal_auth() + hsm_external_auth() for proper flow.
 * @param use_kauth: true to use Kauth key, false to use Km key
 * @param random_sr: Pointer to 16-byte random number from VCI
 * @param signature_vci: Pointer to 32-byte signature from VCI
 * @param random_cr: Pointer to store 16-byte random from HSM
 * @param signature_hsm: Pointer to store 32-byte signature from HSM
 * @param key_id: Pointer to store key ID
 * @param auth_success: Pointer to store authentication result
 * @return HAL_StatusTypeDef
 * @note On success, session key Ks is generated and stored at slot #209
 */
HAL_StatusTypeDef hsm_mutual_authentication(bool use_kauth,
											const uint8_t* random_sr,
											const uint8_t* signature_vci,
											uint8_t* random_cr,
											uint8_t* signature_hsm,
											uint8_t* key_id,
											bool* auth_success);

/**
 * @brief Import Kauth key into HSM
 * @param encrypted_kauth: Pointer to encrypted Kauth (RSA-encrypted)
 * @param kauth_length: Length of encrypted Kauth
 * @return HAL_StatusTypeDef
 * @note Kauth is stored internally at key slot #103
 */
HAL_StatusTypeDef hsm_import_kauth_key(const uint8_t* encrypted_kauth,
									   uint16_t kauth_length);

/**
 * @brief Perform one-way authentication with HSM
 * @param mac_value: Pointer to 16-byte MAC value from VCI
 * @param random_cr: Pointer to store 16-byte random from HSM
 * @param auth_success: Pointer to store authentication result
 * @return HAL_StatusTypeDef
 * @note On success, session key Ks = SHA256(HSN||CR) is generated and stored at slot #209
 */
HAL_StatusTypeDef hsm_oneway_authentication(const uint8_t* mac_value,
											uint8_t* random_cr,
											bool* auth_success);

/*
 * =================================================================
 * Phase 5: Firmware Update (OP Codes 0x90-0x91)
 * =================================================================
 */

/**
 * @brief Perform Device Firmware Upgrade (DFU)
 * @param firmware_type: Firmware type (HSM_DFU_TYPE_HSE_FW or HSM_DFU_TYPE_APP_FW)
 * @param firmware_data: Pointer to firmware binary data
 * @param firmware_size: Total firmware size in bytes
 * @return HAL_StatusTypeDef
 * @note This function handles Start/Update/Finish/Run sequence automatically
 */
//HAL_StatusTypeDef hsm_firmware_upgrade(uint8_t firmware_type,
//									   const uint8_t* firmware_data,
//									   uint32_t firmware_size);
HAL_StatusTypeDef hsm_firmware_upgrade(uint8_t firmware_type);


/**
 * @brief Complete HSM provisioning (IN_FIELD → ACTIVE lifecycle transition)
 * @return HAL_StatusTypeDef
 * @note This triggers Bank Swap. HSM will reboot after this command.
 */
HAL_StatusTypeDef hsm_set_complete(void);

/*
 * =================================================================
 * CSAC Compatibility Functions
 * =================================================================
 */

/**
 * @brief Map Old HSM key_num to New HSM key_id for CSAC
 * @param old_key_num: Old HSM key number (PA_SHA1=0x01, PA_SHA2=0x02, CO_SHA1=0x03, CO_SHA2=0x04)
 * @return New HSM key_id for RSA signing
 */
static inline uint16_t hsm_map_csac_key(uint8_t old_key_num) {
    switch (old_key_num) {
        case 0x01: return HSM_KEY_CSAC_PA_SHA1;  /* PA_SHA1 */
        case 0x02: return HSM_KEY_CSAC_PA_SHA2;  /* PA_SHA2 */
        case 0x03: return HSM_KEY_CSAC_CO_SHA1;  /* CO_SHA1 */
        case 0x04: return HSM_KEY_CSAC_CO_SHA2;  /* CO_SHA2 */
        default:   return HSM_KEY_CSAC_PA_SHA1;
    }
}

/**
 * @brief RSA Sign with Seed (SHA hash + Sign) - For CSAC compatibility
 * @details Wrapper for Old HSM SignPrivateHSM() compatibility.
 *          Calculates SHA hash internally and then signs with RSA.
 * @param seed: Pointer to seed data
 * @param seed_len: Seed length (typically 8 bytes)
 * @param signature: Pointer to store signature (256 bytes)
 * @param sign_len: Pointer to store signature length
 * @param sha_type: SHA algorithm (HSM_SHA_160 or HSM_SHA_256)
 * @param key_id: New HSM key slot ID
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_sign_rsa_with_seed(const uint8_t* seed, uint16_t seed_len,
                                         uint8_t* signature, uint32_t* sign_len,
                                         uint8_t sha_type, uint16_t key_id);


/**
 * @brief Sign seed with RSA using direct padding (no hash) - OLD HSM compatible
 * @details Mimics OLD HSM's SignPrivateHSM: seed is placed directly in digest area
 *          without hashing. Use this for OLD HSM compatibility when seed should
 *          NOT be hashed before signing.
 * @param seed: Pointer to seed data (typically 8 bytes)
 * @param seed_len: Length of seed
 * @param signature: Pointer to store signature (256 bytes)
 * @param sign_len: Pointer to store signature length
 * @param sha_type: SHA type (HSM_SHA_160 or HSM_SHA_256)
 * @param key_id: RSA key slot ID
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_sign_rsa_with_seed_direct(const uint8_t* seed, uint16_t seed_len,
                                                uint8_t* signature, uint32_t* sign_len,
                                                uint8_t sha_type, uint16_t key_id);

/**
 * @brief Sign with Seed using WBC/OLD HSM compatible method (SignPrivateHSM)
 * @details Replicates OLD HSM's SignPrivateHSM behavior using SIGN command.
 *          SIGN internally creates PKCS#1 v1.5 padding with DigestInfo.
 *          By passing seed+zeros as "hash", we achieve WBC compatibility.
 *
 *          SHA-1:   [0x00][0x01][FF×218][0x00][DigestInfo:15][hash:20]
 *          SHA-256: [0x00][0x01][FF×202][0x00][DigestInfo:19][hash:32]
 *
 * @param seed: Pointer to seed data (typically 8 bytes)
 * @param seed_len: Length of seed (max 20 for SHA-1, max 32 for SHA-256)
 * @param signature: Output buffer for signature (256 bytes)
 * @param sign_len: Pointer to store signature length
 * @param key_id: RSA key slot ID
 * @param sha_type: HSM_SHA_160 (SHA-1) or HSM_SHA_256 (SHA-256)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef hsm_sign_rsa_with_seed_new(const uint8_t* seed, uint16_t seed_len,
                                              uint8_t* signature, uint32_t* sign_len,
                                              uint16_t key_id, uint8_t sha_type);

/* ============================================================
 * HSM Test Version persistence (FW_TEST_VERSION_OVERRIDE 전용)
 * firmware.h 의 FW_TEST_VERSION_OVERRIDE 가 정의돼야 활성화됨.
 * ============================================================ */
#ifdef FW_TEST_VERSION_OVERRIDE
#define HSM_TEST_VERSION_FILE       "HSM_TEST_VER.bin"
#define HSM_TEST_VERSION_DEFAULT    1001U
#define HSM_TEST_VERSION_AFTER_UPG  1002U

uint16_t Load_HSM_TestVersion(void);
HAL_StatusTypeDef Save_HSM_TestVersion(uint16_t version);
#endif /* FW_TEST_VERSION_OVERRIDE */

#endif /* GIT_HSM_API_H_ */
