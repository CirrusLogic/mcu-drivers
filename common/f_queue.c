/**
 * @file f_queue.c
 *
 * @brief Generic fixed-size queue implementation.
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
#include "f_queue.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/**
 * Macro to wrap the insert or remove indices once reaching the end of the queue
 *
 * @param [in] q                pointer to queue handling structure
 * @param [in, out] index       index (either insert or remove) to be wrapped
 *
 */
#define f_queue_wrap(q, index) (((index) + 1) % q->size)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * Initialize the Queue
 *
 * Implementation of f_queue_if_t.initialize
 *
 */
static uint32_t f_queue_initialize(f_queue_t *q,
                                   uint32_t size,
                                   void* elements,
                                   uint8_t element_size_bytes,
                                   f_queue_copy cp)
{
    uint32_t ret = F_QUEUE_STATUS_FAIL;

    // Check for NULL pointers or invalid 'size'
    if ((q != NULL) && \
        (size > 0) && \
        (elements != NULL) &&\
        (cp != NULL))
    {
        // Initialize queue handling structure elements
        q->size = size;
        q->insert_index = 0;
        q->remove_index = 0;
        q->elements = elements;
        q->element_size_bytes = element_size_bytes;
        q->cp = cp;

        ret = F_QUEUE_STATUS_OK;
    }

    return ret;
}

/**
 * Insert an element into the Queue
 *
 * Implementation of f_queue_if_t.insert
 *
 */
static uint32_t f_queue_insert(f_queue_t *q, void *new_element_ptr)
{
    uint32_t result = F_QUEUE_STATUS_FAIL;

    // Check for NULL pointers
    if ((q != NULL) && (new_element_ptr != NULL))
    {
        uint32_t ins = q->insert_index;
        uint32_t rem = q->remove_index;
        // Wrap the insert index
        uint32_t new_insert_index = f_queue_wrap(q, ins);

        // Check to see if next insert_index = remove_index, indicating full queue
        if (new_insert_index != rem)
        {
            // Get pointer to element storage at insert index
            void *ins_ptr = q->elements + (q->element_size_bytes * ins);
            // If element copy is successful
            if (q->cp(new_element_ptr, ins_ptr))
            {
                // Update insert index
                q->insert_index = new_insert_index;
                result = F_QUEUE_STATUS_OK;
            }
        }
    }

    return result;
}

/**
 * Remove an element from the Queue
 *
 * Implementation of f_queue_if_t.remove
 *
 */
static uint32_t f_queue_remove(f_queue_t *q, void *element_ptr)
{
    bool result = F_QUEUE_STATUS_FAIL;

    // Check for NULL pointers
    if ((q != NULL) && (element_ptr != NULL))
    {
        uint32_t ins = q->insert_index;
        uint32_t rem = q->remove_index;

        // Check to see if remove_index = insert_index, indicating empty queue
        if (rem != ins)
        {
            // Get pointer to element storage at removal index
            void *rem_ptr = q->elements + (q->element_size_bytes * rem);
            // If copy is successful
            if (q->cp(rem_ptr, element_ptr))
            {
                // Update remove index
                q->remove_index = f_queue_wrap(q, rem);
                result = F_QUEUE_STATUS_OK;
            }
        }
    }

    return result;
}

/**
 * Flush (delete all elements) from the Queue
 *
 * Implementation of f_queue_if_t.flush
 *
 */
static uint32_t f_queue_flush(f_queue_t *q)
{
    uint32_t ret = F_QUEUE_STATUS_FAIL;

    // Check for NULL pointer
    if (q != NULL)
    {
        q->insert_index = 0;
        q->remove_index = 0;

        ret = F_QUEUE_STATUS_OK;
    }

    return ret;
}

/**
 * Function pointer table for Public API
 *
 * @attention Although not const, this should never be changed run-time in an end-product.  It is implemented this
 * way to facilitate unit testing.
 *
 */
f_queue_if_t f_queue_if_s =
{
    .initialize = &f_queue_initialize,
    .insert = &f_queue_insert,
    .remove = &f_queue_remove,
    .flush = &f_queue_flush
};

/**
 * Pointer to Public API implementation
 */
f_queue_if_t *f_queue_if_g = &f_queue_if_s;
