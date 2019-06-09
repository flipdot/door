#include <driver/i2c.h>
#include <esp_log.h>
#include "flipkey.h"


static const char *TAG = "flipkey";
static const i2c_port_t i2c_flipkey_port = I2C_NUM_1;
static const gpio_num_t FLIPKEY_SDA_PIN = GPIO_NUM_32;
static const gpio_num_t FLIPKEY_SCL_PIN = GPIO_NUM_33;

static void flipkey_maintask(){

    i2c_config_t i2c_flipkey_conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = FLIPKEY_SDA_PIN,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_io_num = FLIPKEY_SCL_PIN,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = 400000
    };
    i2c_param_config(i2c_flipkey_port, &i2c_flipkey_conf);

}


esp_err_t flipkey_init(void){

    if(pdTRUE != xTaskCreate(flipkey_maintask, "flipkey", 2048, NULL, 0, NULL)){
        ESP_LOGE(TAG, "failed to create flipkey task");
        return ESP_FAIL;

    }

    return ESP_OK;
}
