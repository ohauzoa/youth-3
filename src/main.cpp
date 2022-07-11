#include <Arduino.h>
#include <lvgl.h>
#include "my_lv_ports.h"
#include "sd_fat.h"
#include "lv_demo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise/linenoise.h"
#include "esp_vfs_common.h"
#include "console.h"
#include "esp_console.h"


static const char *TAG = "main";
static lv_fs_file_t f;
static lv_fs_dir_t d;
static char write_buf[] = "1234567890ABCDEFGHIJ";


void put_rc (FRESULT rc)
{
	const char *str =
		"OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
		"INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
		"INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0"
		"LOCKED\0" "NOT_ENOUGH_CORE\0" "TOO_MANY_OPEN_FILES\0";
	int i;

	for (i = 0; i != rc && *str; i++) {
		while (*str++) ;
	}

	if(rc)printf("\nrc=%u FR_%s", rc, str);
}


void write_file(void)
{
    lv_fs_res_t res;

    res = lv_fs_open(&f, "S:/ttt/file.txt", LV_FS_MODE_WR);
    if (res != LV_FS_RES_OK)
    {
        printf("open file failed.(%d)\n", res);
        return;
    }

    res = lv_fs_write(&f, write_buf, sizeof(write_buf), NULL);
    if (res != LV_FS_RES_OK)
    {
        printf("write file failed.\n");
        return;
    }
    printf("write file ok.\n");
    lv_fs_close(&f);
}

void read_file(void)
{
    lv_fs_res_t res;

    char read_buf[32];

    res = lv_fs_open(&f, "S:/ttt/file.txt", LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK)
    {
        printf("open file failed.(%d)\n", res);
        return;
    }

    res = lv_fs_seek(&f, 10, LV_FS_SEEK_SET);
    if (res != LV_FS_RES_OK)
    {
        printf("seek file error!\n");
        return;
    }

    res = lv_fs_read(&f, read_buf, 5, NULL);
    if (res != LV_FS_RES_OK)
    {
        printf("read file failed.\n");
    }
    else
    {
        printf("read buf : %s\n", read_buf);
    }

    lv_fs_close(&f);
}
static FF_DIR Dir;						/* Directory object */
static FILINFO Finfo;
static FIL File1, File2;			/* File objects */

int command_ls( int argc, char** argv )
{
	char *ptr = argv[1];
	FRESULT res;
	uint32_t p1;
	uint32_t s1, s2;
	FATFS *fs;	
	long long temp; // �����Ұ�. printf("data = %llu \n", temp*512);

	res = f_opendir(&Dir, ptr);
	if (res) { put_rc(res); return 0; }
	p1 = s1 = s2 = 0;
	printf("\n");

	for(;;) {
#if _USE_LFN
		Finfo.lfname = Lfname;
		Finfo.lfsize = sizeof(Lfname);
#endif
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0]) break;
		if (Finfo.fattrib & AM_DIR) {
			s2++;
		} else {
			s1++; p1 += Finfo.fsize;
		}

		printf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  ",
				(Finfo.fattrib & AM_DIR) ? 'D' : '-',
				(Finfo.fattrib & AM_RDO) ? 'R' : '-',
				(Finfo.fattrib & AM_HID) ? 'H' : '-',
				(Finfo.fattrib & AM_SYS) ? 'S' : '-',
				(Finfo.fattrib & AM_ARC) ? 'A' : '-',
				(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
				(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
				Finfo.fsize);
#if _USE_LFN
		if(strlen(Lfname)) printf("  %s\n", Lfname);
		else printf("  %s\n", &(Finfo.fname[0]));
#else
		printf("  %s\n", &(Finfo.fname[0]));
#endif
		}

		printf("%4u File(s),%10lu bytes total\n%4u Dir(s)", s1, p1, s2);
		if (f_getfree(ptr, (DWORD*)&p1, &fs) == FR_OK){
			temp = (long long)(p1 * fs->csize);
			printf(", %llu bytes free\n", temp*512);
		}
	return 0;
}

static void register_cmd_ls(void)
{
    const esp_console_cmd_t cmd_ls = {
        .command = "ls",
        .help = "list the directory",
        .hint = NULL,
        .func = &command_ls,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_ls));
}

static int command_ren( int argc, char** argv )
{
	char *ptr1 = argv[1];
	char *ptr2 = argv[2];

	if( argc < 3 ) return 0;

	put_rc(f_rename(ptr1, ptr2));

	return 0;
}

static void register_cmd_ren(void)
{
    const esp_console_cmd_t cmd_ren = {
        .command = "ren",
        .help = "list the directory",
        .hint = NULL,
        .func = &command_ren,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_ren));
}

void setup()
{
    console_init();
//    Serial.begin(115200); /* prepare for possible serial debug */
//  Wire.begin(ESP32_I2C_SDA, ESP32_I2C_SCL);
//register_cmd_ls();
register_cmd_ren();
    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
    printf("\nI am LVGL_Arduino V%d.%d.%d\n", lv_version_major(), lv_version_minor(), lv_version_patch());
//  Serial.println(LVGL_Arduino);
//  Serial.println("I am LVGL_Arduino");
    lv_init();
    sd_card_init();
    write_file();
    read_file();
//    my_disp_init();

#if 0
    /* Create simple label */
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, LVGL_Arduino.c_str());
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
#else
    /* Try an example from the lv_examples Arduino library
    make sure to include it as written above.
    lv_example_btn_1();
    */

    // uncomment one of these demos
//  lv_demo_widgets(); // OK
//  lv_demo_benchmark(); // OK
//  lv_demo_keypad_encoder();     // works, but I haven't an encoder
//  lv_demo_music();              // NOK
//  lv_demo_printer();
//  lv_demo_stress();             // seems to be OK
#endif
//    Serial.println("Setup done");
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay(5);
}


