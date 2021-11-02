/**
 * @file platform_bsp.c
 *
 * @brief Implementation of the BSP for the HW ID0 platform.
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

/* Definition for USART2 HW resources */
#define USART2_CLK_ENABLE()                     __HAL_RCC_USART2_CLK_ENABLE();
#define USART2_RX_GPIO_CLK_ENABLE()             __HAL_RCC_GPIOA_CLK_ENABLE()
#define USART2_TX_GPIO_CLK_ENABLE()             __HAL_RCC_GPIOA_CLK_ENABLE()
#define USART2_FORCE_RESET()                    __HAL_RCC_USART2_FORCE_RESET()
#define USART2_RELEASE_RESET()                  __HAL_RCC_USART2_RELEASE_RESET()
#define USART2_TX_PIN                           GPIO_PIN_2
#define USART2_TX_GPIO_PORT                     GPIOA
#define USART2_TX_AF                            GPIO_AF7_USART2
#define USART2_RX_PIN                           GPIO_PIN_3
#define USART2_RX_GPIO_PORT                     GPIOA
#define USART2_RX_AF                            GPIO_AF7_USART2

#define USART2_TX_BUFFER_SIZE_BYTES             (1024)
#define USART2_RX_BUFFER_SIZE_BYTES             USART2_TX_BUFFER_SIZE_BYTES

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
#define BSP_DUT_CDC_RESET_PIN                   GPIO_PIN_5
#define BSP_DUT_DSP_RESET_PIN                   GPIO_PIN_1
#define BSP_DUT_RESET_GPIO_PORT                 GPIOC
#define BSP_DUT_INT_CLK_ENABLE                  __HAL_RCC_GPIOA_CLK_ENABLE
#define BSP_DUT_INT_CLK_DISABLE                 __HAL_RCC_GPIOA_CLK_DISABLE
#ifndef CONFIG_L25B
#define BSP_DUT_CDC_INT_PIN                     GPIO_PIN_0
#define BSP_DUT_CDC_INT_GPIO_PORT               GPIOA
#else
#define BSP_DUT_CDC_INT_PIN                     GPIO_PIN_11
#define BSP_DUT_CDC_INT_GPIO_PORT               GPIOC
#endif
#define BSP_DUT_DSP_INT_PIN                     GPIO_PIN_10
#define BSP_DUT_DSP_INT_GPIO_PORT               GPIOA

#define BSP_LN2_RESET_CLK_ENABLE                __HAL_RCC_GPIOA_CLK_ENABLE
#define BSP_LN2_RESET_CLK_DISABLE               __HAL_RCC_GPIOA_CLK_DISABLE
#define BSP_LN2_RESET_PIN                       GPIO_PIN_6
#define BSP_LN2_RESET_GPIO_PORT                 GPIOA

#define BSP_PB_TOTAL                            (5)

#define BSP_LN2_FPGA_I2C_ADDRESS_8BIT           (0x44)
#define BSP_INTP_EXP_I2C_ADDRESS_8BIT           (0x4E)

#define BSP_LED_MODE_FIXED                      (0)
#define BSP_LED_MODE_BLINK                      (1)

#define BSP_UART_STATE_PACKET_STATE_IDLE            (0)
#define BSP_UART_STATE_PACKET_STATE_SOH             (1)
#define BSP_UART_STATE_PACKET_STATE_TYPE            (2)
#define BSP_UART_STATE_PACKET_STATE_COUNT           (3)
#define BSP_UART_STATE_PACKET_STATE_LENGTH          (4)
#define BSP_UART_STATE_PACKET_STATE_SOT             (5)
#define BSP_UART_STATE_PACKET_STATE_PAYLOAD_PARTIAL (6)
#define BSP_UART_STATE_PACKET_STATE_PAYLOAD         (7)
#define BSP_UART_STATE_PACKET_STATE_EO_TEXT         (8)
#define BSP_UART_STATE_PACKET_STATE_CHECKSUM        (9)
#define BSP_UART_STATE_PACKET_STATE_EOT             (10)

#define TEST_FILE_HANDLE            (0xFCU)
#define COVERAGE_FILE_HANDLE        (0xFDU)
#define BRIDGE_WRITE_FILE_HANDLE    (0xFEU)
#define BRIDGE_READ_FILE_HANDLE     (0xFFU)

#define BSP_UART_CHANNEL_ID_STDOUT_IN               0x30
#define BSP_UART_CHANNEL_ID_TEST                    0x31
#define BSP_UART_CHANNEL_ID_COVERAGE                0x32
#define BSP_UART_CHANNEL_ID_BRIDGE                  0x33
#define BSP_UART_RX_CHANNEL_INDEX_STDIN             (0)
#define BSP_UART_RX_CHANNEL_INDEX_BRIDGE            (1)
#define BSP_UART_TX_CHANNEL_INDEX_STDOUT            (0)
#define BSP_UART_TX_CHANNEL_INDEX_TEST              (1)
#define BSP_UART_TX_CHANNEL_INDEX_COVERAGE          (2)
#define BSP_UART_TX_CHANNEL_INDEX_BRIDGE            (3)

#define BSP_UART_CHANNEL_FLAG_TX_WHEN_FULL  (1 << 0)

/* Select the preemption priority level(0 is the highest) */
#define I2S_TX_IRQ_PREPRIO                          (0x7)
#define I2S_RX_IRQ_PREPRIO                          (0x8)
#define BSP_DUT_CDC_INT_PREEMPT_PRIO                (0xE)
#define BSP_DUT_DSP_INT_PREEMPT_PRIO                (0xF)
#define USART2_IRQ_PREPRIO                          (0xF)
#define BSP_TIM2_PREPRIO                            (0x4)
#define BSP_TIM5_PREPRIO                            (0x4)
#define BSP_I2C1_ERROR_PREPRIO                      (0x1)
#define BSP_I2C1_EVENT_PREPRIO                      (0x2)

typedef struct
{
    uint32_t id;
    uint8_t mode;
    bool is_on;
    uint32_t blink_counter_100ms;
    uint32_t blink_counter_100ms_max;
} bsp_led_t;

typedef struct
{
    uint32_t size;
    uint32_t in_index;
    uint32_t out_index;
    uint32_t level;
    uint32_t level_pending;
    uint8_t *buffer;
} bsp_fifo_t;

typedef struct
{
    uint8_t id;
    uint8_t priority;
    uint8_t flags;
    uint8_t status;
    bsp_fifo_t fifo;
    uint8_t packet_count;
} bsp_uart_channel_t;

typedef struct
{
    bool tx_complete;
    bsp_uart_channel_t *current_channel;
    uint8_t packet_state;
    uint16_t packet_size;
    uint8_t packet_checksum;
    uint8_t *packet_buffer;
} bsp_uart_state_t;

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

static uint8_t bsp_interposer_led_status;

/* These PLL parameters are valid when the f(VCO clock) = 1Mhz */
const uint32_t I2SFreq[8] = {8000, 11025, 16000, 22050, 32000, 44100, 48000, 96000};
const uint32_t I2SPLLN[8] = {256, 429, 213, 429, 426, 271, 258, 344};
const uint32_t I2SPLLR[8] = {5, 4, 4, 4, 4, 6, 3, 1};

static uint32_t bsp_fs = BSP_AUDIO_FS_48000_HZ;

static bsp_app_callback_t app_cb = NULL;
static void *app_cb_arg = NULL;

static volatile int32_t bsp_irq_count = 0;

static bsp_callback_t bsp_dut_cdc_int_cb[2] = {NULL, NULL};
static bsp_callback_t bsp_dut_dsp_int_cb[2] = {NULL, NULL};
static void *bsp_dut_cdc_int_cb_arg[2] = {NULL, NULL};
static void *bsp_dut_dsp_int_cb_arg[2] = {NULL, NULL};

static uint32_t spi_baud_prescaler = SPI_BAUDRATEPRESCALER_16;

static bsp_led_t bsp_ld2_led =
{
    .id = 0,
    .mode = BSP_LED_MODE_FIXED,
    .is_on = false,
    .blink_counter_100ms = 0,
    .blink_counter_100ms_max = 0,
};

static uint8_t uart_tx_stdout_buffer[USART2_TX_BUFFER_SIZE_BYTES] = {0};
#ifdef CONFIG_USE_MULTICHANNEL_UART
static uint8_t uart_tx_test_buffer[USART2_TX_BUFFER_SIZE_BYTES] = {0};
static uint8_t uart_tx_coverage_buffer[USART2_TX_BUFFER_SIZE_BYTES] = {0};
static uint8_t uart_tx_bridge_buffer[USART2_TX_BUFFER_SIZE_BYTES] = {0};
#endif
static uint8_t uart_rx_stdin_buffer[USART2_TX_BUFFER_SIZE_BYTES] = {0};
#ifdef CONFIG_USE_MULTICHANNEL_UART
static uint8_t uart_rx_bridge_buffer[USART2_TX_BUFFER_SIZE_BYTES] = {0};
#endif

static bsp_uart_channel_t uart_tx_channels[] =
{
    {
        .id = BSP_UART_CHANNEL_ID_STDOUT_IN,
        .priority = 0,
        .flags = 0,
        .fifo =
        {
            .size = USART2_TX_BUFFER_SIZE_BYTES,
            .in_index = 0,
            .out_index = 0,
            .buffer = uart_tx_stdout_buffer,
        },
        .packet_count = 0,
    },
#ifdef CONFIG_USE_MULTICHANNEL_UART
    {
        .id = BSP_UART_CHANNEL_ID_TEST,
        .priority = 1,
        .flags = BSP_UART_CHANNEL_FLAG_TX_WHEN_FULL,
        .fifo =
        {
            .size = USART2_TX_BUFFER_SIZE_BYTES,
            .in_index = 0,
            .out_index = 0,
            .buffer = uart_tx_test_buffer
        },
        .packet_count = 0,
    },

    {
        .id = BSP_UART_CHANNEL_ID_COVERAGE,
        .priority = 1,
        .flags = 0,
        .fifo =
        {
            .size = USART2_TX_BUFFER_SIZE_BYTES,
            .in_index = 0,
            .out_index = 0,
            .buffer = uart_tx_coverage_buffer
        },
        .packet_count = 0,
    },

    {
        .id = BSP_UART_CHANNEL_ID_BRIDGE,
        .priority = 1,
        .flags = 0,
        .fifo =
        {
            .size = USART2_TX_BUFFER_SIZE_BYTES,
            .in_index = 0,
            .out_index = 0,
            .buffer = uart_tx_bridge_buffer
        },
        .packet_count = 0,
    },
#endif
};

static bsp_uart_channel_t uart_rx_channels[] =
{
    {
        .id = BSP_UART_CHANNEL_ID_STDOUT_IN,
        .priority = 1,
        .flags = 0,
        .fifo =
        {
            .size = USART2_RX_BUFFER_SIZE_BYTES,
            .in_index = 0,
            .out_index = 0,
            .buffer = uart_rx_stdin_buffer
        },
        .packet_count = 0,
    },

#ifdef CONFIG_USE_MULTICHANNEL_UART
    {
        .id = BSP_UART_CHANNEL_ID_BRIDGE,
        .priority = 1,
        .flags = 0,
        .fifo =
        {
            .size = USART2_RX_BUFFER_SIZE_BYTES,
            .in_index = 0,
            .out_index = 0,
            .buffer = uart_rx_bridge_buffer
        },
        .packet_count = 0,
    },
#endif
};

static uint8_t uart_tx_packet_buffer[2] = {0};
static bsp_uart_state_t uart_tx_state =
{
    .tx_complete = false,
    .current_channel = NULL,
    .packet_state = BSP_UART_STATE_PACKET_STATE_IDLE,
    .packet_size = 0,
    .packet_checksum = 0,
    .packet_buffer = uart_tx_packet_buffer,
};

static uint8_t uart_rx_packet_buffer[2] = {0};
static bsp_uart_state_t uart_rx_state =
{
    .tx_complete = false,
    .current_channel = NULL,
    .packet_state = BSP_UART_STATE_PACKET_STATE_IDLE,
    .packet_size = 0,
    .packet_checksum = 0,
    .packet_buffer = uart_rx_packet_buffer,
};

#ifdef USE_CMSIS_OS
static SemaphoreHandle_t mutex_spi;
#endif
/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
TIM_HandleTypeDef tim_drv_handle;
TIM_HandleTypeDef led_tim_drv_handle;
I2C_HandleTypeDef i2c_drv_handle;
I2S_HandleTypeDef i2s_drv_handle;
SPI_HandleTypeDef hspi1;
EXTI_HandleTypeDef exti_pb0_handle, exti_pb1_handle, exti_pb2_handle, exti_pb3_handle, exti_pb4_handle, exti_cdc_int_handle, exti_dsp_int_handle;
UART_HandleTypeDef uart_drv_handle;

FILE __test_file;
FILE __coverage_file;
FILE __bridge_write_file;
FILE __bridge_read_file;

FILE* test_file = &__test_file;
FILE* coverage_file = &__coverage_file;
FILE* bridge_write_file = &__bridge_write_file;
FILE* bridge_read_file = &__bridge_read_file;

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

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  spi_baud_prescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.BaudRatePrescaler = spi_baud_prescaler;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void I2S_Init(uint32_t i2s_fs_hz)
{
    // Configure I2S clocking
    RCC_PeriphCLKInitTypeDef rccclkinit;
    uint8_t freqindex = 0xFF;

    for(uint8_t i = 0; i < 8; i++)
    {
      if(I2SFreq[i] == i2s_fs_hz)
      {
        freqindex = i;
      }
    }
    /* Enable PLLI2S clock */
    HAL_RCCEx_GetPeriphCLKConfig(&rccclkinit);
    if (freqindex != 0xFF)
    {
      rccclkinit.PeriphClockSelection = RCC_PERIPHCLK_I2S;
      rccclkinit.PLLI2S.PLLI2SN = I2SPLLN[freqindex];
      rccclkinit.PLLI2S.PLLI2SR = I2SPLLR[freqindex];
      HAL_RCCEx_PeriphCLKConfig(&rccclkinit);
    }
    else
    {
      rccclkinit.PeriphClockSelection = RCC_PERIPHCLK_I2S;
      rccclkinit.PLLI2S.PLLI2SN = 258;
      rccclkinit.PLLI2S.PLLI2SR = 3;
      HAL_RCCEx_PeriphCLKConfig(&rccclkinit);
    }

    i2s_drv_handle.Instance = I2S_HW;

    __HAL_I2S_DISABLE(&i2s_drv_handle);

    i2s_drv_handle.Init.AudioFreq   = i2s_fs_hz;
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

static void I2S_Deinit(void)
{
    __HAL_I2S_DISABLE(&i2s_drv_handle);

    if(HAL_I2S_DeInit(&i2s_drv_handle) != HAL_OK)
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

static void UART_Init(void)
{
    uart_drv_handle.Instance          = USART2;
    uart_drv_handle.Init.BaudRate     = 115200;
    uart_drv_handle.Init.WordLength   = UART_WORDLENGTH_8B;
    uart_drv_handle.Init.StopBits     = UART_STOPBITS_1;
    uart_drv_handle.Init.Parity       = UART_PARITY_NONE;
    uart_drv_handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    uart_drv_handle.Init.Mode         = UART_MODE_TX_RX;
    uart_drv_handle.Init.OverSampling = UART_OVERSAMPLING_16;

    if(HAL_UART_Init(&uart_drv_handle) != HAL_OK)
    {
        Error_Handler();
    }

    return;
}

#ifdef CONFIG_USE_MULTICHANNEL_UART
int __io_putc(int file, int ch)
{
    bsp_uart_channel_t *channel = NULL;
    int ret = ch;

    switch(file)
    {
        case TEST_FILE_HANDLE:
            channel = &(uart_tx_channels[BSP_UART_TX_CHANNEL_INDEX_TEST]);
            break;

        case COVERAGE_FILE_HANDLE:
            channel = &(uart_tx_channels[BSP_UART_TX_CHANNEL_INDEX_COVERAGE]);
            break;

        case STDOUT_FILENO:
            channel = &(uart_tx_channels[BSP_UART_TX_CHANNEL_INDEX_STDOUT]);
            break;

        case BRIDGE_WRITE_FILE_HANDLE:
            channel = &(uart_tx_channels[BSP_UART_TX_CHANNEL_INDEX_BRIDGE]);
            break;

        default:
            break;
    }

    if (channel != NULL)
    {
        bsp_fifo_t *fifo = &(channel->fifo);
        uint32_t temp_in_index, temp_out_index;
        temp_in_index = fifo->in_index;

        // Wait while buffer is full
        do
        {
            __disable_irq();
            temp_out_index = fifo->out_index;
            __enable_irq();
        } while ((temp_in_index + 1) % fifo->size == temp_out_index);

        // Update input index and add char
        fifo->buffer[temp_in_index] = ch;
        fifo->in_index++;
        fifo->in_index %= fifo->size;

        // If UART is not transmitting, then kick off transmit
        __disable_irq();
        if (uart_tx_state.packet_state == BSP_UART_STATE_PACKET_STATE_IDLE)
        {
            uint32_t hal_ret;

            uart_tx_state.packet_buffer[0] = 0x01;
            uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_SOH;
            uart_tx_state.current_channel = channel;

            hal_ret = HAL_UART_Transmit_IT(&uart_drv_handle, uart_tx_state.packet_buffer, 1);
            if (hal_ret != HAL_OK)
            {
                errno = EIO;
                ret = EOF;
            }
        }
        __enable_irq();
    }
    else
    {
        errno = EBADF;
        ret = EOF;
    }

    return ret;
}
#else
int __io_putc(int file, int ch)
{
    bsp_fifo_t *fifo = &(uart_tx_channels[BSP_UART_TX_CHANNEL_INDEX_STDOUT].fifo);
    int ret = ch;

    if ((file == TEST_FILE_HANDLE) ||
        (file == COVERAGE_FILE_HANDLE) ||
        (file == STDOUT_FILENO) ||
        (file == BRIDGE_WRITE_FILE_HANDLE))
    {
        uint32_t temp_out_index;

        __disable_irq();
        temp_out_index = fifo->out_index;
        __enable_irq();

        // If buffer is not full
        if ((fifo->in_index + 1) % fifo->size != temp_out_index)
        {
            // If UART is not transmitting, then kick off transmit
            if (uart_tx_state.packet_state == BSP_UART_STATE_PACKET_STATE_IDLE)
            {
                uint32_t hal_ret;

                fifo->buffer[fifo->in_index] = ch;

                uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_PAYLOAD;
                hal_ret = HAL_UART_Transmit_IT(&uart_drv_handle, &(fifo->buffer[fifo->in_index]), 1);
                if (hal_ret != HAL_OK)
                {
                    uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_IDLE;
                    errno = EIO;
                    ret = EOF;
                }
                else
                {
                    fifo->in_index++;
                    fifo->in_index %= fifo->size;
                }
            }
            else
            {
                // Update input index and add char
                fifo->buffer[fifo->in_index++] = ch;
                fifo->in_index %= fifo->size;
            }
        }
        else
        {
            errno = EIO;
            ret = EOF;
        }
    }
    else
    {
        errno = EBADF;
        ret = EOF;
    }

    return ret;
}
#endif

#ifdef CONFIG_USE_MULTICHANNEL_UART
int __io_getc(int file)
{
    bsp_fifo_t *fifo = NULL;
    int32_t ret = EOF;

    switch(file)
    {
        case STDIN_FILENO:
            fifo = &(uart_rx_channels[BSP_UART_RX_CHANNEL_INDEX_STDIN].fifo);
            break;

        case BRIDGE_READ_FILE_HANDLE:
            fifo = &(uart_rx_channels[BSP_UART_RX_CHANNEL_INDEX_BRIDGE].fifo);
            break;

        default:
            break;
    }

    if (fifo != NULL)
    {
        __disable_irq();

        if (fifo->level > 0)
        {
            ret = fifo->buffer[fifo->out_index++];
            fifo->out_index %= fifo->size;
            fifo->level--;
        }
        else
        {
            errno = 0;
        }

        __enable_irq();
    }
    else
    {
        errno = EBADF;
    }

    return ret;
}
#else
int __io_getc(int file)
{
    bsp_fifo_t *fifo = &(uart_rx_channels[BSP_UART_RX_CHANNEL_INDEX_STDIN].fifo);
    int32_t ret = EOF;

    if ((file == TEST_FILE_HANDLE) ||
        (file == COVERAGE_FILE_HANDLE) ||
        (file == STDIN_FILENO) ||
        (file == BRIDGE_READ_FILE_HANDLE))
    {
        __disable_irq();

        if (fifo->level > 0)
        {
            ret = fifo->buffer[fifo->out_index++];
            fifo->out_index %= fifo->size;
            fifo->level--;
        }
        else
        {
            errno = 0;
        }

        __enable_irq();
    }
    else
    {
        errno = EBADF;
    }

    return ret;
}
#endif

static void bsp_exti_pb_cb(uint32_t pb_id)
{
    if (pb_id < BSP_PB_ID_NUM)
    {

        bsp_pb_pressed_flags[pb_id] = true;
        if (bsp_pb_cbs[pb_id] != NULL)
        {
            bsp_pb_cbs[pb_id](BSP_STATUS_OK, bsp_pb_cb_args[pb_id]);
        }
    }

    return;
}

static void bsp_exti_pb0_cb(void)
{
    bsp_exti_pb_cb(BSP_PB_ID_USER);

    return;
}

static void bsp_exti_pb1_cb(void)
{
    bsp_exti_pb_cb(BSP_PB_ID_SW1);

    return;
}

static void bsp_exti_pb2_cb(void)
{
    bsp_exti_pb_cb(BSP_PB_ID_SW2);

    return;
}

static void bsp_exti_pb3_cb(void)
{
    bsp_exti_pb_cb(BSP_PB_ID_SW3);

    return;
}

static void bsp_exti_pb4_cb(void)
{
    bsp_exti_pb_cb(BSP_PB_ID_SW4);

    return;
}


static void bsp_exti_cdc_int_cb(void)
{
    if (bsp_dut_cdc_int_cb[0] != NULL)
    {
        bsp_dut_cdc_int_cb[0](BSP_STATUS_OK, bsp_dut_cdc_int_cb_arg[0]);
    }

    if (bsp_dut_cdc_int_cb[1] != NULL)
    {
        bsp_dut_cdc_int_cb[1](BSP_STATUS_OK, bsp_dut_cdc_int_cb_arg[1]);
    }

    if (app_cb != NULL)
    {
        app_cb(BSP_STATUS_DUT_EVENTS, app_cb_arg);
    }
    return;
}

static void bsp_exti_dsp_int_cb(void)
{
    if (bsp_dut_dsp_int_cb[0] != NULL)
    {
        bsp_dut_dsp_int_cb[0](BSP_STATUS_OK, bsp_dut_dsp_int_cb_arg[0]);
    }

    if (bsp_dut_dsp_int_cb[1] != NULL)
    {
        bsp_dut_dsp_int_cb[1](BSP_STATUS_OK, bsp_dut_dsp_int_cb_arg[1]);
    }

    if (app_cb != NULL)
    {
        app_cb(BSP_STATUS_DUT_EVENTS, app_cb_arg);
    }

    return;
}

static void bsp_wait_for_eeprom(void)
{
    uint8_t buffer[2] = {0xFF, 0xFF};
    uint32_t timeout = 0;
    while ((buffer[1] & 1)) // check busy bit
    {
        bsp_eeprom_read_status(buffer);
        bsp_set_timer(5, NULL, NULL);
        if (timeout > 100) // about 0.5s, enough for everything except chip erase (typ. 60s)
        {
            break;
        }
        else
        {
            timeout++;
        }
    }
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
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    BSP_DUT_RESET_CLK_ENABLE();
    BSP_DUT_INT_CLK_ENABLE();
    BSP_LN2_RESET_CLK_ENABLE();

    // Configure the LD2 GPO
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure the EEPROM SS
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // Configure the LN2 Reset GPO
    HAL_GPIO_WritePin(BSP_LN2_RESET_GPIO_PORT, BSP_LN2_RESET_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = BSP_LN2_RESET_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BSP_LN2_RESET_GPIO_PORT, &GPIO_InitStruct);

    // Configure the codec Reset GPO
    HAL_GPIO_WritePin(BSP_DUT_RESET_GPIO_PORT, BSP_DUT_CDC_RESET_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = BSP_DUT_CDC_RESET_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BSP_DUT_RESET_GPIO_PORT, &GPIO_InitStruct);

    // Configure the DSP Reset GPO
    HAL_GPIO_WritePin(BSP_DUT_RESET_GPIO_PORT, BSP_DUT_DSP_RESET_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = BSP_DUT_DSP_RESET_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BSP_DUT_RESET_GPIO_PORT, &GPIO_InitStruct);

    // Configure Interrupt GPIs
    GPIO_InitStruct.Pin = BSP_DUT_CDC_INT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BSP_DUT_CDC_INT_GPIO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = BSP_DUT_DSP_INT_PIN;
    HAL_GPIO_Init(BSP_DUT_DSP_INT_GPIO_PORT, &GPIO_InitStruct);

    EXTI_ConfigTypeDef exti_config;

#ifndef CONFIG_L25B
    exti_config.Line = EXTI_LINE_0;
#else
    exti_config.Line = EXTI_LINE_11;
#endif
    exti_config.Mode = EXTI_MODE_INTERRUPT;
    exti_config.Trigger = EXTI_TRIGGER_FALLING;
    HAL_EXTI_SetConfigLine(&exti_cdc_int_handle, &exti_config);
    HAL_EXTI_RegisterCallback(&exti_cdc_int_handle, HAL_EXTI_COMMON_CB_ID, &bsp_exti_cdc_int_cb);

    exti_config.Line = EXTI_LINE_10;
    HAL_EXTI_SetConfigLine(&exti_dsp_int_handle, &exti_config);
    HAL_EXTI_RegisterCallback(&exti_dsp_int_handle, HAL_EXTI_COMMON_CB_ID, &bsp_exti_dsp_int_cb);

    // Configure the Push Button GPI
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // Configure GPIO pins : PB2 (SW1), PB8 (SW3), PB9 (SW4), PB10 (SW2)
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_10|GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    exti_config.Line = EXTI_LINE_13;
    HAL_EXTI_SetConfigLine(&exti_pb0_handle, &exti_config);
    HAL_EXTI_RegisterCallback(&exti_pb0_handle, HAL_EXTI_COMMON_CB_ID, &bsp_exti_pb0_cb);

    exti_config.Line = EXTI_LINE_2;
    HAL_EXTI_SetConfigLine(&exti_pb1_handle, &exti_config);
    HAL_EXTI_RegisterCallback(&exti_pb1_handle, HAL_EXTI_COMMON_CB_ID, &bsp_exti_pb1_cb);

    exti_config.Line = EXTI_LINE_8;
    HAL_EXTI_SetConfigLine(&exti_pb2_handle, &exti_config);
    HAL_EXTI_RegisterCallback(&exti_pb2_handle, HAL_EXTI_COMMON_CB_ID, &bsp_exti_pb2_cb);

    exti_config.Line = EXTI_LINE_9;
    HAL_EXTI_SetConfigLine(&exti_pb3_handle, &exti_config);
    HAL_EXTI_RegisterCallback(&exti_pb3_handle, HAL_EXTI_COMMON_CB_ID, &bsp_exti_pb3_cb);

    exti_config.Line = EXTI_LINE_10;
    HAL_EXTI_SetConfigLine(&exti_pb4_handle, &exti_config);
    HAL_EXTI_RegisterCallback(&exti_pb4_handle, HAL_EXTI_COMMON_CB_ID, &bsp_exti_pb4_cb);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI2_IRQn, BSP_DUT_DSP_INT_PREEMPT_PRIO, 0);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, BSP_DUT_DSP_INT_PREEMPT_PRIO, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    HAL_NVIC_SetPriority((IRQn_Type)EXTI15_10_IRQn, BSP_DUT_DSP_INT_PREEMPT_PRIO, 0x00);
    HAL_NVIC_EnableIRQ((IRQn_Type)EXTI15_10_IRQn);

#ifndef CONFIG_L25B
    /* Enable and set cdc EXTI Interrupt to a low priority */
    HAL_NVIC_SetPriority((IRQn_Type)EXTI0_IRQn, BSP_DUT_CDC_INT_PREEMPT_PRIO, 0x00);
    HAL_NVIC_EnableIRQ((IRQn_Type)EXTI0_IRQn);
#endif

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    return;
}

/**
* @brief SPI MSP Initialization
* This function configures the hardware resources used in this example
* @param hspi: SPI handle pointer
* @retval None
*/
void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hspi->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspInit 0 */

  /* USER CODE END SPI1_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**SPI1 GPIO Configuration
    PB3     ------> SPI1_SCK
    PB4     ------> SPI1_MISO
    PB5     ------> SPI1_MOSI
    */

    /* Depending on minicard config, the SPI interface and SS will differ */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI1_MspInit 1 */

  /* USER CODE END SPI1_MspInit 1 */
  }

}

/**
* @brief SPI MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hspi: SPI handle pointer
* @retval None
*/
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
  if(hspi->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspDeInit 0 */

  /* USER CODE END SPI1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();

    /**SPI1 GPIO Configuration
    PA15     ------> SPI1_NSS
    PB3     ------> SPI1_SCK
    PB4     ------> SPI1_MISO
    PB5     ------> SPI1_MOSI
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_15);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5);

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8);

  /* USER CODE BEGIN SPI1_MspDeInit 1 */

  /* USER CODE END SPI1_MspDeInit 1 */
  }

}

void HAL_MspDeInit(void)
{
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);

    HAL_GPIO_DeInit(BSP_DUT_RESET_GPIO_PORT, BSP_DUT_CDC_RESET_PIN);
    HAL_GPIO_DeInit(BSP_DUT_RESET_GPIO_PORT, BSP_DUT_DSP_RESET_PIN);
    HAL_GPIO_DeInit(BSP_DUT_CDC_INT_GPIO_PORT, BSP_DUT_CDC_INT_PIN);
    HAL_GPIO_DeInit(BSP_DUT_DSP_INT_GPIO_PORT, BSP_DUT_DSP_INT_PIN);
    HAL_GPIO_DeInit(BSP_LN2_RESET_GPIO_PORT, BSP_LN2_RESET_PIN);

    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOC_CLK_DISABLE();

    BSP_DUT_RESET_CLK_DISABLE();
    BSP_DUT_INT_CLK_DISABLE();
    BSP_LN2_RESET_CLK_DISABLE();

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
        // If LED is in blink mode
        if (bsp_ld2_led.mode == BSP_LED_MODE_BLINK)
        {
            // Increment LED blink counter
            bsp_ld2_led.blink_counter_100ms++;

            // If LED blink counter elapsed, then change LED state
            if (bsp_ld2_led.blink_counter_100ms >= bsp_ld2_led.blink_counter_100ms_max)
            {
                bsp_ld2_led.blink_counter_100ms = 0;

                // Change LED GPIO
                if (bsp_ld2_led.is_on)
                {
                    bsp_ld2_led.is_on = false;
                    bsp_set_gpio(BSP_GPIO_ID_LD2, GPIO_PIN_SET);
                }
                else
                {
                    bsp_ld2_led.is_on = true;
                    bsp_set_gpio(BSP_GPIO_ID_LD2, GPIO_PIN_RESET);
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

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    USART2_TX_GPIO_CLK_ENABLE();
    USART2_RX_GPIO_CLK_ENABLE();

    USART2_CLK_ENABLE();

    GPIO_InitStruct.Pin       = USART2_TX_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = USART2_TX_AF;

    HAL_GPIO_Init(USART2_TX_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = USART2_RX_PIN;
    GPIO_InitStruct.Alternate = USART2_RX_AF;

    HAL_GPIO_Init(USART2_RX_GPIO_PORT, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USART2_IRQn, USART2_IRQ_PREPRIO, 1);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    return;
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    USART2_FORCE_RESET();
    USART2_RELEASE_RESET();

    HAL_GPIO_DeInit(USART2_TX_GPIO_PORT, USART2_TX_PIN);
    HAL_GPIO_DeInit(USART2_RX_GPIO_PORT, USART2_RX_PIN);

    HAL_NVIC_DisableIRQ(USART2_IRQn);

    return;
}

void process_uart_tx(void)
#ifdef CONFIG_USE_MULTICHANNEL_UART
{
    bsp_uart_channel_t *channel = uart_tx_state.current_channel;
    uint32_t tx_size_bytes = 0;
    uint8_t *tx_buffer;

    switch (uart_tx_state.packet_state)
    {
        case BSP_UART_STATE_PACKET_STATE_SOH:
            uart_tx_state.packet_buffer[0] = channel->id;
            tx_size_bytes = 1;
            tx_buffer = uart_tx_state.packet_buffer;

            uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_TYPE;
            break;

        case BSP_UART_STATE_PACKET_STATE_TYPE:
            uart_tx_state.packet_buffer[0] = channel->packet_count++;

            tx_size_bytes = 1;
            tx_buffer = uart_tx_state.packet_buffer;

            uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_COUNT;
            break;

        case BSP_UART_STATE_PACKET_STATE_COUNT:
        {
            bsp_fifo_t *fifo = &(channel->fifo);

            // Determine if buffer is not empty
            if (fifo->out_index != fifo->in_index)
            {
                // Calculate next TX size
                if (fifo->in_index >= fifo->out_index)
                {
                    uart_tx_state.packet_size = fifo->in_index - fifo->out_index;
                }
                else
                {
                    uart_tx_state.packet_size = fifo->size - fifo->out_index;
                }

                uart_tx_state.packet_buffer[0] = (uart_tx_state.packet_size >> 8 & 0x00FF);
                uart_tx_state.packet_buffer[1] = (uart_tx_state.packet_size & 0x00FF);
                tx_size_bytes = 2;
                tx_buffer = uart_tx_state.packet_buffer;

                uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_LENGTH;
            }
            else
            {
                uart_tx_state.packet_buffer[0] = 0x04;
                tx_size_bytes = 1;
                tx_buffer = uart_tx_state.packet_buffer;

                uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_EOT;
            }

            break;
        }

        case BSP_UART_STATE_PACKET_STATE_LENGTH:
            uart_tx_state.packet_buffer[0] = 0x02;
            tx_size_bytes = 1;
            tx_buffer = uart_tx_state.packet_buffer;

            uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_SOT;
            break;

        case BSP_UART_STATE_PACKET_STATE_SOT:
            tx_size_bytes = uart_tx_state.packet_size;
            tx_buffer = channel->fifo.buffer + channel->fifo.out_index;

            // Calculate Checksum

            uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_PAYLOAD;

            break;

        case BSP_UART_STATE_PACKET_STATE_PAYLOAD:
        {
            bsp_fifo_t *fifo = &(channel->fifo);

            // Update out_index
            fifo->out_index += uart_drv_handle.TxXferSize;

            if (fifo->out_index >= fifo->size)
            {
                fifo->out_index = 0;
            }

            uart_tx_state.packet_buffer[0] = 0x03;
            tx_size_bytes = 1;
            tx_buffer = uart_tx_state.packet_buffer;

            uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_EO_TEXT;

            break;
        }

        case BSP_UART_STATE_PACKET_STATE_EO_TEXT:
            tx_size_bytes = 1;
            tx_buffer = &uart_tx_state.packet_checksum;

            uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_CHECKSUM;
            break;

        case BSP_UART_STATE_PACKET_STATE_CHECKSUM:
            uart_tx_state.packet_buffer[0] = 0x04;
            tx_size_bytes = 1;
            tx_buffer = uart_tx_state.packet_buffer;

            uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_EOT;
            break;

        case BSP_UART_STATE_PACKET_STATE_EOT:
            // Check for other unempty channels
            for (uint8_t i = 0; i < (sizeof(uart_tx_channels)/sizeof(bsp_uart_channel_t)); i++)
            {
                channel = &(uart_tx_channels[i]);

                // Determine if buffer is not empty
                if (channel->fifo.out_index != channel->fifo.in_index)
                {
                    uart_tx_state.current_channel = channel;
                    uart_tx_state.packet_buffer[0] = 0x01;
                    tx_size_bytes = 1;
                    tx_buffer = uart_tx_state.packet_buffer;

                    uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_SOH;
                }
            }

            break;

        case BSP_UART_STATE_PACKET_STATE_IDLE:
        default:
            tx_size_bytes = 0;
            break;
    }

    if (tx_size_bytes > 0)
    {
        HAL_UART_Transmit_IT(&uart_drv_handle, tx_buffer, tx_size_bytes);
    }
    else
    {
        uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_IDLE;
        uart_tx_state.current_channel = NULL;
    }

    return;
}
#else
{
    bsp_fifo_t *fifo = &(uart_tx_channels[0].fifo);
    uint32_t tx_size_bytes = 0;
    uint8_t *tx_buffer;

    // Update out_index
    fifo->out_index += uart_drv_handle.TxXferSize;
    if (fifo->out_index >= fifo->size)
    {
        fifo->out_index = 0;
    }

    // Determine if buffer is not empty
    if (fifo->out_index != fifo->in_index)
    {
        // Calculate next TX size
        if (fifo->in_index >= fifo->out_index)
        {
            tx_size_bytes = fifo->in_index - fifo->out_index;
        }
        else
        {
            tx_size_bytes = fifo->size - fifo->out_index;
        }

        tx_buffer = fifo->buffer + fifo->out_index;
        HAL_UART_Transmit_IT(&uart_drv_handle, tx_buffer, tx_size_bytes);
    }
    else
    {
        uart_tx_state.packet_state = BSP_UART_STATE_PACKET_STATE_IDLE;
    }

    return;
}
#endif

void process_uart_rx(void)
#ifdef CONFIG_USE_MULTICHANNEL_UART
{
    bsp_uart_channel_t *channel = uart_rx_state.current_channel;
    uint32_t rx_size_bytes = 0;
    uint8_t *rx_buffer;

    switch (uart_rx_state.packet_state)
    {
        case BSP_UART_STATE_PACKET_STATE_IDLE:
            if (uart_rx_state.packet_buffer[0] == 0x01)
            {
                rx_size_bytes = 1;
                rx_buffer = uart_rx_state.packet_buffer;
                uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_SOH;
            }
            break;

        case BSP_UART_STATE_PACKET_STATE_SOH:
        {
            uart_rx_state.current_channel = NULL;

            for (uint8_t i = 0; i < (sizeof(uart_rx_channels)/sizeof(bsp_uart_channel_t)); i++)
            {
                if (uart_rx_channels[i].id == uart_rx_state.packet_buffer[0])
                {
                    uart_rx_state.current_channel = &(uart_rx_channels[i]);
                    break;
                }
            }

            if (uart_rx_state.current_channel != NULL)
            {
                rx_size_bytes = 1;
                rx_buffer = uart_rx_state.packet_buffer;
                uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_TYPE;
            }
            else
            {
                rx_size_bytes = 1;
                rx_buffer = uart_rx_state.packet_buffer;
                uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_IDLE;
            }

            break;
        }

        case BSP_UART_STATE_PACKET_STATE_TYPE:
            channel->packet_count++;

            // Check for synchronized packet count
            if (channel->packet_count != uart_rx_state.packet_buffer[0])
            {
                // Do what?
            }

            channel->packet_count = uart_rx_state.packet_buffer[0];

            rx_size_bytes = 2;
            rx_buffer = uart_rx_state.packet_buffer;
            uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_COUNT;
            break;

        case BSP_UART_STATE_PACKET_STATE_COUNT:
        {
            bsp_fifo_t *fifo = &(channel->fifo);

            // Convert size from 2 unit8_t to uint16_t
            uart_rx_state.packet_size = uart_rx_state.packet_buffer[0] << 8;
            uart_rx_state.packet_size |= uart_rx_state.packet_buffer[1];

            rx_size_bytes = 1;
            rx_buffer = uart_rx_state.packet_buffer;

            // Determine if there is space in the channel fifo
            if ((fifo->size - fifo->level) < uart_rx_state.packet_size)
            {
                uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_IDLE;
            }
            else
            {
                uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_LENGTH;
            }

            break;
        }

        case BSP_UART_STATE_PACKET_STATE_LENGTH:
            if (uart_rx_state.packet_buffer[0] == 0x02)
            {
                bsp_fifo_t *fifo = &(channel->fifo);

                // If in_index will wrap around, then first read until end of the buffer
                if ((fifo->in_index + uart_rx_state.packet_size) > fifo->size)
                {
                    rx_size_bytes = fifo->size - fifo->in_index;
                }
                else
                {
                    rx_size_bytes = uart_rx_state.packet_size;
                }

                rx_buffer = fifo->buffer + fifo->in_index;

                uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_SOT;
            }
            else
            {
                rx_size_bytes = 1;
                rx_buffer = uart_rx_state.packet_buffer;
                uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_IDLE;
            }

            break;

        case BSP_UART_STATE_PACKET_STATE_SOT:
        {
            bsp_fifo_t *fifo = &(channel->fifo);

            // Update fifo
            fifo->in_index += uart_drv_handle.RxXferSize;
            fifo->in_index %= fifo->size;
            fifo->level_pending += uart_drv_handle.RxXferSize;

            // Update Checksum
            uart_rx_state.packet_checksum = 0;

            // If received entire payload
            if (uart_drv_handle.RxXferSize == uart_rx_state.packet_size)
            {
                fifo->level += fifo->level_pending;
                fifo->level_pending = 0;
                rx_size_bytes = 1;
                rx_buffer = uart_rx_state.packet_buffer;
                uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_PAYLOAD;
            }
            else
            {
                rx_size_bytes = (uart_rx_state.packet_size - uart_drv_handle.RxXferSize);
                rx_buffer = fifo->buffer + fifo->in_index;
                uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_PAYLOAD_PARTIAL;
            }

            break;
        }

        case BSP_UART_STATE_PACKET_STATE_PAYLOAD_PARTIAL:
        {
            bsp_fifo_t *fifo = &(channel->fifo);

            // Update fifo
            fifo->in_index += uart_drv_handle.RxXferSize;
            fifo->in_index %= fifo->size;
            fifo->level += fifo->level_pending + uart_drv_handle.RxXferSize;
            fifo->level_pending = 0;

            // Update Checksum
            uart_rx_state.packet_checksum = 0;

            rx_size_bytes = 1;
            rx_buffer = uart_rx_state.packet_buffer;
            uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_PAYLOAD;

            break;
        }

        case BSP_UART_STATE_PACKET_STATE_PAYLOAD:
            rx_size_bytes = 1;
            rx_buffer = uart_rx_state.packet_buffer;

            if (uart_rx_state.packet_buffer[0] == 0x03)
            {
                uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_EO_TEXT;
            }
            else
            {
                uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_IDLE;
            }

            break;

        case BSP_UART_STATE_PACKET_STATE_EO_TEXT:
            rx_size_bytes = 1;
            rx_buffer = uart_rx_state.packet_buffer;

            // Verify checksum
            if (uart_rx_state.packet_buffer[0] == uart_rx_state.packet_checksum)
            {
                uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_CHECKSUM;
            }
            else
            {
                // TODO:  once real checksum is being used, then this can be used
                //uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_IDLE;
                uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_CHECKSUM;
            }

            break;

        case BSP_UART_STATE_PACKET_STATE_CHECKSUM:
            rx_size_bytes = 1;
            rx_buffer = uart_rx_state.packet_buffer;

            if (uart_rx_state.packet_buffer[0] == 0x04)
            {
                // This was a valid packet
            }

            uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_IDLE;

            break;

        default:
            rx_size_bytes = 0;
            break;
    }

    if (rx_size_bytes > 0)
    {
        HAL_UART_Receive_IT(&uart_drv_handle, rx_buffer, rx_size_bytes);
    }
    else
    {
        uart_rx_state.packet_state = BSP_UART_STATE_PACKET_STATE_IDLE;
    }

    return;
}
#else
{
    bsp_fifo_t *fifo = &(uart_rx_channels[0].fifo);

    // Update fifo
    fifo->buffer[fifo->in_index++] = uart_rx_state.packet_buffer[0];
    fifo->in_index %= fifo->size;
    fifo->level++;

    // If fifo is not full
    if (fifo->level < fifo->size)
    {
        HAL_UART_Receive_IT(&uart_drv_handle, uart_rx_state.packet_buffer, 1);
    }

    return;
}
#endif

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
    if (UartHandle->Instance == USART2)
    {
        process_uart_tx();
    }

    return;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
    if (UartHandle->Instance == USART2)
    {
        process_uart_rx();
    }

    return;
}

 void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
    if (UartHandle->Instance == USART2)
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
    uint8_t buffer[3];
    app_cb = cb;
    app_cb_arg = cb_arg;

#ifdef SEMIHOSTING
    initialise_monitor_handles();
#endif
#ifdef USE_CMSIS_OS
    mutex_spi = xSemaphoreCreateMutex();
    if( mutex_spi == NULL )
    {
        return BSP_STATUS_FAIL; /* There was insufficient heap memory available for the mutex to be created. */
    }
#endif
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    // Configure LD2 LED
    bsp_set_gpio(BSP_GPIO_ID_LD2, GPIO_PIN_SET);
    bsp_ld2_led.is_on = true;
    bsp_ld2_led.blink_counter_100ms_max = 1;
    bsp_ld2_led.mode = BSP_LED_MODE_BLINK;

    setvbuf(stdin, NULL, _IONBF, 0);
    test_file = fdopen(TEST_FILE_HANDLE, "w");
    setvbuf(test_file, NULL, _IONBF, 0);
    coverage_file = fdopen(COVERAGE_FILE_HANDLE, "w");
    setvbuf(coverage_file, NULL, _IONBF, 0);
    bridge_write_file = fdopen(BRIDGE_WRITE_FILE_HANDLE, "w");
    setvbuf(bridge_write_file, NULL, _IONBF, 0);
    bridge_read_file = fdopen(BRIDGE_READ_FILE_HANDLE, "r");
    setvbuf(bridge_read_file, NULL, _IONBF, 0);

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

    /* Initialize all peripheral drivers */
    Timer_Init();
    I2C_Init();
    MX_SPI1_Init();
    UART_Init();
    bsp_audio_set_fs(BSP_AUDIO_FS_48000_HZ);

    // Toggle LN2 Reset
    HAL_GPIO_WritePin(BSP_LN2_RESET_GPIO_PORT, BSP_LN2_RESET_PIN, GPIO_PIN_RESET);
    bsp_set_timer(5, NULL, NULL);
    HAL_GPIO_WritePin(BSP_LN2_RESET_GPIO_PORT, BSP_LN2_RESET_PIN, GPIO_PIN_SET);
    bsp_set_timer(5000, NULL, NULL);
    // Bypass LN2 FPGA
    uint32_t temp_buffer = __builtin_bswap32(0x00EE0000);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    // Setup and check EEPROM
    bsp_eeprom_control(BSP_EEPROM_OPCODE_RESET_ENABLE);
    bsp_eeprom_control(BSP_EEPROM_OPCODE_RESET);
    bsp_eeprom_read_jedecid(buffer);
    if((buffer[0] != 0x1F) ||
       (buffer[1] != 0x42) ||
       (buffer[2] != 0x18))
    {
        return BSP_STATUS_FAIL;
    }

    // Setup UART to Receive
    HAL_UART_Receive_IT(&uart_drv_handle, uart_rx_state.packet_buffer, 1);

    // setup interposer's LEDs
    buffer[0] = 6;
    buffer[1] = 0xF0;

    bsp_i2c_write(BSP_INTP_EXP_DEV_ID, buffer, 2, NULL, NULL);
    bsp_set_gpio(BSP_GPIO_ID_INTP_LED_ALL, 0);
    bsp_interposer_led_status = 0;

    return BSP_STATUS_OK;
}

void bsp_notification_callback(uint32_t event_flags, void *arg)
{
    bsp_toggle_gpio(BSP_GPIO_ID_LD2);
    bsp_toggle_gpio(BSP_GPIO_ID_LD2);
    return;
}

uint32_t bsp_audio_set_fs(uint32_t fs_hz)
{
    if ((fs_hz != 8000) && (fs_hz != 48000) && (fs_hz != 44100))
    {
        return BSP_STATUS_FAIL;
    }

    I2S_Deinit();

    I2S_Init(fs_hz);

    bsp_fs = fs_hz;

    return BSP_STATUS_OK;
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
            if (bsp_fs == BSP_AUDIO_FS_8000_HZ)
            {
#if (BSP_I2S_2BYTES_PER_SUBFRAME == 2)
                playback_content = (uint16_t *) pcm_20dBFs_1kHz_32bit_8000_stereo_single_period;
#else
                playback_content = pcm_20dBFs_1kHz_16bit_8000_stereo_single_period;
#endif
            }
            else
            {
#if (BSP_I2S_2BYTES_PER_SUBFRAME == 2)
                playback_content = (uint16_t *) pcm_20dBFs_1kHz_32bit_stereo_single_period;
#else
                playback_content = pcm_20dBFs_1kHz_16bit_stereo_single_period;
#endif
            }
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
            if (bsp_fs == BSP_AUDIO_FS_8000_HZ)
            {
                dma_transfer_size = PCM_1KTONE_8kHz_SINGLE_PERIOD_LENGTH_2BYTES;
#if (BSP_I2S_2BYTES_PER_SUBFRAME == 2)
                playback_content = (uint16_t *) pcm_20dBFs_1kHz_32bit_8000_stereo_single_period;
#else
                playback_content = pcm_20dBFs_1kHz_16bit_8000_stereo_single_period;
#endif
            }
            else
            {
                dma_transfer_size = PCM_1KHZ_SINGLE_PERIOD_LENGTH_2BYTES;
#if (BSP_I2S_2BYTES_PER_SUBFRAME == 2)
                playback_content = (uint16_t *) pcm_20dBFs_1kHz_32bit_stereo_single_period;
#else
                playback_content = pcm_20dBFs_1kHz_16bit_stereo_single_period;
#endif
            }
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
    uint8_t buffer[2] = {0, 0};
    switch (gpio_id)
    {
        case BSP_GPIO_ID_LD2:
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, (GPIO_PinState) gpio_state);
            break;

        case BSP_GPIO_ID_DUT_CDC_RESET:
        {
            uint32_t temp_buffer = 0x00DF0000;
            if (gpio_state == BSP_GPIO_LOW)
            {
                temp_buffer |= 0x1;
            }

            temp_buffer = __builtin_bswap32(temp_buffer);

            bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
            break;
        }

        case BSP_GPIO_ID_DUT_DSP_RESET:
            HAL_GPIO_WritePin(BSP_DUT_RESET_GPIO_PORT, BSP_DUT_DSP_RESET_PIN, (GPIO_PinState) gpio_state);
            break;

        case BSP_GPIO_ID_GF_GPIO7:
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, (GPIO_PinState) gpio_state);
            break;

        case BSP_GPIO_ID_GF_GPIO2:
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, (GPIO_PinState) gpio_state);
            break;

        case BSP_GPIO_ID_INTP_LED1:
            // Enable only the desired LED
            buffer[0] = 2;
            bsp_interposer_led_status &= ~(1 << 0);
            bsp_interposer_led_status |= (gpio_state << 0);
            buffer[1] = bsp_interposer_led_status;

            bsp_i2c_write(BSP_INTP_EXP_DEV_ID, &buffer[0], 2, NULL, NULL);
            break;

        case BSP_GPIO_ID_INTP_LED2:
            // Enable only the desired LED
            buffer[0] = 2;
            bsp_interposer_led_status &= ~(1 << 1);
            bsp_interposer_led_status |= (gpio_state << 1);
            buffer[1] = bsp_interposer_led_status;

            bsp_i2c_write(BSP_INTP_EXP_DEV_ID, &buffer[0], 2, NULL, NULL);
            break;

        case BSP_GPIO_ID_INTP_LED3:
            // Enable only the desired LED
            buffer[0] = 2;
            bsp_interposer_led_status &= ~(1 << 2);
            bsp_interposer_led_status |= (gpio_state << 2);
            buffer[1] = bsp_interposer_led_status;

            bsp_i2c_write(BSP_INTP_EXP_DEV_ID, &buffer[0], 2, NULL, NULL);

            break;

        case BSP_GPIO_ID_INTP_LED4:
            // Enable only the desired LED
            buffer[0] = 2;
            bsp_interposer_led_status &= ~(1 << 3);
            bsp_interposer_led_status |= (gpio_state << 3);
            buffer[1] = bsp_interposer_led_status;

            bsp_i2c_write(BSP_INTP_EXP_DEV_ID, &buffer[0], 2, NULL, NULL);
            break;

        case BSP_GPIO_ID_INTP_LED_ALL:
            // Update all LEDs at once
            buffer[0] = 2;
            if (gpio_state == BSP_GPIO_HIGH)
            {
                bsp_interposer_led_status = 0x0F;
                buffer[1] = bsp_interposer_led_status;
            }
            else
            {
                bsp_interposer_led_status = 0x00;
                buffer[1] = bsp_interposer_led_status;
            }

            bsp_i2c_write(BSP_INTP_EXP_DEV_ID, &buffer[0], 2, NULL, NULL);
            break;

        default:
            break;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_set_supply(uint32_t supply_id, uint8_t supply_state)
{
    uint8_t tmp[4] = {0x01, 0x1E, 0x00, 0x00};

    switch (supply_id)
    {
        case BSP_SUPPLY_ID_LN2_DCVDD:
            tmp[2] = supply_state ? 0x80 : 0x0;
            bsp_i2c_write(BSP_LN2_DEV_ID, tmp, 4, NULL, NULL);
            // Wait 15ms for supply to finish rising/falling
            bsp_set_timer(15, NULL, NULL);
            break;

        default:
            break;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_toggle_gpio(uint32_t gpio_id)
{
    uint8_t buffer[2] = {0, 0};

    switch (gpio_id)
    {
        case BSP_GPIO_ID_LD2:
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            break;

        case BSP_GPIO_ID_INTP_LED1:
            // Toggle only the desired LED
            buffer[0] = 2;
            bsp_interposer_led_status ^= (1 << 0);
            buffer[1] = bsp_interposer_led_status;

            bsp_i2c_write(BSP_INTP_EXP_DEV_ID, buffer, 2, NULL, NULL);
            break;

        case BSP_GPIO_ID_INTP_LED2:
            // Toggle only the desired LED
            buffer[0] = 2;
            bsp_interposer_led_status ^= (1 << 1);
            buffer[1] = bsp_interposer_led_status;

            bsp_i2c_write(BSP_INTP_EXP_DEV_ID, buffer, 2, NULL, NULL);
            break;

        case BSP_GPIO_ID_INTP_LED3:
            // Toggle only the desired LED
            buffer[0] = 2;
            bsp_interposer_led_status ^= (1 << 2);
            buffer[1] = bsp_interposer_led_status;

            bsp_i2c_write(BSP_INTP_EXP_DEV_ID, buffer, 2, NULL, NULL);
            break;

        case BSP_GPIO_ID_INTP_LED4:
            // Toggle only the desired LED
            buffer[0] = 2;
            bsp_interposer_led_status ^= (1 << 3);
            buffer[1] = bsp_interposer_led_status;

            bsp_i2c_write(BSP_INTP_EXP_DEV_ID, buffer, 2, NULL, NULL);
            break;

        case BSP_GPIO_ID_INTP_LED_ALL:
            // Toggle all LEDs
            buffer[0] = 2;
            bsp_interposer_led_status ^= 0x0F;
            buffer[1] = bsp_interposer_led_status;

            bsp_i2c_write(BSP_INTP_EXP_DEV_ID, &buffer[0], 2, NULL, NULL);
            break;

        default:
            break;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_spi_read(uint32_t bsp_dev_id,
                      uint8_t *addr_buffer,
                      uint32_t addr_length,
                      uint8_t *data_buffer,
                      uint32_t data_length,
                      uint32_t pad_len)
{
    uint8_t padding[4] = { 0x0 };
    HAL_StatusTypeDef ret;
    GPIO_TypeDef *cs_gpio_per;
    uint32_t cs_gpio_pin;

    if (pad_len > 4)
            return BSP_STATUS_FAIL;

    // Select appropriate GPIO peripheral and pin for CS depending on device making request
    switch (bsp_dev_id)
    {
        case BSP_DUT_DEV_ID_SPI2:
            cs_gpio_per = GPIOC;
            cs_gpio_pin = GPIO_PIN_8;
            break;
        case BSP_DUT_DEV_ID:
            cs_gpio_per = GPIOA;
            cs_gpio_pin = GPIO_PIN_15;
            break;
        case BSP_EEPROM_DEV_ID:
            cs_gpio_per = GPIOD;
            cs_gpio_pin = GPIO_PIN_2;
            break;
        default:
            return BSP_STATUS_FAIL;
    }

#ifdef USE_CMSIS_OS
    xSemaphoreTake(mutex_spi, portMAX_DELAY);
#endif
    // Chip select low
    HAL_GPIO_WritePin(cs_gpio_per, cs_gpio_pin, GPIO_PIN_RESET);

    // Transmit R/W bit + register addr
    ret = HAL_SPI_Transmit(&hspi1, addr_buffer, addr_length, HAL_MAX_DELAY);
    if (ret)
    {
        CRUS_THROW(exit_spi_read);
    }

    if (pad_len)
    {
        // Transmit Padding
        ret = HAL_SPI_Transmit(&hspi1, padding, pad_len, HAL_MAX_DELAY);
        if (ret)
        {
            CRUS_THROW(exit_spi_read);
        }
    }

    // Receive value
    ret = HAL_SPI_Receive(&hspi1, data_buffer, data_length, HAL_MAX_DELAY);
    if (ret)
    {
        CRUS_THROW(exit_spi_read);
    }
    HAL_GPIO_WritePin(cs_gpio_per, cs_gpio_pin, GPIO_PIN_SET);

    CRUS_CATCH(exit_spi_read);
#ifdef USE_CMSIS_OS
    xSemaphoreGive(mutex_spi);
#endif
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }
    else
    {
        return BSP_STATUS_OK;
    }
}

uint32_t bsp_spi_write(uint32_t bsp_dev_id,
                      uint8_t *addr_buffer,
                      uint32_t addr_length,
                      uint8_t *data_buffer,
                      uint32_t data_length,
                      uint32_t pad_len)
{
    uint8_t padding[4] = { 0x0 };
    HAL_StatusTypeDef ret;
    GPIO_TypeDef * cs_gpio_per;
    uint32_t cs_gpio_pin;

    if (pad_len > 4)
            return BSP_STATUS_FAIL;

    // Select appropriate GPIO peripheral and pin for CS depending on device making request
    switch (bsp_dev_id)
    {
        case BSP_DUT_DEV_ID_SPI2:
            cs_gpio_per = GPIOC;
            cs_gpio_pin = GPIO_PIN_8;
            break;
        case BSP_DUT_DEV_ID:
            cs_gpio_per = GPIOA;
            cs_gpio_pin = GPIO_PIN_15;
            break;
        case BSP_EEPROM_DEV_ID:
            cs_gpio_per = GPIOD;
            cs_gpio_pin = GPIO_PIN_2;
            break;
        default:
            return BSP_STATUS_FAIL;
    }
#ifdef USE_CMSIS_OS
    xSemaphoreTake(mutex_spi, portMAX_DELAY);
#endif
    // Chip select low
    HAL_GPIO_WritePin(cs_gpio_per, cs_gpio_pin, GPIO_PIN_RESET);

    // Transmit R/W bit + register addr
    ret = HAL_SPI_Transmit(&hspi1, addr_buffer, addr_length, HAL_MAX_DELAY);
    if (ret)
    {
        CRUS_THROW(exit_spi_write);
    }

    if (pad_len)
    {
        // Transmit Padding
        ret = HAL_SPI_Transmit(&hspi1, padding, pad_len, HAL_MAX_DELAY);
        if (ret)
        {
            CRUS_THROW(exit_spi_write);
        }
    }

    // Transmit data
    if (data_length)
    {
        ret = HAL_SPI_Transmit(&hspi1, data_buffer, data_length, HAL_MAX_DELAY);
        if (ret)
        {
            CRUS_THROW(exit_spi_write);
        }
    }

    CRUS_CATCH(exit_spi_write);

    // Chip select high
    HAL_GPIO_WritePin(cs_gpio_per, cs_gpio_pin, GPIO_PIN_SET);

#ifdef USE_CMSIS_OS
    xSemaphoreGive(mutex_spi);
#endif
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }
    else
    {
        return BSP_STATUS_OK;
    }
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

#ifdef BSP_LN2_DEV_ID
        case BSP_LN2_DEV_ID:
            bsp_i2c_transaction_complete = false;
            bsp_i2c_transaction_error = false;
            bsp_i2c_done_cb = cb;
            bsp_i2c_done_cb_arg = cb_arg;
            bsp_i2c_current_transaction_type = BSP_I2C_TRANSACTION_TYPE_READ_REPEATED_START;
            bsp_i2c_read_buffer_ptr = read_buffer;
            bsp_i2c_read_length = read_length;
            bsp_i2c_read_address = BSP_LN2_FPGA_I2C_ADDRESS_8BIT;
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
#endif


#ifdef BSP_INTP_EXP_DEV_ID
        case BSP_INTP_EXP_DEV_ID:
            bsp_i2c_transaction_complete = false;
            bsp_i2c_transaction_error = false;
            bsp_i2c_done_cb = cb;
            bsp_i2c_done_cb_arg = cb_arg;
            bsp_i2c_current_transaction_type = BSP_I2C_TRANSACTION_TYPE_READ_REPEATED_START;
            bsp_i2c_read_buffer_ptr = read_buffer;
            bsp_i2c_read_length = read_length;
            bsp_i2c_read_address = BSP_INTP_EXP_I2C_ADDRESS_8BIT;
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
#endif

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
        case BSP_DUT_DEV_ID_SPI2:
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
                                           BSP_LN2_FPGA_I2C_ADDRESS_8BIT,
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

#ifdef BSP_INTP_EXP_DEV_ID
        case BSP_INTP_EXP_DEV_ID:
            bsp_i2c_transaction_complete = false;
            bsp_i2c_transaction_error = false;
            bsp_i2c_done_cb = cb;
            bsp_i2c_done_cb_arg = cb_arg;
            bsp_i2c_current_transaction_type = BSP_I2C_TRANSACTION_TYPE_WRITE;
            HAL_I2C_Master_Seq_Transmit_IT(&i2c_drv_handle,
                                           BSP_INTP_EXP_I2C_ADDRESS_8BIT,
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
        case BSP_DUT_DEV_ID_SPI2:
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
    switch (gpio_id)
    {
        case BSP_GPIO_ID_DUT_CDC_INT:
            if (bsp_dut_cdc_int_cb[0] == NULL)
            {
                bsp_dut_cdc_int_cb[0] = cb;
                bsp_dut_cdc_int_cb_arg[0] = cb_arg;
            }
            else if (bsp_dut_cdc_int_cb[1] == NULL)
            {
                bsp_dut_cdc_int_cb[1] = cb;
                bsp_dut_cdc_int_cb_arg[1] = cb_arg;
            }
            break;

        case BSP_GPIO_ID_DUT_DSP_INT:
            if (bsp_dut_dsp_int_cb[0] == NULL)
            {
                bsp_dut_dsp_int_cb[0] = cb;
                bsp_dut_dsp_int_cb_arg[0] = cb_arg;
            }
            else if (bsp_dut_dsp_int_cb[1] == NULL)
            {
                bsp_dut_dsp_int_cb[1] = cb;
                bsp_dut_dsp_int_cb_arg[1] = cb_arg;
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

uint32_t bsp_spi_throttle_speed(uint32_t speed_hz)
{
    uint32_t ret = BSP_STATUS_OK;
    uint32_t spi_baud_hz = HAL_RCC_GetPCLK2Freq();  // Currently using SPI1 which is on APB2
    uint32_t temp_spi_baud_prescaler;

    // Save the current prescaler value
    spi_baud_prescaler = temp_spi_baud_prescaler = (hspi1.Init.BaudRatePrescaler >> SPI_CR1_BR_Pos);
    // Get the current SPI Baud in Hz
    spi_baud_hz >>= (temp_spi_baud_prescaler + 1);

    // Check to see if any throttling is needed
    if (speed_hz < spi_baud_hz)
    {
        // Decrement the SPI baud until a rate slow enough is found
        while ((speed_hz < spi_baud_hz) && (temp_spi_baud_prescaler < (SPI_BAUDRATEPRESCALER_256 >> SPI_CR1_BR_Pos)))
        {
            spi_baud_hz >>= 1;
            temp_spi_baud_prescaler++;
        }

        // If no slower SPI baud was found
        if ((speed_hz < spi_baud_hz) && (temp_spi_baud_prescaler >= (SPI_BAUDRATEPRESCALER_256 >> SPI_CR1_BR_Pos)))
        {
            ret = BSP_STATUS_FAIL;
        }
        else
        {
            if (HAL_SPI_DeInit(&hspi1) != HAL_OK)
            {
                Error_Handler();
            }

            hspi1.Init.BaudRatePrescaler = (temp_spi_baud_prescaler << SPI_CR1_BR_Pos);
            if (HAL_SPI_Init(&hspi1) != HAL_OK)
            {
                Error_Handler();
            }
        }

    }

    return ret;
}

uint32_t bsp_spi_restore_speed(void)
{
    // If the SPI baud rate was changed, restore it
    if (spi_baud_prescaler != (hspi1.Init.BaudRatePrescaler >> SPI_CR1_BR_Pos))
    {
        if (HAL_SPI_DeInit(&hspi1) != HAL_OK)
        {
            Error_Handler();
        }

        hspi1.Init.BaudRatePrescaler = (spi_baud_prescaler << SPI_CR1_BR_Pos);
        if (HAL_SPI_Init(&hspi1) != HAL_OK)
        {
            Error_Handler();
        }
    }

    return BSP_STATUS_OK;
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
    if (mode == BSP_LD2_MODE_BLINK)
    {
        bsp_ld2_led.blink_counter_100ms_max = blink_100ms;
        bsp_ld2_led.mode = BSP_LED_MODE_BLINK;
    }
    else
    {
        bsp_ld2_led.mode = BSP_LED_MODE_FIXED;
        if (mode == BSP_LD2_MODE_OFF)
        {
            bsp_set_gpio(BSP_GPIO_ID_LD2, GPIO_PIN_RESET);
            bsp_ld2_led.is_on = false;
        }
        else
        {
            bsp_set_gpio(BSP_GPIO_ID_LD2, GPIO_PIN_SET);
            bsp_ld2_led.is_on = true;
        }
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_eeprom_control(uint8_t command)
{
    uint32_t ret;

    switch (command)
    {
        case BSP_EEPROM_OPCODE_WRITE_ENABLE:
        case BSP_EEPROM_OPCODE_WRITE_DISBLE:
        case BSP_EEPROM_OPCODE_CHIP_ERASE:
        case BSP_EEPROM_OPCODE_RESET_ENABLE:
        case BSP_EEPROM_OPCODE_RESET:
            break;
        default:
            return BSP_STATUS_FAIL;
    }

    bsp_wait_for_eeprom();

    ret = bsp_spi_write(BSP_EEPROM_DEV_ID, (uint8_t*) &(command), 1, NULL, 0, 0);

    return ret;
}

uint32_t bsp_eeprom_read_jedecid(uint8_t *buffer)
{
    uint32_t ret;
    uint8_t cmd = BSP_EEPROM_OPCODE_READ_JEDEC_ID;

    bsp_wait_for_eeprom();

    ret = bsp_spi_read(BSP_EEPROM_DEV_ID, (uint8_t*) &cmd, 1, buffer, 3, 0);

    return ret;
}

uint32_t bsp_eeprom_read_status(uint8_t *buffer)
{
    uint32_t ret;
    uint8_t cmd = BSP_EEPROM_OPCODE_READ_STS_REG_1;

    ret = bsp_spi_read(BSP_EEPROM_DEV_ID, (uint8_t*) &cmd, 1, buffer, 2, 0);

    return ret;
}

uint32_t bsp_eeprom_read(uint32_t addr,
                         uint8_t *data_buffer,
                         uint32_t data_length)
{
    uint32_t ret;
    uint8_t buffer[4];
    uint8_t cmd = BSP_EEPROM_OPCODE_READ_DATA;

    buffer[0] = cmd;
    buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    buffer[3] = GET_BYTE_FROM_WORD(addr, 0);

    bsp_wait_for_eeprom();

    ret = bsp_spi_read(BSP_EEPROM_DEV_ID, buffer, 4, data_buffer, data_length, 0);

    return ret;
}

uint32_t bsp_eeprom_program(uint32_t addr,
                            uint8_t *data_buffer,
                            uint32_t data_length)
{
    uint32_t ret;
    uint8_t buffer[4];
    uint8_t cmd = BSP_EEPROM_OPCODE_PAGE_PROGRAM;
    uint32_t pages;
    uint32_t new_addr;
    uint32_t len_to_write;

    //                                           + 0xFE = round up the integer
    pages = (((addr & 0x000000FF) + data_length) + 0xFE) / 0xFF; // num of pages to write

    for (uint32_t i = 0; i < pages; i++)
    {
        bsp_wait_for_eeprom();
        bsp_eeprom_control(BSP_EEPROM_OPCODE_WRITE_ENABLE);

        // Address
        new_addr = addr + (0x100 * i);
        buffer[0] = cmd;
        buffer[1] = GET_BYTE_FROM_WORD(new_addr, 2);
        buffer[2] = GET_BYTE_FROM_WORD(new_addr, 1);
        buffer[3] = GET_BYTE_FROM_WORD(new_addr, 0);

        if ((new_addr & 0xFF) + data_length > 0xFF)
        {


            len_to_write = 0xFF - (new_addr & 0xFF) + 1;

            ret = bsp_spi_write(BSP_EEPROM_DEV_ID, buffer, 4, data_buffer, len_to_write, 0);
            if (ret)
            {
                CRUS_THROW(exit_eeprom_write);
            }
            data_buffer += len_to_write;
            data_length -= len_to_write;
            addr = addr & 0xFFFFFF00;
        }
        else
        {
            ret = bsp_spi_write(BSP_EEPROM_DEV_ID, buffer, 4, data_buffer, data_length, 0);
            if (ret)
            {
                CRUS_THROW(exit_eeprom_write);
            }
        }
    }
    CRUS_CATCH(exit_eeprom_write);
    return ret;
}

uint32_t bsp_eeprom_program_verify(uint32_t addr,
                                   uint8_t *data_buffer,
                                   uint32_t data_length)
{
    uint32_t ret;
    uint8_t * return_buffer;
    return_buffer = (uint8_t *) bsp_malloc(data_length);

    ret = bsp_eeprom_program(addr, data_buffer, data_length);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }

    ret = bsp_eeprom_read(addr, return_buffer, data_length);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }

    for (uint32_t i = 0; i < data_length; i++)
    {
        if (data_buffer[i] != return_buffer[i])
        {
            bsp_free(return_buffer);
            return BSP_STATUS_FAIL;
        }
    }

    bsp_free(return_buffer);

    return BSP_STATUS_OK;
}

uint32_t bsp_eeprom_erase(uint8_t command, uint32_t addr)
{
    uint32_t ret;
    uint8_t buffer[3];

    switch (command)
    {
        case BSP_EEPROM_OPCODE_BLOCK_ERASE_64KB:
            addr = addr & 0xFFFF0000;
            break;
        case BSP_EEPROM_OPCODE_BLOCK_ERASE_32KB:
            addr = addr & 0xFFFFF000;
            break;
        case BSP_EEPROM_OPCODE_BLOCK_ERASE_4KB:
            addr = addr & 0xFFFFFF00;
            break;
        default:
            return BSP_STATUS_FAIL;
    }

    buffer[0] = GET_BYTE_FROM_WORD(addr, 2);
    buffer[1] = GET_BYTE_FROM_WORD(addr, 1);
    buffer[2] = GET_BYTE_FROM_WORD(addr, 0);

    bsp_wait_for_eeprom();
    bsp_eeprom_control(BSP_EEPROM_OPCODE_WRITE_ENABLE);
    ret = bsp_spi_write(BSP_EEPROM_DEV_ID, (uint8_t*) &(command), 1, buffer, 3, 0);

    return ret;
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
