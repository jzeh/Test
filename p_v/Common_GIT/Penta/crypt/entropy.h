#include "hmac_drbg.h"
#ifndef DUKPT_SERVER
#include "dukpt_cli_config.h"
#endif
#define DAMO_CRYPT_SUCCESS                        00000 /* Success */

int DAMO_CRYPT_Get_SHA256_Entropy_Data(void *data, unsigned char *output, size_t output_len);
