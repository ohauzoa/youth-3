#include <Arduino.h>
#include <lvgl.h>
#include "my_lv_ports.h"
#include "sd_fat.h"
#include "lv_demo.h"
#include "shell.h"
#include "main.h"


static const char *TAG = "main";
static lv_fs_file_t f;
static lv_fs_dir_t d;
static char write_buf[] = "1234567890ABCDEFGHIJ";

void write_file(void)
{
  lv_fs_res_t res;

  res = lv_fs_open(&f, "S:/file.txt", LV_FS_MODE_WR);
  if(res != LV_FS_RES_OK)
  {
    printf("open file failed.(%d)\n", res);
    return;
  }

  res = lv_fs_write(&f, write_buf, sizeof(write_buf), NULL);
  if(res != LV_FS_RES_OK)
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

  res = lv_fs_open(&f, "S:/file.txt", LV_FS_MODE_RD);
  if(res != LV_FS_RES_OK)
  {
    printf("open file failed.(%d)\n", res);
    return;
  }

  res = lv_fs_seek(&f, 10, LV_FS_SEEK_SET);
  if(res != LV_FS_RES_OK)
  {
    printf("seek file error!\n");
    return;
  }

  res = lv_fs_read(&f, read_buf, 5, NULL);
  if(res != LV_FS_RES_OK)
  {
    printf("read file failed.\n");
  }
  else
  {
    printf("read buf : %s\n", read_buf);
  }

  lv_fs_close(&f);
}


char UartGet(void)
{
  char res;
  while(1){
    if(Serial.available()){
      res = Serial.read();
      return res;
    }
    delay(10);
  }  
}


void setup() {
  Serial.begin(115200); /* prepare for possible serial debug */
  //Wire.begin(ESP32_I2C_SDA, ESP32_I2C_SCL);

  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() +
                  "." + lv_version_patch();

  Serial.println(LVGL_Arduino);
  Serial.println("I am LVGL_Arduino");

  lv_init();
  sd_card_init();
  write_file();
  read_file();
  my_disp_init();
  shell_init();

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
  // lv_demo_widgets(); // OK
  lv_demo_benchmark(); // OK
  // lv_demo_keypad_encoder();     // works, but I haven't an encoder
  // lv_demo_music();              // NOK
  // lv_demo_printer();
  // lv_demo_stress();             // seems to be OK
#endif
  Serial.println("Setup done");
}

void loop() {
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}