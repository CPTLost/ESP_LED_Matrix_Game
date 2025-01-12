/// corresponding header to this c file
#include "led_matrix_driver.h"

/// standard librarys
#include <stdio.h>
#include <inttypes.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>

/// Components/other manually created header files
#include "led_strip.h"
#include "return_val.h"

/*Maybe put into SDK config in the main project?*/

#define GPIO_PIN_LED_MATRIX CONFIG_GPIO_PIN_LED_MATRIX

const char *TAG = "LED_MATRIX_DRIVER";

static bool g_led_matrix_configured = false;

static led_strip_handle_t led_matrix;

return_val_t initLedMatrix(void)
{
    // It is not allowed to configure the same Matrix multiple times -> checks if g_led_matrix_configured == true
    if (false == g_led_matrix_configured)
    {
        g_led_matrix_configured = true;
        led_strip_config_t strip_config = {
            .strip_gpio_num = GPIO_PIN_LED_MATRIX,
            .max_leds = MATRIX_SIZE,
            .led_model = LED_MODEL_WS2812,
            .flags.invert_out = false,
        };
        led_strip_rmt_config_t rmt_config = {
            .resolution_hz = 10 * 1000 * 1000, // 10MHz
            .flags.with_dma = false,
        };
        ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_matrix));
        ESP_LOGI(TAG, "LED Matrix configured!");
        return SUCCESS;
    }
    else
    {
        ESP_LOGW(TAG, "LED Matrix ALREADY configured!");
        return ALREADY_CONFIGURED;
    }
}

#ifdef MATRIX_5X5

/// @brief Updates LED Matrix with new data. This data is typically generated by the updateGame() function.
/// @param new_data
/// @return SUCCESS
return_val_t updateLedMatrix(led_matrix_data_t *new_data)
{
    bool set_indices_in_matrix[MATRIX_SIZE] = {0};
    for (uint16_t i = 0; i < new_data->array_length; i += 1)
    {
        set_indices_in_matrix[new_data->ptr_index_array_leds_to_set[i]] = true;
        led_strip_set_pixel(led_matrix,
                            new_data->ptr_index_array_leds_to_set[i],
                            new_data->ptr_rgb_array_leds_to_set[i][0],
                            new_data->ptr_rgb_array_leds_to_set[i][1],
                            new_data->ptr_rgb_array_leds_to_set[i][2]);
    }
    for (uint16_t i = 0; i < MATRIX_SIZE; i += 1)
    {
        if (false == set_indices_in_matrix[i])
        {
            led_strip_set_pixel(led_matrix, new_data->ptr_index_array_leds_to_set[i], 0, 0, 0);
        }
    }
    led_strip_refresh(led_matrix);

    return SUCCESS;
}

#endif

#ifdef MATRIX_16X16
static const uint16_t index_array_of_16x16_led_matrix[MATRIX_SIZE] =
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17,
     16, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52,
     51, 50, 49, 48, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 95, 94, 93, 92, 91, 90, 89, 88, 87,
     86, 85, 84, 83, 82, 81, 80, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 127, 126,
     125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 128, 129, 130, 131, 132, 133, 134, 135, 136,
     137, 138, 139, 140, 141, 142, 143, 159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 144, 160,
     161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 191, 190, 189, 188, 187, 186, 185,
     184, 183, 182, 181, 180, 179, 178, 177, 176, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205,
     206, 207, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210, 209, 208, 224, 225, 226, 227, 228,
     229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244,
     243, 242, 241, 240};

/// @brief Updates LED Matrix with new data. This data is typically generated by the updateGame() function.
/// @param new_data
/// @return SUCCESS
return_val_t updateLedMatrix(led_matrix_data_t *new_data)
{
    bool set_indices_in_matrix[MATRIX_SIZE] = {0};
    for (uint16_t i = 0; i < new_data->array_length; i += 1)
    {
        set_indices_in_matrix[new_data->ptr_index_array_leds_to_set[i]] = true;
        led_strip_set_pixel(led_matrix,
                            index_array_of_16x16_led_matrix[new_data->ptr_index_array_leds_to_set[i]],
                            new_data->ptr_rgb_array_leds_to_set[i][0],
                            new_data->ptr_rgb_array_leds_to_set[i][1],
                            new_data->ptr_rgb_array_leds_to_set[i][2]);
    }
    for (uint16_t i = 0; i < MATRIX_SIZE; i += 1)
    {
        if (false == set_indices_in_matrix[i])
        {
            led_strip_set_pixel(led_matrix, index_array_of_16x16_led_matrix[i], 0, 0, 0);
        }
    }
    led_strip_refresh(led_matrix);

    return SUCCESS;
}

#endif

#ifdef MATRIX_32X32

/// Lookup Tabel for the actual physical LED index in the 32x32 LED Matrix
static const uint16_t index_array_of_32x32_led_matrix[MATRIX_SIZE] =
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 256, 257, 258, 259,
     260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17,
     16, 287, 286, 285, 284, 283, 282, 281, 280, 279, 278, 277, 276, 275, 274, 273, 272, 32, 33, 34, 35, 36, 37, 38, 39, 40,
     41, 42, 43, 44, 45, 46, 47, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 63, 62, 61,
     60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 319, 318, 317, 316, 315, 314, 313, 312, 311, 310, 309, 308, 307, 306,
     305, 304, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 320, 321, 322, 323, 324, 325, 326, 327, 328,
     329, 330, 331, 332, 333, 334, 335, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 351, 350, 349, 348,
     347, 346, 345, 344, 343, 342, 341, 340, 339, 338, 337, 336, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108,
     109, 110, 111, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 127, 126, 125, 124, 123,
     122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 383, 382, 381, 380, 379, 378, 377, 376, 375, 374, 373, 372, 371,
     370, 369, 368, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 384, 385, 386, 387, 388,
     389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147,
     146, 145, 144, 415, 414, 413, 412, 411, 410, 409, 408, 407, 406, 405, 404, 403, 402, 401, 400, 160, 161, 162, 163, 164,
     165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 416, 417, 418, 419, 420, 421, 422, 423, 424, 425, 426, 427, 428,
     429, 430, 431, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176, 447, 446, 445, 444, 443,
     442, 441, 440, 439, 438, 437, 436, 435, 434, 433, 432, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204,
     205, 206, 207, 448, 449, 450, 451, 452, 453, 454, 455, 456, 457, 458, 459, 460, 461, 462, 463, 223, 222, 221, 220, 219,
     218, 217, 216, 215, 214, 213, 212, 211, 210, 209, 208, 479, 478, 477, 476, 475, 474, 473, 472, 471, 470, 469, 468, 467,
     466, 465, 464, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 480, 481, 482, 483, 484,
     485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 495, 255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243,
     242, 241, 240, 511, 510, 509, 508, 507, 506, 505, 504, 503, 502, 501, 500, 499, 498, 497, 496, 512, 513, 514, 515, 516,
     517, 518, 519, 520, 521, 522, 523, 524, 525, 526, 527, 768, 769, 770, 771, 772, 773, 774, 775, 776, 777, 778, 779, 780,
     781, 782, 783, 543, 542, 541, 540, 539, 538, 537, 536, 535, 534, 533, 532, 531, 530, 529, 528, 799, 798, 797, 796, 795,
     794, 793, 792, 791, 790, 789, 788, 787, 786, 785, 784, 544, 545, 546, 547, 548, 549, 550, 551, 552, 553, 554, 555, 556,
     557, 558, 559, 800, 801, 802, 803, 804, 805, 806, 807, 808, 809, 810, 811, 812, 813, 814, 815, 575, 574, 573, 572, 571,
     570, 569, 568, 567, 566, 565, 564, 563, 562, 561, 560, 831, 830, 829, 828, 827, 826, 825, 824, 823, 822, 821, 820, 819,
     818, 817, 816, 576, 577, 578, 579, 580, 581, 582, 583, 584, 585, 586, 587, 588, 589, 590, 591, 832, 833, 834, 835, 836,
     837, 838, 839, 840, 841, 842, 843, 844, 845, 846, 847, 607, 606, 605, 604, 603, 602, 601, 600, 599, 598, 597, 596, 595,
     594, 593, 592, 863, 862, 861, 860, 859, 858, 857, 856, 855, 854, 853, 852, 851, 850, 849, 848, 608, 609, 610, 611, 612,
     613, 614, 615, 616, 617, 618, 619, 620, 621, 622, 623, 864, 865, 866, 867, 868, 869, 870, 871, 872, 873, 874, 875, 876,
     877, 878, 879, 639, 638, 637, 636, 635, 634, 633, 632, 631, 630, 629, 628, 627, 626, 625, 624, 895, 894, 893, 892, 891,
     890, 889, 888, 887, 886, 885, 884, 883, 882, 881, 880, 640, 641, 642, 643, 644, 645, 646, 647, 648, 649, 650, 651, 652,
     653, 654, 655, 896, 897, 898, 899, 900, 901, 902, 903, 904, 905, 906, 907, 908, 909, 910, 911, 671, 670, 669, 668, 667,
     666, 665, 664, 663, 662, 661, 660, 659, 658, 657, 656, 927, 926, 925, 924, 923, 922, 921, 920, 919, 918, 917, 916, 915,
     914, 913, 912, 672, 673, 674, 675, 676, 677, 678, 679, 680, 681, 682, 683, 684, 685, 686, 687, 928, 929, 930, 931, 932,
     933, 934, 935, 936, 937, 938, 939, 940, 941, 942, 943, 703, 702, 701, 700, 699, 698, 697, 696, 695, 694, 693, 692, 691,
     690, 689, 688, 959, 958, 957, 956, 955, 954, 953, 952, 951, 950, 949, 948, 947, 946, 945, 944, 704, 705, 706, 707, 708,
     709, 710, 711, 712, 713, 714, 715, 716, 717, 718, 719, 960, 961, 962, 963, 964, 965, 966, 967, 968, 969, 970, 971, 972,
     973, 974, 975, 735, 734, 733, 732, 731, 730, 729, 728, 727, 726, 725, 724, 723, 722, 721, 720, 991, 990, 989, 988, 987,
     986, 985, 984, 983, 982, 981, 980, 979, 978, 977, 976, 736, 737, 738, 739, 740, 741, 742, 743, 744, 745, 746, 747, 748,
     749, 750, 751, 992, 993, 994, 995, 996, 997, 998, 999, 1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 767, 766, 765, 764,
     763, 762, 761, 760, 759, 758, 757, 756, 755, 754, 753, 752, 1023, 1022, 1021, 1020, 1019, 1018, 1017, 1016, 1015, 1014,
     1013, 1012, 1011, 1010, 1009, 1008};

/// @brief Updates LED Matrix with new data. This data is typically generated by the updateGame() function.
/// @param new_data
/// @return SUCCESS
return_val_t updateLedMatrix(led_matrix_data_t *new_data)
{
    bool set_indices_in_matrix[MATRIX_SIZE] = {0};

    for (uint16_t i = 0; i < new_data->array_length; i += 1)
    {
        set_indices_in_matrix[new_data->ptr_index_array_leds_to_set[i]] = true;
        led_strip_set_pixel(led_matrix,
                            index_array_of_32x32_led_matrix[new_data->ptr_index_array_leds_to_set[i]],
                            new_data->ptr_rgb_array_leds_to_set[i][0],
                            new_data->ptr_rgb_array_leds_to_set[i][1],
                            new_data->ptr_rgb_array_leds_to_set[i][2]);
    }
    for (uint16_t i = 0; i < MATRIX_SIZE; i += 1)
    {
        if (false == set_indices_in_matrix[i])
        {
            led_strip_set_pixel(led_matrix, index_array_of_32x32_led_matrix[i], 0, 0, 0);
        }
    }

    led_strip_refresh(led_matrix);

    return SUCCESS;
}

#endif