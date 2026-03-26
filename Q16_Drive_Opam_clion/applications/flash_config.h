
#ifndef FLASH_CONFIG_H
#define FLASH_CONFIG_H
#include "easyflash.h"
#include "encoder_alignment.h"
#include "hall_adjustment.h"

extern motor_flash_config_t g_motor_flash_cfg;
extern hall_save_param_t hall_save_param;
extern uint32_t can_save_id;

#define FLASH_CAN_ID_MAGIC "FlashCanid"
#define FLASH_LINE_HALL_MAGIC "FlashLineHall"
#define FLASH_ENCODER_MAGIC "FlashEncoder"

#define FLASH_ENV_SET_SIZE 3
/**
 * @brief   默认环境变量表
 * @note    这些变量会从Flash加载到RAM中
 */
extern const ef_env default_env_set[FLASH_ENV_SET_SIZE];

#endif
