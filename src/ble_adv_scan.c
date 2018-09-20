/*
** This is based on the following codes
https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/gatt_client/main/gattc_demo.c
https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/gatt_server/main/gatts_demo.c
*/
/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdint.h>
#include <string.h>

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define esp_err_to_name(ret) #ret /* esp_err_to_name is not defined in "esp_err.h" for PlatformIO */

#define BLE_ADV_NAME "ESP32-"
#define MAX_ADV_NAME 32

#define SCANNING_DURATION 5
#define SCAN_PAUSE_PERIOD 10

#define SCAN_DEBUG 1 /* for debugging */

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 512,
    .max_interval = 1024,
    .appearance = 0,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL, //manufacturer data is what we will use to broadcast our info
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_NON_LIMIT_DISC)};

static esp_ble_adv_params_t _adv_params = {
    .adv_int_min = 512,
    .adv_int_max = 1024,
    .adv_type = ADV_TYPE_NONCONN_IND, // Excelent description of this parameter here: https://www.esp32.com/viewtopic.php?t=2267
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .peer_addr = {
        0x00,
    },
    .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50,
    .scan_window = 0x30,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE};

int restartCount = 0;

static const char *GAP_TAG = "BLE-scan";

void restartAdvertising()
{
    char advertiseName[MAX_ADV_NAME];
    sprintf(advertiseName, "%s%d", BLE_ADV_NAME, restartCount++);
    ESP_LOGI(GAP_TAG, "advertiseName = %s", advertiseName);

    ESP_ERROR_CHECK(esp_ble_gap_set_device_name(advertiseName));
    ESP_ERROR_CHECK(esp_ble_gap_config_adv_data(&adv_data));
}

void restartScanning()
{
#if SCAN_DEBUG
    uint32_t ticks = SCAN_PAUSE_PERIOD * 1000 / portTICK_RATE_MS;
    ESP_LOGI(GAP_TAG, "delay %d ticks", ticks);
    vTaskDelay(ticks);
#endif
    uint32_t duration = SCANNING_DURATION;
    ESP_ERROR_CHECK(esp_ble_gap_start_scanning(duration));
}

static void gap_cb_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
    {
        restartScanning();
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GAP_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        ESP_LOGI(GAP_TAG, "scan start success");

        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
    {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt)
        {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            // output
            {
                uint8_t *p = scan_result->scan_rst.bda;
                char macAddress[2 + (2 * 6) + 1];
                sprintf(macAddress, "0x%02x%02x%02x%02x%02x%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
                ESP_LOGI(GAP_TAG, "MAC: %s, RSSI: %d", macAddress, scan_result->scan_rst.rssi); // add
            }
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            ESP_LOGI(GAP_TAG, "ESP_GAP_SEARCH_INQ_CMPL_EVT");

            restartAdvertising(); // for testing
            restartScanning();

            break;
        default:
            ESP_LOGI(GAP_TAG, "scan_result->scan_rst.search_evt = %d", scan_result->scan_rst.search_evt);
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GAP_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GAP_TAG, "stop scan successfully");
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GAP_TAG, "adv stop failed, error status = %x", param->adv_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GAP_TAG, "stop adv successfully");
        break;

    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(GAP_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                 param->update_conn_params.status,
                 param->update_conn_params.min_int,
                 param->update_conn_params.max_int,
                 param->update_conn_params.conn_int,
                 param->update_conn_params.latency,
                 param->update_conn_params.timeout);
        break;
    default:
        ESP_LOGI(GAP_TAG, "ESP_GAP_BLE event = %d", event);
        break;
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    if (event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT)
    {
        restartAdvertising();
    }
}

void setupBLEAdvertizing()
{
    const uint8_t *p = esp_bt_dev_get_address();
    char macAddress[2 + (2 * 6) + 1];
    sprintf(macAddress, "0x%02x%02x%02x%02x%02x%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
    ESP_LOGI(GAP_TAG, "%s macAddress = %s", __func__, macAddress);

    ESP_ERROR_CHECK(esp_ble_gap_set_device_name(BLE_ADV_NAME));
    ESP_ERROR_CHECK(esp_ble_gap_config_adv_data(&adv_data));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gap_start_advertising(&_adv_params));
}

void setupBLEScanning()
{
    //register the  callback function to the gap module
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_cb_event_handler));

    ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&ble_scan_params));
}

void initializeBLE()
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    setupBLEAdvertizing();

    setupBLEScanning();
}
