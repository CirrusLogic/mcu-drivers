/**
 * @file mock_bsp.c
 *
 * @brief The CS35L56 Driver Unit Test module
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
#include "mock_bsp.h"
#include <stddef.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define MOCK_BSP_MAX_ARGS_PER_TEST (10)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
uint8_t mock_bsp_called_count;
void *mock_bsp_call_queue[MOCK_MAX_CALLS_PER_TEST];
uint32_t mock_bsp_ret_array[MOCK_MAX_CALLS_PER_TEST];
uint32_t mock_bsp_args[MOCK_MAX_CALLS_PER_TEST * MOCK_BSP_MAX_ARGS_PER_TEST];
void *mock_bsp_ptr_args[MOCK_MAX_CALLS_PER_TEST * MOCK_BSP_MAX_ARGS_PER_TEST];
uint32_t mock_bsp_args_by_ref[MOCK_MAX_CALLS_PER_TEST * MOCK_BSP_MAX_ARGS_PER_TEST];

uint8_t mock_bsp_cp_write_buffer[MOCK_SERIAL_BUFFER_LENGTH];
uint8_t mock_bsp_cp_read_buffer[MOCK_SERIAL_BUFFER_LENGTH];

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
void mock_bsp_initialize(void)
{
	mock_bsp_called_count = 0;

	for (int i = 0; i < MOCK_MAX_CALLS_PER_TEST; i++) {
		mock_bsp_call_queue[i] = NULL;
		mock_bsp_ret_array[i] = BSP_STATUS_OK;

		for (int j = 0; j < MOCK_BSP_MAX_ARGS_PER_TEST; j++) {
			mock_bsp_args[MOCK_ARGS_INDEXER(i, j)] = 0;
			mock_bsp_ptr_args[MOCK_ARGS_INDEXER(i, j)] = NULL;
			mock_bsp_args_by_ref[MOCK_ARGS_INDEXER(i, j)] = 0;
		}
	}

	for (int i = 0; i < MOCK_SERIAL_BUFFER_LENGTH; i++) {
		mock_bsp_cp_write_buffer[i] = 0;
		mock_bsp_cp_read_buffer[i] = 0;
	}

	return;
}

static uint32_t mock_bsp_set_gpio(uint32_t gpio_id, uint8_t gpio_state)
{
	mock_bsp_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 0)] = gpio_id;
	mock_bsp_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 1)] = gpio_state;

	mock_bsp_call_queue[mock_bsp_called_count] = mock_bsp_set_gpio;
	return mock_bsp_ret_array[mock_bsp_called_count++];
}

static uint32_t mock_bsp_set_timer(uint32_t duration_ms, bsp_callback_t cb, void *cb_arg)
{
	mock_bsp_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 0)] = duration_ms;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 1)] = (void *)cb;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 2)] = (void *)cb_arg;

	mock_bsp_call_queue[mock_bsp_called_count] = mock_bsp_set_timer;
	return mock_bsp_ret_array[mock_bsp_called_count++];
}

static uint32_t mock_bsp_i2c_read_repeated_start(uint32_t bsp_dev_id, uint8_t *write_buffer,
						 uint32_t write_length, uint8_t *read_buffer,
						 uint32_t read_length, bsp_callback_t cb,
						 void *cb_arg)
{

	mock_bsp_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 0)] = bsp_dev_id;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 1)] = (void *)write_buffer;
	mock_bsp_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 2)] = write_length;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 3)] = (void *)read_buffer;
	mock_bsp_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 4)] = read_length;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 5)] = (void *)cb;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 6)] = (void *)cb_arg;

	for (int i = 0; i < write_length; i++) {
		mock_bsp_cp_write_buffer[i] = write_buffer[i];
	}

	// If it is a syncrhonous call
	if (cb == NULL) {
		for (int i = 0; i < read_length; i++) {
			read_buffer[i] = mock_bsp_cp_read_buffer[i];
		}
	}

	mock_bsp_call_queue[mock_bsp_called_count] = mock_bsp_i2c_read_repeated_start;
	return mock_bsp_ret_array[mock_bsp_called_count++];
}

static uint32_t mock_bsp_i2c_write(uint32_t bsp_dev_id, uint8_t *write_buffer,
				   uint32_t write_length, bsp_callback_t cb, void *cb_arg)
{

	mock_bsp_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 0)] = bsp_dev_id;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 1)] = (void *)write_buffer;
	mock_bsp_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 2)] = write_length;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 3)] = (void *)cb;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 4)] = (void *)cb_arg;

	for (int i = 0; i < write_length; i++) {
		mock_bsp_cp_write_buffer[i] = write_buffer[i];
	}

	mock_bsp_call_queue[mock_bsp_called_count] = mock_bsp_i2c_write;
	return mock_bsp_ret_array[mock_bsp_called_count++];
}

static uint32_t mock_bsp_i2c_db_write(uint32_t bsp_dev_id, uint8_t *write_buffer_0,
				      uint32_t write_length_0, uint8_t *write_buffer_1,
				      uint32_t write_length_1, bsp_callback_t cb, void *cb_arg)
{

	mock_bsp_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 0)] = bsp_dev_id;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 1)] = (void *)write_buffer_0;
	mock_bsp_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 2)] = write_length_0;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 3)] = (void *)write_buffer_1;
	mock_bsp_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 4)] = write_length_1;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 5)] = (void *)cb;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 6)] = (void *)cb_arg;

	mock_bsp_call_queue[mock_bsp_called_count] = mock_bsp_i2c_db_write;
	return mock_bsp_ret_array[mock_bsp_called_count++];
}

static uint32_t mock_bsp_register_gpio_cb(uint32_t gpio_id, bsp_callback_t cb, void *cb_arg)
{

	mock_bsp_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 0)] = gpio_id;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 1)] = (void *)cb;
	mock_bsp_ptr_args[MOCK_ARGS_INDEXER(mock_bsp_called_count, 2)] = (void *)cb_arg;

	mock_bsp_call_queue[mock_bsp_called_count] = mock_bsp_register_gpio_cb;
	return mock_bsp_ret_array[mock_bsp_called_count++];
}

static bsp_driver_if_t bsp_driver_if_s = {
	.set_gpio = &mock_bsp_set_gpio,
	.set_timer = &mock_bsp_set_timer,
	.i2c_read_repeated_start = &mock_bsp_i2c_read_repeated_start,
	.i2c_write = &mock_bsp_i2c_write,
	.i2c_db_write = &mock_bsp_i2c_db_write,
	.register_gpio_cb = &mock_bsp_register_gpio_cb,
};

bsp_driver_if_t *bsp_driver_if_g = &bsp_driver_if_s;
