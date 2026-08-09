#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "MUTEX_DEMO";
static SemaphoreHandle_t xMutex = NULL;

void task_core0(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "--- [Core 0 Task] Locked Mutex ---");
            vTaskDelay(pdMS_TO_TICKS(1000)); // จำลองการประมวลผล
            ESP_LOGI(TAG, "--- [Core 0 Task] Unlocked Mutex ---");
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task_core1(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, ">>> [Core 1 Task] Locked Mutex <<<");
            vTaskDelay(pdMS_TO_TICKS(500));
            ESP_LOGI(TAG, ">>> [Core 1 Task] Unlocked Mutex <<<");
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}

void app_main(void) {
    xMutex = xSemaphoreCreateMutex();

    if (xMutex != NULL) {
        // ใช้ xTaskCreatePinnedToCore เพื่อเลือก Core บน ESP32 (0 หรือ 1)
        xTaskCreatePinnedToCore(task_core0, "Task_Core0", 2048, NULL, 5, NULL, 0);
        xTaskCreatePinnedToCore(task_core1, "Task_Core1", 2048, NULL, 5, NULL, 1);
    } else {
        ESP_LOGE(TAG, "Failed to create Mutex!");
    }
}