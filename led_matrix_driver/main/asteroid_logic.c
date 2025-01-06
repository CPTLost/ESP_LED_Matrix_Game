#include "asteroid_logic.h"

#include <inttypes.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <esp_random.h>
#include <stdlib.h>
#include <assert.h>

#include "led_matrix_driver.h"
#include "return_val.h"

#define ARRAY_LENGTH(x) sizeof(x) / sizeof(x[0])

#define MAX_AST_OBJ_ALLOWED 64
#define MAX_OFFSET CONFIG_MAX_OFFSET
#define MIN_OFFSET CONFIG_MIN_OFFSET

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

typedef struct
{
    uint8_t *block_shape_array;
    uint8_t array_size;
} block_t;

typedef struct
{
    block_t *block;
    uint8_t call_counter;
    uint16_t start_index;
} asteroid_t;

#if CONFIG_MATRIX_5X5
static const uint8_t block_s_shape_array[] = {1};
static const uint8_t block_m_shape_array[] = {2, 2};
static const uint8_t block_l_shape_array[] = {3, 3};
#elif CONFIG_MATRIX_16X16
static const uint8_t block_s_shape_array[] = {1, 3, 1};
static const uint8_t block_m_shape_array[] = {2, 4, 4, 2};
static const uint8_t block_l_shape_array[] = {3, 5, 5, 3};
#elif CONFIG_MATRIX_32X32
static const uint8_t block_s_shape_array[] = {2, 4, 4, 2};
static const uint8_t block_m_shape_array[] = {3, 5, 5, 3};
static const uint8_t block_l_shape_array[] = {4, 6, 6, 4};
#else
#error "Unknown BLOCK_TYPE"
#endif

static const block_t block_s = {.block_shape_array = block_s_shape_array, .array_size = ARRAY_LENGTH(block_s_shape_array)};
static const block_t block_m = {.block_shape_array = block_m_shape_array, .array_size = ARRAY_LENGTH(block_m_shape_array)};
static const block_t block_l = {.block_shape_array = block_l_shape_array, .array_size = ARRAY_LENGTH(block_l_shape_array)};

static asteroid_t **asteroid_objects = NULL;
static list_t asteroid_indices_list = {0};
static node_t *current_node_ptr = NULL;

static const block_t *block_type_array[] = {&block_s, &block_m, &block_l};

static uint8_t row_offset = 0;

static uint8_t objects_created_counter = 0;
static uint8_t update_call_counter = 0;

static uint32_t sum_of_array_lengths = 0;

typedef void (*nodeCallback)(node_t *node, uint8_t node_position);

// Circular Linked List
static return_val_t insertAtEnd_circular(list_t *p_list, uint32_t *array, uint8_t array_length)
{
    node_t *new_node = malloc(sizeof(node_t));
    if (NULL == new_node)
    {
        ESP_LOGE(TAG, "\nMemory allocation failed while creating:\nnew_node\n");
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
        ESP_LOGW(TAG, "List is empty");
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
        ESP_LOGI(TAG, "node value [%d] = %ld   node position = %d\n", i, node->array[i], node_position);
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
    /// Init of asteroid_objects
    if (NULL == asteroid_objects)
    {
        if (NULL == (asteroid_objects = malloc(sizeof(asteroid_t *))))
        {
            ESP_LOGE(TAG, "Memory allocation failed while creating:\nasteroid_objects\n");
            return MEM_ALLOC_ERROR;
        }
    }
    bool new_asteroid_created = false;
    asteroid_t *new_asteroid = NULL;

    /// Checks if new objects should be created, the update should wait for row_offset number of calls before creating new obj
    if (update_call_counter == 0)
    {
        /// create asteroid object and init it with a random block
        objects_created_counter += 1;

        if (NULL == (asteroid_objects = realloc(asteroid_objects, sizeof(asteroid_t *) * objects_created_counter)) ||
            NULL == (new_asteroid = malloc(sizeof(asteroid_t))))
        {
            ESP_LOGE(TAG, "Memory allocation failed while creating:\nasteroid_objects\nnew_asteroid\n");
            return MEM_ALLOC_ERROR;
        }
        new_asteroid_created = true;
        new_asteroid->call_counter = 0;
        new_asteroid->start_index = NULL;

        new_asteroid->block = block_type_array[esp_random() % ARRAY_LENGTH(block_type_array)];

        asteroid_objects[objects_created_counter - 1] = new_asteroid;
    }
    if (update_call_counter == row_offset)
    {
        update_call_counter = 0;
        row_offset = MIN_OFFSET + (esp_random() % (MAX_OFFSET - MIN_OFFSET));
    }
    else
    {
        update_call_counter += 1;
    }

    /// calc index array size
    uint32_t index_array_size = 0;
    uint8_t expired_object_list[MAX_AST_OBJ_ALLOWED] = {0};
    uint8_t amount_of_expired_objects = 0;
    for (uint8_t i = 0; i < objects_created_counter; i += 1)
    {
        if (asteroid_objects[i]->call_counter < asteroid_objects[i]->block->array_size)
        {
            index_array_size += asteroid_objects[i]->block->block_shape_array[asteroid_objects[i]->call_counter];
            asteroid_objects[i]->call_counter += 1;
        }
        else
        {
            expired_object_list[i] = 1;
            amount_of_expired_objects += 1;
        }
    }

    /// Updating the asteroid_object array, removing expired ones
    asteroid_t **updated_asteroid_objects = NULL;
    if ((objects_created_counter - amount_of_expired_objects) >= 1)
    {
        // assert(objects_created_counter - amount_of_expired_objects >= 1);
        if (NULL == (updated_asteroid_objects = malloc(sizeof(asteroid_t *) * (objects_created_counter - amount_of_expired_objects))))
        {
            ESP_LOGE(TAG, "Memory allocation failed while creating:\nupdated_asteroid_objects\n");
            return MEM_ALLOC_ERROR;
        }

        uint8_t updated_index_counter = 0;
        for (uint8_t i = 0; i < objects_created_counter; i += 1)
        {
            if (1 != expired_object_list[i])
            {
                updated_asteroid_objects[updated_index_counter] = asteroid_objects[i];
                updated_index_counter += 1;
            }
            else
            {
                free(asteroid_objects[i]);
            }
        }
        free(asteroid_objects);
        objects_created_counter -= amount_of_expired_objects;
        asteroid_objects = updated_asteroid_objects;
    }
    else
    {
        for (uint8_t i = 0; i < objects_created_counter; i += 1)
        {
            free(asteroid_objects[i]);
        }
        free(asteroid_objects);
        objects_created_counter = 0;
        /// It should be okay to assign NULL because it is only accessed in loops with the
        /// iterator beeing objects_created_counter this is set to 0 -> loops dont execute
        /// And because its NULL in the next updated call it will be initilaised again
        asteroid_objects = NULL;
    }

    /// Creating the index_array which will be added to a node
    uint32_t *index_array = NULL;
    if (0 != objects_created_counter)
    {
        if (NULL == (index_array = malloc(sizeof(*index_array) * index_array_size)))
        {
            ESP_LOGE(TAG, "Memory allocation failed while creating:\nindex_array\n");
            return MEM_ALLOC_ERROR;
        }
    }

    /// assigns random start positions to the new asteroid object
    if (true == new_asteroid_created)
    {
        for (uint8_t i = 0; i < objects_created_counter; i += 1)
        {
            if (NULL == asteroid_objects[i]->start_index)
            {
                asteroid_objects[i]->start_index = MAX_INDEX - esp_random() % MATRIX_SIDE_LENGTH;
            }
        }
    }

    /// Function to generate the pixels according to the asteroid shape beginning at the start index
    uint8_t values_added_counter = 0;
    for (uint8_t i = 0; i < objects_created_counter; i += 1)
    {
        uint8_t current_asteroid_block_shape_value = asteroid_objects[i]->block->block_shape_array[asteroid_objects[i]->call_counter - 1];
        index_array[values_added_counter] = asteroid_objects[i]->start_index;
        values_added_counter += 1;
        if (0 == current_asteroid_block_shape_value % 2)
        {
            for (uint8_t j = 0; j < (current_asteroid_block_shape_value / 2); j += 1)
            {
                index_array[values_added_counter] =
                    (MAX_INDEX >= asteroid_objects[i]->start_index + j + 1) ? asteroid_objects[i]->start_index + j + 1 : MAX_INDEX;
                values_added_counter += 1;
            }
            for (uint8_t j = 1; j < (current_asteroid_block_shape_value / 2); j += 1)
            {
                index_array[values_added_counter] =
                    (MAX_INDEX - MATRIX_SIDE_LENGTH < asteroid_objects[i]->start_index - j) ? asteroid_objects[i]->start_index - j : MAX_INDEX - (MATRIX_SIDE_LENGTH - 1);
                values_added_counter += 1;
            }
        }
        else
        {
            for (uint8_t j = 0; j < (current_asteroid_block_shape_value / 2); j += 1)
            {
                index_array[values_added_counter] =
                    (MAX_INDEX >= asteroid_objects[i]->start_index + j + 1) ? asteroid_objects[i]->start_index + j + 1 : MAX_INDEX;
                values_added_counter += 1;
                index_array[values_added_counter] =
                    (MAX_INDEX - MATRIX_SIDE_LENGTH < asteroid_objects[i]->start_index - j - 1) ? asteroid_objects[i]->start_index - j - 1 : MAX_INDEX - (MATRIX_SIDE_LENGTH - 1);
                values_added_counter += 1;
            }
        }
    }

    /// Checking if max ring buffer size is reached (= MATRIX_SIDE_LENGTH)
    if (MATRIX_SIDE_LENGTH > asteroid_indices_list.size)
    {
        insertAtEnd_circular(&asteroid_indices_list, index_array, index_array_size);
        /// current_ptr corresponds to the Tail of the list inside this if statement
        traversList_once(&asteroid_indices_list, getCurrentPtr);
    }
    else
    {
        current_node_ptr = current_node_ptr->p_next;
        /// deallocate previously allocated memory for the start index and set it to the new allocated index_array
        free(current_node_ptr->array);
        current_node_ptr->array = index_array;
        current_node_ptr->array_length = index_array_size;
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
        ESP_LOGE(TAG, "Memory allocation failed while creating:\nptr_index_array\nptr_rgb_array\nmatrix_data");
        return MEM_ALLOC_ERROR;
    }

    /// Decrementing (coz Top to Bottom) the indices by the matrix side length -> asteriods fall to next row
    /// The counter = 1 and current_ptr = p_next is because the tail node (= current ptr) does not need to be decremented
    /// if there is no new index data
    uint8_t counter = 1;
    node_t *current_ptr = current_node_ptr->p_next;
    if (0 == index_array_size)
    {
        counter = 0;
        current_ptr = current_node_ptr;
    }
    while (counter < asteroid_indices_list.size)
    {
        for (uint8_t i = 0; i < current_ptr->array_length; i += 1)
        {
            current_ptr->array[i] -= MATRIX_SIDE_LENGTH;
        }
        current_ptr = current_ptr->p_next;
        counter += 1;
    }

    /// Traversing list and copying all list arrays to the ptr_index_array
    uint8_t elements_added_counter = 0;
    counter = 0;
    current_ptr = asteroid_indices_list.p_head;
    while (counter < asteroid_indices_list.size)
    {
        uint8_t array_length = current_ptr->array_length;
        for (uint8_t i = 0; i < array_length; i += 1)
        {
            ptr_index_array[elements_added_counter] = current_ptr->array[i];
            elements_added_counter += 1;
        }
        current_ptr = current_ptr->p_next;
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