#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <string.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <freertos/event_groups.h>
#include "door_ctrl.h"

static const char* TAG = "DOOR_CTRL";
static const int MOTOR_DELAY_MS = 200;
static const gpio_num_t DOOR_CTRL_OPEN_PIN = GPIO_NUM_4;
static const gpio_num_t DOOR_CTRL_CLOSE_PIN = GPIO_NUM_5;
static const gpio_num_t DOOR_CTRL_BTN_PIN = GPIO_NUM_36;
static const adc1_channel_t DOOR_POT_ADC_CHAN = ADC1_CHANNEL_3;
QueueHandle_t door_ctrl_queue;
static QueueHandle_t door_btn_intr_queue;


extern EventGroupHandle_t door_state_event_group;

QueueHandle_t sem;

static const int NO_OF_SAMPLES = 10;

static esp_adc_cal_characteristics_t *adc_chars;
static void door_ctr_get_pot(){
     uint32_t adc_reading = 0;
        //Multisampling
        for (int i = 0; i < NO_OF_SAMPLES; i++) {
            adc_reading += adc1_get_raw((adc1_channel_t)DOOR_POT_ADC_CHAN);
        }
        adc_reading /= NO_OF_SAMPLES;
        //Convert adc_reading to voltage in mV
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    ESP_LOGD(TAG, "adc %dmV", voltage);
    if(voltage > 1200){
        xEventGroupSetBits(door_state_event_group, 1);
    } else{
        xEventGroupClearBits(door_state_event_group, 1);
    }

}
static void door_ctrl_open_door(){
    ESP_LOGI(TAG, "open");
    gpio_set_level(DOOR_CTRL_OPEN_PIN, 1);
    vTaskDelay(MOTOR_DELAY_MS/portTICK_PERIOD_MS);
    gpio_set_level(DOOR_CTRL_OPEN_PIN, 0);
}
static void door_ctrl_close_door(){
    ESP_LOGI(TAG, "close");
    gpio_set_level(DOOR_CTRL_CLOSE_PIN, 1);
    vTaskDelay(MOTOR_DELAY_MS/portTICK_PERIOD_MS);
    gpio_set_level(DOOR_CTRL_CLOSE_PIN, 0);
}

static void door_ctrl_maintask(void *p){
    door_ctrl_cmd_t door_ctrl_cmd;
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(DOOR_POT_ADC_CHAN,ADC_ATTEN_DB_6);
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_6, ADC_WIDTH_BIT_12, 1100, adc_chars);
    while (1){
        if(pdPASS == xQueueReceive(door_ctrl_queue, &door_ctrl_cmd, 1000/portTICK_PERIOD_MS)){
            switch(door_ctrl_cmd){
                case DOOR_CTRL_CMD_OPEN:
                    door_ctrl_open_door();
                    break;
                case DOOR_CTRL_CMD_CLOSE:
                    door_ctrl_close_door();
                    break;
                case DOOR_CTRL_CMD_GET_STATE:

                default:
                    ESP_LOGE(TAG, "unknown door_ctrl_cmd %d", door_ctrl_cmd);
            }
        }
        door_ctr_get_pot();
    }
}
static void door_ctrl_maintask_(void *p){
    while(1){

        door_ctrl_open_door();
        vTaskDelay(500/portTICK_PERIOD_MS);
        door_ctrl_close_door();
        vTaskDelay(500/portTICK_PERIOD_MS);
    }

}

static void door_ctrl_btn_task(void *p){
   int level;
   door_ctrl_cmd_t cmd = DOOR_CTRL_CMD_CLOSE;
   while(1){
       //if(pdTRUE == xQueueReceive(door_btn_intr_queue, &level, portMAX_DELAY)){
//       if(pdTRUE == xSemaphoreTake(sem, portMAX_DELAY)){
        if(gpio_get_level(DOOR_CTRL_BTN_PIN) == 1){
           xQueueSend(door_ctrl_queue, &cmd, 1000 / portTICK_PERIOD_MS);
           //debounce
           vTaskDelay(3000/portTICK_PERIOD_MS);
        }else{
            vTaskDelay(500/portTICK_PERIOD_MS);
        }

   }
}

esp_err_t door_ctrl_init(){
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL<<DOOR_CTRL_OPEN_PIN) | (1ULL<<DOOR_CTRL_CLOSE_PIN));
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_set_level(DOOR_CTRL_OPEN_PIN, 0);
    gpio_set_level(DOOR_CTRL_CLOSE_PIN, 0);
    gpio_config(&io_conf);
    gpio_set_level(DOOR_CTRL_OPEN_PIN, 0);
    gpio_set_level(DOOR_CTRL_CLOSE_PIN, 0);

    /* Configure close button intr */
    gpio_set_direction(DOOR_CTRL_BTN_PIN, GPIO_MODE_INPUT);
    gpio_pullup_dis(DOOR_CTRL_BTN_PIN);
    gpio_pulldown_en(DOOR_CTRL_BTN_PIN);
    gpio_set_intr_type(DOOR_CTRL_BTN_PIN, GPIO_INTR_POSEDGE);
    door_btn_intr_queue = xQueueCreate(10, sizeof(uint32_t));
    if (NULL == door_btn_intr_queue) {
        ESP_LOGE(TAG, "Failed to create door_btn_intr_queue");
        return ESP_FAIL;
    }
    //sem = xSemaphoreCreateBinary();
    //gpio_install_isr_service(0);
    //gpio_isr_handler_add(DOOR_CTRL_BTN_PIN, door_ctrl_btn_isr, (void*)DOOR_CTRL_BTN_PIN);

    door_ctrl_queue = xQueueCreate(2, sizeof(door_ctrl_cmd_t));
    if (NULL == door_ctrl_queue){
        ESP_LOGE(TAG, "Failed to create door_ctrl queue");
        return ESP_FAIL;
    }
    if(pdPASS != xTaskCreate(door_ctrl_maintask, "door_ctrl", 2048, NULL, 0, NULL)){
        ESP_LOGE(TAG, "Failed to create door_ctrl task");
        return ESP_FAIL;
    }
    if(pdPASS != xTaskCreate(door_ctrl_btn_task, "door_ctrl_btn", 1024, NULL, 0, NULL)){
        ESP_LOGE(TAG, "Failed to create door_ctrl_btn task");
        return ESP_FAIL;
    }
    return ESP_OK;
}


