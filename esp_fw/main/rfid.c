#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <string.h>
#include <esp_spiffs.h>
#include <cJSON.h>
#include "rfid.h"
#include "door_ctrl.h"


void rfid_maintask(void *);

static const char* TAG = "RFID";
static const int RFID_ID_LEN  = 128;
static uart_port_t rfid_uart_port = UART_NUM_2;
static QueueHandle_t rfid_uart_queue;
extern QueueHandle_t door_ctrl_queue;

esp_err_t rfid_init(void){
    if(pdPASS != xTaskCreate(rfid_maintask, "RFID", 4096, NULL, 12, NULL)){
        ESP_LOGI(TAG, "failed to create RFID task");
        return ESP_FAIL;
    }
    return ESP_OK;
}


esp_err_t rfid_check_id(uint8_t *buf){
    FILE *f;
    esp_err_t ret = ESP_FAIL;

    f = fopen("/spiffs/rfidtags.json", "rb");
    if(f == NULL){
        ESP_LOGE(TAG, "failed to open rfid.bin");
        return ESP_FAIL;
    }
    fseek(f, 0, SEEK_END);
    int file_size = ftell(f);
    rewind(f);
    ESP_LOGI(TAG, "file contains %d bytes", file_size);
    char* json_str = (char*) malloc(file_size);
    if(NULL == json_str){
        ESP_LOGE(TAG, "json_str malloc failed");
        goto rfid_check_close;
    }
    int size;
    if (file_size != (size = fread(json_str, 1, file_size, f))){
        ESP_LOGE(TAG, "failed to read json_str %d", size);
        goto rfid_check_free;
    }
    cJSON *root = cJSON_Parse(json_str);
    cJSON *subitem;
    cJSON_ArrayForEach(subitem, root){
        ESP_LOGD(TAG, "parse key %s", subitem->valuestring);
        if(strncmp(subitem->valuestring, (char*)buf, 12) == 0){
            ESP_LOGI(TAG, "id matched");
            door_ctrl_cmd_t cmd = DOOR_CTRL_CMD_OPEN;
            if(pdPASS != xQueueSend(door_ctrl_queue, &cmd, 100/portTICK_PERIOD_MS)){
                ESP_LOGE(TAG, "queueSend failed");
            }else{
                ESP_LOGD(TAG, "queueSend success");
            }
            ret = ESP_OK;
            goto rfid_check_delete;
        }
	}
	ESP_LOGI(TAG, "Not found '%s'", buf);

    rfid_check_delete:
    cJSON_Delete(root);
    rfid_check_free:
    free(json_str);
    rfid_check_close:
    fclose(f);
    return ret;
}

esp_err_t spifs_init(){
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = "storage",
            .max_files = 5,
            .format_if_mount_failed = false,
    };

    esp_err_t ret;
    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }
    return ESP_OK;
}

void rfid_maintask(void *p) {
    uart_event_t event;
    uint8_t* rx_buffer = (uint8_t*) malloc(RFID_ID_LEN);
    uart_config_t uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(rfid_uart_port, &uart_config);

    uart_set_pin(rfid_uart_port, GPIO_NUM_14, GPIO_NUM_35, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(rfid_uart_port, RFID_ID_LEN * 2, RFID_ID_LEN * 2, 20, &rfid_uart_queue, 0);

    spifs_init();
    while (1) {
        //Waiting for UART event.
        if (xQueueReceive(rfid_uart_queue, (void *) &event, (portTickType) portMAX_DELAY)) {
            memset(rx_buffer, '\0', RFID_ID_LEN);
            ESP_LOGI(TAG, "uart[%d] event:", rfid_uart_port);
            switch (event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:
                    uart_read_bytes(rfid_uart_port, rx_buffer, event.size, portMAX_DELAY);
                    ESP_LOGD(TAG, "received (%d byte): %s", event.size, rx_buffer);
                    if(event.size == 14) {
                        rfid_check_id(rx_buffer + 1);
                    }else{
                       ESP_LOGE(TAG, "received %d bytes", event.size);
                    }
                    break;
                    //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(rfid_uart_port);
                    xQueueReset(rfid_uart_queue);
                    break;
                    //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(rfid_uart_port);
                    xQueueReset(rfid_uart_queue);
                    break;
                    //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI(TAG, "uart rx break");
                    break;
                    //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGE(TAG, "uart parity error");
                    break;
                    //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGE(TAG, "uart frame error");
                    break;
                    //UART_PATTERN_DET
                    //Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }
    free(rx_buffer);
}
