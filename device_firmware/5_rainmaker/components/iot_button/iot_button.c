#include "iot_button.h"              // Header định nghĩa kiểu dữ liệu button_handle_t, button_config_t,...
#include "driver/gpio.h"         // Thư viện điều khiển GPIO
#include "esp_log.h"             // Thư viện ghi log
#include "freertos/FreeRTOS.h"   // Thư viện FreeRTOS
#include "freertos/task.h"       // Thư viện tạo task trong FreeRTOS

// Tag dùng cho ghi log
static const char *TAG = "button";

// Cấu trúc nội bộ để lưu thông tin của nút
typedef struct {
    gpio_num_t gpio_num;       // Chân GPIO được cấu hình cho nút
    int active_level;          // Mức logic được coi là "nhấn" (0 hoặc 1)
    void (*cb)(void*);         // Hàm callback được gọi khi nút được nhấn
    void *cb_arg;              // Tham số truyền vào callback
} button_internal_t;

// Task loop để đọc nút nhấn (chạy liên tục trong FreeRTOS)
static void button_task(void *pvParameters) {
    // Ép kiểu lại để dùng cấu trúc nội bộ
    button_internal_t *btn = (button_internal_t*) pvParameters;
    // Lấy trạng thái ban đầu của nút
    int last_state = gpio_get_level(btn->gpio_num);

    while (1) {
        // Đọc trạng thái hiện tại của chân GPIO
        int level = gpio_get_level(btn->gpio_num);
        // Nếu trạng thái thay đổi so với lần trước
        if (level != last_state) {   
            last_state = level; // Cập nhật trạng thái mới

            // Nếu trạng thái mới đúng bằng active_level và có callback
            if (level == btn->active_level && btn->cb) {
                // Gọi callback khi nhấn nút
                btn->cb(btn->cb_arg);
            }
        }
        // Delay 50ms để chống dội phím (debounce)
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}

// Hàm khởi tạo nút nhấn
button_handle_t iot_button_create(const button_config_t *cfg) {
    // Cấp phát bộ nhớ cho cấu trúc nội bộ của nút
    button_internal_t *btn = calloc(1, sizeof(button_internal_t));
    if (!btn) return NULL;

    // Gán thông tin từ cấu hình vào cấu trúc nội bộ
    btn->gpio_num = cfg->gpio_button_config.gpio_num;
    btn->active_level = cfg->active_level;

    // Cấu hình chân GPIO cho nút
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << btn->gpio_num,    // chọn chân GPIO
        .mode = GPIO_MODE_INPUT,                  // chế độ input
        .pull_up_en = GPIO_PULLUP_ENABLE,         // bật kéo lên
        .pull_down_en = GPIO_PULLDOWN_DISABLE,    // tắt kéo xuống
        .intr_type = GPIO_INTR_DISABLE,           // không dùng ngắt
    };
    gpio_config(&io_conf); // Áp dụng cấu hình

    // Tạo một task FreeRTOS để đọc nút nhấn
    xTaskCreate(button_task, "button_task", 2048, btn, 10, NULL);

    // Ghi log khi đã khởi tạo nút
    ESP_LOGI(TAG, "Button created on GPIO %d", btn->gpio_num);
    // Trả về handle của nút (ép kiểu sang button_handle_t)
    return (button_handle_t)btn;
}

// Hàm đăng ký callback cho nút
void iot_button_register_cb(button_handle_t handle, button_event_t event,
                            void (*cb)(void*), void *arg) {
    // Ép kiểu handle sang cấu trúc nội bộ
    button_internal_t *btn = (button_internal_t*)handle;
    // Lưu callback và tham số truyền vào callback
    btn->cb = cb;
    btn->cb_arg = arg;
    // Ghi log khi đăng ký callback
    ESP_LOGI(TAG, "Callback registered for button on GPIO %d", btn->gpio_num);
}
