/**
 * @file f_queue.h
 *
 * @brief
 * Generic fixed-size queue interface.
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

#ifndef F_QUEUE_H
#define F_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup F_QUEUE_STATUS_
 * @brief Return values for all public API calls
 *
 * @{
 */
#define F_QUEUE_STATUS_OK       (0)
#define F_QUEUE_STATUS_FAIL     (1)
/** @} */

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/**
 * Abstract method for copying from one queue element type to another
 *
 * Since the queue copies contents into/out of the queue when inserting/removing, the initializer of the queue needs to
 * provide the implementation for copying contents.
 *
 * @param [in] from             pointer to element to copy contents from
 * @param [in] to               pointer of empty element to which the contents should be copied
 *
 * @return                      success of copy (true = successful, false = failed)
 *
 */
typedef bool (*f_queue_copy)(void *from, void* to);

/**
 * Queue handling data structure
 *
 * @attention Allocating this structure does NOT allocate memory for the queue contents!  That must be done before
 * queue initialization.
 *
 */
typedef struct
{
    uint32_t size;                  ///< Size of queue element storage
    uint32_t remove_index;          ///< Index into queue storage for next element to remove
    uint32_t insert_index;          ///< Index into queue storage for next space to insert into
    void* elements;                 ///< Pointer to queue element storage
    uint8_t element_size_bytes;     ///< Queue element size in bytes
    f_queue_copy cp;                ///< Queue element copy method
} f_queue_t;

/**
 * Fixed Queue public API
 *
 * All API calls require a handle (f_queue_t *) to the queueu handling data structure.
 * All API calls return a status @see F_QUEUE_STATUS_
 *
 */
typedef struct
{
    /**
     * Initialize the Queue
     *
     * @param [in] q                    pointer to the queue handling structure
     * @param [in] size                 size of queue element storage already allocated
     * @param [in] elements             pointer to queue element storage
     * @param [in] element_size_bytes   size of queue element in bytes
     * @param [in] cp                   pointer to queue element copy method implementation
     *
     * @return
     * - F_QUEUE_STATUS_FAIL        if any pointers are NULL or if the size is <= 0
     * - F_QUEUE_STATUS_OK          otherwise
     *
     */
    uint32_t (*initialize)(f_queue_t *q,
                           uint32_t size,
                           void* elements,
                           uint8_t element_size_bytes,
                           f_queue_copy cp);

    /**
     * Insert an element into the Queue
     *
     * @attention Queue insertion copies the contents into queue storage.
     *
     * @param [in] q                    pointer to the queue handling structure
     * @param [in] new_element_ptr      pointer to queue element to be inserted
     *
     * @return
     * - F_QUEUE_STATUS_FAIL        if any pointers are NULL, if the queue is full, or if the copy fails
     * - F_QUEUE_STATUS_OK          otherwise
     *
     */
    uint32_t (*insert)(f_queue_t*, void*);

    /**
     * Remove an element from the Queue
     *
     * @attention Queue removal copies the contents from queue storage.
     *
     * @param [in] q                pointer to the queue handling structure
     * @param [in] element_ptr      pointer to queue element to copy removed element's contents
     *
     * @return
     * - F_QUEUE_STATUS_FAIL        if any pointers are NULL, if the queue is empty, or if the copy fails
     * - F_QUEUE_STATUS_OK          otherwise
     *
     */
    uint32_t (*remove)(f_queue_t*, void*);

    /**
     * Flush (delete all elements) from the Queue
     *
     * After a flush, the queue will be empty
     *
     * @param [in] q                pointer to the queue handling structure
     *
     * @return
     * - F_QUEUE_STATUS_FAIL        if pointer is NULL
     * - F_QUEUE_STATUS_OK          otherwise
     *
     */
    uint32_t (*flush)(f_queue_t*);
} f_queue_if_t;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/**
 * Pointer to Public API implementation
 */
extern f_queue_if_t *f_queue_if_g;

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // F_QUEUE_H
