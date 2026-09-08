#ifndef GIT_BKSRAM_H
#define GIT_BKSRAM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bkSRAM_Init(void);
void bkSRAM_WriteVariables(uint16_t start_address, uint8_t *data, uint16_t length);
void bkSRAM_ReadVariables(uint16_t start_address, uint8_t *read_data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* GIT_BKSRAM_H */