/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include <stdarg.h>

#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"

#include "app.h"
#include "main.h"
#include "repl.h"

static const char * const TAG = "REPL";

static FILE * repl_out=NULL;

void repl_run(const char * line, FILE * output)
{
	if ((xEventGroupWaitBits(app_egroup, APP_EG_CONSOLE, pdTRUE, pdFALSE, 5000/portTICK_PERIOD_MS) & APP_EG_CONSOLE) !=APP_EG_CONSOLE)
		fprintf(output, "REPL is too busy to answer.\r\n");
	else {
		repl_out=output;

		int ret;
		fflush(output);
		esp_err_t err = esp_console_run(line, &ret);
		fflush(output);

		xEventGroupSetBits(app_egroup, APP_EG_CONSOLE);

		if (err == ESP_ERR_NOT_FOUND) {
			fprintf (output, "Unrecognized command.\r\n");
		} else if (err == ESP_ERR_INVALID_ARG) {
			fprintf (output, "Invalid arguments.\r\n");
		} else if (err == ESP_OK && ret != ESP_OK) {
			fprintf (output, "Command failed (0x%x): %s.\r\n", ret, esp_err_to_name(ret));
		} else if (err != ESP_OK) {
			fprintf (output, "Internal error (0x%x): %s.\r\n", err, esp_err_to_name(err));
		}
	}
}

static void repl_task()
{
	char* line=NULL;
	setvbuf(stdin, NULL, _IONBF, 0);

    while(1) {
    	if (!(xEventGroupWaitBits(app_egroup, APP_EG_READY, 0, 0, 10) & APP_EG_READY))
    		continue;

    	vTaskDelay(1);
		line = linenoise(REPL_PROMPT_NO_ESC);
		fprintf(stdout, "\r\n");

		if (line == NULL)
			continue;

		if (strlen(line)>0) {
			/* Add the command to the history if not empty*/
			if (REPL_HISTORY_LINES) {
				linenoiseHistoryAdd(line);
				linenoiseHistorySave(FS_REPL_HISTORY);
			}

			/* Try to run the command */
			repl_run(line, stdout);
		}

		/* linenoise allocates line buffer on the heap, so need to free it */
		linenoiseFree(line);
    }
}

esp_err_t repl_init()
{
	esp_err_t ret;

	/* Enable multiline editing. If not set, long commands will scroll within single line */
	linenoiseSetMultiLine(1);
    /* Tell linenoise where to get command completions and hints */
	linenoiseSetCompletionCallback(&esp_console_get_completion);
	linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);
	/* Set command history size */
	linenoiseHistorySetMaxLen(REPL_HISTORY_LINES);
	/* Set command maximum length */
	linenoiseSetMaxLineLen(REPL_LINE_MAX);
	/* Don't return empty lines, return NULL if nothing scaned */
	linenoiseAllowEmpty(false);
	/* Load command history from filesystem */
	if (REPL_HISTORY_LINES)
		linenoiseHistoryLoad(FS_REPL_HISTORY);

	esp_console_config_t concfg = {
		.max_cmdline_args = REPL_ARGS_MAX,
		.max_cmdline_length = REPL_LINE_MAX,
		.hint_color = REPL_HINT_COLOR,
		.hint_bold = 0
	};
	ESP_RETURN_ON_ERROR(ret=esp_console_init(&concfg)
			, TAG, "Failed to init REPL console (%d:%s)", ret, esp_err_to_name(ret));
	ESP_RETURN_ON_ERROR(ret=esp_console_register_help_command()
			, TAG, "Failed to register help command to REPL console (%d:%s)", ret, esp_err_to_name(ret));

    /* Install UART driver for interrupt-driven reads and writes */
    uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
        .source_clk = UART_SCLK_REF_TICK,
#else
        .source_clk = UART_SCLK_XTAL,
#endif
    };
    ESP_RETURN_ON_FALSE((ret=uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config))==ESP_OK
    		         && (ret=uart_set_pin(CONFIG_ESP_CONSOLE_UART_NUM, PIN_UART_TX, PIN_UART_RX, -1, -1))==ESP_OK
			    	 && (ret=uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, REPL_LINE_MAX, 0, 0, NULL, 0))==ESP_OK
					 , ret, TAG, "Failed to config and install UART (%d:%s)", ret, esp_err_to_name(ret));

   	/* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));
    /* use as blocked UART file */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    /* Unable to use linenoise normal mode with IDF monitor */
    linenoiseSetDumbMode(1);

    ESP_RETURN_ON_FALSE(xTaskCreatePinnedToCore(repl_task, "REPL_TASK", REPL_STACK, NULL, 2, NULL, REPL_CORE)==pdPASS
    		, ESP_FAIL, TAG, "Failed to create REPL task");

    /* start console mutex */
	ESP_RETURN_ON_FALSE(xEventGroupSetBits(app_egroup, APP_EG_CONSOLE) & APP_EG_CONSOLE
    		, ESP_FAIL, TAG, "Failed to set CONSOLE bit on event group");

	return ESP_OK;
}

int repl_printf(const char * fmt, ...)
{
	if (!repl_out) return 0;

	va_list args;
	va_start(args, fmt);
	int ret=vfprintf(repl_out, fmt, args);
	va_end(args);
	return ret;
}
