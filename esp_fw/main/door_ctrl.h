#ifndef ESP_DOOR_DOOR_CTRL_H
#define ESP_DOOR_DOOR_CTRL_H

typedef enum {
    DOOR_CTRL_CMD_OPEN,
    DOOR_CTRL_CMD_CLOSE,
    DOOR_CTRL_CMD_GET_STATE
} door_ctrl_cmd_t;

esp_err_t door_ctrl_init();
#endif //ESP_DOOR_DOOR_CTRL_H
