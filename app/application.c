// Tower Kit documentation https://tower.hardwario.com/
// SDK API description https://sdk.hardwario.com/
// Forum https://forum.hardwario.com/

#define GPIO_TEST_SETUP_TIME 1500
#define GPIO_SUB_TEST_STEP_WAIT_TIME 2000

#include <application.h>

// LED instance
twr_led_t led;

// Expander insance
twr_pcal6416aevj_t pcal6416aevj;

// Proggress of the whole test sequence
int test_progress = 0;

twr_button_t button_left;
twr_button_t button_right;

// Initial state for the tester function
test_state state = TEST_EXPANDER;

// Initial state for the GPIO sub test
gpio_test_state gpio_state = TEST_GPIO_STATE_1;

twr_scheduler_task_id_t lcd_print_task;
twr_scheduler_task_id_t tester_task;
twr_scheduler_task_id_t gpio_test_task;

twr_led_t lcdLedRed;
twr_led_t lcdLedGreen;
twr_led_t lcdLedBlue;

// Results of each test
bool test_results[5];

// Some flags for the tester
bool gpio_test_done = false;
bool gpio_test_in_progress = false;
bool gpio_test_setup_done = false;
bool x0A_version = false;

// Result voltages of each GPIO sub test
float gpio_test_results_voltages[5];

// Parameters for the GPIO tests
twr_pcal6416aevj_pin_t pu_pin;
twr_pcal6416aevj_pin_t on_pin;
twr_pcal6416aevj_pin_t pd_pin;
twr_pcal6416aevj_pin_t cl_pin;
twr_adc_channel_t adc_channel;


void tester();
void lcd_print_results();

// Task to print the results onto LCD 
void lcd_print_results()
{
    if(!twr_module_lcd_is_ready())
    {
        twr_scheduler_plan_current_relative(20);
        return;
    }

    twr_system_pll_enable();

    twr_module_lcd_clear();

    twr_module_lcd_set_font(&twr_font_ubuntu_15);
    twr_module_lcd_draw_string(0, 5, "CHESTER TESTER X0", 1);
    twr_module_lcd_draw_string(0, 15, "--------------------------", 1);

    if(test_progress == 0)
    {
        twr_module_lcd_set_font(&twr_font_ubuntu_13);
        twr_module_lcd_draw_string(15, 43, "Insert chester X0", 1);
        twr_module_lcd_draw_string(40, 58, "and hold", 1);
        twr_module_lcd_draw_string(8, 73, "right button for X0A", 1);
        twr_module_lcd_draw_string(12, 88, "left button for X0B", 1);

    }

    twr_module_lcd_set_font(&twr_font_ubuntu_13);

    if(test_progress > 0)
    {
        if(test_results[0])
        {
            twr_module_lcd_draw_string(0, 25, "Expander test: OK", 1);
        }
        else
        {
            twr_module_lcd_draw_string(3, 25, "Expander test: FAILED", 1);
        }
    }

    if(test_progress > 1)
    {
        if(test_results[1])
        {
            twr_module_lcd_draw_string(0, 35, "GP0/A0 test: OK", 1);
        }
        else
        {
            twr_module_lcd_draw_string(0, 35, "GP0/A0 test: FAILED", 1);
        }
    }

    if(test_progress > 2)
    {
        if(test_results[2])
        {
            twr_module_lcd_draw_string(0, 45, "GP1/A1 test: OK", 1);
        }
        else
        {
            twr_module_lcd_draw_string(0, 45, "GP1/A1 test: FAILED", 1);
        }
    }

    if(test_progress > 3)
    {
        if(test_results[3])
        {
            twr_module_lcd_draw_string(0, 55, "GP2/A2 test: OK", 1);
        }
        else
        {
            twr_module_lcd_draw_string(0, 55, "GP2/A2 test: FAILED", 1);
        }
    }

    if(test_progress > 4)
    {
        if(test_results[4])
        {
            twr_module_lcd_draw_string(0, 65, "GP3/A3 test: OK", 1);
        }
        else
        {
            twr_module_lcd_draw_string(0, 65, "GP3/A3 test: FAILED", 1);
        }

        twr_module_lcd_draw_string(0, 75, "--------------------------", 1);

        twr_module_lcd_draw_string(0, 85, "Hold right button", 1);
        twr_module_lcd_draw_string(0, 95, "to test another", 1);
        twr_module_lcd_draw_string(0, 105, "chester X0", 1);

        int numberOfErrors = 0;
        for(int i = 0; i < 5; i++)
        {
            if(!test_results[i])
            {
                numberOfErrors++;
            }
        }

        if(numberOfErrors == 0)
        {
            twr_led_set_mode(&lcdLedGreen, TWR_LED_MODE_ON);
        }
        else
        {
            twr_led_set_mode(&lcdLedRed, TWR_LED_MODE_ON);
        }

    }

    twr_module_lcd_update();

    twr_system_pll_disable();

    twr_scheduler_plan_current_relative(200);
}

// Reset all the pins into the default state (INPUT with 1 at the output register). Cleanup after each test
void reset_gpio()
{
    twr_pcal6416aevj_set_pin_direction(&pcal6416aevj, pu_pin, TWR_PCAL6416AEVJ_PIN_DIRECTION_INPUT);
    twr_pcal6416aevj_set_pin_direction(&pcal6416aevj, on_pin, TWR_PCAL6416AEVJ_PIN_DIRECTION_INPUT);
    twr_pcal6416aevj_set_pin_direction(&pcal6416aevj, pd_pin, TWR_PCAL6416AEVJ_PIN_DIRECTION_INPUT);
    twr_pcal6416aevj_set_pin_direction(&pcal6416aevj, cl_pin, TWR_PCAL6416AEVJ_PIN_DIRECTION_INPUT);

    twr_pcal6416aevj_write_pin(&pcal6416aevj, pu_pin, 1);
    twr_pcal6416aevj_write_pin(&pcal6416aevj, on_pin, 1);
    twr_pcal6416aevj_write_pin(&pcal6416aevj, pd_pin, 1);
    twr_pcal6416aevj_write_pin(&pcal6416aevj, cl_pin, 1);
}

// Get the voltage for each GPIO sub test
void gpio_test_get_voltage(int index)
{
    uint16_t adc;
    twr_adc_get_value(adc_channel, &adc);

    float vdda_voltage;
    twr_adc_get_vdda_voltage(&vdda_voltage);
    
    twr_log_debug("REF %.5f", vdda_voltage);

    float voltage = (adc * vdda_voltage) / 65536.f;

    twr_log_debug("TEST %d: %.5f", index + 1, voltage);
    gpio_test_results_voltages[index] = voltage;

    reset_gpio();
}

// Task that takes care of the GPIO sub tests.
void gpio_test()
{
    switch(gpio_state)
    {
        // First GPIO sub test PUx disable/ONx disable/PDx disable/CLx disable
        case TEST_GPIO_STATE_1:
            if(!gpio_test_setup_done)
            {
                reset_gpio();
                gpio_test_setup_done = true;

                twr_scheduler_plan_current_relative(GPIO_TEST_SETUP_TIME);
                return;
            }

            gpio_test_get_voltage(0);

            gpio_state = TEST_GPIO_STATE_2;

            gpio_test_setup_done = false;
            twr_scheduler_plan_current_relative(GPIO_SUB_TEST_STEP_WAIT_TIME);
            return;

        // Second GPIO sub test PUx enable(Output => 0)/ONx disable/PDx disable/CLx disable
        case TEST_GPIO_STATE_2:
            if(!gpio_test_setup_done)
            {
                twr_pcal6416aevj_set_pin_direction(&pcal6416aevj, pu_pin, TWR_PCAL6416AEVJ_PIN_DIRECTION_OUTPUT);
                twr_pcal6416aevj_write_pin(&pcal6416aevj, pu_pin, 0);

                gpio_test_setup_done = true;
                twr_scheduler_plan_current_relative(GPIO_TEST_SETUP_TIME);
                return;
            }

            gpio_test_get_voltage(1);

            if(x0A_version)
            {
                gpio_state = TEST_GPIO_STATE_3;
            }
            else
            {
                gpio_state = TEST_GPIO_STATE_4;
            }

            gpio_test_setup_done = false;
            twr_scheduler_plan_current_relative(GPIO_SUB_TEST_STEP_WAIT_TIME);
            return;
        
        // Third GPIO sub test PUx disable/ONx enable(Output => 0)/PDx enable(Output => 1)/CLx disable
        case TEST_GPIO_STATE_3:
            if(!gpio_test_setup_done)
            {
                twr_pcal6416aevj_set_pin_direction(&pcal6416aevj, on_pin, TWR_PCAL6416AEVJ_PIN_DIRECTION_OUTPUT);
                twr_pcal6416aevj_write_pin(&pcal6416aevj, on_pin, 0);
                twr_pcal6416aevj_set_pin_direction(&pcal6416aevj, pd_pin, TWR_PCAL6416AEVJ_PIN_DIRECTION_OUTPUT);
                twr_pcal6416aevj_write_pin(&pcal6416aevj, pd_pin, 1);

                gpio_test_setup_done = true;
                twr_scheduler_plan_current_relative(GPIO_TEST_SETUP_TIME);
                return;
            }

            gpio_test_get_voltage(2);
            
            gpio_state = TEST_GPIO_STATE_4;

            gpio_test_setup_done = false;
            twr_scheduler_plan_current_relative(GPIO_SUB_TEST_STEP_WAIT_TIME);
            return;

        // Forth GPIO sub test PUx enable(Output => 0)/ONx disable/PDx disable/CLx enable(Output => 1)
        case TEST_GPIO_STATE_4:

            if(!gpio_test_setup_done)
            {
                twr_pcal6416aevj_set_pin_direction(&pcal6416aevj, pu_pin, TWR_PCAL6416AEVJ_PIN_DIRECTION_OUTPUT);
                twr_pcal6416aevj_write_pin(&pcal6416aevj, pu_pin, 0);
                twr_pcal6416aevj_set_pin_direction(&pcal6416aevj, cl_pin, TWR_PCAL6416AEVJ_PIN_DIRECTION_OUTPUT);
                twr_pcal6416aevj_write_pin(&pcal6416aevj, cl_pin, 1);

                gpio_test_setup_done = true;
                twr_scheduler_plan_current_relative(GPIO_TEST_SETUP_TIME);
                return;
            }

            gpio_test_get_voltage(3);
            
            gpio_state = TEST_GPIO_STATE_5;

            gpio_test_setup_done = false;
            twr_scheduler_plan_current_relative(GPIO_SUB_TEST_STEP_WAIT_TIME);
            return;

        // Forth GPIO sub test PUx enable(Output => 0)/ONx disable/PDx enable(Output => 1)/CLx disable
        case TEST_GPIO_STATE_5:

            if(!gpio_test_setup_done)
            {
                twr_pcal6416aevj_set_pin_direction(&pcal6416aevj, pu_pin, TWR_PCAL6416AEVJ_PIN_DIRECTION_OUTPUT);
                twr_pcal6416aevj_write_pin(&pcal6416aevj, pu_pin, 0);
                twr_pcal6416aevj_set_pin_direction(&pcal6416aevj, pd_pin, TWR_PCAL6416AEVJ_PIN_DIRECTION_OUTPUT);
                twr_pcal6416aevj_write_pin(&pcal6416aevj, pd_pin, 1);

                gpio_test_setup_done = true;
                twr_scheduler_plan_current_relative(GPIO_TEST_SETUP_TIME);
                return;
            }

            gpio_test_get_voltage(4);
            
            gpio_state = TEST_GPIO_STATE_DONE;

            gpio_test_setup_done = false;
            twr_scheduler_plan_current_relative(GPIO_SUB_TEST_STEP_WAIT_TIME);
            return;

        // Evaluation of the whole GPIO test
        case TEST_GPIO_STATE_DONE:
            if(x0A_version)
            {
                if(gpio_test_results_voltages[0] <= gpio_test_results_voltages[3] &&
                   gpio_test_results_voltages[0] < 0.02f &&
                   gpio_test_results_voltages[1] < 0.15f && gpio_test_results_voltages[1] > 0.08f &&
                   gpio_test_results_voltages[2] < 0.47f && gpio_test_results_voltages[2] > 0.42f &&
                   gpio_test_results_voltages[3] < 0.04f &&
                   gpio_test_results_voltages[4] > 0.007 && gpio_test_results_voltages[4] < 0.03f)
                {
                    test_results[test_progress] = true;
                }
                else
                {
                    test_results[test_progress] = false;
                }
            }
            else
            {
                if(gpio_test_results_voltages[0] <= gpio_test_results_voltages[3] &&
                   gpio_test_results_voltages[0] < 0.02f &&
                   gpio_test_results_voltages[1] < 0.15f && gpio_test_results_voltages[1] > 0.08f &&
                   gpio_test_results_voltages[3] < 0.04f &&
                   gpio_test_results_voltages[4] > 0.007 && gpio_test_results_voltages[4] < 0.03f)
                {
                    test_results[test_progress] = true;
                }
                else
                {
                    test_results[test_progress] = false;
                }
            }
            
            gpio_test_done = true;
            gpio_state = TEST_GPIO_STATE_1;

            return;
        
        default:
            test_results[test_progress] = false;
    }
}

// LCD button handler. Left hold starts the test for Chester X0B (GPIO sub test 3 is skiped), Right hold starts the test for Chester X0A
void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    (void) self;

    if(self == &button_right && event == TWR_BUTTON_EVENT_HOLD && test_progress == 0)
    {
        x0A_version = true;
        twr_scheduler_plan_now(tester_task);
    }
    if(self == &button_left && event == TWR_BUTTON_EVENT_HOLD && test_progress == 0)
    {
        x0A_version = false;
        twr_scheduler_plan_now(tester_task);
    }
    else if(self == &button_right && event == TWR_BUTTON_EVENT_HOLD && test_progress > 4)
    {
        x0A_version = true;

        twr_led_set_mode(&lcdLedRed, TWR_LED_MODE_OFF);
        twr_led_set_mode(&lcdLedGreen, TWR_LED_MODE_OFF);
        twr_led_set_mode(&lcdLedBlue, TWR_LED_MODE_OFF);

        test_progress = 0;
        state = TEST_EXPANDER;
        twr_scheduler_plan_now(tester_task);
    }
    else if(self == &button_left && event == TWR_BUTTON_EVENT_HOLD && test_progress > 4)
    {
        x0A_version = false;

        twr_led_set_mode(&lcdLedRed, TWR_LED_MODE_OFF);
        twr_led_set_mode(&lcdLedGreen, TWR_LED_MODE_OFF);
        twr_led_set_mode(&lcdLedBlue, TWR_LED_MODE_OFF);

        test_progress = 0;
        state = TEST_EXPANDER;
        twr_scheduler_plan_now(tester_task);
    }
}

// Application initialization function which is called once after boot
void application_init(void)
{
    // Initialize logging
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, 0);
    twr_led_pulse(&led, 2000);

    // Initialize ADC
    twr_adc_init();
    twr_adc_calibration();

    twr_adc_resolution_set(TWR_ADC_CHANNEL_A2, TWR_ADC_RESOLUTION_12_BIT);
    twr_adc_resolution_set(TWR_ADC_CHANNEL_A3, TWR_ADC_RESOLUTION_12_BIT);
    twr_adc_resolution_set(TWR_ADC_CHANNEL_A4, TWR_ADC_RESOLUTION_12_BIT);
    twr_adc_resolution_set(TWR_ADC_CHANNEL_A5, TWR_ADC_RESOLUTION_12_BIT);

    // Initialize LCD
    twr_module_lcd_init();
    twr_module_lcd_set_font(&twr_font_ubuntu_13);
    twr_module_lcd_update();

    const twr_button_driver_t* lcdButtonDriver =  twr_module_lcd_get_button_driver();
    twr_button_init_virtual(&button_left, 0, lcdButtonDriver, 0);
    twr_button_init_virtual(&button_right, 1, lcdButtonDriver, 0);
    twr_button_set_event_handler(&button_left, button_event_handler, (int*)0);
    twr_button_set_event_handler(&button_right, button_event_handler, (int*)1);

    const twr_led_driver_t* driver = twr_module_lcd_get_led_driver();
    twr_led_init_virtual(&lcdLedRed, TWR_MODULE_LCD_LED_RED, driver, 1);
    twr_led_init_virtual(&lcdLedGreen, TWR_MODULE_LCD_LED_GREEN, driver, 1);
    twr_led_init_virtual(&lcdLedBlue, TWR_MODULE_LCD_LED_BLUE, driver, 1);

    twr_button_set_hold_time(&button_left, 300);
    twr_button_set_hold_time(&button_right, 300);

    twr_button_set_debounce_time(&button_left, 30);
    twr_button_set_debounce_time(&button_right, 30);

    // Set up all the tasks
    twr_scheduler_plan_relative(0, 10000);
    lcd_print_task = twr_scheduler_register(lcd_print_results, NULL, 1000);
    tester_task = twr_scheduler_register(tester, NULL, TWR_TICK_INFINITY);
    gpio_test_task = twr_scheduler_register(gpio_test, NULL, TWR_TICK_INFINITY);
}

// State machine of the whole tester
void tester()
{
    switch(state)
    {
        // Simple test if the expander is communicatig well
        case TEST_EXPANDER:
            if(!twr_pcal6416aevj_init(&pcal6416aevj, TWR_I2C_I2C0, 0x20))
            {
                test_results[test_progress] = false;

                state = TEST_GPIO_1;
                test_progress++;
                twr_scheduler_plan_current_from_now(100);
                return;
            }
            else
            {
                test_results[test_progress] = true;

                state = TEST_GPIO_1;
                test_progress++;
                twr_scheduler_plan_current_from_now(500);
                return;
            }
            break;

            // Test of the GP0/A0 pin
            case TEST_GPIO_1:
                if(!test_results[0])
                {
                    test_results[test_progress] = false;
                    test_progress++;
                    state = TEST_GPIO_2;

                    twr_scheduler_plan_current_from_now(100);
                    return;
                }

                //Pin setup for the test PU_PIN = P15 / ON_PIN = P13 / PD_PIN = P17 / CL_PIN = P16
                if(!gpio_test_in_progress)
                {
                    twr_log_debug("GPIO 1 START");
                    gpio_test_in_progress = true;
                    pu_pin = TWR_PCAL6416AEVJ_PIN_P1_5;
                    on_pin = TWR_PCAL6416AEVJ_PIN_P1_3;
                    pd_pin = TWR_PCAL6416AEVJ_PIN_P1_7;
                    cl_pin = TWR_PCAL6416AEVJ_PIN_P1_6;
                    adc_channel = TWR_ADC_CHANNEL_A2;

                    twr_scheduler_plan_now(gpio_test_task);
                }
                if(!gpio_test_done)
                {
                    twr_scheduler_plan_current_from_now(50);
                    return;
                }   
                else
                {
                    gpio_test_in_progress = false;
                    gpio_test_done = false;

                    state = TEST_GPIO_2;
                    test_progress++;
                    twr_log_debug("GPIO 1 DONE");

                    twr_log_debug("----------------------------------------------------------------");

                    twr_scheduler_plan_current_from_now(1000);
                    return;
                }

            // Test of the GP1/A1 pin
            case TEST_GPIO_2:
                if(!test_results[0])
                {
                    test_results[test_progress] = false;
                    test_progress++;
                    state = TEST_GPIO_3;

                    twr_scheduler_plan_current_from_now(100);
                    return;
                }

                //Pin setup for the test PU_PIN = P10 / ON_PIN = P12 / PD_PIN = P14 / CL_PIN = P11
                if(!gpio_test_in_progress)
                {
                    twr_log_debug("GPIO 2 START");
                    gpio_test_in_progress = true;
                    pu_pin = TWR_PCAL6416AEVJ_PIN_P1_0;
                    on_pin = TWR_PCAL6416AEVJ_PIN_P1_2;
                    pd_pin = TWR_PCAL6416AEVJ_PIN_P1_4;
                    cl_pin = TWR_PCAL6416AEVJ_PIN_P1_1;
                    adc_channel = TWR_ADC_CHANNEL_A3;

                    twr_scheduler_plan_now(gpio_test_task);
                }
                if(!gpio_test_done)
                {
                    twr_scheduler_plan_current_from_now(50);
                    return;
                }   
                else
                {
                    gpio_test_in_progress = false;
                    gpio_test_done = false;

                    twr_log_debug("GPIO 2 DONE");
                    twr_log_debug("----------------------------------------------------------------");

                    state = TEST_GPIO_3;
                    test_progress++;

                    twr_scheduler_plan_current_from_now(1000);
                    return;
                }

            // Test of the GP2/A2 pin
            case TEST_GPIO_3:
                if(!test_results[0])
                {
                    test_results[test_progress] = false;
                    test_progress++;
                    state = TEST_GPIO_4;

                    twr_scheduler_plan_current_from_now(100);
                    return;
                }

                //Pin setup for the test PU_PIN = P05 / ON_PIN = P03 / PD_PIN = P07 / CL_PIN = P06
                if(!gpio_test_in_progress)
                {
                    twr_log_debug("GPIO 3 START");
                    gpio_test_in_progress = true;

                    pu_pin = TWR_PCAL6416AEVJ_PIN_P0_5;
                    on_pin = TWR_PCAL6416AEVJ_PIN_P0_3;
                    pd_pin = TWR_PCAL6416AEVJ_PIN_P0_7;
                    cl_pin = TWR_PCAL6416AEVJ_PIN_P0_6;
                    adc_channel = TWR_ADC_CHANNEL_A4;

                    twr_scheduler_plan_now(gpio_test_task);
                }
                if(!gpio_test_done)
                {
                    twr_scheduler_plan_current_from_now(50);
                    return;
                }   
                else
                {
                    gpio_test_in_progress = false;
                    gpio_test_done = false;

                    twr_log_debug("GPIO 3 DONE");
                    twr_log_debug("----------------------------------------------------------------");

                    state = TEST_GPIO_4;
                    test_progress++;

                    twr_scheduler_plan_current_from_now(1000);
                    return;
                }

            // Test of the GP3/A3 pin
            case TEST_GPIO_4:
                if(!test_results[0])
                {
                    test_results[test_progress] = false;
                    test_progress++;

                    return;
                }

                //Pin setup for the test PU_PIN = P02 / ON_PIN = P00 / PD_PIN = P04 / CL_PIN = P01
                if(!gpio_test_in_progress)
                {
                    twr_log_debug("GPIO 4 START");
                    gpio_test_in_progress = true;

                    pu_pin = TWR_PCAL6416AEVJ_PIN_P0_2;
                    on_pin = TWR_PCAL6416AEVJ_PIN_P0_0;
                    pd_pin = TWR_PCAL6416AEVJ_PIN_P0_4;
                    cl_pin = TWR_PCAL6416AEVJ_PIN_P0_1;
                    adc_channel = TWR_ADC_CHANNEL_A5;

                    twr_scheduler_plan_now(gpio_test_task);
                }
                if(!gpio_test_done)
                {
                    twr_scheduler_plan_current_from_now(50);
                    return;
                }   
                else
                {
                    gpio_test_in_progress = false;
                    gpio_test_done = false;

                    test_progress++;

                    twr_log_debug("GPIO 4 DONE");
                    twr_log_debug("----------------------------------------------------------------");
                    return;
                }
        default:
            return;
    }
}
