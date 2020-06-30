/**
 * @file hw_0_bsp.c
 *
 * @brief Implementation of the BSP for the HW ID0 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
 *
 * This code and information are provided 'as-is' without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a particular
 * purpose.
 *
 */
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stddef.h>
#include "hw_0_bsp.h"
#include "stm32f4xx_hal.h"
#include "test_tone_tables.h"

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

/* I2S peripheral configuration defines */
#define I2S_HW                          SPI2
#define I2S_CLK_ENABLE()                __HAL_RCC_SPI2_CLK_ENABLE()
#define I2S_CLK_DISABLE()               __HAL_RCC_SPI2_CLK_DISABLE()
#define I2S_LRCLK_SCLK_SDOUT_AF         GPIO_AF5_SPI2
#define I2S_SDIN_AF                     GPIO_AF6_I2S2ext
#define I2S_GPIO_PORT_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()
#define I2S_GPIO_PORT                   GPIOB
#define I2S_LRCLK_GPIO_PIN              GPIO_PIN_12
#define I2S_SCLK_GPIO_PIN               GPIO_PIN_13
#define I2S_SDOUT_GPIO_PIN              GPIO_PIN_15
#define I2S_SDIN_PIN                    GPIO_PIN_14

/* I2S DMA Stream definitions */
#define I2S_TX_DMAx_CLK_ENABLE()        __HAL_RCC_DMA1_CLK_ENABLE()
#define I2S_TX_DMAx_CLK_DISABLE()       __HAL_RCC_DMA1_CLK_DISABLE()
#define I2S_TX_DMAx_STREAM              DMA1_Stream4
#define I2S_TX_DMAx_CHANNEL             DMA_CHANNEL_0
#define I2S_TX_DMAx_IRQ                 DMA1_Stream4_IRQn
#define I2S_TX_DMAx_PERIPH_DATA_SIZE    DMA_PDATAALIGN_HALFWORD
#define I2S_TX_DMAx_MEM_DATA_SIZE       DMA_MDATAALIGN_HALFWORD
#define I2S_TX_IRQHandler               DMA1_Stream4_IRQHandler

#define I2S_RX_DMAx_CLK_ENABLE()        __HAL_RCC_DMA1_CLK_ENABLE()
#define I2S_RX_DMAx_CLK_DISABLE()       __HAL_RCC_DMA1_CLK_DISABLE()
#define I2S_RX_DMAx_STREAM              DMA1_Stream3
#define I2S_RX_DMAx_CHANNEL             DMA_CHANNEL_3
#define I2S_RX_DMAx_IRQ                 DMA1_Stream3_IRQn
#define I2S_RX_DMAx_PERIPH_DATA_SIZE    DMA_PDATAALIGN_HALFWORD
#define I2S_RX_DMAx_MEM_DATA_SIZE       DMA_MDATAALIGN_HALFWORD
#define I2S_RX_IRQHandler               DMA1_Stream3_IRQHandler

/* Select the interrupt preemption priority and subpriority for the DMA interrupt */
#define I2S_TX_IRQ_PREPRIO                      0x0E   /* Select the preemption priority level(0 is the highest) */
#define I2S_RX_IRQ_PREPRIO                      0x0F   /* Select the preemption priority level(0 is the highest) */

/* BSP Audio Format definitions */
#define BSP_I2S_STANDARD                        I2S_STANDARD_PHILIPS
#define BSP_I2S_FS_HZ                           (I2S_AUDIOFREQ_48K)
#define BSP_I2S_WORD_SIZE_BITS                  (32)
#if (BSP_I2S_WORD_SIZE_BITS == 32)
    #define BSP_I2S_DATA_FORMAT                 (I2S_DATAFORMAT_32B)
    #define BSP_I2S_SUBFRAME_SIZE_BITS          (32)
    #define BSP_I2S_2BYTES_PER_SUBFRAME         (2)
#elif (BSP_I2S_WORD_SIZE_BITS == 24)
    #define BSP_I2S_DATA_FORMAT                 (I2S_DATAFORMAT_24B)
    #define BSP_I2S_SUBFRAME_SIZE_BITS          (32)
    #define BSP_I2S_2BYTES_PER_SUBFRAME         (2)
#elif (BSP_I2S_WORD_SIZE_BITS == 16)
    #define BSP_I2S_DATA_FORMAT                 (I2S_DATAFORMAT_16B)
    #define BSP_I2S_SUBFRAME_SIZE_BITS          (16)
    #define BSP_I2S_2BYTES_PER_SUBFRAME         (1)
#else
#error "BSP_I2S_WORD_SIZE_BITS is unsupported"
#endif
#define BSP_I2S_WORD_SIZE_BYTES                 (BSP_I2S_WORD_SIZE_BITS/8)
#define BSP_I2S_SUBFRAME_SIZE_BYTES             (BSP_I2S_SUBFRAME_SIZE_BITS/8)
#define BSP_I2S_CHANNEL_NBR                     (2)

#ifdef TEST_TONES_INCLUDE_100HZ
/* Playback PCM buffer size of 10ms */
#define PLAYBACK_BUFFER_SIZE_SUBFRAMES          (BSP_I2S_FS_HZ / 100 * BSP_I2S_CHANNEL_NBR)
#else
/* Playback PCM buffer size of 1ms */
#define PLAYBACK_BUFFER_SIZE_SUBFRAMES          (BSP_I2S_FS_HZ / 1000 * BSP_I2S_CHANNEL_NBR)
#endif
#define PLAYBACK_BUFFER_SIZE_2BYTES             (PLAYBACK_BUFFER_SIZE_SUBFRAMES * BSP_I2S_2BYTES_PER_SUBFRAME)
#define BSP_I2S_DMA_SIZE                        (PLAYBACK_BUFFER_SIZE_SUBFRAMES)
#define PLAYBACK_BUFFER_DEFAULT_VALUE           (0xABCD)
#define PLAYBACK_BUFFER_DEFAULT_L_VALUE         (0x1234)
#define PLAYBACK_BUFFER_DEFAULT_R_VALUE         (0xABCD)
#define RECORD_BUFFER_SIZE_2BYTES               (PLAYBACK_BUFFER_SIZE_2BYTES)
#define RECORD_BUFFER_DEFAULT_VALUE             (0xEEEE)

#define BSP_DUT_RESET_CLK_ENABLE                __HAL_RCC_GPIOC_CLK_ENABLE
#define BSP_DUT_RESET_CLK_DISABLE               __HAL_RCC_GPIOC_CLK_DISABLE
#define BSP_DUT_RESET_PIN                       GPIO_PIN_0
#define BSP_DUT_RESET_GPIO_PORT                 GPIOC
#define BSP_DUT_INT_CLK_ENABLE                  __HAL_RCC_GPIOA_CLK_ENABLE
#define BSP_DUT_INT_CLK_DISABLE                 __HAL_RCC_GPIOA_CLK_DISABLE
#define BSP_DUT_INT_PIN                         GPIO_PIN_0
#define BSP_DUT_INT_GPIO_PORT                   GPIOA

#define BSP_GPIO_ID_LD2                         (0)

#define BSP_PB_TOTAL                            (1)

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

static uint16_t playback_buffer[PLAYBACK_BUFFER_SIZE_2BYTES];
static uint16_t record_buffer[RECORD_BUFFER_SIZE_2BYTES];
static uint16_t *playback_content;

static bool bsp_pb_pressed_flags[BSP_PB_TOTAL] = {false};
static bsp_app_callback_t bsp_pb_cbs[BSP_PB_TOTAL] = {NULL};
static void* bsp_pb_cb_args[BSP_PB_TOTAL] = {NULL};


/* These PLL parameters are valid when the f(VCO clock) = 1Mhz */
const uint32_t I2SFreq[8] = {8000, 11025, 16000, 22050, 32000, 44100, 48000, 96000};
const uint32_t I2SPLLN[8] = {256, 429, 213, 429, 426, 271, 258, 344};
const uint32_t I2SPLLR[8] = {5, 4, 4, 4, 4, 6, 3, 1};

static bsp_app_callback_t app_cb = NULL;
static void *app_cb_arg = NULL;

static volatile int32_t bsp_irq_count = 0;

static bsp_callback_t bsp_dut_int_cb = NULL;
static void *bsp_dut_int_cb_arg = NULL;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
TIM_HandleTypeDef tim_drv_handle;
I2C_HandleTypeDef i2c_drv_handle;
I2S_HandleTypeDef i2s_drv_handle;

uint32_t bsp_toggle_gpio(uint32_t gpio_id);
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

    // Configure I2S clocking
    RCC_PeriphCLKInitTypeDef rccclkinit;
    uint8_t index = 0, freqindex = 0xFF;

    for(index = 0; index < 8; index++)
    {
      if(I2SFreq[index] == BSP_I2S_FS_HZ)
      {
        freqindex = index;
      }
    }
    /* Enable PLLI2S clock */
    HAL_RCCEx_GetPeriphCLKConfig(&rccclkinit);
    /* PLLI2S_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
    if ((freqindex & 0x7) == 0)
    {
      /* I2S clock config
      PLLI2S_VCO = f(VCO clock) = f(PLLI2S clock input) * (PLLI2SN/PLLM)
      I2SCLK = f(PLLI2S clock output) = f(VCO clock) / PLLI2SR */
      rccclkinit.PeriphClockSelection = RCC_PERIPHCLK_I2S;
      rccclkinit.PLLI2S.PLLI2SN = I2SPLLN[freqindex];
      rccclkinit.PLLI2S.PLLI2SR = I2SPLLR[freqindex];
      HAL_RCCEx_PeriphCLKConfig(&rccclkinit);
    }
    else
    {
      /* I2S clock config
      PLLI2S_VCO = f(VCO clock) = f(PLLI2S clock input) * (PLLI2SN/PLLM)
      I2SCLK = f(PLLI2S clock output) = f(VCO clock) / PLLI2SR */
      rccclkinit.PeriphClockSelection = RCC_PERIPHCLK_I2S;
      rccclkinit.PLLI2S.PLLI2SN = 258;
      rccclkinit.PLLI2S.PLLI2SR = 3;
      HAL_RCCEx_PeriphCLKConfig(&rccclkinit);
    }
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

static void I2S_Init(void)
{
  i2s_drv_handle.Instance = I2S_HW;

  __HAL_I2S_DISABLE(&i2s_drv_handle);

  i2s_drv_handle.Init.AudioFreq   = BSP_I2S_FS_HZ;
  i2s_drv_handle.Init.ClockSource = I2S_CLOCK_PLL;
  i2s_drv_handle.Init.CPOL        = I2S_CPOL_LOW;
  i2s_drv_handle.Init.DataFormat  = BSP_I2S_DATA_FORMAT;
  i2s_drv_handle.Init.MCLKOutput  = I2S_MCLKOUTPUT_DISABLE;
  i2s_drv_handle.Init.Mode        = I2S_MODE_MASTER_TX;
  i2s_drv_handle.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_ENABLE;
  i2s_drv_handle.Init.Standard    = BSP_I2S_STANDARD;

  if(HAL_I2S_Init(&i2s_drv_handle) != HAL_OK)
  {
      Error_Handler();
  }

  return;
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

/***********************************************************************************************************************
 * MCU HAL FUNCTIONS
 **********************************************************************************************************************/
void HAL_MspInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable clocks to ports used
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    BSP_DUT_RESET_CLK_ENABLE();
    BSP_DUT_INT_CLK_ENABLE();

    // Configure the LD2 GPO
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure the Haptic Reset GPO
    HAL_GPIO_WritePin(BSP_DUT_RESET_GPIO_PORT, BSP_DUT_RESET_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = BSP_DUT_RESET_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BSP_DUT_RESET_GPIO_PORT, &GPIO_InitStruct);

    // Configure Haptic Interrupt GPI
    GPIO_InitStruct.Pin = BSP_DUT_INT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BSP_DUT_INT_GPIO_PORT, &GPIO_InitStruct);

    HAL_NVIC_SetPriority((IRQn_Type)EXTI0_IRQn, 0x0F, 0x00);
    HAL_NVIC_EnableIRQ((IRQn_Type)EXTI0_IRQn);

    // Configure the Push Button GPI
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    HAL_NVIC_SetPriority((IRQn_Type)EXTI15_10_IRQn, 0x0F, 0x00);
    HAL_NVIC_EnableIRQ((IRQn_Type)EXTI15_10_IRQn);

    return;
}

void HAL_MspDeInit(void)
{
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);

    HAL_GPIO_DeInit(BSP_DUT_RESET_GPIO_PORT, BSP_DUT_RESET_PIN);
    HAL_GPIO_DeInit(BSP_DUT_INT_GPIO_PORT, BSP_DUT_INT_PIN);

    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOC_CLK_DISABLE();

    BSP_DUT_RESET_CLK_DISABLE();
    BSP_DUT_INT_CLK_DISABLE();

    return;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BSP_DUT_INT_PIN)
    {
        if (bsp_dut_int_cb != NULL)
        {
            bsp_dut_int_cb(BSP_STATUS_OK, bsp_dut_int_cb_arg);
            app_cb(BSP_STATUS_DUT_EVENTS, app_cb_arg);
        }
    }

    if (GPIO_Pin == GPIO_PIN_13)
    {
        bsp_pb_pressed_flags[BSP_PB_ID_USER] = true;
        if (bsp_pb_cbs[BSP_PB_ID_USER] != NULL)
        {
            bsp_pb_cbs[BSP_PB_ID_USER](BSP_STATUS_OK, bsp_pb_cb_args[BSP_PB_ID_USER]);
        }
    }

    bsp_irq_count++;

    return;
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        __HAL_RCC_TIM2_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM2_IRQn, 4, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
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

        HAL_NVIC_SetPriority(I2C1_ER_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
        HAL_NVIC_SetPriority(I2C1_EV_IRQn, 2, 0);
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

void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s)
{
    static DMA_HandleTypeDef hdma_i2sTx;
    static DMA_HandleTypeDef hdma_i2sRx;
    GPIO_InitTypeDef  GPIO_InitStruct;

    if(hi2s->Instance == I2S_HW)
    {
        I2S_CLK_ENABLE();
        I2S_GPIO_PORT_CLK_ENABLE();

        GPIO_InitStruct.Pin         = I2S_LRCLK_GPIO_PIN | I2S_SCLK_GPIO_PIN | I2S_SDOUT_GPIO_PIN;
        GPIO_InitStruct.Mode        = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull        = GPIO_NOPULL;
        GPIO_InitStruct.Speed       = GPIO_SPEED_FAST;
        GPIO_InitStruct.Alternate   = I2S_LRCLK_SCLK_SDOUT_AF;
        HAL_GPIO_Init(I2S_GPIO_PORT, &GPIO_InitStruct);

        GPIO_InitStruct.Pin         = I2S_SDIN_PIN;
        GPIO_InitStruct.Alternate   = I2S_SDIN_AF;
        HAL_GPIO_Init(I2S_GPIO_PORT, &GPIO_InitStruct);

        I2S_TX_DMAx_CLK_ENABLE();
        I2S_RX_DMAx_CLK_ENABLE();

        hdma_i2sTx.Init.Channel             = I2S_TX_DMAx_CHANNEL;
        hdma_i2sTx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
        hdma_i2sTx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma_i2sTx.Init.MemInc              = DMA_MINC_ENABLE;
        hdma_i2sTx.Init.PeriphDataAlignment = I2S_TX_DMAx_PERIPH_DATA_SIZE;
        hdma_i2sTx.Init.MemDataAlignment    = I2S_TX_DMAx_MEM_DATA_SIZE;
        hdma_i2sTx.Init.Mode                = DMA_CIRCULAR;
        hdma_i2sTx.Init.Priority            = DMA_PRIORITY_HIGH;
        hdma_i2sTx.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
        hdma_i2sTx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
        hdma_i2sTx.Init.MemBurst            = DMA_MBURST_SINGLE;
        hdma_i2sTx.Init.PeriphBurst         = DMA_PBURST_SINGLE;
        hdma_i2sTx.Instance                 = I2S_TX_DMAx_STREAM;

        hdma_i2sRx.Init.Channel             = I2S_RX_DMAx_CHANNEL;
        hdma_i2sRx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        hdma_i2sRx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma_i2sRx.Init.MemInc              = DMA_MINC_ENABLE;
        hdma_i2sRx.Init.PeriphDataAlignment = I2S_RX_DMAx_PERIPH_DATA_SIZE;
        hdma_i2sRx.Init.MemDataAlignment    = I2S_RX_DMAx_MEM_DATA_SIZE;
        hdma_i2sRx.Init.Mode                = DMA_CIRCULAR;
        hdma_i2sRx.Init.Priority            = DMA_PRIORITY_HIGH;
        hdma_i2sRx.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
        hdma_i2sRx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
        hdma_i2sRx.Init.MemBurst            = DMA_MBURST_SINGLE;
        hdma_i2sRx.Init.PeriphBurst         = DMA_PBURST_SINGLE;
        hdma_i2sRx.Instance                 = I2S_RX_DMAx_STREAM;

        __HAL_LINKDMA(hi2s, hdmatx, hdma_i2sTx);
        HAL_DMA_DeInit(&hdma_i2sTx);
        HAL_DMA_Init(&hdma_i2sTx);

        __HAL_LINKDMA(hi2s, hdmarx, hdma_i2sRx);
        HAL_DMA_DeInit(&hdma_i2sRx);
        HAL_DMA_Init(&hdma_i2sRx);


        /* I2S DMA IRQ Channel configuration */
        HAL_NVIC_SetPriority(I2S_TX_DMAx_IRQ, I2S_TX_IRQ_PREPRIO, 0);
        HAL_NVIC_EnableIRQ(I2S_TX_DMAx_IRQ);

        HAL_NVIC_SetPriority(I2S_RX_DMAx_IRQ, I2S_RX_IRQ_PREPRIO, 0);
        HAL_NVIC_EnableIRQ(I2S_RX_DMAx_IRQ);
    }

    return;
}

void HAL_I2S_MspDeInit(I2S_HandleTypeDef *hi2s)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    HAL_NVIC_DisableIRQ(I2S_TX_DMAx_IRQ);
    HAL_NVIC_DisableIRQ(I2S_RX_DMAx_IRQ);

    if(hi2s->Instance == I2S_HW)
    {
        HAL_DMA_DeInit(hi2s->hdmatx);
        HAL_DMA_DeInit(hi2s->hdmarx);
    }

    __HAL_I2S_DISABLE(hi2s);

    GPIO_InitStruct.Pin = I2S_LRCLK_GPIO_PIN | I2S_SCLK_GPIO_PIN | I2S_SDOUT_GPIO_PIN | I2S_SDIN_PIN;
    HAL_GPIO_DeInit(I2S_GPIO_PORT, GPIO_InitStruct.Pin);

    I2S_CLK_DISABLE();

    return;
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if(hi2s->Instance == I2S_HW)
    {
        bsp_audio_play(0);
    }

    bsp_irq_count++;

    return;
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    return;
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    return;
}

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    return;
}

void HAL_I2SEx_TxRxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    return;
}

void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if(hi2s->Instance == I2S_HW)
    {
        ;
    }

    bsp_irq_count++;

    return;
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    if(hi2s->Instance == I2S_HW)
    {
        Error_Handler();
    }

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

    /* Initialize all peripheral drivers */
    Timer_Init();
    I2C_Init();
    I2S_Init();

    // Initialize playback buffer
    for (int i = 0; i < PLAYBACK_BUFFER_SIZE_2BYTES;)
    {
        record_buffer[i] = RECORD_BUFFER_DEFAULT_VALUE;
        playback_buffer[i] = i;
        i++;
        record_buffer[i] = RECORD_BUFFER_DEFAULT_VALUE;
        playback_buffer[i] = i;
        i++;
    }

    playback_content = playback_buffer;

    bsp_timer_cb = NULL;
    bsp_timer_cb_arg = NULL;
    bsp_timer_has_started = false;
    bsp_i2c_done_cb = NULL;
    bsp_i2c_done_cb_arg = NULL;
    bsp_i2c_current_transaction_type = BSP_I2C_TRANSACTION_TYPE_INVALID;

    for (int i = 0; i < BSP_PB_TOTAL; i++)
    {
        bsp_pb_pressed_flags[i] = false;
    }

    return BSP_STATUS_OK;
}

void bsp_notification_callback(uint32_t event_flags, void *arg)
{
    bsp_toggle_gpio(BSP_GPIO_ID_LD2);
    return;
}

uint32_t bsp_audio_play(uint8_t content)
{
    switch (content)
    {
        case BSP_PLAY_SILENCE:
#if (BSP_I2S_2BYTES_PER_SUBFRAME == 2)
            playback_content = (uint16_t *) pcm_silence_32bit_stereo_single_period;
#else
            playback_content = pcm_silence_16bit_stereo_single_period;
#endif
            break;

        case BSP_PLAY_STEREO_1KHZ_20DBFS:
#if (BSP_I2S_2BYTES_PER_SUBFRAME == 2)
            playback_content = (uint16_t *) pcm_20dBFs_1kHz_32bit_stereo_single_period;
#else
            playback_content = pcm_20dBFs_1kHz_16bit_stereo_single_period;
#endif
            break;

        case BSP_PLAY_STEREO_100HZ_20DBFS:
#ifdef TEST_TONES_INCLUDE_100HZ
#if (BSP_I2S_2BYTES_PER_SUBFRAME == 2)
            playback_content = pcm_20dBFs_100Hz_32bit_stereo_single_period;
#else
            playback_content = pcm_20dBFs_100Hz_16bit_stereo_single_period;
#endif
#else
            return BSP_STATUS_FAIL;
#endif
            break;

        default:
        case BSP_PLAY_STEREO_PATTERN:
            playback_content = playback_buffer;
            break;
    }

    if (HAL_OK == HAL_I2S_Transmit_DMA(&i2s_drv_handle, playback_content, BSP_I2S_DMA_SIZE))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_audio_record(void)
{
    if (HAL_OK == HAL_I2S_Receive_DMA(&i2s_drv_handle, record_buffer, BSP_I2S_DMA_SIZE))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_audio_play_record(uint8_t content)
{
    uint16_t dma_transfer_size = 0;
    switch (content)
    {
        case BSP_PLAY_SILENCE:
            dma_transfer_size = PCM_1KHZ_SINGLE_PERIOD_LENGTH_2BYTES;
#if (BSP_I2S_2BYTES_PER_SUBFRAME == 2)
            playback_content = (uint16_t *) pcm_silence_32bit_stereo_single_period;
#else
            playback_content = pcm_silence_16bit_stereo_single_period;
#endif
            break;

        case BSP_PLAY_STEREO_1KHZ_20DBFS:
            dma_transfer_size = PCM_1KHZ_SINGLE_PERIOD_LENGTH_2BYTES;
#if (BSP_I2S_2BYTES_PER_SUBFRAME == 2)
            playback_content = (uint16_t *) pcm_20dBFs_1kHz_32bit_stereo_single_period;
#else
            playback_content = pcm_20dBFs_1kHz_16bit_stereo_single_period;
#endif
            break;

        case BSP_PLAY_STEREO_100HZ_20DBFS:
#ifdef TEST_TONES_INCLUDE_100HZ
#if (BSP_I2S_2BYTES_PER_SUBFRAME == 2)
            playback_content = pcm_20dBFs_100Hz_32bit_stereo_single_period;
#else
            playback_content = pcm_20dBFs_100Hz_16bit_stereo_single_period;
#endif
#else
            return BSP_STATUS_FAIL;
#endif
            break;

        default:
        case BSP_PLAY_STEREO_PATTERN:
            playback_content = playback_buffer;
            break;
    }

    if (HAL_OK == HAL_I2SEx_TransmitReceive_DMA(&i2s_drv_handle, playback_content, record_buffer, dma_transfer_size))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_audio_pause(void)
{
    if (HAL_OK == HAL_I2S_DMAPause(&i2s_drv_handle))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_audio_resume(void)
{
    if (HAL_OK == HAL_I2S_DMAResume(&i2s_drv_handle))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_audio_stop(void)
{
    if (HAL_OK == HAL_I2S_DMAStop(&i2s_drv_handle))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

bool bsp_was_pb_pressed(uint8_t pb_id)
{
    bool ret = bsp_pb_pressed_flags[pb_id];
    if (ret)
    {
        bsp_pb_pressed_flags[pb_id] = false;
    }

    return ret;
}

uint32_t bsp_set_gpio(uint32_t gpio_id, uint8_t gpio_state)
{
    switch (gpio_id)
    {
        case BSP_GPIO_ID_LD2:
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, (GPIO_PinState) gpio_state);
            break;

        case BSP_GPIO_ID_DUT_RESET:
            HAL_GPIO_WritePin(BSP_DUT_RESET_GPIO_PORT, BSP_DUT_RESET_PIN, (GPIO_PinState) gpio_state);
            break;

        default:
            break;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_toggle_gpio(uint32_t gpio_id)
{
    switch (gpio_id)
    {
        case BSP_GPIO_ID_LD2:
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            break;

        default:
            break;
    }

    return BSP_STATUS_OK;
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
        while (!bsp_timer_elapsed);
    }

    return BSP_STATUS_OK;
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
            bsp_i2c_done_cb = cb;
            bsp_i2c_done_cb_arg = cb_arg;
            bsp_i2c_current_transaction_type = BSP_I2C_TRANSACTION_TYPE_READ_REPEATED_START;
            bsp_i2c_read_buffer_ptr = read_buffer;
            bsp_i2c_read_length = read_length;
            bsp_i2c_read_address = BSP_DUT_I2C_ADDRESS_8BIT;
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

    return BSP_STATUS_OK;
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
                                           BSP_DUT_I2C_ADDRESS_8BIT,
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

#ifdef BSP_LN2_DEV_ID
        case BSP_LN2_DEV_ID:
            bsp_i2c_transaction_complete = false;
            bsp_i2c_transaction_error = false;
            bsp_i2c_done_cb = cb;
            bsp_i2c_done_cb_arg = cb_arg;
            bsp_i2c_current_transaction_type = BSP_I2C_TRANSACTION_TYPE_WRITE;
            HAL_I2C_Master_Seq_Transmit_IT(&i2c_drv_handle,
                                           0x44,
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
#endif

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
            bsp_i2c_read_address = BSP_DUT_I2C_ADDRESS_8BIT;
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
    bsp_dut_int_cb = cb;
    bsp_dut_int_cb_arg = cb_arg;

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

uint32_t bsp_register_pb_cb(uint32_t pb_id, bsp_app_callback_t cb, void *cb_arg)
{
    if (pb_id < BSP_PB_TOTAL)
    {
        bsp_pb_cbs[pb_id] = cb;
        bsp_pb_cb_args[pb_id] = cb_arg;
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

static bsp_driver_if_t bsp_driver_if_s =
{
    .set_gpio = &bsp_set_gpio,
    .toggle_gpio = &bsp_toggle_gpio,
    .register_gpio_cb = &bsp_register_gpio_cb,
    .set_timer = &bsp_set_timer,
    .i2c_read_repeated_start = &bsp_i2c_read_repeated_start,
    .i2c_write = &bsp_i2c_write,
    .i2c_db_write = &bsp_i2c_db_write,
    .i2c_reset = &bsp_i2c_reset,
    .enable_irq = &bsp_enable_irq,
    .disable_irq = &bsp_disable_irq,
};

bsp_driver_if_t *bsp_driver_if_g = &bsp_driver_if_s;
