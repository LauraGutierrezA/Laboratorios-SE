#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

// Nuestras Librerías (excepto RTC)
#include "SPI.h"
#include "RFID.h"
#include "I2C.h"
#include "LCD.h"

// Pines SPI
#define SPI_MOSI_PIN  23
#define SPI_MISO_PIN  19
#define SPI_CLK_PIN   18
#define SPI_CS_PIN     5
#define PIN_RST        4

// Pines I2C
#define I2C_SDA_PIN   21
#define I2C_SCL_PIN   22
#define LCD_I2C_ADDR  0x27
#define RTC_ADDR      0x68

// Llaves registradas
static const uint8_t KEY_AUTORIZADO[4]    = {0xFB, 0x55, 0x14, 0x07};
static const uint8_t KEY_NO_AUTORIZADO[4] = {0x74, 0xA9, 0x2C, 0x07};
static const uint8_t KEY_REINAS[4]        = {0x88, 0x04, 0x5B, 0x9C};

// ======================================================================
// ==================== FUNCIONES NATIVAS DEL RTC =======================
// ======================================================================

uint8_t dec2bcd(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }
uint8_t bcd2dec(uint8_t bcd) { return ((bcd >> 4) * 10) + (bcd & 0x0F); }

void rtc_set_time(uint8_t h, uint8_t m, uint8_t s, uint8_t dow, uint8_t dom, uint8_t month, uint8_t year) {
    uint8_t data[8];
    data[0] = 0x00;
    data[1] = dec2bcd(s) & 0x7F; // CH=0: oscilador encendido
    data[2] = dec2bcd(m);
    data[3] = dec2bcd(h) & 0x3F; // formato 24h
    data[4] = dec2bcd(dow);
    data[5] = dec2bcd(dom);
    data[6] = dec2bcd(month);
    data[7] = dec2bcd(year);
    i2c_master_write_to_device(I2C_NUM_0, RTC_ADDR, data, 8, pdMS_TO_TICKS(100));
}

bool rtc_get_time(uint8_t *h, uint8_t *m, uint8_t *s) {
    uint8_t reg = 0x00;
    esp_err_t err_w = i2c_master_write_to_device(I2C_NUM_0, RTC_ADDR, &reg, 1, pdMS_TO_TICKS(50));
    if (err_w != ESP_OK) return false;
    uint8_t data[3];
    esp_err_t err_r = i2c_master_read_from_device(I2C_NUM_0, RTC_ADDR, data, 3, pdMS_TO_TICKS(50));
    if (err_r != ESP_OK) return false;
    *s = bcd2dec(data[0] & 0x7F);
    *m = bcd2dec(data[1]);
    *h = bcd2dec(data[2] & 0x3F);
    return true;
}

// ======================================================================
// ========================== FUNCIÓN PRINCIPAL =========================
// ======================================================================

extern "C" void app_main() {
    // 1. Reset físico del lector RFID
    gpio_set_direction((gpio_num_t)PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level((gpio_num_t)PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // 2. Inicializar I2C y LCD
    I2CMaster i2cBus(I2C_NUM_0, I2C_SDA_PIN, I2C_SCL_PIN, 100000);
    i2cBus.init();

    LCD miLcd(I2C_NUM_0, LCD_I2C_ADDR);
    miLcd.init();

    // 3. Ajuste nativo de la hora
    // Viernes 22 de Mayo de 2026 a las 17:44:37
    rtc_set_time(17, 44, 37, 5, 22, 5, 26);

    // 4. Inicializar SPI y RFID
    SPI spiBus(SPI2_HOST, SPI_MOSI_PIN, SPI_MISO_PIN, SPI_CLK_PIN, SPI_CS_PIN, 0, true);
    spiBus.init();

    Rfid lector(spiBus);
    lector.init();

    printf("=== CONTROL DE ACCESO LISTO ===\n");

    char bufferHora[20] = "";
    uint8_t ultimo_segundo = 60;

    while (1) {
        uint8_t h = 0, m = 0, s = 0;

        if (!rtc_get_time(&h, &m, &s)) {
            printf("Error I2C: No se encuentra el RTC en 0x%02X\n", RTC_ADDR);
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        if (lector.leerTarjeta()) {
            snprintf(bufferHora, sizeof(bufferHora), "%02d:%02d:%02d", h, m, s);

            if (lector.verificarTarjeta(KEY_AUTORIZADO)) {
                printf("AUTORIZADO [%s]\n", bufferHora);
                miLcd.mostrarMensaje("Acceso Concedido", bufferHora);

            } else if (lector.verificarTarjeta(KEY_NO_AUTORIZADO)) {
                printf("NO AUTORIZADO [%s]\n", bufferHora);
                miLcd.mostrarMensaje("Acceso Denegado", bufferHora);

            } else if (lector.verificarTarjeta(KEY_REINAS)) {
                printf("REINAS DE LA ELECTRONICA [%s]\n", bufferHora);
                miLcd.mostrarMensaje("Hola Reinas!", bufferHora);

            } else {
                printf("TARJETA DESCONOCIDA [%s]\n", bufferHora);
                miLcd.mostrarMensaje("Acceso Denegado", "UID no reg.");
            }

            vTaskDelay(pdMS_TO_TICKS(2000));
            ultimo_segundo = 60;

        } else {
            if (s != ultimo_segundo) {
                snprintf(bufferHora, sizeof(bufferHora), "Hora: %02d:%02d:%02d", h, m, s);
                miLcd.mostrarMensaje("Panel bloqueado", bufferHora);
                ultimo_segundo = s;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
