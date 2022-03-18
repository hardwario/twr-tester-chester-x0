#ifndef _APPLICATION_H
#define _APPLICATION_H

#ifndef VERSION
#define VERSION "vdev"
#endif

#include <twr.h>
#include <twr_pcal6416aevj.h>
#include <bcl.h>

typedef enum
{
    TEST_ADC = 0,
    TEST_GPIO_1 = 1,
    TEST_GPIO_2 = 2,
    TEST_GPIO_3 = 3,
    TEST_GPIO_4 = 4

} test_state;

typedef enum
{
    TEST_GPIO_STATE_1 = 1,
    TEST_GPIO_STATE_2 = 2,
    TEST_GPIO_STATE_3 = 3,
    TEST_GPIO_STATE_4 = 4,
    TEST_GPIO_STATE_5 = 5,
    TEST_GPIO_STATE_DONE = 6

} gpio_test_state;

#endif
