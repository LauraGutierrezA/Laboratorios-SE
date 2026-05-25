#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

#include "SPI.h"
#include "RFID.h"
#include "I2C.h"
#include "LCD.h"
#include "BleNus.h"

#define SPI_MOSI_PIN  23
#define SPI_MISO_PIN  19
#define SPI_CLK_PIN   18
#define SPI_CS_PIN     5
#define PIN_RST        4

#define I2C_SDA_PIN   21
#define I2C_SCL_PIN   22
#define LCD_I2C_ADDR  0x27
#define RTC_ADDR      0x68

#define PIN_LED_ROJO    32
#define PIN_LED_VERDE   33
#define PIN_LED_AZUL    25

#define PIN_BUZZER      14


static const uint8_t KEY_SUPERVISOR[4] = {0xFB, 0x55, 0x14, 0x07};
static const uint8_t KEY_REINAS[4]     = {0x88, 0x04, 0x5B, 0x9C};

enum EstadoSistema { ESTADO_BLOQUEADO, ESTADO_ACTIVO };

uint8_t dec2bcd(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }
uint8_t bcd2dec(uint8_t bcd) { return ((bcd >> 4) * 10) + (bcd & 0x0F); }

bool rtc_get_time(uint8_t *h, uint8_t *m, uint8_t *s) {
    uint8_t reg = 0x00;
    if (i2c_master_write_to_device(I2C_NUM_0, RTC_ADDR, &reg, 1, pdMS_TO_TICKS(50)) != ESP_OK) return false;
    uint8_t data[3];
    if (i2c_master_read_from_device(I2C_NUM_0, RTC_ADDR, data, 3, pdMS_TO_TICKS(50)) != ESP_OK) return false;
    *s = bcd2dec(data[0] & 0x7F);
    *m = bcd2dec(data[1]);
    *h = bcd2dec(data[2] & 0x3F);
    return true;
}

static void lcd_seguro(Rfid &lector, LCD &lcd, const char *linea1, const char *linea2) {
    lector.disableAntenna();
    vTaskDelay(pdMS_TO_TICKS(50));
    lcd.mostrarMensaje(linea1, linea2);
    vTaskDelay(pdMS_TO_TICKS(10));
    lector.enableAntenna();
}

extern "C" void app_main() {

    // 1. Reset físico del RFID
    gpio_set_direction((gpio_num_t)PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level((gpio_num_t)PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Inicialización de Actuadores
    gpio_set_direction((gpio_num_t)PIN_LED_ROJO, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)PIN_LED_VERDE, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)PIN_LED_AZUL, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)PIN_BUZZER, GPIO_MODE_OUTPUT);

    gpio_set_level((gpio_num_t)PIN_LED_ROJO, 0);
    gpio_set_level((gpio_num_t)PIN_LED_VERDE, 0);
    gpio_set_level((gpio_num_t)PIN_LED_AZUL, 0);
    gpio_set_level((gpio_num_t)PIN_BUZZER, 0);

    I2CMaster i2cBus(I2C_NUM_0, I2C_SDA_PIN, I2C_SCL_PIN, 10000);
    i2cBus.init();
    LCD miLcd(I2C_NUM_0, LCD_I2C_ADDR);
    miLcd.init();

    SPI spiBus(SPI2_HOST, SPI_MOSI_PIN, SPI_MISO_PIN, SPI_CLK_PIN, SPI_CS_PIN, 0, true);
    spiBus.init();
    Rfid lector(spiBus);
    lector.init();

    BleNus miBluetooth;
    miBluetooth.init("PanelHMI");

    printf("=== PANEL HMI LISTO ===\n");

    EstadoSistema estadoActual   = ESTADO_BLOQUEADO;
    char bufferBle[17]           = "Sin mensajes";
    char bufferHora[20]          = "";
    char tempBle[64]             = "";
    bool refrescarBloqueo        = true;
    bool primerIngresoActivo     = true;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        uint8_t h = 0, m = 0, s = 0;
        bool tieneHora = rtc_get_time(&h, &m, &s);

        switch (estadoActual) {

        case ESTADO_BLOQUEADO:

            // Forzar estados de LEDs correctos de forma continua en bucle de bloqueo
            gpio_set_level((gpio_num_t)PIN_LED_ROJO, 1);
            gpio_set_level((gpio_num_t)PIN_LED_VERDE, 0);
            gpio_set_level((gpio_num_t)PIN_LED_AZUL, 0);

            if (refrescarBloqueo) {
                lcd_seguro(lector, miLcd, "Panel bloqueado", "Acerque credencial");
                refrescarBloqueo = false;
            }

            miBluetooth.leer(tempBle);

            if (lector.leerTarjeta()) {
                lector.disableAntenna();
                vTaskDelay(pdMS_TO_TICKS(50)); 

                if (lector.verificarTarjeta(KEY_SUPERVISOR)) {
                    if (tieneHora)
                        snprintf(bufferHora, sizeof(bufferHora), "%02d:%02d:%02d", h, m, s);
                    
                    miLcd.mostrarMensaje("Acceso concedido", tieneHora ? bufferHora : "");
                    printf("ACCESO CONCEDIDO [%s]\n", bufferHora);
                    
        
                    gpio_set_level((gpio_num_t)PIN_LED_ROJO, 0);
                    gpio_set_level((gpio_num_t)PIN_LED_AZUL, 0); 
                    gpio_set_level((gpio_num_t)PIN_LED_VERDE, 1);
                    gpio_set_level((gpio_num_t)PIN_BUZZER, 1);
                    vTaskDelay(pdMS_TO_TICKS(500)); 
                    gpio_set_level((gpio_num_t)PIN_BUZZER, 0);
                    
                    vTaskDelay(pdMS_TO_TICKS(500)); 
                    gpio_set_level((gpio_num_t)PIN_LED_VERDE, 0); 

                    strcpy(bufferBle, "Sin mensajes");
                    estadoActual        = ESTADO_ACTIVO;
                    primerIngresoActivo = true;

                } else if (lector.verificarTarjeta(KEY_REINAS)) {
                    miLcd.mostrarMensaje("Hola Reinas!", "");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    lector.enableAntenna();
                    refrescarBloqueo = true;

                } else {
                    miLcd.mostrarMensaje("Acceso denegado", "UID no registrado");
                    printf("ACCESO DENEGADO\n");

                    gpio_set_level((gpio_num_t)PIN_BUZZER, 1);
                    for (int i = 0; i < 3; i++) {
                        gpio_set_level((gpio_num_t)PIN_LED_ROJO, 0);
                        vTaskDelay(pdMS_TO_TICKS(333));
                        gpio_set_level((gpio_num_t)PIN_LED_ROJO, 1);
                        vTaskDelay(pdMS_TO_TICKS(333));
                    }
                    gpio_set_level((gpio_num_t)PIN_BUZZER, 0);
                    
                    lector.enableAntenna();
                    refrescarBloqueo = true;
                }
            }
            break;

        case ESTADO_ACTIVO:

            
            gpio_set_level((gpio_num_t)PIN_LED_ROJO, 0);
            gpio_set_level((gpio_num_t)PIN_LED_VERDE, 0);
            gpio_set_level((gpio_num_t)PIN_LED_AZUL, 1); 

            if (primerIngresoActivo) {
                lector.enableAntenna();
                miLcd.mostrarMensaje(bufferBle, "");
                primerIngresoActivo = false;
            }

            if (miBluetooth.leer(tempBle)) {
                strncpy(bufferBle, tempBle, 16);
                bufferBle[16] = '\0';
                
                lector.disableAntenna();
                vTaskDelay(pdMS_TO_TICKS(50));
                miLcd.setCursor(0, 0);
                miLcd.print("                ");
                miLcd.setCursor(0, 0);
                miLcd.print(bufferBle);
                lector.enableAntenna();
            }

            if (tieneHora) {
                snprintf(bufferHora, sizeof(bufferHora), "%02d:%02d:%02d", h, m, s);
                lector.disableAntenna();
                vTaskDelay(pdMS_TO_TICKS(50));
                miLcd.setCursor(1, 0);
                miLcd.print(bufferHora);
                lector.enableAntenna();
            }

            if (lector.leerTarjeta()) {
                lector.disableAntenna();
                vTaskDelay(pdMS_TO_TICKS(50));

                if (lector.verificarTarjeta(KEY_SUPERVISOR)) {
                    miLcd.mostrarMensaje("Sesion cerrada", "");
                    printf("SESION CERRADA\n");

                    gpio_set_level((gpio_num_t)PIN_LED_AZUL, 0);
                    gpio_set_level((gpio_num_t)PIN_BUZZER, 1);
                    vTaskDelay(pdMS_TO_TICKS(500)); 
                    gpio_set_level((gpio_num_t)PIN_BUZZER, 0);

                    lector.enableAntenna();
                    estadoActual     = ESTADO_BLOQUEADO;
                    refrescarBloqueo = true;
                } else {
                    lector.enableAntenna();
                }
            }
            break;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));
    }
}