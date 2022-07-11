#include "sd_fat.h"

//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
//#include "esp_log.h"

static const char *TAG = "TF_card";
static const char mount_point[] = MOUNT_POINT;

#define SPI_DMA_CHAN    1

#define PIN_NUM_MISO    19
#define PIN_NUM_MOSI    23
#define PIN_NUM_CLK     18
#define PIN_NUM_CS      4
#define PIN_NUM_CD      19

esp_err_t sd_card_init(void)
{
    esp_err_t ret;
    sdmmc_card_t *card;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .allocation_unit_size = 16 * 1024,
        .format_if_mount_failed = false,
        .max_files = 5,
    };

    ESP_LOGI(TAG, "Initializing SD Card\n");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = VSPI_HOST;

    spi_bus_config_t bus_config = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4 * 1024 * sizeof(uint8_t)
    };

    ret = spi_bus_initialize(host.slot, &bus_config, host.slot);
    if(ret != ESP_OK)
    {
        ESP_LOGI(TAG, "Failed to initialize bus.\n");
        return ret;
    }
    ESP_LOGI(TAG, "host slot (SPI%d)\n", host.slot);

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.gpio_cd = -1;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if(ret != ESP_OK)
    {
        if(ret == ESP_FAIL)
        {
            ESP_LOGI(TAG, "Failed to mount filesystem.\n"
            "If you want to card to be formatted, set the EXAMOLE_FORMAT_IF_MOUNT_FAILERD menuconfig option.\n");
        }
        else
        {
            ESP_LOGI(TAG, "Failed to initialize the card (%s)."
            "Make sure SD card line have pull-up resistors in place.", esp_err_to_name(ret));
        }

        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Filesystem mounted");
    
    sdmmc_card_print_info(stdout, card);

    return ESP_OK;
}

