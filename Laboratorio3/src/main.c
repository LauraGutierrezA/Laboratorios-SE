#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_rom_sys.h"

#define UART_PORT   UART_NUM_0
#define TXD_PIN     1
#define RDX_PIN     3

// Motor
#define IN1 GPIO_NUM_16
#define IN2 GPIO_NUM_17   
#define IN3 GPIO_NUM_18   
#define IN4 GPIO_NUM_19


#define CALEF 23

#define LED_PWM_PIN 22


void motor_apagar(void){
    gpio_set_level(IN1, 0);
    gpio_set_level(IN2, 0);
    gpio_set_level(IN3, 0);
    gpio_set_level(IN4, 0);
}

// Wave-drive (un pin a la vez) — misma secuencia del código que funciona
void paso_horario(int paso){
    motor_apagar();
    switch(paso){
        case 0: gpio_set_level(IN1, 1); break;
        case 1: gpio_set_level(IN2, 1); break;
        case 2: gpio_set_level(IN3, 1); break;
        case 3: gpio_set_level(IN4, 1); break;
    }
}

void paso_antihorario(int paso){
    motor_apagar();
    switch(paso){
        case 0: gpio_set_level(IN4, 1); break;
        case 1: gpio_set_level(IN3, 1); break;
        case 2: gpio_set_level(IN2, 1); break;
        case 3: gpio_set_level(IN1, 1); break;
    }
}

// Globales para el log UART (igual que el código principal)
int direccion_glob  = 0;
int velocidad_glob  = 0;
static int paso_glob = 0;   // paso persistente entre llamadas

void motor_mover(int direccion, int velocidad){
    direccion_glob = direccion;
    velocidad_glob = velocidad;

    if(velocidad <= 0 || direccion == 0){
        motor_apagar();
        return;
    }

    // pasos_por_ciclo: cuántos pasos dar en este ciclo de 10 ms
    // 100 steps/s → 1 paso,  300 → 3,  600 → 6
    int pasos_por_ciclo = velocidad / 100;

    for(int k = 0; k < pasos_por_ciclo; k++){
        if(direccion == 1){
            paso_horario(paso_glob);
        } else {
            paso_antihorario(paso_glob);
        }

        paso_glob++;
        if(paso_glob >= 4) paso_glob = 0;

        esp_rom_delay_us(1000);   // pausa entre pasos dentro del mismo ciclo
    }
}


adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t         adc_cali_handle;


void app_main(void){

    vTaskPrioritySet(NULL, 0); // Para no asfixiar al WDT

    // COnfiguración UART
    uart_config_t uart_config = {
        .baud_rate  = 9600,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, TXD_PIN, RDX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_0, 1024, 1024, 0, NULL, 0);

    // Configuración ADC
    adc_oneshot_unit_init_cfg_t init_config = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN_DB_12       // rango 0-3.3 V
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &chan_config); // LM35
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &chan_config); // LDR

    adc_cali_line_fitting_config_t cali_config = {
        .unit_id  = ADC_UNIT_1,
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_cali_create_scheme_line_fitting(&cali_config, &adc_cali_handle);

    // Config INs motor y Calefacción
    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << IN1) | (1ULL << IN2) |
                        (1ULL << IN3) | (1ULL << IN4) |
                        (1ULL << CALEF),
        .mode         = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&out_cfg);
    motor_apagar();

    // Config Timer
    timer_config_t timer_conf_main = {
        .divider     = 80,              // 80 MHz / 80 = 1 tick por µs
        .counter_dir = TIMER_COUNT_UP,
        .counter_en  = TIMER_PAUSE,
        .alarm_en    = TIMER_ALARM_DIS,
        .auto_reload = false
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &timer_conf_main);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);

    // COnfig PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .timer_num      = LEDC_TIMER_0,
        .duty_resolution= LEDC_TIMER_12_BIT,
        .freq_hz        = 5000,
        .clk_cfg        = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = LED_PWM_PIN,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&ledc_channel);

    // Mensaje Inicio
    char *mensaje_inicio = "\nLABORATORIO DE DOMÓTICA - LECTURA DE SENSORES\n";
    uart_write_bytes(UART_PORT, mensaje_inicio, strlen(mensaje_inicio));

    uint8_t data;
    int Tc = 23;

    int raw_temp, raw_luz;
    int temp_mv,  luz_mv;
    int temp, luz;

    char mensaje_temp[100];
    char mensaje_car[100];
    char rx_buffer[64];
    int  i = 0;

    uint64_t tiempo_main_us;
    uint64_t ultimo_tiempo_impresion_us = 0;

    // Calibración LDR 
    int mV_ilumMax = 3139;
    int mV_ilumMin = 1990;

    uint16_t porcentaje_leds = 0;


    while(1){

        // Timer para Impresión
        timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &tiempo_main_us);

        // Leer ADC
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &raw_temp);
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &raw_luz);
        adc_cali_raw_to_voltage(adc_cali_handle, raw_temp, &temp_mv);
        adc_cali_raw_to_voltage(adc_cali_handle, raw_luz,  &luz_mv);

        temp = temp_mv / 10.0;
        luz  = (100 * (luz_mv - mV_ilumMax) / (mV_ilumMax - mV_ilumMin)) + 100;
        if(luz < 0)   luz = 0;
        if(luz > 100) luz = 100;

        // Recibir SET_TEMP:
        int len = uart_read_bytes(UART_PORT, &data, 1, 0);
        if(len > 0){
            rx_buffer[i] = data;
            i++;
            if(i >= 63) i = 0;

            if(data == '\n' || data == '\r'){
                rx_buffer[i] = '\0';
                int Tc_nueva;
                if(sscanf(rx_buffer, "SET_TEMP:%d", &Tc_nueva) == 1){
                    Tc = Tc_nueva;
                    char *conf = "Temperatura control actualizada\n";
                    uart_write_bytes(UART_PORT, conf, strlen(conf));
                }
                i = 0;
            }
        }

        // Impresión sostenida
        if((tiempo_main_us - ultimo_tiempo_impresion_us) >= 1000000){
            sprintf(mensaje_temp,
                "Luz[mV] %d | Luz[%%] %d | Temp[mV] = %d | Temp[°] = %d | Tc[°] = %d \r\n",
                luz_mv, luz, temp_mv, temp, Tc);
            uart_write_bytes(UART_PORT, mensaje_temp, strlen(mensaje_temp));

            sprintf(mensaje_car,
                "Steps motor = %d | Sentido Motor = %d | Calefaccion = %d \r\n",
                velocidad_glob, direccion_glob, gpio_get_level(CALEF));
            uart_write_bytes(UART_PORT, mensaje_car, strlen(mensaje_car));

            ultimo_tiempo_impresion_us = tiempo_main_us;
        }

        // FSM Temp
        if(temp < (Tc - 1)){
            gpio_set_level(CALEF, 1);
            motor_mover(1, 100);
        }
        else if((Tc - 1) <= temp && temp <= (Tc + 1)){
            gpio_set_level(CALEF, 0);
            motor_mover(0, 0);
        }
        else if((Tc + 1) < temp && temp <= (Tc + 3)){
            gpio_set_level(CALEF, 0);
            motor_mover(-1, 100);
        }
        else if((Tc + 3) < temp && temp <= (Tc + 5)){
            gpio_set_level(CALEF, 0);
            motor_mover(-1, 300);
        }
        else if(temp > (Tc + 5)){
            gpio_set_level(CALEF, 0);
            motor_mover(-1, 600);
        }

        //FSM
        if(luz < 20){
            porcentaje_leds = 100;
        } else if(luz <= 30){
            porcentaje_leds = 80;
        } else if(luz <= 40){
            porcentaje_leds = 60;
        } else if(luz <= 60){
            porcentaje_leds = 50;
        } else if(luz <= 80){
            porcentaje_leds = 30;
        } else {
            porcentaje_leds = 0;
        }

        uint32_t duty_cycle = ((100 - porcentaje_leds) * 4095) / 100;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}