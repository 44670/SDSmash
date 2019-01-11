/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "driver/gpio.h"
#include "freertos/task.h"

static const char *TAG = "SDSmash";

#define PIN_LED (21)
//#define PIN_CONFIRM_BTN (0)


void Sleep(uint32_t ms) { 
    vTaskDelay(ms / portTICK_PERIOD_MS); 
}

sdmmc_card_t card;

esp_err_t sdmmc_send_cmd(sdmmc_card_t* card, sdmmc_command_t* cmd);
esp_err_t eraseEntireSDCard() {
    printf("Sending CMD32\n");
    sdmmc_command_t cmd = {
            .opcode = 32,
            .arg = 0,
            .flags = SCF_CMD_AC | SCF_RSP_R1
    };
    esp_err_t err = sdmmc_send_cmd(&card, &cmd);
    if (err != ESP_OK) {
        return err;
    }
    printf("Sending CMD33\n");
    sdmmc_command_t cmd2 = {
        .opcode = 33,
        .arg = card.csd.capacity - 1,
        .flags = SCF_CMD_AC | SCF_RSP_R1
    };
    err = sdmmc_send_cmd(&card, &cmd2);
    if (err != ESP_OK) {
        return err;
    }
    printf("Sending CMD38\n");
    sdmmc_command_t cmd3 = {
        .opcode = 38,
        .flags =  SCF_RSP_R1B,
        .timeout_ms = 300000
    };
    err = sdmmc_send_cmd(&card, &cmd3);
    if (err != ESP_OK) {
        return err;
    }
    printf("Erase success!\n");
    return ESP_OK;
}

void setLed(int on) {
    gpio_set_level(PIN_LED, on ? 1 : 0);
}

void blinkLed(int count) {
    int i;
    for (i = 0; i < count; i++) {
        setLed(1);
        Sleep(500);
        setLed(0);
        Sleep(500);
    }
}

/*
void waitConfirmKey() {
    // wait for 1
    while (gpio_get_level(PIN_CONFIRM_BTN) != 1);
    // then wait for 0
    while (gpio_get_level(PIN_CONFIRM_BTN) != 0);
    // then wait for 1
    while (gpio_get_level(PIN_CONFIRM_BTN) != 1);
}
*/

void app_main(void)
{
  
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1 << PIN_LED) ;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

/*
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1 << PIN_CONFIRM_BTN);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    */

    setLed(1);
    Sleep(3000);
    setLed(0);

    printf("Initializing SD card\n");
    printf("Using SDMMC peripheral\n");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    // Use 1-line SD mode, uncomment the following line:
    host.flags = SDMMC_HOST_FLAG_1BIT;
    (host.init)();
    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    sdmmc_host_init_slot(host.slot, &slot_config);

    // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
    // Internal pull-ups are not sufficient. However, enabling internal pull-ups
    // does make a difference some boards, so we do that here.
    gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
    gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
    gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    
    esp_err_t ret = sdmmc_card_init(&host, &card);
    if (ret != ESP_OK) {
        printf("Failed to initialize the card (%s). \n", esp_err_to_name(ret));
           // Make sure SD card lines have pull-up resistors in place.
        while(1) {
            blinkLed(1);
            Sleep(2000);
        }
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, &card);
    if (eraseEntireSDCard() == ESP_OK) {
        printf("Erase entire sdcard OK!\n");
        while(1) {
            blinkLed(5);
            Sleep(2000);
        }
        
    }

}
