/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "stm32f4xx_hal.h"
#ifdef USE_CMSIS_OS
#include "cmsis_os.h"
#endif

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* Handlers declared in "main.c" file */
extern TIM_HandleTypeDef tim_drv_handle;
extern TIM_HandleTypeDef led_tim_drv_handle;
extern I2C_HandleTypeDef i2c_drv_handle;
extern I2S_HandleTypeDef i2s_drv_handle;
extern I2S_HandleTypeDef i2s3_drv_handle;
extern EXTI_HandleTypeDef exti_pb0_handle, exti_pb1_handle, exti_pb2_handle, exti_pb3_handle, exti_pb4_handle, exti_cdc_int_handle, exti_dsp_int_handle;
extern UART_HandleTypeDef uart_drv_handle;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
    /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

    /* USER CODE END NonMaskableInt_IRQn 0 */
    /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

    /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
    /* USER CODE BEGIN HardFault_IRQn 0 */

    /* USER CODE END HardFault_IRQn 0 */
    while (1)
    {
        /* USER CODE BEGIN W1_HardFault_IRQn 0 */
        /* USER CODE END W1_HardFault_IRQn 0 */
    }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
    /* USER CODE BEGIN MemoryManagement_IRQn 0 */

    /* USER CODE END MemoryManagement_IRQn 0 */
    while (1)
    {
        /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
        /* USER CODE END W1_MemoryManagement_IRQn 0 */
    }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
    /* USER CODE BEGIN BusFault_IRQn 0 */

    /* USER CODE END BusFault_IRQn 0 */
    while (1)
    {
        /* USER CODE BEGIN W1_BusFault_IRQn 0 */
        /* USER CODE END W1_BusFault_IRQn 0 */
    }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
    /* USER CODE BEGIN UsageFault_IRQn 0 */

    /* USER CODE END UsageFault_IRQn 0 */
    while (1)
    {
        /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
        /* USER CODE END W1_UsageFault_IRQn 0 */
    }
}

#ifndef USE_CMSIS_OS
/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
    /* USER CODE BEGIN SVCall_IRQn 0 */

    /* USER CODE END SVCall_IRQn 0 */
    /* USER CODE BEGIN SVCall_IRQn 1 */

    /* USER CODE END SVCall_IRQn 1 */
}
#endif

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
    /* USER CODE BEGIN DebugMonitor_IRQn 0 */

    /* USER CODE END DebugMonitor_IRQn 0 */
    /* USER CODE BEGIN DebugMonitor_IRQn 1 */

    /* USER CODE END DebugMonitor_IRQn 1 */
}

#ifndef USE_CMSIS_OS
/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
    /* USER CODE BEGIN PendSV_IRQn 0 */

    /* USER CODE END PendSV_IRQn 0 */
    /* USER CODE BEGIN PendSV_IRQn 1 */

    /* USER CODE END PendSV_IRQn 1 */
}
#endif

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
#ifdef USE_CMSIS_OS
    osSystickHandler();
#else
    HAL_IncTick();
#endif
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/
#ifndef CONFIG_L25B
void EXTI0_IRQHandler(void)
{
    if (HAL_EXTI_GetPending(&exti_cdc_int_handle, EXTI_TRIGGER_RISING))
    {
        HAL_EXTI_IRQHandler(&exti_cdc_int_handle);
    }
}
#endif

/**
  * @brief This function handles EXTI line2 interrupt.
  */
void EXTI2_IRQHandler(void)
{
    if (HAL_EXTI_GetPending(&exti_pb1_handle, EXTI_TRIGGER_RISING))
    {
        HAL_EXTI_IRQHandler(&exti_pb1_handle);
    }
}

/**
  * @brief This function handles EXTI line[9:5] interrupts.
  */
void EXTI9_5_IRQHandler(void)
{
    if (HAL_EXTI_GetPending(&exti_pb2_handle, EXTI_TRIGGER_RISING))
    {
        HAL_EXTI_IRQHandler(&exti_pb2_handle);
    }

    if (HAL_EXTI_GetPending(&exti_pb3_handle, EXTI_TRIGGER_RISING))
    {
        HAL_EXTI_IRQHandler(&exti_pb3_handle);
    }
}

void EXTI15_10_IRQHandler(void)
{
    if (HAL_EXTI_GetPending(&exti_pb0_handle, EXTI_TRIGGER_RISING))
    {
        HAL_EXTI_IRQHandler(&exti_pb0_handle);
    }

    if (HAL_EXTI_GetPending(&exti_pb4_handle, EXTI_TRIGGER_RISING))
    {
        HAL_EXTI_IRQHandler(&exti_pb4_handle);
    }

    if (HAL_EXTI_GetPending(&exti_dsp_int_handle, EXTI_TRIGGER_RISING))
    {
        HAL_EXTI_IRQHandler(&exti_dsp_int_handle);
    }

#ifdef CONFIG_L25B
    // If ALERTb is routed to PC11 for L25B
    if (HAL_EXTI_GetPending(&exti_cdc_int_handle, EXTI_TRIGGER_RISING))
    {
        HAL_EXTI_IRQHandler(&exti_cdc_int_handle);
    }
#endif
}

void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&tim_drv_handle);
}

void TIM5_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&led_tim_drv_handle);
}

void I2C1_EV_IRQHandler(void)
{
  HAL_I2C_EV_IRQHandler(&i2c_drv_handle);
}

void I2C1_ER_IRQHandler(void)
{
  HAL_I2C_ER_IRQHandler(&i2c_drv_handle);
}


void DMA1_Stream4_IRQHandler(void)
{
  HAL_DMA_IRQHandler(i2s_drv_handle.hdmatx);
}

void DMA1_Stream3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(i2s_drv_handle.hdmarx);
}

void DMA1_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(i2s3_drv_handle.hdmarx);
}

void DMA1_Stream5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(i2s3_drv_handle.hdmatx);
}

void USART2_IRQHandler(void)
{
  HAL_UART_IRQHandler(&uart_drv_handle);
}
