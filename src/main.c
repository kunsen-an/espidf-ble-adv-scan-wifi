#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

extern void initializeBLE();
extern void initializeWiFi();

void loop()
{
    int count = 0;
    while (1)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
        printf("count = %d\n", count++);
    }
}

void app_main()
{
    // Initialize NVS.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    initializeBLE();
    initializeWiFi();

    loop();
}
