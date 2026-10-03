#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lora.h"

#define BUTTON_PIN 4
//#define BUTTON_PIN 10

typedef struct {
    bool buttonPressed;
    char display[16];
} struct_message;

static struct_message myData;
static bool lastButtonState = false;

void app_main() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    lora_init();
    lora_set_frequency(865e6);
    lora_set_spreading_factor(11);      // most robust
    lora_set_bandwidth(125e3);          // standard
    lora_set_coding_rate(5);            // 4/5
    lora_set_sync_word(0xF3);
    lora_set_tx_power(17);
    lora_enable_crc();
    lora_explicit_header_mode();

    while (1) {
        bool currentButtonState = !gpio_get_level(BUTTON_PIN); // active low

        if (currentButtonState && !lastButtonState) {
            myData.buttonPressed = true;
            strcpy(myData.display, "A104");

            lora_send_packet((uint8_t *)&myData, sizeof(myData));
            printf("Button Pressed - Data Sent\n");
        }

        lastButtonState = currentButtonState;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}