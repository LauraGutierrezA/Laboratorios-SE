#include "I2C.h"

I2CMaster::I2CMaster(i2c_port_t port, int sdaPin, int sclPin, uint32_t speedHz = 100000){
    _port = port; 
    _sdaPin = sdaPin; 
    _sclPin = sclPin; 
    _speedHz = speedHz;
}

void I2CMaster::init(){
    i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = _sdaPin, 
		.scl_io_num = _sclPin, 
		.sda_pullup_en = GPIO_PULLUP_DISABLE,
		.scl_pullup_en = GPIO_PULLDOWN_DISABLE,
		.master = { .clk_speed = _speedHz}
	};
	i2c_param_config(_port, &conf); 
	i2c_driver_install(_port, conf.mode, 0, 0, 0);
	
}

bool I2CMaster::readBytes(uint8_t slaveAddr, uint8_t regAddr, uint8_t *data, size_t length){
    if (length == 0) return false; 

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    //Apuntar al Address y register que se quiere leer
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slaveAddr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, regAddr, true); 

    //Otra vez start para modo lectura (escribir para leer)
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slaveAddr << 1) | I2C_MASTER_READ, true); 
    if (length > 1) {
        i2c_master_read(cmd, data, length -1, I2C_MASTER_ACK);
    };
    i2c_master_read_byte(cmd, data + length-1, I2C_MASTER_NACK); 
    i2c_master_stop(cmd); 
    esp_err_t err = i2c_master_cmd_begin(_port, cmd, pdMS_TO_TICKS(1000)); 
    i2c_cmd_link_delete(cmd);
    return (err == ESP_OK);
}

bool I2CMaster::writeBytes(uint8_t slaveAddr, uint8_t regAddr, uint8_t *data, size_t length){
    if (length == 0) return false; 

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    //Apuntar al Address y register que se quiere leer
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slaveAddr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, regAddr, true); 

    for (size_t i = 0; i < length; i++) {
        i2c_master_write_byte(cmd, data[i], true);
    }

    i2c_master_stop(cmd);
    
    esp_err_t err = i2c_master_cmd_begin(_port, cmd, pdMS_TO_TICKS(1000)); 
    i2c_cmd_link_delete(cmd);
    return (err == ESP_OK);
}

I2CMaster::~I2CMaster()
{
}