/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
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
#include "main.h"
#include "i2c.h"
#include "i3c.h"
#include "icache.h"
#include "memorymap.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "version.h"
#include "mspp_hooks_logger_ex.h"
#include "mspp_hooks_tick.h"
#include "mspp_hooks_i2ccon.h"
#include "bsp_dut.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ERROR_LOOP_LED_COUNT                                                                       \
	(1000000) ///< @def ERROR_LOOP_LED_COUNT Loop count to cycle the error LED
#define STATUS_LOOP_LED_COUNT                                                                      \
	(10000000) ///< @def STATUS_LOOP_LED_COUNT Loop count to cycle the status LED
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// Ticker
MSPPHooks_Tick_Settings GlobalTickerSettings = {.increment = 1};
MSPPHooks_Tick_Handle GlobalTicker = {0};

// Logger
MSPPHooks_Logger_Settings GlobalLoggerSettings = {.ticker = &GlobalTicker, .uart = &huart3};
MSPPHooks_Logger_Handle GlobalLogger = {0};

#ifdef __DEBUG
char debug_print = 1;
#else
char debug_print = 0;
#endif
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/*
 * @brief Method to initialize the various (e.g. GPIO, I2C, Tick, Logger)
 * interfaces from the MSPP Hooks library.
 *
 * @note This method will generally log errors out to the global logging
 * interface if there is an error. If there is an error before or during the
 * logger's initialization, no error message will go out, but such an error will
 * be considered a fatal error. On a fatal error, the `Error_Handler()` method
 * will be called.
 *
 * @return
 *  - 0: No error occurred during initialization
 *  - 1: A non-fatal error occurred during initialization
 */
static uint32_t Main_InitInterfaces(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static uint32_t Main_InitInterfaces(void)
{
	uint32_t ret;

	// Initialize the Error LED
	ret = MSPPHooks_GPIO_Init(GPIO_LED_ERR_Settings, &GPIO_LED_ERR);
	if (ret != MSPP_HOOKS_GPIO_STS_OK) {
		Error_Handler();
	}

	// Initialize the Status LED
	ret = MSPPHooks_GPIO_Init(GPIO_LED_STS_Settings, &GPIO_LED_STS);
	if (ret != MSPP_HOOKS_GPIO_STS_OK) {
		Error_Handler();
	}

	// Initialize the SPI1_EN_B GPIO (whether CL_SPI1 is enabled should already be known)
	ret = MSPPHooks_GPIO_Init(GPIO_SPI1_EN_B_Settings, &GPIO_SPI1_EN_B);
	if (ret != MSPP_HOOKS_GPIO_STS_OK) {
		Error_Handler();
	}

	// Initialize the global tick interface
	ret = MSPPHooks_Tick_Init(&GlobalTickerSettings, &GlobalTicker);
	if (ret != MSPP_HOOKS_TICK_STS_OK) {
		Error_Handler();
	}

	// Initialize the global logging interface
	ret = MSPPHooks_Logger_Init(&GlobalLoggerSettings, &GlobalLogger);
	if (ret != MSPP_HOOKS_LOGGER_STS_OK) {
		Error_Handler();
	}
	// Logger is initialized, so we can start logging
	// Disable logging if we want
	if (UART_LoggingEnable == false) {
		ret = MSPPHooks_Logger_Disable(&GlobalLogger, MSPP_HOOKS_LOGGER_LVL_ERROR);
		if (ret != MSPP_HOOKS_LOGGER_STS_OK) {
			Error_Handler();
		}
	}

	// Set cursor to start of screen and reset screen (will have a new line after, but okay)
	ret = MSPPHooks_Logger_LogInfo(&GlobalLogger, "\033[1;1H\033[2J");
	// Log that we have the logger initialized
	if (ret == MSPP_HOOKS_LOGGER_STS_OK) {
		ret = MSPPHooks_Logger_LogInfo(&GlobalLogger,
					       "MAIN: Logging interface initialized");
	}
	// Report the firmware information
	if (ret == MSPP_HOOKS_LOGGER_STS_OK) {
#ifndef __DEBUG
		ret = MSPPHooks_Logger_LogInfo(
			&GlobalLogger, "MAIN: FIRMWARE VERSION: %d.%d.%d [RELEASE]",
			CLMCU_STARTER_MAJOR, CLMCU_STARTER_MINOR, CLMCU_STARTER_PATCH);
#else
		ret = MSPPHooks_Logger_LogInfo(
			&GlobalLogger, "MAIN: FIRMWARE VERSION: %d.%d.%d [DEBUG]",
			CLMCU_STARTER_MAJOR, CLMCU_STARTER_MINOR, CLMCU_STARTER_PATCH);
#endif
	}
	if (ret != MSPP_HOOKS_LOGGER_STS_OK) {
		Error_Handler();
	}

	// Initialize the ID I2C bus
	ret = MSPPHooks_I2CCon_Init((MSPPHooks_I2CCon_Settings *)ID_I2C_Settings, &ID_I2C);
	if (ret != MSPP_HOOKS_I2CCON_STS_OK) {
		ret = MSPPHooks_Logger_LogError(
			&GlobalLogger, "MAIN: Error initializing EEPROM ID I2C bus: %d", ret);
		Error_Handler();
	}

	// Set ID I2C bus to 100kHz by default
	ret = MSPPHooks_I2CCon_SetClk(&ID_I2C, 100000);
	if (ret != MSPP_HOOKS_I2CCON_STS_OK) {
		ret = MSPPHooks_Logger_LogError(
			&GlobalLogger, "MAIN: Error setting EEPROM ID I2C bus clock to 100kHz: %d",
			ret);
		Error_Handler();
	}

	// Initialize the main DUT I2C bus
	ret = MSPPHooks_I2CCon_Init((MSPPHooks_I2CCon_Settings *)CL_I2C0_Settings, &CL_I2C0);
	if (ret != MSPP_HOOKS_I2CCON_STS_OK) {
		ret = MSPPHooks_Logger_LogError(&GlobalLogger,
						"MAIN: Error initializing I2C0 bus: %d", ret);
		Error_Handler();
	}

	unsigned int addr = 0x60;
	uint8_t regaddr[] = {0, 0, 0, 0};
	uint8_t rcvbuffer[] = {0, 0, 0, 0};
	for (addr = 0x60; addr < 0x64; addr++) {
		rcvbuffer[0] = 0;
		rcvbuffer[1] = 0;
		rcvbuffer[2] = 0;
		rcvbuffer[3] = 0;
		MSPPHooks_I2CCon_RegRead(&CL_I2C0, addr, regaddr, 4, rcvbuffer, 4, 100, true, NULL,
					 NULL);
		MSPPHooks_Logger_LogError(&GlobalLogger, "MAIN: I2C0 bus addr %x: %02x%02x%02x%02x",
					  addr, rcvbuffer[0], rcvbuffer[1], rcvbuffer[2],
					  rcvbuffer[3]);
		for (uint32_t i = 0; i < STATUS_LOOP_LED_COUNT; i += 1) {
			;
		}
	}

	// Initialize the secondary DUT I2C bus
	ret = MSPPHooks_I2CCon_Init((MSPPHooks_I2CCon_Settings *)CL_I2C1_Settings, &CL_I2C1);
	if (ret != MSPP_HOOKS_I2CCON_STS_OK) {
		ret = MSPPHooks_Logger_LogError(&GlobalLogger,
						"MAIN: Error initializing I2C1 bus: %d", ret);
		Error_Handler();
	}

	// Initialize the I3C bus
	ret = MSPPHooks_I3CCtrl_Init((MSPPHooks_I3CCtrl_Settings *)CL_I3C0_Settings, &CL_I3C0);
	if (ret != MSPP_HOOKS_I2CCON_STS_OK) {
		ret = MSPPHooks_Logger_LogError(&GlobalLogger,
						"MAIN: Error initializing I3C0 bus: %s",
						MSPPHooks_I3CCtrl_Status_Strings[ret]);
		Error_Handler();
	}

	// Initialize the CL_SPI0 bus
	ret = MSPPHooks_SPICtrl_Init(CL_SPI0_Settings, &CL_SPI0);
	if (ret != MSPP_HOOKS_SPICTRL_STS_OK) {
		ret = MSPPHooks_Logger_LogError(&GlobalLogger,
						"MAIN: Error initializing SPI0 bus: %s",
						MSPPHooks_SPICtrl_Status_Strings[ret]);
	}
	ret = MSPPHooks_SPICtrl_SetClk(SPI_Handles[0], 2000000);
	if (ret != MSPP_HOOKS_I2CCON_STS_OK) {
		Error_Handler();
	}

	// Initialize the CL_SPI1 bus, if CL_SPI1 is enabled
	if (SPI_IsEnabled(CL_SPI1_Settings->hal_handle)) {
		// Only initialize if the CL_SPI1 bus is being used
		ret = MSPPHooks_SPICtrl_Init(CL_SPI1_Settings, &CL_SPI1);
		if (ret != MSPP_HOOKS_SPICTRL_STS_OK) {
			ret = MSPPHooks_Logger_LogError(&GlobalLogger,
							"MAIN: Error initializing SPI1 bus: %s",
							MSPPHooks_SPICtrl_Status_Strings[ret]);
		}

		ret = MSPPHooks_SPICtrl_SetClk(SPI_Handles[1], 2000000);
		if (ret != MSPP_HOOKS_I2CCON_STS_OK) {
			Error_Handler();
		}
	}

	// Initialize the CirrusLink GPIO/INT interfaces
	ret = CL_GPIO_Init();
	if (ret != CL_GPIO_STS_OK) {
		ret = MSPPHooks_Logger_LogError(
			&GlobalLogger, "MAIN: Failed to initialize all the CirrusLink GPIOs");
		Error_Handler();
	}

	return 0;
}

/* USER CODE END 0 */

void i2c_ping(uint16_t addr)
{
	MSPPHooks_I2CCon_Status sts = MSPPHooks_I2CCon_Ping(&CL_I2C0, addr, 500, NULL, NULL);

	if (sts == MSPP_HOOKS_I2CCON_STS_OK) {
		MSPPHooks_Logger_LogInfo(&GlobalLogger, "Device found at CL_I2C0 address 0x%02X",
					 addr);
	} else if (sts == MSPP_HOOKS_I2CCON_STS_NACK) {
		MSPPHooks_Logger_LogInfo(&GlobalLogger, "No device found at CL_I2C0 address 0x%02X",
					 addr);
	} else {
		MSPPHooks_Logger_LogError(&GlobalLogger, "Error pinging address 0x%02X", addr);
	}
}

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_I2C1_Init();
	MX_I2C2_Init();
	MX_I2C3_Init();
	MX_I3C1_Init();
	MX_ICACHE_Init();
	MX_SPI1_Init();
	MX_SPI2_Init();
	MX_USART3_UART_Init();
	/* USER CODE BEGIN 2 */

	HAL_GPIO_WritePin(GPIO1_DUT_RST1_GPIO_Port, GPIO1_DUT_RST1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIO0_DUT_RST0_GPIO_Port, GPIO0_DUT_RST0_Pin, GPIO_PIN_SET);

	Main_InitInterfaces();

	i2c_ping(0x60);
	i2c_ping(0x62);

	bsp_dut_initialize();
	bsp_dut_reset();

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		MSPPHooks_Logger_LogInfo(&GlobalLogger, "...");
		for (uint32_t i = 0; i < STATUS_LOOP_LED_COUNT; i += 1) {
			;
		}
		HAL_GPIO_TogglePin(LED_STS_GPIO_Port, LED_STS_Pin);

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Configure the main internal regulator output voltage
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
	}

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_CSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV2;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.CSIState = RCC_CSI_ON;
	RCC_OscInitStruct.CSICalibrationValue = RCC_CSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLL1_SOURCE_CSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 129;
	RCC_OscInitStruct.PLL.PLLP = 4;
	RCC_OscInitStruct.PLL.PLLQ = 8;
	RCC_OscInitStruct.PLL.PLLR = 2;
	RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1_VCIRANGE_2;
	RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
	RCC_OscInitStruct.PLL.PLLFRACN = 0;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
				      RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
				      RCC_CLOCKTYPE_PCLK3;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
		Error_Handler();
	}

	/** Configure the programming delay
	 */
	__HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_2);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/* USER CODE BEGIN Callback 0 */

	/* USER CODE END Callback 0 */
	if (htim->Instance == TIM1) {
		HAL_IncTick();
	}
	/* USER CODE BEGIN Callback 1 */
	if (htim->Instance == TIM1) {
		MSPPHooks_Tick_Tick(&GlobalTicker);
	}
	/* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
		for (uint32_t i = 0; i < ERROR_LOOP_LED_COUNT; i += 1) {
			;
		}
		HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_ERR_Pin);
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
