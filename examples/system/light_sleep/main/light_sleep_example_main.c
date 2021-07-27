/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* Light sleep example */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "soc/uart_struct.h"
#include "soc/rtc.h"
#include "esp_pm.h"
#include "esp32c3/pm.h"

#define TAG              "UART"
#define TEST_UART_NUM    1
#define TEST_BUF_SIZE    1024

static QueueHandle_t uart0_queue;

void light_sleep_wakeup_config(void)
{
    ESP_ERROR_CHECK(gpio_sleep_set_direction(6, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_sleep_set_pull_mode(6, GPIO_PULLUP_ONLY));

    if (uart_set_wakeup_threshold(TEST_UART_NUM, 3) != ESP_OK) {
        ESP_LOGE(TAG, "set uart1 wakeup threshold failed");
    }
    if (esp_sleep_enable_uart_wakeup(TEST_UART_NUM) != ESP_OK) {
        ESP_LOGE(TAG, "set uart1 wakeup failed");
    }
    ESP_LOGI(TAG, "set_light_sleep_wakeup ok");
}

void light_sleep_setup(void)
{
    light_sleep_wakeup_config();

    esp_pm_config_esp32c3_t pm_config = {
        .max_freq_mhz = CONFIG_ESP32C3_DEFAULT_CPU_FREQ_MHZ,
        .min_freq_mhz = (int) rtc_clk_xtal_freq_get(),
        .light_sleep_enable = true
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
}

#define RD_BUF_SIZE 1024

static void uart_wakeup_task(void *arg)
{
    uart_event_t event;
    // esp_light_sleep_start();

    esp_pm_lock_handle_t lock = ((struct { esp_pm_lock_handle_t lock; } *)arg)->lock;
    light_sleep_setup();

    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);

    for(;;) {
        //Waiting for UART event.
        if(xQueueReceive(uart0_queue, (void * )&event, (portTickType)portMAX_DELAY)) {

            esp_pm_lock_acquire(lock);

            ESP_LOGI(TAG, "uar%d recved event:%d (wk:%d)", TEST_UART_NUM, event.type, UART_WAKEUP);
            switch(event.type) {
                case UART_DATA:
                    ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(TEST_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    ESP_LOGI(TAG, "[DATA EVT]:");
                    uart_write_bytes(TEST_UART_NUM, (const char *)dtmp, event.size);
                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(TEST_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(TEST_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI(TAG, "uart rx break");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error");
                    break;
                case UART_WAKEUP:
                    ESP_LOGI(TAG, "uart uart wakeup");
                    break;
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
            ESP_LOGI(TAG, "uart[%d] esp_pm_lock_release()", TEST_UART_NUM);
            esp_pm_lock_release(lock);
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_XTAL,
    };
    //Install UART driver, and get the queue.
    uart_driver_install(TEST_UART_NUM, TEST_BUF_SIZE * 2, TEST_BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(TEST_UART_NUM, &uart_config);
    uart_set_pin(TEST_UART_NUM, 7, 6, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    light_sleep_wakeup_config();

    static esp_pm_lock_handle_t uart_event_lock;
    ESP_ERROR_CHECK(esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "uart_evt", &uart_event_lock));
    struct { esp_pm_lock_handle_t lock; } args = { .lock = uart_event_lock };
    xTaskCreate(uart_wakeup_task, "uart_wakeup_task", 2048, &args, 12, NULL);
}
