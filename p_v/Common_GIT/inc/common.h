#ifndef __GIT_COMMON_H__
#define __GIT_COMMON_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <stdbool.h>
#include "HAL_safe_limits.h"
#include "HAL_Safe_types.h"

#if defined(STM32F427X)
#include "STM32F4xx.h"
#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#endif
typedef enum {HAL_DISABLE = 0, HAL_ENABLE = !HAL_DISABLE} eHalFunctionalState;
typedef enum {HAL_RESET = 0, HAL_SET = !HAL_RESET} eHalFlagStatus;
typedef enum {HAL_RETURN_SUCCESS = 0, HAL_RETURN_FAIL = (-1)} eHalReturnStatus;
typedef enum {HAL_ERROR = 0,  HAL_SUCCESS = !HAL_ERROR} eHalErrorStatus;






#endif /* __GIT_COMMON_H__*/
