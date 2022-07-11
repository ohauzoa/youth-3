#include "console.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_common.h"

#include <driver/i2c.h>
#include <driver/uart.h>

#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"

TaskHandle_t TaskConsole;



int led_test(int argc, char** argv)
{
    int count = 10;
    if (argc > 1)
        count = atoi(argv[1]);

    printf("Running LED test (%d)\n", count);
    for (int j = 0; j < count; ++j)
    {
        //gpio_set_level(BUILTIN_LED, 1);
        vTaskDelay(500/portTICK_PERIOD_MS);
        //gpio_set_level(GPIO_INTERNAL_LED, 0);
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
    return 0;
}

int motor_test(int argc, char** argv)
{
    int count = 1;
    if (argc > 1)
        count = atoi(argv[1]);

    printf("Running motor test (%d)\n", count);
    return 0;
}

int i2c_cmd(int argc, char** argv)
{
    printf("Running i2c (%d)\n", 1);
    return 0;
}

int read_battery(int argc, char** argv)
{
    printf("read_battery\n");

    return 0;
}

int control_peripherals(int argc, char** argv)
{
    printf("control_peripherals\n");

    return 0;
}

void initialize_console(void)
{
    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    //esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CRLF);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    uart_config_t uart_config;
    memset(&uart_config, 0, sizeof(uart_config));
    uart_config.baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.source_clk = UART_SCLK_REF_TICK;
    ESP_ERROR_CHECK(uart_param_config((uart_port_t) CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install((uart_port_t) CONFIG_ESP_CONSOLE_UART_NUM,
                                         256, 0, 0, NULL, 0));

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config;
    memset(&console_config, 0, sizeof(console_config));
    console_config.max_cmdline_args = 4;
    console_config.max_cmdline_length = 64;
#if CONFIG_LOG_COLORS
    console_config.hint_color = atoi(LOG_COLOR_CYAN);
#endif
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(10);
}

void run_console(void * parameter)
{
    initialize_console();

    /* Register commands */
    esp_console_register_help_command();

    const esp_console_cmd_t cmd1 = {
        .command = "motortest",
        .help = "Test the motors",
        .hint = NULL,
        .func = &motor_test,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd1));

    const esp_console_cmd_t cmd2 = {
        .command = "i2c",
        .help = "Manage I2C devices",
        .hint = NULL,
        .func = &i2c_cmd,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd2));

    const esp_console_cmd_t cmd3 = {
        .command = "readbattery",
        .help = "Read battery voltage",
        .hint = NULL,
        .func = &read_battery,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd3));

    const esp_console_cmd_t cmd4 = {
        .command = "peripherals",
        .help = "Control peripherals",
        .hint = NULL,
        .func = &control_peripherals,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd4));

    const esp_console_cmd_t cmd5 = {
        .command = "ledtest",
        .help = "Test the LED",
        .hint = NULL,
        .func = &led_test,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd5));

    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    const char* prompt = LOG_COLOR_I "esp32> " LOG_RESET_COLOR;

    printf("\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n");

    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status)
    {
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        /* Since the terminal doesn't support escape sequences,
         * don't use color codes in the prompt.
         */
        prompt = "esp32> ";
#endif //CONFIG_LOG_COLORS
    }

    while (true)
    {
        char* line = linenoise(prompt);
        if (line == NULL)
            continue;

        linenoiseHistoryAdd(line);

        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND)
            printf("Unrecognized command\n");
        else if (err == ESP_ERR_INVALID_ARG)
            ; // command was empty
        else if (err == ESP_OK && ret != ESP_OK)
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
        else if (err != ESP_OK)
            printf("Internal error: %s\n", esp_err_to_name(err));

        linenoiseFree(line);
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}

// Local Variables:
// compile-command: "(cd ..; make)"
// End:
void console_init(void)
{
  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
    xTaskCreatePinnedToCore(
                    run_console,   /* Task function. */
                    "Task3",     /* name of task. */
                    4096,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    25,           /* priority of the task */
                    &TaskConsole,      /* Task handle to keep track of created task */
                    PRO_CPU_NUM);          /* pin task to core 0 */                  
}