/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for NormalTask */
osThreadId_t NormalTaskHandle;
uint32_t NormalTaskBuffer[ 128 ];
osStaticThreadDef_t NormalTaskControlBlock;
const osThreadAttr_t NormalTask_attributes = {
  .name = "NormalTask",
  .stack_mem = &NormalTaskBuffer[0],
  .stack_size = sizeof(NormalTaskBuffer),
  .cb_mem = &NormalTaskControlBlock,
  .cb_size = sizeof(NormalTaskControlBlock),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for DebugTask */
osThreadId_t DebugTaskHandle;
uint32_t DebugTaskBuffer[ 256 ];
osStaticThreadDef_t DebugTaskControlBlock;
const osThreadAttr_t DebugTask_attributes = {
  .name = "DebugTask",
  .stack_mem = &DebugTaskBuffer[0],
  .stack_size = sizeof(DebugTaskBuffer),
  .cb_mem = &DebugTaskControlBlock,
  .cb_size = sizeof(DebugTaskControlBlock),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for CanCommTask */
osThreadId_t CanCommTaskHandle;
uint32_t CanCommTaskBuffer[ 256 ];
osStaticThreadDef_t CanCommTaskControlBlock;
const osThreadAttr_t CanCommTask_attributes = {
  .name = "CanCommTask",
  .stack_mem = &CanCommTaskBuffer[0],
  .stack_size = sizeof(CanCommTaskBuffer),
  .cb_mem = &CanCommTaskControlBlock,
  .cb_size = sizeof(CanCommTaskControlBlock),
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for CaclulateTask */
osThreadId_t CaclulateTaskHandle;
uint32_t CaclulateTaskBuffer[ 256 ];
osStaticThreadDef_t CaclulateTaskControlBlock;
const osThreadAttr_t CaclulateTask_attributes = {
  .name = "CaclulateTask",
  .stack_mem = &CaclulateTaskBuffer[0],
  .stack_size = sizeof(CaclulateTaskBuffer),
  .cb_mem = &CaclulateTaskControlBlock,
  .cb_size = sizeof(CaclulateTaskControlBlock),
  .priority = (osPriority_t) osPriorityAboveNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartNormalTask(void *argument);
void StartDebugTask(void *argument);
void StartCanCommTask(void *argument);
void StartCaculateTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of NormalTask */
  NormalTaskHandle = osThreadNew(StartNormalTask, NULL, &NormalTask_attributes);

  /* creation of DebugTask */
  DebugTaskHandle = osThreadNew(StartDebugTask, NULL, &DebugTask_attributes);

  /* creation of CanCommTask */
  CanCommTaskHandle = osThreadNew(StartCanCommTask, NULL, &CanCommTask_attributes);

  /* creation of CaclulateTask */
  CaclulateTaskHandle = osThreadNew(StartCaculateTask, NULL, &CaclulateTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartNormalTask */
/**
  * @brief  Function implementing the NormalTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartNormalTask */
__weak void StartNormalTask(void *argument)
{
  /* USER CODE BEGIN StartNormalTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartNormalTask */
}

/* USER CODE BEGIN Header_StartDebugTask */
/**
* @brief Function implementing the DebugTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDebugTask */
__weak void StartDebugTask(void *argument)
{
  /* USER CODE BEGIN StartDebugTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDebugTask */
}

/* USER CODE BEGIN Header_StartCanCommTask */
/**
* @brief Function implementing the CanCommTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCanCommTask */
__weak void StartCanCommTask(void *argument)
{
  /* USER CODE BEGIN StartCanCommTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartCanCommTask */
}

/* USER CODE BEGIN Header_StartCaculateTask */
/**
* @brief Function implementing the CaclulateTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCaculateTask */
__weak void StartCaculateTask(void *argument)
{
  /* USER CODE BEGIN StartCaculateTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartCaculateTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

