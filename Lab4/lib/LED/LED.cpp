#include "LED.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

Led::Led(uint8_t gpio) {
    _pin = gpio;
}

Led::~Led() {
    gpio_reset_pin((gpio_num_t)_pin);
}

void Led::init(void){
    gpio_config_t io_conf = {};
    
    io_conf.pin_bit_mask = (1ULL << _pin);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    gpio_config(&io_conf);
}

void Led::on(void){
    gpio_set_level((gpio_num_t)_pin, 1);
}

void Led::off(void){
    gpio_set_level((gpio_num_t)_pin, 0);
}

void Led::toggle(uint8_t freq_hz){
    gpio_set_level((gpio_num_t)_pin, 1); 
    vTaskDelay(pdMS_TO_TICKS(1000 / (2 * freq_hz)));

    gpio_set_level((gpio_num_t)_pin, 0); 
    vTaskDelay(pdMS_TO_TICKS(1000 / (2 * freq_hz)));
}

void Led::breathe(uint16_t speed){
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.timer_num = LEDC_TIMER_0; 
    timer_conf.duty_resolution = LEDC_TIMER_10_BIT; 
    timer_conf.freq_hz = 5000; 
    timer_conf.clk_cfg = LEDC_AUTO_CLK;

    ledc_timer_config(&timer_conf); 

    ledc_channel_config_t channel_conf = {};
    channel_conf.gpio_num = _pin;
    channel_conf.speed_mode = LEDC_LOW_SPEED_MODE; 
    channel_conf.channel = LEDC_CHANNEL_0;
    channel_conf.intr_type = LEDC_INTR_DISABLE;
    channel_conf.timer_sel = LEDC_TIMER_0;
    channel_conf.duty = 0;
    channel_conf.hpoint = 0; 

    ledc_channel_config(&channel_conf); 

    for (int duty = 0; duty < 1024; duty += 10) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0); 
        vTaskDelay(pdMS_TO_TICKS(speed));  
    }

    for (int duty = 1023; duty >= 0; duty -= 10) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0); 
        vTaskDelay(pdMS_TO_TICKS(speed));  
    }
}

