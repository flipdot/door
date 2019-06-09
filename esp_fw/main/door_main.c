#include "freertos/FreeRTOS.h"
#include <freertos/event_groups.h>
#include "esp_log.h"
#include "door_ctrl.h"
#include "rpi_com.h"
#include "rfid.h"

static const char *TAG = "MAIN";
EventGroupHandle_t door_state_event_group;


void app_main(){
    //esp_log_level_set(TAG, ESP_LOG_INFO);

    door_state_event_group = xEventGroupCreate();
    if(NULL == door_state_event_group){
        ESP_LOGE(TAG, "failed to create event group");
        //TODO was machen?
    }

    door_ctrl_init();
    rpi_com_init();
    rfid_init();

}
