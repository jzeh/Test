/* Return Value */
#define DAMO_CRYPT_SUCCESS                 00000 /* Success */
#define DAMO_CRYPT_ERR_HMAC_NULL_POINTER  -30000 /* HMAC계산시 NULL 포인터 에러 */

typedef enum
{
	HMAC_SHA256 = 0,
	HMAC_TYPE_END = 255
} HMAC_TYPE;

typedef struct
{
	HMAC_TYPE hmac_type;
	void *ctx;
} HMAC_CTX;

typedef struct
{
	HMAC_TYPE type;

	int digest_len;

	/* HMAC init function */
	void  (*hmac_init_func)( void *ctx, const unsigned char *key, size_t key_len );

	/* HMAC update function */
	void  (*hmac_update_func)( void *ctx, const unsigned char *data, size_t data_len );

	/* HMAC final function */
	void  (*hmac_final_func)( void *ctx, unsigned char *output);

	/* HMAC context reset function */
	void  (*hmac_reset_func)( void *ctx );

	/* HMAC function */
	int   (*hmac_func)( const unsigned char *key, size_t key_len,
			const unsigned char *data, size_t data_len, unsigned char *digest );

	/* Create HMAC context */
	void *(*hmac_ctx_create_func)( void );

	/* Destroy HMAC context */
	void  (*hmac_ctx_destroy_func)( void *ctx );
} HMAC_Desc;


int DAMO_CRYPT_HMAC_CTX_Create( HMAC_CTX *ctx, HMAC_TYPE hmac_type );

void DAMO_CRYPT_HMAC_CTX_Destroy( HMAC_CTX *ctx );

int DAMO_CRYPT_HMAC_Init( HMAC_CTX *ctx, const unsigned char *key, size_t key_len );

int DAMO_CRYPT_HMAC_Update( HMAC_CTX *ctx, const unsigned char *data, size_t data_len );

int DAMO_CRYPT_HMAC_Final( HMAC_CTX *ctx, unsigned char *digest, size_t *digest_len );

int DAMO_CRYPT_HMAC_Reset( HMAC_CTX *ctx );

int DAMO_CRYPT_HMAC(HMAC_TYPE hmac_type, const unsigned char *key, size_t key_len, const unsigned char *data, size_t data_len, void *digest, size_t *digest_len);

int DAMO_CRYPT_HMAC_GET_DIGEST_LEN(HMAC_TYPE hmac_type);

