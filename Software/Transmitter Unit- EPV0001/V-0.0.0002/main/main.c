// #include <stdio.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/gpio.h"
// #include "esp_system.h"
// #include "esp_wifi.h"
// #include "lora.h"
// #include "esp_mac.h"

// #define BUTTON_PIN 10

// void app_main() {
//     // Configure button input
//     gpio_config_t io_conf = {
//         .pin_bit_mask = (1ULL << BUTTON_PIN),
//         .mode = GPIO_MODE_INPUT,
//         .pull_up_en = GPIO_PULLUP_ENABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE
//     };
//     gpio_config(&io_conf);

//     // LoRa configuration
//     lora_init();
//     lora_set_frequency(865e6);
//     lora_set_spreading_factor(11);
//     lora_set_bandwidth(125e3);
//     lora_set_coding_rate(5);
//     lora_set_sync_word(0xF3);
//     lora_set_tx_power(17);
//     lora_enable_crc();
//     lora_explicit_header_mode();

//     uint8_t mac[6];
//     char mac_str[18];  // "AA:BB:CC:DD:EE:FF"

//     bool lastButtonState = false;

//     while (1) {
//         bool currentButtonState = !gpio_get_level(BUTTON_PIN); // active low

//         if (currentButtonState && !lastButtonState) {
//             // Fetch MAC address
//             esp_read_mac(mac, ESP_MAC_WIFI_STA);
//             snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
//                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

//             // Send MAC over LoRa
//             lora_send_packet((uint8_t *)mac_str, strlen(mac_str));
//             printf("MAC Sent: %s\n", mac_str);
//         }

//         lastButtonState = currentButtonState;
//         vTaskDelay(pdMS_TO_TICKS(50));
//     }
// }


/*
//Working code//
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "lora.h"

#define BUTTON_PIN 4

typedef struct {
    bool buttonPressed;
    char display[18];  // 17 for MAC + 1 for '\0'
} struct_message;

void app_main() {
    // Configure button
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // LoRa init
    lora_init();
    lora_set_frequency(865e6);
    lora_set_spreading_factor(11);
    lora_set_bandwidth(125e3);
    lora_set_coding_rate(5);
    lora_set_sync_word(0xF3);
    lora_set_tx_power(17);
    lora_enable_crc();
    lora_explicit_header_mode();

    struct_message myData;
    uint8_t mac[6];
    char mac_str[18];  // MAC address string with null terminator

    bool lastButtonState = false;

    while (1) {
        bool currentButtonState = !gpio_get_level(BUTTON_PIN); // active low

        if (currentButtonState && !lastButtonState) {
            esp_read_mac(mac, ESP_MAC_WIFI_STA);
            snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

            myData.buttonPressed = true;
            strncpy(myData.display, mac_str, sizeof(myData.display));
            myData.display[sizeof(myData.display) - 1] = '\0';

            lora_send_packet((uint8_t *)&myData, sizeof(myData));
            printf("MAC Sent: %s\n", myData.display);
            vTaskDelay(pdMS_TO_TICKS(2000));

        }

        lastButtonState = currentButtonState;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
*/


#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "lora.h"

#define TAG "MAC_SENDER"
#define BUTTON_PIN 4   // Button connected here (active LOW)

// ================== Send MAC ==================
static void send_mac_once(void) {
    uint8_t mac[6];
    char mac_str[18];

    // Read device MAC (station MAC)
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Send over LoRa
    lora_send_packet((uint8_t *)mac_str, strlen(mac_str));
    ESP_LOGI(TAG, "MAC Sent: %s", mac_str);
}

// ================== Main ==================
void app_main(void) {
    // Init LoRa
    lora_init();
    lora_set_frequency(865e6);
    lora_set_spreading_factor(11);
    lora_set_bandwidth(125e3);
    lora_set_coding_rate(5);
    lora_set_sync_word(0xF3);
    lora_set_tx_power(17);
    lora_enable_crc();
    lora_explicit_header_mode();

    // Check why we woke up
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        ESP_LOGI(TAG, "Woke up by button press");
        send_mac_once();
        vTaskDelay(pdMS_TO_TICKS(2000));  // Allow LoRa TX to complete
    } else {
        ESP_LOGI(TAG, "Cold boot or other wakeup");
    }

    // Configure button as wake-up source (active LOW)
    esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 0);

    ESP_LOGI(TAG, "Going to deep sleep... Press button to wake up");
    vTaskDelay(pdMS_TO_TICKS(100));  // Give UART time to flush

    lora_sleep();
    esp_deep_sleep_start();
}
