#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_netif.h" // เพิ่มไลบรารีนี้สำหรับจัดการ NAPT

// ==========================================
// ตั้งค่า WiFi สำหรับไปเกาะ (STA) และปล่อย (AP)
// ==========================================
#define STA_SSID      "CCTvQPlus"
#define STA_PASS      "1234567890"
#define AP_SSID       "ESP32"
#define AP_PASS       "12345678"
#define AP_MAX_CONN   4

static const char *TAG = "wifi_ap_sta";

// ฟังก์ชันสำหรับจัดการ Event ของ WiFi และ IP
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect(); // เริ่มเชื่อมต่อ WiFi ทันทีที่โหมด STA พร้อม
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect(); // เชื่อมต่อใหม่เมื่อหลุด
        ESP_LOGI(TAG, "Disconnected from STA, retrying...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_apsta(void)
{
    // 1. กำหนดค่าเริ่มต้นให้กับ Network Interface และ Event Loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // สร้าง Default Interface สำหรับ AP และ STA
    esp_netif_t *netif_ap = esp_netif_create_default_wifi_ap(); // เก็บค่า pointer สำหรับเปิด NAPT
    esp_netif_create_default_wifi_sta();

    // 2. กำหนดค่าเริ่มต้นของ WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 3. ลงทะเบียน Event Handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // 4. ตั้งค่า Config สำหรับฝั่ง STA
    wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = STA_SSID,
            .password = STA_PASS,
        },
    };

    // 5. ตั้งค่า Config สำหรับฝั่ง AP
    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .password = AP_PASS,
            .max_connection = AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    // ถ้าไม่ตั้งรหัสผ่าน ให้ปรับโหมดเป็น Open
    if (strlen(AP_PASS) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    // 6. กำหนดโหมดการทำงานและเริ่มรัน WiFi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // ==========================================
    // เปิดใช้งาน NAT / NAPT
    // ==========================================
    esp_err_t err = esp_netif_napt_enable(netif_ap);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "NAPT Enabled successfully. ESP32 is now a WiFi Repeater!");
    } else {
        ESP_LOGE(TAG, "Failed to enable NAPT: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "WiFi AP_STA initialization complete.");
}

void app_main(void)
{
    // การใช้ WiFi ใน ESP-IDF ต้อง Initialize NVS (Non-Volatile Storage) ก่อนเสมอ
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Starting ESP32 WiFi in AP+STA Mode");
    wifi_init_apsta();
}