#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/event_groups.h>
#include "rpi_com.h"
#include "door_ctrl.h"

static i2c_port_t i2c_rpi_port = I2C_NUM_0;
static const gpio_num_t RPI_I2C_SDA_PIN = GPIO_NUM_15;
static const gpio_num_t RPI_I2C_SCL_PIN = GPIO_NUM_16;
static const int I2C_SLAVE_RX_LEN = 128;
static const int I2C_SLAVE_TX_LEN = 128;
static const char* TAG = "RPI";
extern QueueHandle_t door_ctrl_queue;
extern EventGroupHandle_t door_state_event_group;

typedef enum{
    RPI_CMD_OPEN = 0x23,
    RPI_CMD_CLOSE = 0x42,
    RPI_CMD_TOGGLE = 0xAA,
    RPI_CMD_STATE = 0xBB,
} rpi_cmd_t;

static void rpi_maintask(void* p){

    i2c_config_t i2c_pi_conf = {
            .mode = I2C_MODE_SLAVE,
            .sda_io_num = RPI_I2C_SDA_PIN,
            .sda_pullup_en = GPIO_PULLUP_DISABLE,
            .scl_io_num = RPI_I2C_SCL_PIN,
            .scl_pullup_en = GPIO_PULLUP_DISABLE,
            .slave.addr_10bit_en = 0,
            .slave.slave_addr = 0x42,
    };
    ESP_ERROR_CHECK(i2c_param_config(i2c_rpi_port, &i2c_pi_conf));
    ESP_ERROR_CHECK(i2c_driver_install(
            i2c_rpi_port,
            i2c_pi_conf.mode,
            I2C_SLAVE_RX_LEN,
            I2C_SLAVE_TX_LEN,
            0
    ));
    uint8_t data_frame[4];

    while (1){

        size_t data_size = i2c_slave_read_buffer(
            i2c_rpi_port,
            data_frame,
            3,
            portMAX_DELAY
        );
        rpi_cmd_t cmd = data_frame[0];
        uint16_t val = data_frame[2] << 8 | data_frame[1];
        switch (cmd){
            case RPI_CMD_OPEN:
                if(val == 0xCAFE){ //TODO  replace with crc?
                    door_ctrl_cmd_t cmd = DOOR_CTRL_CMD_OPEN;
                    if(pdPASS != xQueueSend(door_ctrl_queue, &cmd, 100/portTICK_PERIOD_MS)){
                        ESP_LOGE(TAG, "queueSend failed");
                    }else{
                        ESP_LOGD(TAG, "queueSend success");
                    }
                }
                ESP_LOGI(TAG, "door open cmd %02X", val);
                break;
            case RPI_CMD_CLOSE:
                if(val == 0xCAFE){ //TODO  replace with crc?
                    door_ctrl_cmd_t cmd = DOOR_CTRL_CMD_CLOSE;
                    if(pdPASS != xQueueSend(door_ctrl_queue, &cmd, 100/portTICK_PERIOD_MS)){
                        ESP_LOGE(TAG, "queueSend failed");
                    }else{
                        ESP_LOGD(TAG, "queueSend success");
                    }
                }
                ESP_LOGI(TAG, "door close cmd %02X", val);
                break;
            case RPI_CMD_STATE:
                if(val == 0xCAFE){
                    uint8_t tx_data[3];
                    tx_data[0] = xEventGroupGetBits(door_state_event_group);
                    tx_data[1] = 'F';

                    i2c_slave_write_buffer(i2c_rpi_port,
                        tx_data, 2, 100 / portTICK_PERIOD_MS);
                }
                ESP_LOGI(TAG, "door open state %02X", val);
                break;
            default:
                ESP_LOGE(TAG, "unknown cmd %d", cmd);
        }
        i2c_reset_rx_fifo(i2c_rpi_port);

    }
vTaskDelete(NULL);

}

esp_err_t rpi_com_init(void){


    if(pdPASS != xTaskCreate(rpi_maintask, "RPI com Task", 2048, NULL, 0, NULL)){

        ESP_LOGE(TAG, "Failed to create rpi com task");
        return ESP_FAIL;
    }
    return ESP_OK;
}
