/**
 * @file platform_bsp.c
 *
 * @brief Implementation of the BSP for the Live Oak platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021 All Rights Reserved, http://www.cirrus.com/
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdlib.h>
#include "platform_bsp.h"
#include "stm32f4xx_hal.h"
#include "test_tone_tables.h"
#ifdef USE_CMSIS_OS
#include "FreeRTOS.h"
#include "semphr.h"
#endif
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define BSP_MCU_CLOCK_CFG_HSI           (0)
#define BSP_MCU_CLOCK_CFG_HSE           (1)
#define BSP_MCU_CLOCK_CFG               (BSP_MCU_CLOCK_CFG_HSI)

#define BSP_I2C_TRANSACTION_TYPE_WRITE                  (0)
#define BSP_I2C_TRANSACTION_TYPE_READ_REPEATED_START    (1)
#define BSP_I2C_TRANSACTION_TYPE_DB_WRITE               (2)
#define BSP_I2C_TRANSACTION_TYPE_INVALID                (3)

#define BSP_DUT_RESET_CLK_ENABLE                __HAL_RCC_GPIOB_CLK_ENABLE
#define BSP_DUT_RESET_CLK_DISABLE               __HAL_RCC_GPIOB_CLK_DISABLE
#define BSP_DUT_RESET_PIN                       GPIO_PIN_4
#define BSP_DUT_RESET_GPIO_PORT                 GPIOB
#define BSP_DUT_INT_CLK_ENABLE                  __HAL_RCC_GPIOH_CLK_ENABLE
#define BSP_DUT_INT_CLK_DISABLE                 __HAL_RCC_GPIOH_CLK_DISABLE
#define BSP_DUT_INT_PIN                         GPIO_PIN_0
#define BSP_DUT_INT_GPIO_PORT                   GPIOH

#define BSP_PB_TOTAL                            (0)

#define BSP_LED_MODE_FIXED                      (0)
#define BSP_LED_MODE_BLINK                      (1)

/* Select the preemption priority level(0 is the highest) */
#define BSP_DUT_INT_PREEMPT_PRIO                    (0xF)
#define USART2_IRQ_PREPRIO                          (0xF)
#define BSP_TIM2_PREPRIO                            (0x4)
#define BSP_TIM5_PREPRIO                            (0x4)
#define BSP_I2C1_ERROR_PREPRIO                      (0x1)
#define BSP_I2C1_EVENT_PREPRIO                      (0x2)

#define BSP_LED_PASS                                (0)
#define BSP_LED_FAIL                                (1)
#define BSP_LED_TOTAL                               (2)

typedef struct
{
    uint32_t id;
    uint8_t mode;
    bool is_on;
    uint32_t blink_counter_100ms;
    uint32_t blink_counter_100ms_max;
} bsp_led_t;

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static bsp_callback_t bsp_timer_cb;
static void *bsp_timer_cb_arg;
static bool bsp_timer_has_started;
static bool bsp_timer_elapsed;

static bsp_callback_t bsp_i2c_done_cb;
static void *bsp_i2c_done_cb_arg;
static uint8_t bsp_i2c_current_transaction_type;
static uint8_t *bsp_i2c_read_buffer_ptr;
static uint32_t bsp_i2c_read_length;
static uint8_t bsp_i2c_read_address;
static uint32_t bsp_i2c_write_length;
static uint8_t *bsp_i2c_write_buffer_ptr;
static bool bsp_i2c_transaction_complete;
static bool bsp_i2c_transaction_error;

static uint32_t bsp_switch_state = 0;

static bsp_app_callback_t app_cb = NULL;
static void *app_cb_arg = NULL;

static volatile int32_t bsp_irq_count = 0;

static bsp_callback_t bsp_dut_int_cb = NULL;
static void *bsp_dut_int_cb_arg = NULL;

static uint32_t spi_baud_prescaler = SPI_BAUDRATEPRESCALER_16;

static bsp_led_t bsp_leds[BSP_LED_TOTAL] =
{
    {
        .id = BSP_GPIO_ID_INTP_LED1,
        .mode = BSP_LED_MODE_FIXED,
        .is_on = false,
        .blink_counter_100ms = 0,
        .blink_counter_100ms_max = 0,
    },
    {
        .id = BSP_GPIO_ID_INTP_LED2,
        .mode = BSP_LED_MODE_FIXED,
        .is_on = false,
        .blink_counter_100ms = 0,
        .blink_counter_100ms_max = 0,
    }
};

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
TIM_HandleTypeDef tim_drv_handle;
TIM_HandleTypeDef led_tim_drv_handle;
I2C_HandleTypeDef i2c_drv_handle;
SPI_HandleTypeDef hspi2;
EXTI_HandleTypeDef exti_sel_gpi_1_handle, exti_sel_gpi_2_handle, exti_sel_gpi_3_handle, exti_sel_gpi_4_handle, exti_int_handle;

uint32_t bsp_set_timer(uint32_t duration_ms, bsp_callback_t cb, void *cb_arg);
uint32_t bsp_set_gpio(uint32_t gpio_id, uint8_t gpio_state);
/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
static void Error_Handler(void)
{
    while(1);

    return;
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

#if (BSP_MCU_CLOCK_CFG == BSP_MCU_CLOCK_CFG_HSE)
    /**
      * @brief  System Clock Configuration
      *         The system Clock is configured as follow :
      *            System Clock source            = PLL (HSE)
      *            SYSCLK(Hz)                     = 84000000
      *            HCLK(Hz)                       = 84000000
      *            AHB Prescaler                  = 1
      *            APB1 Prescaler                 = 2
      *            APB2 Prescaler                 = 1
      *            HSE Frequency(Hz)              = 8000000
      *            PLL_M                          = 8
      *            PLL_N                          = 336
      *            PLL_P                          = 4
      *            PLL_Q                          = 7
      *            VDD(V)                         = 3.3
      *            Main regulator output voltage  = Scale2 mode
      *            Flash Latency(WS)              = 2
      * @param  None
      * @retval None
      */

    /* Enable Power Control clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is
       clocked below the maximum system frequency, to update the voltage scaling value
       regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
       clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | \
                                   RCC_CLOCKTYPE_HCLK | \
                                   RCC_CLOCKTYPE_PCLK1 | \
                                   RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
      Error_Handler();
    }
#elif (BSP_MCU_CLOCK_CFG == BSP_MCU_CLOCK_CFG_HSI)
    /**
      * @brief  System Clock Configuration
      *         The system Clock is configured as follow :
      *            System Clock source            = PLL (HSI)
      *            SYSCLK(Hz)                     = 84000000
      *            HCLK(Hz)                       = 84000000
      *            AHB Prescaler                  = 1
      *            APB1 Prescaler                 = 2
      *            APB2 Prescaler                 = 1
      *            HSI Frequency(Hz)              = 16000000
      *            PLL_M                          = 16
      *            PLL_N                          = 336
      *            PLL_P                          = 4
      *            PLL_Q                          = 7
      *            VDD(V)                         = 3.3
      *            Main regulator output voltage  = Scale2 mode
      *            Flash Latency(WS)              = 2
      * @param  None
      * @retval None
      */

    /* Enable Power Control clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is
       clocked below the maximum system frequency, to update the voltage scaling value
       regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    /* Enable HSI Oscillator and activate PLL with HSI as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
       clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | \
                                   RCC_CLOCKTYPE_HCLK | \
                                   RCC_CLOCKTYPE_PCLK1 | \
                                   RCC_CLOCKTYPE_PCLK2);
    //RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
      Error_Handler();
    }
#endif
}

static void I2C_Init(void)
{
    i2c_drv_handle.Instance = I2C1;
    i2c_drv_handle.Init.ClockSpeed = 100000;
    i2c_drv_handle.Init.DutyCycle = I2C_DUTYCYCLE_2;
    i2c_drv_handle.Init.OwnAddress1 = 0;
    i2c_drv_handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c_drv_handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    i2c_drv_handle.Init.OwnAddress2 = 0;
    i2c_drv_handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c_drv_handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&i2c_drv_handle) != HAL_OK)
    {
      Error_Handler();
    }

    return;
}

static void SPI_Init(void)
{
    /* SPI1 parameter configuration*/
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    spi_baud_prescaler = SPI_BAUDRATEPRESCALER_16;
    hspi2.Init.BaudRatePrescaler = spi_baud_prescaler;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi2) != HAL_OK)
    {
        Error_Handler();
    }
}

static void Timer_Init(void)
{
    uint32_t uwPrescalerValue;
    /*##-1- Configure the TIM peripheral #######################################*/
     /* -----------------------------------------------------------------------
       In this example TIM2 input clock (TIM2CLK) is set to 2 * APB1 clock (PCLK1),
       since APB1 prescaler is different from 1.
         TIM2CLK = 2 * PCLK1
         PCLK1 = HCLK / 2
         => TIM2CLK = HCLK = SystemCoreClock
       To get TIM2 counter clock at 10 KHz, the Prescaler is computed as following:
       Prescaler = (TIM2CLK / TIM2 counter clock) - 1
       Prescaler = (SystemCoreClock /10 KHz) - 1

       Note:
        SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f4xx.c file.
        Each time the core clock (HCLK) changes, user had to update SystemCoreClock
        variable value. Otherwise, any configuration based on this variable will be incorrect.
        This variable is updated in three ways:
         1) by calling CMSIS function SystemCoreClockUpdate()
         2) by calling HAL API function HAL_RCC_GetSysClockFreq()
         3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
     ----------------------------------------------------------------------- */

     /* Compute the prescaler value to have TIM2 counter clock equal to 10 KHz */
     uwPrescalerValue = (uint32_t) ((SystemCoreClock / 10000) - 1);

     /* Set TIMx instance */
     tim_drv_handle.Instance = TIM2;

     /* Initialize TIM3 peripheral as follow:
          + Period = 10000 - 1
          + Prescaler = ((SystemCoreClock/2)/10000) - 1
          + ClockDivision = 0
          + Counter direction = Up
     */
     tim_drv_handle.Init.Period = 10000 - 1;
     tim_drv_handle.Init.Prescaler = uwPrescalerValue;
     tim_drv_handle.Init.ClockDivision = 0;
     tim_drv_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
     tim_drv_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    /* Set TIMx instance */
    led_tim_drv_handle.Instance = TIM5;

    // Configure LED blink timer for 100Hz/100ms delay
    /* Initialize TIM3 peripheral as follow:
         + Period = 1000 - 1
         + Prescaler = ((SystemCoreClock/2)/10000) - 1 = 10kHz
         + ClockDivision = 0
         + Counter direction = Up
    */
    led_tim_drv_handle.Init.Period = 1000 - 1;
    led_tim_drv_handle.Init.Prescaler = uwPrescalerValue;
    led_tim_drv_handle.Init.ClockDivision = 0;
    led_tim_drv_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    led_tim_drv_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if(HAL_TIM_Base_Init(&led_tim_drv_handle) != HAL_OK)
    {
      Error_Handler();
    }

    if(HAL_TIM_Base_Start_IT(&led_tim_drv_handle) != HAL_OK)
    {
      Error_Handler();
    }

    return;
}

static void Timer_Start(uint32_t delay_100us)
{
    if(HAL_TIM_Base_Stop_IT(&tim_drv_handle) != HAL_OK)
    {
      Error_Handler();
    }

    tim_drv_handle.Init.Period = delay_100us;
    if(HAL_TIM_Base_Init(&tim_drv_handle) != HAL_OK)
    {
      Error_Handler();
    }

    if(HAL_TIM_Base_Start_IT(&tim_drv_handle) != HAL_OK)
    {
      Error_Handler();
    }

    return;
}

static void bsp_exti_int_cb(void)
{
    if (bsp_dut_int_cb != NULL)
    {
        bsp_dut_int_cb(BSP_STATUS_OK, bsp_dut_int_cb_arg);
    }

    if (app_cb != NULL)
    {
        app_cb(BSP_STATUS_DUT_EVENTS, app_cb_arg);
    }
    return;
}

/***********************************************************************************************************************
 * MCU HAL FUNCTIONS
 **********************************************************************************************************************/
void HAL_MspInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable clocks to ports used
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    // Configure the LED_PASS and LED_FAIL GPO
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = (GPIO_PIN_5 | GPIO_PIN_8);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Configure the HAP_RSTb GPO
    HAL_GPIO_WritePin(BSP_DUT_RESET_GPIO_PORT, BSP_DUT_RESET_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = BSP_DUT_RESET_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BSP_DUT_RESET_GPIO_PORT, &GPIO_InitStruct);

    // Configure HAP_INTb GPI
    GPIO_InitStruct.Pin = BSP_DUT_INT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BSP_DUT_INT_GPIO_PORT, &GPIO_InitStruct);

    EXTI_ConfigTypeDef exti_config;
    exti_config.Line = EXTI_LINE_0;
    exti_config.Mode = EXTI_MODE_INTERRUPT;
    exti_config.Trigger = EXTI_TRIGGER_FALLING;
    HAL_EXTI_SetConfigLine(&exti_int_handle, &exti_config);
    HAL_EXTI_RegisterCallback(&exti_int_handle, HAL_EXTI_COMMON_CB_ID, &bsp_exti_int_cb);

    // Configure GPIO pins : SEL_GPI_1, 2, 3, 4 (PA8, 9, 10, 11)
    GPIO_InitStruct.Pin = (GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Enable and set cdc EXTI Interrupt to a low priority */
    HAL_NVIC_SetPriority((IRQn_Type)EXTI0_IRQn, BSP_DUT_INT_PREEMPT_PRIO, 0x00);
    HAL_NVIC_EnableIRQ((IRQn_Type)EXTI0_IRQn);

    // Configure CLK_SRC_EN (PB10)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    return;
}

void HAL_MspDeInit(void)
{
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
    HAL_GPIO_DeInit(BSP_DUT_RESET_GPIO_PORT, BSP_DUT_RESET_PIN);
    HAL_GPIO_DeInit(BSP_DUT_INT_GPIO_PORT, BSP_DUT_INT_PIN);
    HAL_GPIO_DeInit(GPIOA, (GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11));
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);

    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_GPIOH_CLK_DISABLE();

    return;
}

void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if(hspi->Instance==SPI2)
    {
        __HAL_RCC_SPI2_CLK_ENABLE();

        __HAL_RCC_GPIOB_CLK_ENABLE();

        // F_SPI_CSb (PB10)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = 0;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = (GPIO_PIN_12 | GPIO_PIN_13| GPIO_PIN_14 | GPIO_PIN_15);
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }

    return;
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
    if(hspi->Instance==SPI2)
    {
        __HAL_RCC_SPI2_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOB, (GPIO_PIN_10 | GPIO_PIN_12 | GPIO_PIN_13| GPIO_PIN_14 | GPIO_PIN_15));
    }

    return;
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        __HAL_RCC_TIM2_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM2_IRQn, BSP_TIM2_PREPRIO, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
    }

    if (htim->Instance == TIM5)
    {
        __HAL_RCC_TIM5_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM5_IRQn, BSP_TIM5_PREPRIO, 0);
        HAL_NVIC_EnableIRQ(TIM5_IRQn);
    }

    return;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        if (bsp_timer_has_started)
        {
            if(HAL_TIM_Base_Stop_IT(&tim_drv_handle) != HAL_OK)
            {
              Error_Handler();
            }

            bsp_timer_elapsed = true;

            if (bsp_timer_cb != NULL)
            {
                bsp_timer_cb(BSP_STATUS_OK, bsp_timer_cb_arg);
                bsp_timer_cb = NULL;
                bsp_timer_cb_arg = NULL;
            }
        }

        bsp_timer_has_started = !bsp_timer_has_started;
    }

    if (htim->Instance == TIM5)
    {
        bsp_led_t *led = bsp_leds;
        // If LED is in blink mode
        if (led->mode == BSP_LED_MODE_BLINK)
        {
            // Increment LED blink counter
            led->blink_counter_100ms++;

            // If LED blink counter elapsed, then change LED state
            if (led->blink_counter_100ms >= led->blink_counter_100ms_max)
            {
                led->blink_counter_100ms = 0;

                // Change LED GPIO
                if (led->is_on)
                {
                    led->is_on = false;
                    bsp_set_gpio(led->id, GPIO_PIN_SET);
                }
                else
                {
                    led->is_on = true;
                    bsp_set_gpio(led->id, GPIO_PIN_RESET);
                }
            }
        }

        led++;
        if (led->mode == BSP_LED_MODE_BLINK)
        {
            // Increment LED blink counter
            led->blink_counter_100ms++;

            // If LED blink counter elapsed, then change LED state
            if (led->blink_counter_100ms >= led->blink_counter_100ms_max)
            {
                led->blink_counter_100ms = 0;

                // Change LED GPIO
                if (led->is_on)
                {
                    led->is_on = false;
                    bsp_set_gpio(led->id, GPIO_PIN_SET);
                }
                else
                {
                    led->is_on = true;
                    bsp_set_gpio(led->id, GPIO_PIN_RESET);
                }
            }
        }
    }

    bsp_irq_count++;

    return;
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if(hi2c->Instance==I2C1)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        __HAL_RCC_I2C1_CLK_ENABLE();

        HAL_NVIC_SetPriority(I2C1_ER_IRQn, BSP_I2C1_ERROR_PREPRIO, 0);
        HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
        HAL_NVIC_SetPriority(I2C1_EV_IRQn, BSP_I2C1_EVENT_PREPRIO, 0);
        HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    }

    return;
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
    if(hi2c->Instance==I2C1)
    {
        __HAL_RCC_I2C1_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);

        HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
        HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    }

    return;
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (HAL_I2C_GetState(hi2c) == HAL_I2C_STATE_READY)
    {
        if (bsp_i2c_current_transaction_type == BSP_I2C_TRANSACTION_TYPE_READ_REPEATED_START)
        {
            HAL_I2C_Master_Seq_Receive_IT(hi2c,
                                          bsp_i2c_read_address,
                                          bsp_i2c_read_buffer_ptr,
                                          bsp_i2c_read_length,
                                          I2C_LAST_FRAME);
        }
        else if (bsp_i2c_current_transaction_type == BSP_I2C_TRANSACTION_TYPE_WRITE)
        {
            bsp_i2c_transaction_complete = true;
            if (bsp_i2c_done_cb != NULL)
            {
                bsp_i2c_done_cb(BSP_STATUS_OK, bsp_i2c_done_cb_arg);
            }
        }
        else if (bsp_i2c_current_transaction_type == BSP_I2C_TRANSACTION_TYPE_DB_WRITE)
        {
            if (bsp_i2c_write_length == 0)
            {
                bsp_i2c_transaction_complete = true;
                if (bsp_i2c_done_cb != NULL)
                {
                    bsp_i2c_done_cb(BSP_STATUS_OK, bsp_i2c_done_cb_arg);
                }
            }
            else
            {
                HAL_I2C_Master_Seq_Transmit_IT(hi2c,
                                               bsp_i2c_read_address,
                                               bsp_i2c_write_buffer_ptr,
                                               bsp_i2c_write_length,
                                               I2C_LAST_FRAME);
                bsp_i2c_write_length = 0;
            }
        }
    }

    bsp_irq_count++;

    return;
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (HAL_I2C_GetState(hi2c) == HAL_I2C_STATE_READY)
    {
        if (bsp_i2c_current_transaction_type != BSP_I2C_TRANSACTION_TYPE_INVALID)
        {
            bsp_i2c_transaction_complete = true;
            if (bsp_i2c_done_cb != NULL)
            {
                bsp_i2c_done_cb(BSP_STATUS_OK, bsp_i2c_done_cb_arg);
            }
        }
    }

    bsp_irq_count++;

    return;
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    bsp_i2c_transaction_error = true;
    if (bsp_i2c_done_cb != NULL)
    {
        bsp_i2c_done_cb(BSP_STATUS_FAIL, bsp_i2c_done_cb_arg);
    }

    return;
}

void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c)
{
    //Error_Handler();

    return;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
#ifdef SEMIHOSTING
extern void initialise_monitor_handles(void);
#endif

uint32_t bsp_initialize(bsp_app_callback_t cb, void *cb_arg)
{
    app_cb = cb;
    app_cb_arg = cb_arg;

#ifdef SEMIHOSTING
    initialise_monitor_handles();
#endif
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    // Configure LED_PASS and LED_FAIL
    bsp_set_gpio(BSP_GPIO_ID_INTP_LED1, GPIO_PIN_SET);
    bsp_set_gpio(BSP_GPIO_ID_INTP_LED2, GPIO_PIN_RESET);
    bsp_leds[BSP_LED_PASS].is_on = true;
    bsp_leds[BSP_LED_PASS].blink_counter_100ms_max = 1;
    bsp_leds[BSP_LED_PASS].mode = BSP_LED_MODE_BLINK;

    bsp_timer_cb = NULL;
    bsp_timer_cb_arg = NULL;
    bsp_timer_has_started = false;
    bsp_i2c_done_cb = NULL;
    bsp_i2c_done_cb_arg = NULL;
    bsp_i2c_current_transaction_type = BSP_I2C_TRANSACTION_TYPE_INVALID;

    /* Initialize all peripheral drivers */
    Timer_Init();
    I2C_Init();
    SPI_Init();

    return BSP_STATUS_OK;
}

void bsp_notification_callback(uint32_t event_flags, void *arg)
{
    bsp_toggle_gpio(BSP_GPIO_ID_INTP_LED2);
    bsp_toggle_gpio(BSP_GPIO_ID_INTP_LED2);
    return;
}

uint32_t bsp_audio_set_fs(uint32_t fs_hz)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_audio_play(uint8_t content)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_audio_record(void)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_audio_play_record(uint8_t content)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_audio_pause(void)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_audio_resume(void)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_audio_stop(void)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_set_timer(uint32_t duration_ms, bsp_callback_t cb, void *cb_arg)
{
    bsp_timer_cb = cb;
    bsp_timer_cb_arg = cb_arg;
    bsp_timer_has_started = false;
    bsp_timer_elapsed = false;

    Timer_Start(duration_ms * 10);
    if (cb == NULL)
    {
        bool temp_bool = false;
        while (!temp_bool)
        {
            __disable_irq();
            temp_bool = bsp_timer_elapsed;
            __enable_irq();
        }
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_set_gpio(uint32_t gpio_id, uint8_t gpio_state)
{
    switch (gpio_id)
    {
        case BSP_GPIO_ID_DUT_CDC_RESET:
            HAL_GPIO_WritePin(BSP_DUT_RESET_GPIO_PORT, BSP_DUT_RESET_PIN, (GPIO_PinState) gpio_state);
            break;

        case BSP_GPIO_ID_INTP_LED1:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, (GPIO_PinState) gpio_state);
            break;

        case BSP_GPIO_ID_INTP_LED2:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, (GPIO_PinState) gpio_state);
            break;

        default:
            break;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_set_supply(uint32_t supply_id, uint8_t supply_state)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_toggle_gpio(uint32_t gpio_id)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_spi_read(uint32_t bsp_dev_id,
                      uint8_t *addr_buffer,
                      uint32_t addr_length,
                      uint8_t *data_buffer,
                      uint32_t data_length,
                      uint32_t pad_len)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_spi_write(uint32_t bsp_dev_id,
                      uint8_t *addr_buffer,
                      uint32_t addr_length,
                      uint8_t *data_buffer,
                      uint32_t data_length,
                      uint32_t pad_len)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_i2c_read_repeated_start(uint32_t bsp_dev_id,
                                     uint8_t *write_buffer,
                                     uint32_t write_length,
                                     uint8_t *read_buffer,
                                     uint32_t read_length,
                                     bsp_callback_t cb,
                                     void *cb_arg)
{
    switch (bsp_dev_id)
    {
        case BSP_DUT_DEV_ID:
            bsp_i2c_transaction_complete = false;
            bsp_i2c_transaction_error = false;
            bsp_i2c_done_cb = cb;
            bsp_i2c_done_cb_arg = cb_arg;
            bsp_i2c_current_transaction_type = BSP_I2C_TRANSACTION_TYPE_READ_REPEATED_START;
            bsp_i2c_read_buffer_ptr = read_buffer;
            bsp_i2c_read_length = read_length;
            bsp_i2c_read_address = 0x86;
            HAL_I2C_Master_Seq_Transmit_IT(&i2c_drv_handle,
                                           bsp_i2c_read_address,
                                           write_buffer,
                                           write_length,
                                           I2C_FIRST_FRAME);

            if (cb == NULL)
            {
                while (!bsp_i2c_transaction_complete);
            }

            break;

        default:
            break;
    }

    if (bsp_i2c_transaction_error)
    {
        return BSP_STATUS_FAIL;
    }
    else
    {
        return BSP_STATUS_OK;
    }
}

uint32_t bsp_i2c_write(uint32_t bsp_dev_id,
                       uint8_t *write_buffer,
                       uint32_t write_length,
                       bsp_callback_t cb,
                       void *cb_arg)
{
    uint32_t ret = BSP_STATUS_OK;

    switch (bsp_dev_id)
    {
        case BSP_DUT_DEV_ID:
            bsp_i2c_transaction_complete = false;
            bsp_i2c_transaction_error = false;
            bsp_i2c_done_cb = cb;
            bsp_i2c_done_cb_arg = cb_arg;
            bsp_i2c_current_transaction_type = BSP_I2C_TRANSACTION_TYPE_WRITE;
            HAL_I2C_Master_Seq_Transmit_IT(&i2c_drv_handle,
                                           0x86,
                                           write_buffer,
                                           write_length,
                                           I2C_FIRST_AND_LAST_FRAME);

            if (cb == NULL)
            {
                while ((!bsp_i2c_transaction_complete) && (!bsp_i2c_transaction_error));
                if (bsp_i2c_transaction_error)
                {
                    ret = BSP_STATUS_FAIL;
                }
            }
            break;

        default:
            break;
    }

    return ret;
}

uint32_t bsp_i2c_db_write(uint32_t bsp_dev_id,
                          uint8_t *write_buffer_0,
                          uint32_t write_length_0,
                          uint8_t *write_buffer_1,
                          uint32_t write_length_1,
                          bsp_callback_t cb,
                          void *cb_arg)
{
    switch (bsp_dev_id)
    {
        case BSP_DUT_DEV_ID:
            bsp_i2c_transaction_complete = false;
            bsp_i2c_done_cb = cb;
            bsp_i2c_done_cb_arg = cb_arg;
            bsp_i2c_read_address = 0x86;
            bsp_i2c_write_length = write_length_1;
            bsp_i2c_write_buffer_ptr = write_buffer_1;
            bsp_i2c_current_transaction_type = BSP_I2C_TRANSACTION_TYPE_DB_WRITE;

            HAL_I2C_Master_Seq_Transmit_IT(&i2c_drv_handle, bsp_i2c_read_address, write_buffer_0, write_length_0, I2C_FIRST_FRAME);

            if (cb == NULL)
            {
                while (!bsp_i2c_transaction_complete);
            }

            break;

        default:
            break;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_register_gpio_cb(uint32_t gpio_id, bsp_callback_t cb, void *cb_arg)
{
    switch (gpio_id)
    {
        case BSP_GPIO_ID_DUT_CDC_INT:
            if (bsp_dut_int_cb == NULL)
            {
                bsp_dut_int_cb = cb;
                bsp_dut_int_cb_arg = cb_arg;
            }
            break;

        default:
            return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_i2c_reset(uint32_t bsp_dev_id, bool *was_i2c_busy)
{
    if (was_i2c_busy != NULL)
    {
        *was_i2c_busy = false;
    }

    if (HAL_I2C_GetState(&i2c_drv_handle) != HAL_I2C_STATE_READY)
    {
        if (was_i2c_busy != NULL)
        {
            *was_i2c_busy = true;
        }

        switch (bsp_dev_id)
        {
            case BSP_DUT_DEV_ID:
            case BSP_DUT_DEV_ID_SPI2:
                HAL_I2C_Master_Abort_IT(&i2c_drv_handle, BSP_DUT_I2C_ADDRESS_8BIT);
                break;

            default:
                break;
        }
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_enable_irq(void)
{
    __enable_irq();
    return BSP_STATUS_OK;
}

uint32_t bsp_disable_irq(void)
{
    __disable_irq();
    return BSP_STATUS_OK;
}

void bsp_sleep(void)
{
    __disable_irq();
    bsp_irq_count--;

    if (bsp_irq_count <= 0)
    {
        bsp_irq_count = 0;
        __enable_irq();
        __WFI();
    }
    else
    {
        __enable_irq();
    }

    return;
}

uint32_t bsp_spi_throttle_speed(uint32_t speed_hz)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_spi_restore_speed(void)
{
    return BSP_STATUS_FAIL;
}

void* bsp_malloc(size_t size)
{
#ifdef NO_OS
    return malloc(size);
#else
    return pvPortMalloc(size);
#endif
}

void bsp_free(void* ptr)
{
#ifdef NO_OS
    return free(ptr);
#else
    return vPortFree(ptr);
#endif
}

uint32_t bsp_set_ld2(uint8_t mode, uint32_t blink_100ms)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_set_led(uint32_t index, uint8_t mode, uint32_t blink_100ms)
{
    bsp_led_t *led = bsp_leds;

    if (index < BSP_LED_TOTAL)
    {
        led += index;
    }

    if (mode == BSP_LD2_MODE_BLINK)
    {
        led->blink_counter_100ms_max = blink_100ms;
        led->mode = BSP_LED_MODE_BLINK;
    }
    else
    {
        led->mode = BSP_LED_MODE_FIXED;
        if (mode == BSP_LD2_MODE_OFF)
        {
            bsp_set_gpio(led->id, GPIO_PIN_RESET);
            led->is_on = false;
        }
        else
        {
            bsp_set_gpio(led->id, GPIO_PIN_SET);
            led->is_on = true;
        }
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_get_switch_state(void)
{
    uint32_t temp_switch_state = 0;

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_SET)
    {
        temp_switch_state |= 0x1;
    }

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) == GPIO_PIN_SET)
    {
        temp_switch_state |= 0x2;
    }

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10) == GPIO_PIN_SET)
    {
        temp_switch_state |= 0x4;
    }

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11) == GPIO_PIN_SET)
    {
        temp_switch_state |= 0x8;
    }

    return temp_switch_state;
}

void bsp_get_switch_state_changes(uint8_t *state, uint8_t *change_mask)
{
    *state = bsp_get_switch_state();

    if (change_mask != NULL)
    {
        *change_mask = *state ^ bsp_switch_state;
    }

    bsp_switch_state = *state;

    return;
}

static bsp_driver_if_t bsp_driver_if_s =
{
    .set_gpio = &bsp_set_gpio,
    .set_supply = &bsp_set_supply,
    .register_gpio_cb = &bsp_register_gpio_cb,
    .set_timer = &bsp_set_timer,
    .i2c_read_repeated_start = &bsp_i2c_read_repeated_start,
    .i2c_write = &bsp_i2c_write,
    .i2c_db_write = &bsp_i2c_db_write,
    .spi_read = &bsp_spi_read,
    .spi_write = &bsp_spi_write,
    .i2c_reset = &bsp_i2c_reset,
    .enable_irq = &bsp_enable_irq,
    .disable_irq = &bsp_disable_irq,
    .spi_throttle_speed = &bsp_spi_throttle_speed,
    .spi_restore_speed = &bsp_spi_restore_speed
};

bsp_driver_if_t *bsp_driver_if_g = &bsp_driver_if_s;
