#include "asteroid_logic.h"

#include <inttypes.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <esp_random.h>
#include <stdlib.h>

#include "led_matrix_driver.h"
#include "return_val.h"

/*Maybe put into SDK config in the main project?*/

/// Selection of the used led matrix hardware
#define MATRIX_32X32
// #define MATRIX_16X16
// #define MATRIX_5X5

#ifdef MATRIX_5X5
#define MATRIX_SIDE_LENGTH 5
#define MAX_INDEX 24
#endif
#ifdef MATRIX_16X16
#define MATRIX_SIDE_LENGTH 16
#define MAX_INDEX 255
#endif
#ifdef MATRIX_32X32
#define MATRIX_SIDE_LENGTH 32
#define MAX_INDEX 1023
#endif

static const char *TAG = "ASTEROID_LOGIC";
typedef struct _node_t
{
    struct _node_t *p_next;
    uint32_t *array;
    uint8_t array_length;
} node_t;

typedef struct
{
    node_t *p_head;
    uint32_t size;
} list_t;

static uint8_t asteroids_per_row = 2;
static list_t asteroid_indices_list = {0};
static node_t *current_node_ptr = NULL;
static uint32_t sum_of_array_lengths = 0;

typedef void (*nodeCallback)(node_t *node, uint8_t node_position);

// Circular Linked List
static return_val_t insertAtEnd_circular(list_t *p_list, uint32_t *array, uint8_t array_length)
{
    node_t *new_node = malloc(sizeof(node_t));
    if (NULL == new_node)
    {
        printf("\nMemory allocation failed\n");
        return MEM_ALLOC_ERROR;
    }

    p_list->size += 1;
    new_node->array = array;
    new_node->array_length = array_length;

    if (p_list->p_head == NULL)
    {
        p_list->p_head = new_node;
        new_node->p_next = p_list->p_head; // Points to itself
        return SUCCESS;
    }

    node_t *p_current = p_list->p_head;
    while (p_current->p_next != p_list->p_head)
    {
        p_current = p_current->p_next;
    }
    p_current->p_next = new_node;
    new_node->p_next = p_list->p_head; // Completes the circle
    return SUCCESS;
}

static void traversList_once(list_t *p_list, nodeCallback nodeCallback)
{
    if (p_list->p_head == NULL)
    {
        printf("List is empty");
        return;
    }
    uint8_t counter = 0;
    node_t *p_current = p_list->p_head;
    while (counter < p_list->size)
    {
        nodeCallback(p_current, counter);
        p_current = p_current->p_next;
        counter += 1;
    }
}

static void deleteList(list_t *p_list)
{
    node_t *p_current = p_list->p_head;
    node_t *p_next = NULL;
    uint8_t counter = 0;
    while (counter < p_list->size)
    {
        p_next = p_current->p_next;
        free(p_current);
        p_current = p_next;
        counter += 1;
    }
    p_list->p_head = NULL;
    p_list->size = 0;
}

static void printNodeValue(node_t *node, uint8_t node_position)
{
    for (int i = 0; i < node->array_length; i += 1)
    {
        printf("node value [%d] = %ld   node position = %d\n", i, node->array[i], node_position);
    }
}

/// when this function is called more than once sum_of_array_lengths must be manually reset to 0
static void getSumOfArrayLengths(node_t *node, uint8_t node_position)
{
    sum_of_array_lengths += node->array_length;
}

static void getCurrentPtr(node_t *node, uint8_t node_position)
{
    current_node_ptr = node;
}

void circular_linked_list_test()
{
    list_t list_1 = {0};
    uint8_t node_amount = 5;
    static uint32_t array[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < node_amount; i += 1)
    {
        insertAtEnd_circular(&list_1, array, 5);
    }

    traversList_once(&list_1, printNodeValue);

    deleteList(&list_1);
}

/// THE USER MUST DEALLOCATE THE RETURNED DATA WHEN NOT USED ANYMORE
led_matrix_data_t *updateAsteroidField()
{
    /// First part of update is getting valid random indices and adding them to the circular linked list ///
    // uint32_t start_index_array[asteroids_per_row];
    uint32_t *start_index_array;
    if (NULL == (start_index_array = malloc(sizeof(*start_index_array) * asteroids_per_row)))
    {
        ESP_LOGE(TAG, "Memory allocation failed!\n");
        return MEM_ALLOC_ERROR;
    }

    /// For the first update it is not necessary to check if the random values are valid
    if (0 == asteroid_indices_list.size)
    {
        /// creates random start indices
        for (uint8_t i = 0; i < asteroids_per_row; i += 1)
        {
            /// The esp_random() automatically waits until enough entropy has gathered to return a new random number
            start_index_array[i] = MAX_INDEX - esp_random() % MATRIX_SIDE_LENGTH;
        }
    }
    /// For every update except the first it is necessary to check if random values are valid
    /// That means the new value must not be directly behind the last value and there must be a possible path to take for the player
    else
    {
        // here comes validity check code ....
        // for now its ignored
        for (uint8_t i = 0; i < asteroids_per_row; i += 1)
        {
            start_index_array[i] = MAX_INDEX - esp_random() % MATRIX_SIDE_LENGTH;
        }
    }
    // array_length for more than one LED must be changed !!!

    // Checking if max ring buffer size is reached (= MATRIX_SIDE_LENGTH)
    if (MATRIX_SIDE_LENGTH > asteroid_indices_list.size)
    {
        insertAtEnd_circular(&asteroid_indices_list, start_index_array, asteroids_per_row); // asteroids per row is not ideal for more than one LED sized asteroid...
        /// current_ptr corresponds to the Tail of the list inside this if statement
        traversList_once(&asteroid_indices_list, getCurrentPtr);
    }
    else
    {
        current_node_ptr = current_node_ptr->p_next;
        /// deallocate previously allocated memory for the start index and set it to the new allocated start_index_array
        free(current_node_ptr->array);
        current_node_ptr->array = start_index_array;
        current_node_ptr->array_length = asteroids_per_row; // asteroids per row is not ideal for more than one LED sized asteroid..........
    }

    /// Second part of the update function is to generate the return data as led_matrix_data_t ///
    traversList_once(&asteroid_indices_list, getSumOfArrayLengths);
    uint32_t index_array_length = sum_of_array_lengths;

    /// must be manually set to 0 after use, else future summation would add to current value of the sum
    sum_of_array_lengths = 0;

    led_matrix_data_t *matrix_data;
    uint16_t *ptr_index_array;
    uint8_t(*ptr_rgb_array)[3];

    if (NULL == (ptr_index_array = malloc(sizeof(*ptr_index_array) * index_array_length)) ||
        NULL == (ptr_rgb_array = malloc(sizeof(**ptr_rgb_array) * sizeof(*ptr_rgb_array) * index_array_length)) ||
        NULL == (matrix_data = malloc(sizeof(*matrix_data))))
    {
        ESP_LOGE(TAG, "Memory allocation failed!\n");
        return MEM_ALLOC_ERROR;
    }

    /// Decrementing (coz Top to Bottom) the indices by the matrix side length -> asteriods fall to next row
    uint8_t counter = 1;
    node_t *current_ptr = current_node_ptr->p_next;
    while (counter < asteroid_indices_list.size && 1 != asteroid_indices_list.size)
    {
        for (uint8_t i = 0; i < current_ptr->array_length; i += 1)
        {
            current_ptr->array[i] -= MATRIX_SIDE_LENGTH;
        }
        current_ptr = current_ptr->p_next;
        counter += 1;
    }

    /// Traversing list and copying all list arrays to index_array
    counter = 0;
    node_t *p_current = asteroid_indices_list.p_head;
    while (counter < asteroid_indices_list.size)
    {
        uint8_t array_length = p_current->array_length;
        for (uint8_t i = 0; i < array_length; i += 1)
        {
            ptr_index_array[counter * array_length + i] = p_current->array[i];
        }

        p_current = p_current->p_next;
        counter += 1;
    }

    /// Setting all asteroid colors to red
    uint8_t rgb_color[3] = {25, 0, 0};
    for (uint32_t i = 0; i < index_array_length; i += 1)
    {
        for (int j = 0; j < 3; j += 1)
        {
            ptr_rgb_array[i][j] = rgb_color[j];
        }
    }

    matrix_data->ptr_index_array_leds_to_set = ptr_index_array;
    matrix_data->ptr_rgb_array_leds_to_set = ptr_rgb_array;
    matrix_data->array_length = index_array_length;

    /// THE USER MUST DEALLOCATE THE RETURNED DATA WHEN NOT USED ANYMORE
    return matrix_data;
}