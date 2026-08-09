#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "MUTEX_DEMO";
static SemaphoreHandle_t xMutex = NULL;

static void run_task(const char *name)
{
    for (;;) {
        TickType_t t_request = xTaskGetTickCount();
        ESP_LOGI(TAG, "[%s] Requesting mutex...", name);

        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            TickType_t t_acquired = xTaskGetTickCount();
            TickType_t waited_ms = (t_acquired - t_request) * portTICK_PERIOD_MS;

            ESP_LOGE(TAG, "[%s] Locked Mutex (waited %lu ms)",
                     name, (unsigned long)waited_ms);

            vTaskDelay(pdMS_TO_TICKS(1000)); // จำลองการประมวลผล

            ESP_LOGI(TAG, "[%s] Unlocked Mutex", name);
            xSemaphoreGive(xMutex);
        }

        // สุ่ม delay 3000-10000 ms แทนค่าคงที่ 10000
        // เพื่อทำลาย pattern เดิมที่ทำให้ period ตรงกันพอดี
        uint32_t rand_delay = 3000 + (esp_random() % 7000);
        vTaskDelay(pdMS_TO_TICKS(rand_delay));
    }
}

void task_1(void *a){ run_task("Core0 Task"); }
void task_2(void *a){ run_task("Core1 Task"); }

void app_main(void){
    xMutex = xSemaphoreCreateMutex();

    if (xMutex != NULL) {
        xTaskCreatePinnedToCore(task_1, "task1", 2560, NULL, 5, NULL, 0);
        xTaskCreatePinnedToCore(task_2, "task2", 2560, NULL, 5, NULL, 1);
        ESP_LOGI(TAG, "Task created!");
    } else {
        ESP_LOGI(TAG, "Failed to create Mutex!");
    }
}