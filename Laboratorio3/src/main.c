#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define UART_PORT UART_NUM_0 //Porque 0 es el del USB
#define TXD_PIN 1
#define RDX_PIN 3



adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc_cali_handle;

void app_main(void){
	uart_config_t uart_config = {
		.baud_rate = 9600, //El único que se toca por si necesito más velocidad
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE, 
		.stop_bits = UART_STOP_BITS_1, 
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE, 
		.source_clk = UART_SCLK_DEFAULT,
	};
	uart_param_config(UART_NUM_0, &uart_config); // Puedo usar NUM_0, 1 o 2
	uart_set_pin(UART_NUM_0, TXD_PIN, RDX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE); //Cambiamos TDX_PIN y RDX_pin por el pin que queremos
	uart_driver_install(UART_NUM_0, 1024, 1024, 0, NULL, 0);//inicializa el módulo del ESP. 
	//los 1024 son buffers que son vectores (en este caso de 1024 bits por defecto). Si el sistema tiene muy poquitos recursos, ahí si calcula
	
    adc_oneshot_unit_init_cfg_t init_config = {
	    .unit_id = ADC_UNIT_1
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, 
        .atten = ADC_ATTEN_DB_12 //rango 0-3.3c por atten 12db
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &chan_config); //LM35
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &chan_config); //LDR
    

    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1, 
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT, 
    };
    adc_cali_create_scheme_line_fitting(&cali_config, &adc_cali_handle); 


	char *mensaje_inicio = "\nLABORATORIO DE DOMÓTICA - LECTURA DE SENSORES\n";
	//El puntero * dice a partir de cual posición de memoria empieza a contar el mensaje
	uart_write_bytes(UART_PORT, mensaje_inicio, strlen(mensaje_inicio)); 
	//Esta función, a diferencia de printf, es muy eficiente porque solo manda lo que necesita, no crea espacios innecesarios

    //Declaraciones
	uint8_t data;
    int Tc = 25;
    int Tsens = 26; 
    int Ilum = 60; 

    int raw_temp, raw_luz; 
    int temp_mv, luz_mv; 
    int temp, luz;

    char mensaje_salida[100]; 
    char mensaje_temp[100]; 
    char rx_buffer[64];
    int i = 0; 

    int hold_impresion = 0;



    //Cali LDR
    int mV_ilumMax = 3139;
    int mV_ilimMin = 1883;

	while(1) {
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &raw_temp);
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &raw_luz);
        adc_cali_raw_to_voltage(adc_cali_handle, raw_temp, &temp_mv); 
        adc_cali_raw_to_voltage(adc_cali_handle, raw_luz, &luz_mv); 

        temp = temp_mv/10.0;
        luz = 100*luz_mv/mV_ilumMax;

		int len = uart_read_bytes(UART_PORT, &data, 1, pdMS_TO_TICKS(10)); 

		if(len>0){
            rx_buffer[i] = data; 
            i = i+1; 

			//Sedguro para desbordamiento 
			if(i>=63){
				i = 0; 
			}

            if(data == '\n' || data == '\r'){
                rx_buffer[i] = '\0';
                int Tc_nueva; 

                if(sscanf(rx_buffer, "SET_TEMP:%d", &Tc_nueva) == 1){
                    Tc = Tc_nueva;
                    char *conf = "Temperatura control actualizada \n";
                    uart_write_bytes(UART_PORT, conf, strlen(conf)); 
                }
                i = 0;
            }

		}
        if(hold_impresion >= 100){ // no imprime si no hasta 100*10ms = 1seg
            sprintf(mensaje_salida, "Tc = %d | Tsens = %d | %%Ilum = %d \r\n", Tc, Tsens, Ilum );
            uart_write_bytes(UART_PORT, mensaje_salida, strlen(mensaje_salida)); 
            hold_impresion = 0;

            sprintf(mensaje_temp, "Luz[mV] %d | Luz[%%] %d | Temp[mV] = %d | Temp[°] = %d \r\n", luz_mv, luz, temp_mv, temp );
            uart_write_bytes(UART_PORT, mensaje_temp, strlen(mensaje_temp)); 
            hold_impresion = 0;

        }
        hold_impresion = hold_impresion +1;
        vTaskDelay(pdMS_TO_TICKS(10));
	};
}
