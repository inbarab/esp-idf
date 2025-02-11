/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

// DESCRIPTION:
// This example contains minimal code to make ESP32-S2 based device
// recognizable by USB-host devices as a USB Serial Device printing output from
// the application.

#include <stdio.h>
#include <stdlib.h>
#include <sys/reent.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"
#include "vfs_tinyusb.h"
#include "sdkconfig.h"
#include "esp_console.h"
#include "linenoise/linenoise.h"

static const char *TAG = "example";
static esp_console_repl_t *repl = NULL;

void set_dumb(void)
{
    linenoiseSetDumbMode(1);
    return ;
    
    int probe_status = linenoiseProbe();
    if (probe_status != linenoiseIsDumbMode()) {
        linenoiseSetDumbMode(probe_status);
        fprintf(stderr, "resetting dumb mode %d\n", probe_status);
    }
}

static int do_switch_cli(int argc, char **argv)
{
    static bool cli_uart = true;

    if (cli_uart) {
        ESP_LOGI(TAG, "log -> UART");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        fprintf(stdout, "example: print -> stdout\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        fprintf(stderr, "example: print -> stderr\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_tusb_init_console(TINYUSB_CDC_ACM_0); // log to usb
        set_dumb();        
        cli_uart = false;
    } else {        
        ESP_LOGI(TAG, "log -> USB");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        fprintf(stdout, "example: print -> stdout\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        fprintf(stderr, "example: print -> stderr\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_tusb_deinit_console(TINYUSB_CDC_ACM_0); // log to uart
        set_dumb();
        cli_uart = false;
    }

    return 0;
}

void app_main(void)
{
    esp_err_t ret = ESP_OK;
    esp_console_repl_config_t repl_config = {
        .max_history_len = 32,
        .history_save_path = NULL,
        .task_stack_size = 4096,
        .task_priority = 5,
        .prompt = ">",
        .max_cmdline_length = 1024
    };

    /* Setting TinyUSB up */
    ESP_LOGI(TAG, "USB initialization");

    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    /* startthe UART as a*/
    ret = esp_console_new_repl_uart(&hw_config, &repl_config, &repl);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed initializing UART console");
    } else {
        ret = esp_console_start_repl(repl);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed initializing UART console");
        }
    }

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false, // In the most cases you need to use a `false` value
        .configuration_descriptor = NULL,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = { 0 }; // the configuration uses default values
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

    /* complete tusb-cdc console config */
    esp_vfs_tusb_cdc_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    esp_vfs_tusb_cdc_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    ESP_LOGI(TAG, "USB initialization DONE");

    esp_console_cmd_t command = {
        .command = "switch",
        .help = "Switch from usb to uart cli",
        .func = &do_switch_cli,
        .argtable = NULL,
    };

    esp_console_cmd_register(&command);


    while (1) {
        ESP_LOGI(TAG, "Keepalive");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }

    while (0) {
        ESP_LOGI(TAG, "log -> UART");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        fprintf(stdout, "example: print -> stdout\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        fprintf(stderr, "example: print -> stderr\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        esp_tusb_init_console(TINYUSB_CDC_ACM_0); // log to usb
        ESP_LOGI(TAG, "log -> USB");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        fprintf(stdout, "example: print -> stdout\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        fprintf(stderr, "example: print -> stderr\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_tusb_deinit_console(TINYUSB_CDC_ACM_0); // log to uart
    }
}
