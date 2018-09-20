/*
This code is based on the following codes.
https://github.com/espressif/esp-idf/blob/master/examples/wifi/scan/main/scan.c
*/
/* Scan Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
//#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event_loop.h"

#include <string.h>

#define DEFAULT_AP_SSID "ESP32-AP"
#define DEFAULT_AP_PWD "password"
#define DEFAULT_AP_CHAN 5

#define MAX_NUM_OF_AP 50
#define SCAN_PERIOD 500 /* ms */

static const char *TAG = "WiFi-scan";

void wifi_list_ap()
{
    ESP_LOGI(TAG, "[%s] wifi_list_ap()", __func__);
    uint16_t apCount = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&apCount));
    ESP_LOGI(TAG, "apCount = %d", apCount);

    static wifi_ap_record_t ap_list[MAX_NUM_OF_AP];
    memset(ap_list, 0, sizeof(ap_list));
    uint16_t size = MAX_NUM_OF_AP;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&size, ap_list));
    ESP_LOGI(TAG, "size = %d", size);

    for (uint16_t i = 0; i < size; i++)
    {
        wifi_ap_record_t *ap = ap_list + i;
        char *ssid = (char *)ap->ssid;
        uint8_t*    bssid = ap->bssid;

        char bssid_str[2*6+2+1];
        sprintf(bssid_str, "0x%02x%02x%02x%02x%02x%02x", 
            bssid[0],bssid[1],bssid[2],bssid[3],bssid[4],bssid[5]);

        char ap_str[20] = {0};
        sprintf(ap_str, "%d,%s", ap->rssi,
                ap->authmode == WIFI_AUTH_OPEN ? "open" : ap->authmode == WIFI_AUTH_WEP ? "wep" : ap->authmode == WIFI_AUTH_WPA_PSK ? "wpa-psk" : ap->authmode == WIFI_AUTH_WPA2_PSK ? "wpa-psk" : ap->authmode == WIFI_AUTH_WPA_WPA2_PSK ? "wpa-wpa2-psk" : ap->authmode == WIFI_AUTH_WPA2_ENTERPRISE ? "wpa-eap" : "unknown");
        ESP_LOGI(TAG, "%s %s : %s", bssid_str, ssid, ap_str);
    }
}

void wifi_start_scan()
{
    wifi_scan_config_t wifi_scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0, // all channels
        .scan_type = WIFI_SCAN_TYPE_PASSIVE,
        .show_hidden = true,
        .scan_time.passive = SCAN_PERIOD,
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&wifi_scan_config, false /* non-blocking */));
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    //    esp_err_t ret;
    switch (event->event_id)
    {
    case SYSTEM_EVENT_SCAN_DONE:
        ESP_LOGI(TAG, "[%s] SYSTEM_EVENT_SCAN_DONE", __func__);

        wifi_list_ap();
        wifi_start_scan(); // restart scan
        break;
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        //        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        ESP_LOGI(TAG, "Got IP: %s\n",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    case SYSTEM_EVENT_AP_START:
        ESP_LOGI(TAG, "SYSTEM_EVENT_AP_START");
        break;
    default:
        ESP_LOGI(TAG, "event->event_id %d", event->event_id);
        break;
    }
    return ESP_OK;
}

/* Initialize Wi-Fi as sta and set scan method */
void initializeWiFi()
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid = DEFAULT_AP_SSID,
            .password = DEFAULT_AP_PWD,

            .ssid_len = 0,                      /**< Length of SSID. If softap_config.ssid_len==0, check the SSID until there is a termination character; otherwise, set the SSID length according to softap_config.ssid_len. */
            .channel = DEFAULT_AP_CHAN,         /**< Channel of ESP32 soft-AP */
            .authmode = WIFI_AUTH_WPA_WPA2_PSK, /**< Auth mode of ESP32 soft-AP. Do not support AUTH_WEP in soft-AP mode */
            .ssid_hidden = 0,                   /**< Broadcast SSID or not, default 0, broadcast the SSID */
            .max_connection = 1,                /**< Max number of stations allowed to connect in, default 4, max 4 */
            .beacon_interval = 100,             /**< Beacon interval, 100 ~ 60000 ms, default 100 ms */
        },
    };
    // setup AP SSID
    uint8_t wifiMacAddress[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, wifiMacAddress);
    sprintf((char *)&wifi_config_ap.ap.ssid, "ESP32-%02x%02x%02x%02x%02x%02x",
            wifiMacAddress[0], wifiMacAddress[1], wifiMacAddress[2],
            wifiMacAddress[3], wifiMacAddress[4], wifiMacAddress[5]);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));

    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_start_scan();
}