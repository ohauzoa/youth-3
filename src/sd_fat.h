#ifndef __SD_FAT_H
#define __SD_FAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"

#define MOUNT_POINT "/sdcard"

esp_err_t sd_card_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

