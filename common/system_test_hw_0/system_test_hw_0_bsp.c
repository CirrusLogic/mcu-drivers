/**
 * @file system_test_hw_0_bsp.c
 *
 * @brief Implementation of the BSP for the system_test_hw_0 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019 All Rights Reserved, http://www.cirrus.com/
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
#include "system_test_hw_0_bsp.h"
#include "stm32f4xx_hal.h"
#ifdef TARGET_CS35L41
#include "cs35l41.h"
#endif
#if TARGET_CS40L25
#include "cs40l25.h"
#endif
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

#ifdef TARGET_CS35L41
#define BSP_AMP_RESET_CLK_ENABLE                __HAL_RCC_GPIOC_CLK_ENABLE
#define BSP_AMP_RESET_CLK_DISABLE               __HAL_RCC_GPIOC_CLK_DISABLE
#define BSP_AMP_RESET_PIN                       GPIO_PIN_0
#define BSP_AMP_RESET_GPIO_PORT                 GPIOC
#define BSP_AMP_INT_CLK_ENABLE                  __HAL_RCC_GPIOA_CLK_ENABLE
#define BSP_AMP_INT_CLK_DISABLE                 __HAL_RCC_GPIOA_CLK_DISABLE
#define BSP_AMP_INT_PIN                         GPIO_PIN_0
#define BSP_AMP_INT_GPIO_PORT                   GPIOA
#endif
#ifdef TARGET_CS40L25
#define BSP_HAPTIC_RESET_CLK_ENABLE             __HAL_RCC_GPIOC_CLK_ENABLE
#define BSP_HAPTIC_RESET_CLK_DISABLE            __HAL_RCC_GPIOC_CLK_DISABLE
#define BSP_HAPTIC_RESET_PIN                    GPIO_PIN_0
#define BSP_HAPTIC_RESET_GPIO_PORT              GPIOC
#define BSP_HAPTIC_INT_CLK_ENABLE               __HAL_RCC_GPIOA_CLK_ENABLE
#define BSP_HAPTIC_INT_CLK_DISABLE              __HAL_RCC_GPIOA_CLK_DISABLE
#define BSP_HAPTIC_INT_PIN                      GPIO_PIN_0
#define BSP_HAPTIC_INT_GPIO_PORT                GPIOA
#endif

#define BSP_PB_TOTAL                            (1)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
#ifdef TARGET_CS35L41
static cs35l41_t amp_driver;
static uint8_t bsp_amp_boot_status;
static uint32_t bsp_amp_volume = CS35L41_AMP_VOLUME_0DB;
#endif
#ifdef TARGET_CS40L25
static cs40l25_t haptic_driver;
static uint8_t bsp_haptic_control_status;
static uint32_t bsp_haptic_volume = CS40L25_AMP_VOLUME_0DB;
#endif

static bsp_callback_t bsp_timer_cb;
static void *bsp_timer_cb_arg;
static bool bsp_timer_has_started;

static uint8_t transmit_buffer[32];
static uint8_t receive_buffer[256];
static bsp_callback_t bsp_i2c_done_cb;
static void *bsp_i2c_done_cb_arg;
static uint8_t bsp_i2c_current_transaction_type;
static uint8_t *bsp_i2c_read_buffer_ptr;
static uint32_t bsp_i2c_read_length;
static uint8_t bsp_i2c_read_address;
static uint32_t bsp_i2c_write_length;
static uint8_t *bsp_i2c_write_buffer_ptr;

static uint16_t playback_buffer[PLAYBACK_BUFFER_SIZE_2BYTES];
static uint16_t record_buffer[RECORD_BUFFER_SIZE_2BYTES];
#ifdef TARGET_CS35L41
static bsp_callback_t bsp_amp_int_cb;
static void *bsp_amp_int_cb_arg;
#endif
#ifdef TARGET_CS40L25
static bsp_callback_t bsp_haptic_int_cb;
static void *bsp_haptic_int_cb_arg;
static cs40l25_fw_revision_t fw_revision;
static cs40l25_dynamic_f0_table_entry_t dynamic_f0;
static uint32_t dynamic_redc;
#endif
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

#ifdef TARGET_CS35L41
static cs35l41_boot_config_t amp_boot_config;
#endif
#ifdef TARGET_CS40L25
static cs40l25_boot_config_t haptic_boot_config;
#endif

static volatile int32_t bsp_irq_count = 0;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
TIM_HandleTypeDef tim_drv_handle;
I2C_HandleTypeDef i2c_drv_handle;
I2S_HandleTypeDef i2s_drv_handle;

uint32_t bsp_i2c_read_repeated_start(uint32_t bsp_dev_id,
                                     uint8_t *write_buffer,
                                     uint32_t write_length,
                                     uint8_t *read_buffer,
                                     uint32_t read_length,
                                     bsp_callback_t cb,
                                     void *cb_arg);

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
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
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
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
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
#ifdef TARGET_CS35L41
    BSP_AMP_RESET_CLK_ENABLE();
    BSP_AMP_INT_CLK_ENABLE();
#endif
#ifdef TARGET_CS40L25
    BSP_HAPTIC_RESET_CLK_ENABLE();
    BSP_HAPTIC_INT_CLK_ENABLE();
#endif

    // Configure the LD2 GPO
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

#ifdef TARGET_CS35L41
    // Configure the Amp Reset GPO
    HAL_GPIO_WritePin(BSP_AMP_RESET_GPIO_PORT, BSP_AMP_RESET_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = BSP_AMP_RESET_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BSP_AMP_RESET_GPIO_PORT, &GPIO_InitStruct);

    // Configure Amp Interrupt GPI
    GPIO_InitStruct.Pin = BSP_AMP_INT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BSP_AMP_INT_GPIO_PORT, &GPIO_InitStruct);
#endif

#ifdef TARGET_CS40L25
    // Configure the Haptic Reset GPO
    HAL_GPIO_WritePin(BSP_HAPTIC_RESET_GPIO_PORT, BSP_HAPTIC_RESET_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = BSP_HAPTIC_RESET_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BSP_HAPTIC_RESET_GPIO_PORT, &GPIO_InitStruct);

    // Configure Haptic Interrupt GPI
    GPIO_InitStruct.Pin = BSP_HAPTIC_INT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BSP_HAPTIC_INT_GPIO_PORT, &GPIO_InitStruct);
#endif

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

#ifdef TARGET_CS35L41
    HAL_GPIO_DeInit(BSP_AMP_RESET_GPIO_PORT, BSP_AMP_RESET_PIN);
    HAL_GPIO_DeInit(BSP_AMP_INT_GPIO_PORT, BSP_AMP_INT_PIN);
#endif
#ifdef TARGET_CS40L25
    HAL_GPIO_DeInit(BSP_HAPTIC_RESET_GPIO_PORT, BSP_HAPTIC_RESET_PIN);
    HAL_GPIO_DeInit(BSP_HAPTIC_INT_GPIO_PORT, BSP_HAPTIC_INT_PIN);
#endif

    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOC_CLK_DISABLE();
#ifdef TARGET_CS35L41
    BSP_AMP_RESET_CLK_DISABLE();
    BSP_AMP_INT_CLK_DISABLE();
#endif
#ifdef TARGET_CS40L25
    BSP_HAPTIC_RESET_CLK_DISABLE();
    BSP_HAPTIC_INT_CLK_DISABLE();
#endif

    return;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
#ifdef TARGET_CS35L41
        if (bsp_amp_int_cb != NULL)
        {
            bsp_amp_int_cb(BSP_STATUS_OK, bsp_amp_int_cb_arg);
        }
#endif
#ifdef TARGET_CS40L25
        if (bsp_haptic_int_cb != NULL)
        {
            bsp_haptic_int_cb(BSP_STATUS_OK, bsp_haptic_int_cb_arg);
        }
#endif
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
        if ((bsp_timer_has_started) && (bsp_timer_cb != NULL))
        {
            if(HAL_TIM_Base_Stop_IT(&tim_drv_handle) != HAL_OK)
            {
              Error_Handler();
            }

            bsp_timer_cb(BSP_STATUS_OK, bsp_timer_cb_arg);
            bsp_timer_cb = NULL;
            bsp_timer_cb_arg = NULL;
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
            if (bsp_i2c_done_cb != NULL)
            {
                bsp_i2c_done_cb(BSP_STATUS_OK, bsp_i2c_done_cb_arg);
            }
        }
        else if (bsp_i2c_current_transaction_type == BSP_I2C_TRANSACTION_TYPE_DB_WRITE)
        {
            if (bsp_i2c_write_length == 0)
            {
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
    bsp_i2c_done_cb(BSP_STATUS_FAIL, bsp_i2c_done_cb_arg);

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

uint32_t bsp_initialize(bsp_app_callback_t *cb, void *cb_arg)
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

#ifdef TARGET_CS35L41
uint32_t bsp_amp_initialize(void)
{
    uint32_t ret = BSP_STATUS_OK;
    uint32_t amp_status;
    cs35l41_config_t amp_config;

    bsp_amp_boot_status = 0;
    bsp_amp_int_cb = NULL;
    bsp_amp_int_cb_arg = NULL;

    memset(&amp_config, 0, sizeof(cs35l41_config_t));

    // Initialize chip drivers
    amp_status = cs35l41_functions_g->initialize(&amp_driver);
    if (amp_status == CS35L41_STATUS_OK)
    {
        amp_config.bsp_dev_id = BSP_AMP_DEV_ID;
        amp_config.bsp_reset_gpio_id = BSP_GPIO_ID_CS35L41_RESET;
        amp_config.bus_type = CS35L41_BUS_TYPE_I2C;
        amp_config.cp_write_buffer = transmit_buffer;
        amp_config.cp_read_buffer = receive_buffer;
        amp_config.notification_cb = &bsp_notification_callback;
        amp_config.notification_cb_arg = NULL;

        // Set all defaults
        amp_config.audio_config.hw.amp_dre_en = true;
        amp_config.audio_config.hw.amp_ramp_pcm = 0;
        amp_config.audio_config.hw.bclk_inv = false;
        amp_config.audio_config.hw.dout_hiz_ctrl = 0x2;
        amp_config.audio_config.hw.fsync_inv = false;
        amp_config.audio_config.hw.is_master_mode = false;
        amp_config.audio_config.hw.ng_enable = false;

        amp_config.audio_config.clock.global_fs = 48000;
        amp_config.audio_config.clock.refclk_freq = 3072000;
        amp_config.audio_config.clock.sclk = 3072000;
        amp_config.audio_config.clock.refclk_sel = CS35L41_PLL_REFLCLK_SEL_BCLK;

        amp_config.audio_config.asp.is_i2s = true;
        amp_config.audio_config.asp.rx_width = 32;
        amp_config.audio_config.asp.rx_wl = 24;
        amp_config.audio_config.asp.tx_width = 32;
        amp_config.audio_config.asp.tx_wl = 24;
        amp_config.audio_config.asp.rx1_slot = 0;
        amp_config.audio_config.asp.rx2_slot = 1;
        amp_config.audio_config.asp.tx1_slot = 0;
        amp_config.audio_config.asp.tx2_slot = 1;

        amp_config.audio_config.volume = 0;

        amp_config.audio_config.routing.dac_src = CS35L41_INPUT_SRC_DSP1TX1;
        amp_config.audio_config.routing.asp_tx1_src = CS35L41_INPUT_SRC_VMON;
        amp_config.audio_config.routing.asp_tx2_src = CS35L41_INPUT_SRC_IMON;
        amp_config.audio_config.routing.asp_tx3_src = CS35L41_INPUT_SRC_DISABLE;
        amp_config.audio_config.routing.asp_tx4_src = CS35L41_INPUT_SRC_DISABLE;
        amp_config.audio_config.routing.dsp_rx1_src = CS35L41_INPUT_SRC_ASPRX1;
        amp_config.audio_config.routing.dsp_rx2_src = CS35L41_INPUT_SRC_DISABLE;

        amp_config.amp_config.boost_inductor_value_nh = 1000;   // 1uH on Prince DC
        amp_config.amp_config.boost_capacitor_value_uf = 10;    // 10uF on Prince DC
        amp_config.amp_config.boost_ipeak_ma = 2000;
        amp_config.amp_config.bst_ctl = 0;  // VBST = VP
        amp_config.amp_config.classh_enable = true;
        amp_config.amp_config.bst_ctl_sel = 1;  // Class-H tracking
        amp_config.amp_config.bst_ctl_lim_en = false;
        amp_config.amp_config.ch_mem_depth = 5; // 333.33 - 335.93 us
        amp_config.amp_config.ch_hd_rm = 0xB; // 1.1V
        amp_config.amp_config.ch_rel_rate = 0x4; // 20us
        amp_config.amp_config.wkfet_amp_delay = 0x4; // 100ms
        amp_config.amp_config.wkfet_amp_thld = 0x1; // 0.05V
        amp_config.amp_config.temp_warn_thld = 0x2; // 125C

        amp_config.cal_data.is_valid = false;

        amp_status = cs35l41_functions_g->configure(&amp_driver, &amp_config);
    }

    if (amp_status != CS35L41_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }

    return ret;
}

void bsp_amp_control_callback(uint8_t id, uint32_t status, void *arg)
{
    if ((id == CS35L41_CONTROL_ID_CONFIGURE) ||
        (id == CS35L41_CONTROL_ID_POWER_UP) ||
        (id == CS35L41_CONTROL_ID_POWER_DOWN) ||
        (id == CS35L41_CONTROL_ID_GET_VOLUME) ||
        (id == CS35L41_CONTROL_ID_SET_VOLUME) ||
        (id == CS35L41_CONTROL_ID_GET_HALO_HEARTBEAT) ||
        (id == CS35L41_CONTROL_ID_CALIBRATION) ||
        (id == CS35L41_CONTROL_ID_GET_DSP_STATUS))
    {
        if (app_cb == NULL)
        {
            if (status == CS35L41_STATUS_OK)
            {
                bsp_amp_boot_status = 1;
            }
            else
            {
                bsp_amp_boot_status = 2;
            }
        }
        else
        {
            uint32_t bsp_status = (status == CS35L41_STATUS_OK) ? BSP_STATUS_OK : BSP_STATUS_FAIL;
            app_cb(bsp_status, app_cb_arg);
        }
    }

    return;
}

uint32_t bsp_amp_boot(uint8_t boot_type)
{
    uint32_t amp_status;

    if (boot_type == BOOT_AMP_TYPE_NO_FW)
    {
        amp_boot_config.fw_blocks = NULL;
    }
    else
    {
        amp_boot_config.total_fw_blocks = cs35l41_total_fw_blocks;
        amp_boot_config.fw_blocks = cs35l41_fw_blocks;
    }

    if (boot_type == BOOT_AMP_TYPE_NO_TUNE)
    {
        amp_boot_config.coeff_blocks = NULL;
    }
    else
    {
        if (boot_type == BOOT_AMP_TYPE_NORMAL_TUNE)
        {
            amp_boot_config.total_coeff_blocks = cs35l41_total_coeff_blocks;
            amp_boot_config.coeff_blocks = cs35l41_coeff_blocks;
        }
        else
        {
            amp_boot_config.total_coeff_blocks = cs35l41_total_calibration_coeff_blocks;
            amp_boot_config.coeff_blocks = cs35l41_calibration_coeff_blocks;
        }
    }

    amp_driver.boot_config = &amp_boot_config;

    bsp_amp_boot_status = 0;

    amp_status = cs35l41_functions_g->boot(&amp_driver, bsp_amp_control_callback, NULL);

    if ((amp_status == CS35L41_STATUS_OK) && (app_cb == NULL))
    {
        while (bsp_amp_boot_status == 0)
        {
            cs35l41_functions_g->process(&amp_driver);
        }

        if (bsp_amp_boot_status == 1)
        {
            amp_status = BSP_STATUS_OK;
        }
        else
        {
            amp_status = BSP_STATUS_FAIL;
        }
    }

    return amp_status;
}

uint32_t bsp_amp_calibrate(void)
{
    uint32_t amp_status;

    bsp_amp_boot_status = 0;

    amp_status = cs35l41_functions_g->calibrate(&amp_driver, 23, bsp_amp_control_callback, NULL);

    if ((amp_status == CS35L41_STATUS_OK) && (app_cb == NULL))
    {
        while (bsp_amp_boot_status == 0)
        {
            cs35l41_functions_g->process(&amp_driver);
        }

        if (bsp_amp_boot_status == 1)
        {
            amp_status = BSP_STATUS_OK;
        }
        else
        {
            amp_status = BSP_STATUS_FAIL;
        }
    }

    return amp_status;
}

uint32_t bsp_amp_power_up(void)
{
    uint32_t amp_status;

    bsp_amp_boot_status = 0;
    amp_status = cs35l41_functions_g->power(&amp_driver, CS35L41_POWER_UP, bsp_amp_control_callback, NULL);

    if ((amp_status == CS35L41_STATUS_OK) && (app_cb == NULL))
    {
        while (bsp_amp_boot_status == 0)
        {
            cs35l41_functions_g->process(&amp_driver);
        }

        if (bsp_amp_boot_status == 1)
        {
            amp_status = BSP_STATUS_OK;
        }
        else
        {
            amp_status = BSP_STATUS_FAIL;
        }
    }

    return amp_status;
}

uint32_t bsp_amp_power_down(void)
{
    uint32_t amp_status;

    bsp_amp_boot_status = 0;
    amp_status = cs35l41_functions_g->power(&amp_driver, CS35L41_POWER_DOWN, bsp_amp_control_callback, NULL);

    if ((amp_status == CS35L41_STATUS_OK) && (app_cb == NULL))
    {
        while (bsp_amp_boot_status == 0)
        {
            cs35l41_functions_g->process(&amp_driver);
        }

        if (bsp_amp_boot_status == 1)
        {
            amp_status = BSP_STATUS_OK;
        }
        else
        {
            amp_status = BSP_STATUS_FAIL;
        }
    }

    return amp_status;
}

uint32_t bsp_amp_mute(bool is_mute)
{
    uint32_t amp_status;

    bsp_amp_boot_status = 0;
    cs35l41_control_request_t req;

    bsp_amp_boot_status = 0;
    req.id = CS35L41_CONTROL_ID_SET_VOLUME;
    if (is_mute)
    {
        req.arg = CS35L41_AMP_VOLUME_MUTE;
    }
    else
    {
        req.arg = bsp_amp_volume;
    }
    req.cb = bsp_amp_control_callback;
    req.cb_arg = NULL;
    amp_status = cs35l41_functions_g->control(&amp_driver, req);

    if ((amp_status == CS35L41_STATUS_OK) && (app_cb == NULL))
    {
        while (bsp_amp_boot_status == 0)
        {
            cs35l41_functions_g->process(&amp_driver);
        }

        if (bsp_amp_boot_status == 1)
        {
            amp_status = BSP_STATUS_OK;
        }
        else
        {
            amp_status = BSP_STATUS_FAIL;
        }
    }

    return amp_status;
}

uint32_t bsp_amp_is_processing(bool *is_processing)
{
    uint32_t amp_status;
    cs35l41_dsp_status_t status;
    cs35l41_control_request_t req;

    status.is_calibration_applied = false;
    status.is_hb_inc = false;
    status.is_temp_changed = false;

    bsp_amp_boot_status = 0;

    req.id = CS35L41_CONTROL_ID_GET_DSP_STATUS;
    req.arg = &status;
    req.cb = bsp_amp_control_callback;
    req.cb_arg = NULL;
    bsp_amp_boot_status = 0;
    amp_status = cs35l41_functions_g->control(&amp_driver, req);

    if ((amp_status == CS35L41_STATUS_OK) && (app_cb == NULL))
    {
        while (bsp_amp_boot_status == 0)
        {
            cs35l41_functions_g->process(&amp_driver);
        }

        if (bsp_amp_boot_status == 2)
        {
            amp_status = BSP_STATUS_FAIL;
        }
    }
    else
    {
        amp_status = BSP_STATUS_FAIL;
    }

    *is_processing = status.is_hb_inc;

    return amp_status;
}
#endif

#ifdef TARGET_CS40L25
uint32_t bsp_haptic_initialize(uint8_t boot_type)
{
    uint32_t ret = BSP_STATUS_OK;
    uint32_t haptic_status;
    cs40l25_config_t haptic_config;

    bsp_haptic_control_status = 0;
    bsp_haptic_int_cb = NULL;
    bsp_haptic_int_cb_arg = NULL;

    memset(&haptic_config, 0, sizeof(cs40l25_config_t));

    // Initialize chip drivers
    haptic_status = cs40l25_functions_g->initialize(&haptic_driver);
    if (haptic_status == CS40L25_STATUS_OK)
    {
        uint32_t coeff_file_no = 0;

        haptic_config.bsp_dev_id = BSP_AMP_DEV_ID;
        haptic_config.bsp_reset_gpio_id = BSP_GPIO_ID_CS40L25_RESET;
        haptic_config.bus_type = CS40L25_BUS_TYPE_I2C;
        haptic_config.cp_write_buffer = transmit_buffer;
        haptic_config.cp_read_buffer = receive_buffer;
        haptic_config.notification_cb = &bsp_notification_callback;
        haptic_config.notification_cb_arg = NULL;

        // Set all defaults
        haptic_config.audio_config.hw.amp_dre_en = false;
        haptic_config.audio_config.hw.amp_ramp_pcm = 0;
        haptic_config.audio_config.hw.bclk_inv = false;
        haptic_config.audio_config.hw.fsync_inv = false;
        haptic_config.audio_config.hw.is_master_mode = false;
        haptic_config.audio_config.hw.ng_enable = false;

        haptic_config.audio_config.clock.global_fs = 48000;
        haptic_config.audio_config.clock.refclk_freq = 32768;
        haptic_config.audio_config.clock.sclk = 3072000;
        haptic_config.audio_config.clock.refclk_sel = CS40L25_PLL_REFLCLK_SEL_MCLK;

        haptic_config.audio_config.asp.is_i2s = true;
        haptic_config.audio_config.asp.rx_width = 32;
        haptic_config.audio_config.asp.rx_wl = 24;
        haptic_config.audio_config.asp.tx_width = 32;
        haptic_config.audio_config.asp.tx_wl = 24;
        haptic_config.audio_config.asp.rx1_slot = 0;
        haptic_config.audio_config.asp.rx2_slot = 1;
        haptic_config.audio_config.asp.tx1_slot = 0;
        haptic_config.audio_config.asp.tx2_slot = 1;

        haptic_config.audio_config.volume = 0x3E;

        haptic_config.audio_config.routing.dac_src = CS40L25_INPUT_SRC_DSP1TX1;
        haptic_config.audio_config.routing.asp_tx1_src = CS40L25_INPUT_SRC_DISABLE;
        haptic_config.audio_config.routing.asp_tx2_src = CS40L25_INPUT_SRC_DISABLE;
        haptic_config.audio_config.routing.asp_tx3_src = CS40L25_INPUT_SRC_DISABLE;
        haptic_config.audio_config.routing.asp_tx4_src = CS40L25_INPUT_SRC_DISABLE;
        haptic_config.audio_config.routing.dsp_rx1_src = CS40L25_INPUT_SRC_DISABLE;
        haptic_config.audio_config.routing.dsp_rx2_src = CS40L25_INPUT_SRC_VMON;
        haptic_config.audio_config.routing.dsp_rx3_src = CS40L25_INPUT_SRC_IMON;
        haptic_config.audio_config.routing.dsp_rx4_src = CS40L25_INPUT_SRC_VPMON;

        haptic_config.amp_config.boost_inductor_value_nh = 1000;   // 1uH on Prince DC
        haptic_config.amp_config.boost_capacitor_value_uf = 10;    // 10uF on Prince DC
        haptic_config.amp_config.boost_ipeak_ma = 4500;
        haptic_config.amp_config.bst_ctl = 0xaa;
        haptic_config.amp_config.classh_enable = true;
        haptic_config.amp_config.bst_ctl_sel = 1;  // Class-H tracking
        haptic_config.amp_config.bst_ctl_lim_en = true;

        haptic_config.amp_config.wksrc_gpio1_en = true;
        haptic_config.amp_config.wksrc_sda_en = true;
        haptic_config.amp_config.wksrc_sda_falling_edge = true;

        haptic_config.cal_data.is_valid_f0 = false;
        haptic_config.cal_data.is_valid_qest = false;

        haptic_config.dsp_config_ctrls.dsp_gpio1_button_detect_enable = true;
        haptic_config.dsp_config_ctrls.dsp_gpio2_button_detect_enable = true;
        haptic_config.dsp_config_ctrls.dsp_gpio3_button_detect_enable = true;
        haptic_config.dsp_config_ctrls.dsp_gpio4_button_detect_enable = true;
        haptic_config.dsp_config_ctrls.dsp_gpio_enable = true;
        haptic_config.dsp_config_ctrls.dsp_gpi_gain_control = 0;
        haptic_config.dsp_config_ctrls.dsp_ctrl_gain_control = 0;
        haptic_config.dsp_config_ctrls.dsp_gpio1_index_button_press = 1;
        haptic_config.dsp_config_ctrls.dsp_gpio2_index_button_press = 1;
        haptic_config.dsp_config_ctrls.dsp_gpio3_index_button_press = 1;
        haptic_config.dsp_config_ctrls.dsp_gpio4_index_button_press = 1;
        haptic_config.dsp_config_ctrls.dsp_gpio1_index_button_release = 2;
        haptic_config.dsp_config_ctrls.dsp_gpio2_index_button_release = 2;
        haptic_config.dsp_config_ctrls.dsp_gpio3_index_button_release = 2;
        haptic_config.dsp_config_ctrls.dsp_gpio4_index_button_release = 2;

        haptic_config.dsp_config_ctrls.clab_enable = true;
        haptic_config.dsp_config_ctrls.peak_amplitude = 0x400000;

        haptic_config.event_control.hardware = 1;
        haptic_config.event_control.playback_end_suspend = 1;

        haptic_status = cs40l25_functions_g->configure(&haptic_driver, &haptic_config);


#ifdef INCLUDE_CAL
        if (boot_type & BOOT_HAPTIC_TYPE_CAL) {
            haptic_boot_config.total_cal_blocks = cs40l25_cal_total_fw_blocks;
            haptic_boot_config.cal_blocks = cs40l25_cal_fw_blocks;
        }
#endif // INCLUDE_CAL

        if (boot_type & BOOT_HAPTIC_TYPE_NO_BIN) {
            haptic_boot_config.coeff_files[0].data = NULL;
            haptic_boot_config.coeff_files[0].total_blocks = 0;
            haptic_boot_config.total_coeff_blocks = 0;
        }
        else
        {
            if (boot_type & BOOT_HAPTIC_TYPE_WT) {
                haptic_boot_config.coeff_files[coeff_file_no].data = cs40l25_coeff_blocks_wt;
                haptic_boot_config.coeff_files[coeff_file_no].total_blocks = cs40l25_total_coeff_blocks_wt;
                haptic_boot_config.total_coeff_blocks = cs40l25_total_coeff_blocks_wt;
                coeff_file_no++;
            }

            if (boot_type & BOOT_HAPTIC_TYPE_CLAB) {
                haptic_boot_config.coeff_files[coeff_file_no].data = cs40l25_coeff_blocks_clab;
                haptic_boot_config.coeff_files[coeff_file_no].total_blocks = cs40l25_total_coeff_blocks_clab;
                haptic_boot_config.total_coeff_blocks += cs40l25_total_coeff_blocks_clab;
                coeff_file_no++;
            }
        }

        if (coeff_file_no == 0 && !(boot_type & BOOT_HAPTIC_TYPE_NO_BIN)) {
            return BSP_STATUS_FAIL;
        }

        haptic_boot_config.total_fw_blocks = cs40l25_total_fw_blocks;
        haptic_boot_config.fw_blocks = cs40l25_fw_blocks;

        haptic_driver.boot_config = &haptic_boot_config;
    }

    if (haptic_status != CS40L25_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }

    return ret;
}

void bsp_haptic_control_callback(uint8_t id, uint32_t status, void *arg)
{
    if ((id == CS40L25_CONTROL_ID_CONFIGURE) ||
        (id == CS40L25_CONTROL_ID_POWER_UP) ||
        (id == CS40L25_CONTROL_ID_POWER_DOWN) ||
        (id == CS40L25_CONTROL_ID_GET_VOLUME) ||
        (id == CS40L25_CONTROL_ID_SET_VOLUME) ||
        (id == CS40L25_CONTROL_ID_GET_HALO_HEARTBEAT) ||
        (id == CS40L25_CONTROL_ID_CALIBRATION) ||
        (id == CS40L25_CONTROL_ID_SET_TRIGGER_INDEX) ||
        (id == CS40L25_CONTROL_ID_SET_TRIGGER_MS) ||
        (id == CS40L25_CONTROL_ID_SET_TIMEOUT_MS) ||
        (id == CS40L25_CONTROL_ID_SET_GPIO_ENABLE) ||
        (id == CS40L25_CONTROL_ID_SET_GPIO1_BUTTON_DETECT) ||
        (id == CS40L25_CONTROL_ID_SET_GPIO2_BUTTON_DETECT) ||
        (id == CS40L25_CONTROL_ID_SET_GPIO3_BUTTON_DETECT) ||
        (id == CS40L25_CONTROL_ID_SET_GPIO4_BUTTON_DETECT) ||
        (id == CS40L25_CONTROL_ID_SET_GPI_GAIN_CONTROL) ||
        (id == CS40L25_CONTROL_ID_SET_CTRL_PORT_GAIN_CONTROL) ||
        (id == CS40L25_CONTROL_ID_SET_GPIO1_INDEX_BUTTON_PRESS) ||
        (id == CS40L25_CONTROL_ID_SET_GPIO2_INDEX_BUTTON_PRESS) ||
        (id == CS40L25_CONTROL_ID_SET_GPIO3_INDEX_BUTTON_PRESS) ||
        (id == CS40L25_CONTROL_ID_SET_GPIO4_INDEX_BUTTON_PRESS) ||
        (id == CS40L25_CONTROL_ID_SET_GPIO1_INDEX_BUTTON_RELEASE) ||
        (id == CS40L25_CONTROL_ID_SET_GPIO2_INDEX_BUTTON_RELEASE) ||
        (id == CS40L25_CONTROL_ID_SET_GPIO3_INDEX_BUTTON_RELEASE) ||
        (id == CS40L25_CONTROL_ID_SET_GPIO4_INDEX_BUTTON_RELEASE) ||
        (id == CS40L25_CONTROL_ID_SET_CLAB_ENABLED) ||
        (id == CS40L25_CONTROL_ID_GET_FW_REVISION) ||
        (id == CS40L25_CONTROL_ID_GET_DYNAMIC_REDC) ||
        (id == CS40L25_CONTROL_ID_GET_DYNAMIC_F0) ||
        (id == CS40L25_CONTROL_ID_ENABLE_DYNAMIC_F0) ||
        (id == CS40L25_CONTROL_ID_GET_DSP_STATUS))
    {
        if (app_cb == NULL)
        {
            if (status == CS40L25_STATUS_OK)
            {
                bsp_haptic_control_status = 1;
            }
            else
            {
                bsp_haptic_control_status = 2;
            }
        }
        else
        {
            uint32_t bsp_status = (status == CS40L25_STATUS_OK) ? BSP_STATUS_OK : BSP_STATUS_FAIL;
            app_cb(bsp_status, app_cb_arg);
        }
    }

    return;
}

uint32_t bsp_haptic_reset()
{
    uint32_t haptic_status;

    bsp_haptic_control_status = 0;

    haptic_status = cs40l25_functions_g->reset(&haptic_driver, bsp_haptic_control_callback, NULL);

    if ((haptic_status == CS40L25_STATUS_OK) && (app_cb == NULL))
    {
        while (bsp_haptic_control_status == 0)
        {
            cs40l25_functions_g->process(&haptic_driver);
        }

        if (bsp_haptic_control_status == 1)
        {
            haptic_status = BSP_STATUS_OK;
        }
        else
        {
            haptic_status = BSP_STATUS_FAIL;
        }
    }

    return haptic_status;
}

uint32_t bsp_haptic_boot(bool cal_boot)
{
    uint32_t haptic_status;

    bsp_haptic_control_status = 0;

    haptic_status = cs40l25_functions_g->boot(&haptic_driver, cal_boot, bsp_haptic_control_callback, NULL);

    if (haptic_status == CS40L25_STATUS_OK)
    {
        cs40l25_control_request_t req;
        req.id = CS40L25_CONTROL_ID_GET_FW_REVISION;
        req.arg = (void *) &fw_revision;
        req.cb = bsp_haptic_control_callback;
        req.cb_arg = NULL;

        haptic_status = cs40l25_functions_g->control(&haptic_driver, req);
    }

    if (haptic_status == CS40L25_STATUS_OK)
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_haptic_calibrate(void)
{
    uint32_t haptic_status;

    bsp_haptic_control_status = 0;

    haptic_status = cs40l25_functions_g->calibrate(&haptic_driver, CS40L25_CALIB_ALL, bsp_haptic_control_callback, NULL);

    if ((haptic_status == CS40L25_STATUS_OK) && (app_cb == NULL))
    {
        while (bsp_haptic_control_status == 0)
        {
            cs40l25_functions_g->process(&haptic_driver);
        }

        if (bsp_haptic_control_status == 1)
        {
            haptic_status = BSP_STATUS_OK;
        }
        else
        {
            haptic_status = BSP_STATUS_FAIL;
        }
    }

    return haptic_status;
}

uint32_t bsp_haptic_power_up(void)
{
    uint32_t haptic_status;

    bsp_haptic_control_status = 0;
    haptic_status = cs40l25_functions_g->power(&haptic_driver, CS40L25_POWER_UP, bsp_haptic_control_callback, NULL);

    if ((haptic_status == CS40L25_STATUS_OK) && (app_cb == NULL))
    {
        while (bsp_haptic_control_status == 0)
        {
            cs40l25_functions_g->process(&haptic_driver);
        }

        if (bsp_haptic_control_status != 1)
        {
            haptic_status = BSP_STATUS_FAIL;
        }

        haptic_status = BSP_STATUS_OK;
    }

    return haptic_status;
}

uint32_t bsp_haptic_power_down(void)
{
    uint32_t haptic_status;

    bsp_haptic_control_status = 0;
    haptic_status = cs40l25_functions_g->power(&haptic_driver, CS40L25_POWER_DOWN, bsp_haptic_control_callback, NULL);

    if ((haptic_status == CS40L25_STATUS_OK) && (app_cb == NULL))
    {
        while (bsp_haptic_control_status == 0)
        {
            cs40l25_functions_g->process(&haptic_driver);
        }

        if (bsp_haptic_control_status == 1)
        {
            haptic_status = BSP_STATUS_OK;
        }
        else
        {
            haptic_status = BSP_STATUS_FAIL;
        }
    }

    return haptic_status;
}

uint32_t bsp_haptic_hibernate(void)
{
    uint32_t haptic_status;
    bsp_haptic_control_status = 0;
    haptic_status = cs40l25_functions_g->power(&haptic_driver, CS40L25_POWER_HIBERNATE, bsp_haptic_control_callback, NULL);

    if ((haptic_status == CS40L25_STATUS_OK) && (app_cb == NULL))
    {
        while (bsp_haptic_control_status == 0)
        {
            cs40l25_functions_g->process(&haptic_driver);
        }

        if (bsp_haptic_control_status == 1)
        {
            haptic_status = BSP_STATUS_OK;
        }
        else
        {
            haptic_status = BSP_STATUS_FAIL;
        }
    }

    return haptic_status;
}

uint32_t bsp_haptic_wake(void)
{
    uint32_t haptic_status;
    bsp_haptic_control_status = 0;
    haptic_status = cs40l25_functions_g->power(&haptic_driver, CS40L25_POWER_WAKE, bsp_haptic_control_callback, NULL);

    if ((haptic_status == CS40L25_STATUS_OK) && (app_cb == NULL))
    {
        while (bsp_haptic_control_status == 0)
        {
            cs40l25_functions_g->process(&haptic_driver);
        }

        if (bsp_haptic_control_status == 1)
        {
            haptic_status = BSP_STATUS_OK;
        }
        else
        {
            haptic_status = BSP_STATUS_FAIL;
        }
    }

    return haptic_status;
}

uint32_t bsp_haptic_mute(bool is_mute)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_haptic_is_processing(bool *is_processing)
{
    return BSP_STATUS_FAIL;
}

uint32_t bsp_haptic_process(void)
{
    uint32_t ret;

    ret = cs40l25_functions_g->process(&haptic_driver);

    if (ret != CS40L25_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_haptic_control(uint32_t id, uint32_t arg)
{
    uint32_t ret;

    cs40l25_control_request_t req;
    req.id = id;
    req.arg = arg;
    req.cb = bsp_haptic_control_callback;
    req.cb_arg = NULL;

    ret = cs40l25_functions_g->control(&haptic_driver, req);

    if (ret != CS40L25_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_haptic_dynamic_calibrate(void)
{
    uint32_t ret;
    cs40l25_control_request_t req;

    // Enable Dynamic F0
    req.id = CS40L25_CONTROL_ID_ENABLE_DYNAMIC_F0;
    req.arg = 1;
    req.cb = bsp_haptic_control_callback;
    req.cb_arg = NULL;

    ret = cs40l25_functions_g->control(&haptic_driver, req);

    // Read Dynamic F0 from WT Index 0
    req.id = CS40L25_CONTROL_ID_GET_DYNAMIC_F0;
    dynamic_f0.index = 0;
    req.arg = &dynamic_f0;
    req.cb = bsp_haptic_control_callback;
    req.cb_arg = NULL;

    ret = cs40l25_functions_g->control(&haptic_driver, req);

    // Get Dynamic ReDC
    req.id = CS40L25_CONTROL_ID_GET_DYNAMIC_REDC;
    req.arg = &dynamic_redc;
    req.cb = bsp_haptic_control_callback;
    req.cb_arg = NULL;

    ret = cs40l25_functions_g->control(&haptic_driver, req);

    if (ret != CS40L25_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}
#endif

uint32_t bsp_audio_play(uint8_t content)
{
    switch (content)
    {
        case BSP_PLAY_SILENCE:
#if (BSP_I2S_2BYTES_PER_SUBFRAME == 2)
            playback_content = pcm_silence_32bit_stereo_single_period;
#else
            playback_content = pcm_silence_16bit_stereo_single_period;
#endif
            break;

        case BSP_PLAY_STEREO_1KHZ_20DBFS:
#if (BSP_I2S_2BYTES_PER_SUBFRAME == 2)
            playback_content = pcm_20dBFs_1kHz_32bit_stereo_single_period;
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
            playback_content = pcm_silence_32bit_stereo_single_period;
#else
            playback_content = pcm_silence_16bit_stereo_single_period;
#endif
            break;

        case BSP_PLAY_STEREO_1KHZ_20DBFS:
            dma_transfer_size = PCM_1KHZ_SINGLE_PERIOD_LENGTH_2BYTES;
#if (BSP_I2S_2BYTES_PER_SUBFRAME == 2)
            playback_content = pcm_20dBFs_1kHz_32bit_stereo_single_period;
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

#ifdef TARGET_CS35L41
        case BSP_GPIO_ID_CS35L41_RESET:
            HAL_GPIO_WritePin(BSP_AMP_RESET_GPIO_PORT, BSP_AMP_RESET_PIN, (GPIO_PinState) gpio_state);
            break;
#endif
#ifdef TARGET_CS40L25
        case BSP_GPIO_ID_CS40L25_RESET:
            HAL_GPIO_WritePin(BSP_HAPTIC_RESET_GPIO_PORT, BSP_HAPTIC_RESET_PIN, (GPIO_PinState) gpio_state);
            break;
#endif

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

    Timer_Start(duration_ms * 10);

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
        case BSP_AMP_DEV_ID:
            if (cb == NULL)
            {
                HAL_I2C_Master_Seq_Transmit_IT(&i2c_drv_handle, 0x80, write_buffer, write_length, I2C_FIRST_FRAME);
                while (HAL_I2C_GetState(&i2c_drv_handle) != HAL_I2C_STATE_READY)
                {
                }
                HAL_I2C_Master_Seq_Receive_IT(&i2c_drv_handle, 0x80, read_buffer, read_length, I2C_LAST_FRAME);
                while (HAL_I2C_GetState(&i2c_drv_handle) != HAL_I2C_STATE_READY)
                {
                }
            }
            else
            {
                bsp_i2c_done_cb = cb;
                bsp_i2c_done_cb_arg = cb_arg;
                bsp_i2c_current_transaction_type = BSP_I2C_TRANSACTION_TYPE_READ_REPEATED_START;
                bsp_i2c_read_buffer_ptr = read_buffer;
                bsp_i2c_read_length = read_length;
                bsp_i2c_read_address = 0x80;
                HAL_I2C_Master_Seq_Transmit_IT(&i2c_drv_handle,
                                               bsp_i2c_read_address,
                                               write_buffer,
                                               write_length,
                                               I2C_FIRST_FRAME);
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
    switch (bsp_dev_id)
    {
        case BSP_AMP_DEV_ID:
            if (cb == NULL)
            {
                HAL_I2C_Master_Seq_Transmit_IT(&i2c_drv_handle, 0x80, write_buffer, write_length, I2C_FIRST_AND_LAST_FRAME);
                while (HAL_I2C_GetState(&i2c_drv_handle) != HAL_I2C_STATE_READY)
                {
                }
            }
            else
            {
                bsp_i2c_done_cb = cb;
                bsp_i2c_done_cb_arg = cb_arg;
                bsp_i2c_current_transaction_type = BSP_I2C_TRANSACTION_TYPE_WRITE;
                HAL_I2C_Master_Seq_Transmit_IT(&i2c_drv_handle,
                                               0x80,
                                               write_buffer,
                                               write_length,
                                               I2C_FIRST_AND_LAST_FRAME);
            }

            break;

        case BSP_DEV_ID_NULL:
            if (cb == NULL)
            {
                HAL_I2C_Master_Seq_Transmit_IT(&i2c_drv_handle, 0xAA, write_buffer, write_length, I2C_FIRST_AND_LAST_FRAME);
                while (HAL_I2C_GetState(&i2c_drv_handle) != HAL_I2C_STATE_READY)
                {
                }
            }
            break;

        default:
            break;
    }

    return BSP_STATUS_OK;
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
        case BSP_AMP_DEV_ID:
            if (cb == NULL)
            {
                HAL_I2C_Master_Seq_Transmit_IT(&i2c_drv_handle, 0x80, write_buffer_0, write_length_0, I2C_FIRST_FRAME);
                while (HAL_I2C_GetState(&i2c_drv_handle) != HAL_I2C_STATE_READY)
                {
                }
                HAL_I2C_Master_Seq_Transmit_IT(&i2c_drv_handle, 0x80, write_buffer_0, write_length_0, I2C_LAST_FRAME);
                while (HAL_I2C_GetState(&i2c_drv_handle) != HAL_I2C_STATE_READY)
                {
                }
            }
            else
            {
                bsp_i2c_done_cb = cb;
                bsp_i2c_done_cb_arg = cb_arg;
                bsp_i2c_read_address = 0x80;
                bsp_i2c_write_length = write_length_1;
                bsp_i2c_write_buffer_ptr = write_buffer_1;

                bsp_i2c_current_transaction_type = BSP_I2C_TRANSACTION_TYPE_DB_WRITE;

                HAL_I2C_Master_Seq_Transmit_IT(&i2c_drv_handle, 0x80, write_buffer_0, write_length_0, I2C_FIRST_FRAME);
            }

            break;

        default:
            break;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_register_gpio_cb(uint32_t gpio_id, bsp_callback_t cb, void *cb_arg)
{
#ifdef TARGET_CS35L41
    bsp_amp_int_cb = cb;
    bsp_amp_int_cb_arg = cb_arg;
#endif
#ifdef TARGET_CS40L25
    bsp_haptic_int_cb = cb;
    bsp_haptic_int_cb_arg = cb_arg;
#endif

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
            case BSP_AMP_DEV_ID:
                HAL_I2C_Master_Abort_IT(&i2c_drv_handle, 0x80);
                break;

            default:
                break;
        }
    }

    return BSP_STATUS_OK;
}

#ifdef TARGET_CS35L41
uint32_t bsp_amp_process(void)
{
    uint32_t ret;

    ret = cs35l41_functions_g->process(&amp_driver);

    if (ret == CS35L41_STATUS_OK)
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }

    return ret;
}
#endif

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
};

bsp_driver_if_t *bsp_driver_if_g = &bsp_driver_if_s;
