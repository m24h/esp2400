/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "esp_vfs_fat.h"
#include "wear_levelling.h"
#include "esp_task_wdt.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_console.h"

#include "conf.h"
#include "repl.h"
#include "ui.h"
#include "samp.h"
#include "output.h"
#include "stat.h"
#include "wifi.h"
#include "web.h"
#include "main.h"
#include "app.h"

static const char * const TAG = "APP";

esp_event_loop_handle_t    app_eloop;

ESP_EVENT_DEFINE_BASE(APP_E_BASE);

EventGroupHandle_t         app_egroup;

static wl_handle_t         wl_storage = WL_INVALID_HANDLE;

static const TCHAR storage_drv[]={'0',':','/',0}; /* the first */

static StaticEventGroup_t  egroup;
static vprintf_like_t log_original=NULL;

static char log_file[256]={0};

static void app_on_shutdown()
{
	if (wl_storage!=WL_INVALID_HANDLE) {
		esp_vfs_fat_spiflash_unmount(FS_STORAGE_MNT, wl_storage);
		wl_storage = WL_INVALID_HANDLE;
	}

	esp_vfs_fat_rawflash_unmount(FS_RESOURCE_MNT, FS_RESOURCE_PAR);

	nvs_flash_deinit();

	gpio_uninstall_isr_service();

	gpio_config_t io = {
		.pin_bit_mask = SOC_GPIO_VALID_GPIO_MASK,
		.mode = GPIO_MODE_DISABLE
	};
	gpio_config(&io);
}

esp_err_t app_reset()
{
	ESP_LOGI(TAG, "Reset obeying instruction\r\n");
	if (app_egroup) xEventGroupClearBits(app_egroup, APP_EG_READY);

	vTaskDelay(1000/portTICK_PERIOD_MS);
	esp_restart();

	return ESP_OK;
}

esp_err_t app_format()
{
	FRESULT res;
	size_t workbuf_size = 4096;
	if (workbuf_size<CONFIG_WL_SECTOR_SIZE) workbuf_size=CONFIG_WL_SECTOR_SIZE;
    void *workbuf = ff_memalloc(workbuf_size);
    ESP_RETURN_ON_FALSE(workbuf
 			, ESP_ERR_NO_MEM, TAG, "Failed to allocate formating work buffer");

    esp_err_t ret=ESP_OK;
    ESP_GOTO_ON_FALSE((res=f_mkfs(storage_drv, FM_ANY | FM_SFD, CONFIG_WL_SECTOR_SIZE, workbuf, workbuf_size))==FR_OK
 			, ESP_FAIL, err, TAG, "Failed to make file system (%d)", res);

err:
	ff_memfree(workbuf);
	return ret;
}

static esp_err_t cmd_version(int argc, char **argv)
{
	repl_printf("App Version: %d\r\n", VERSION);
	repl_printf("Revision: %s\r\n", REVISION);
	repl_printf("IDF Version: %s\r\n", esp_get_idf_version());
    esp_chip_info_t info;
    esp_chip_info(&info);
    repl_printf("Chip model: %d\r\nChip Cores: %d\r\nChip Features: 0x%08X\r\nChip Revision: %d\r\n",
    		info.model, info.cores, info.features, info.revision);
	repl_printf("FAN Voltage: %lg V\r\n", (double)(VFAN));
	repl_printf("Rotary: %d\r\n", ROTARY_TYPE);
	repl_printf("Voltage: %d - %d mV\r\n", V_MIN, V_MAX);
	repl_printf("Current: %d - %d mA\r\n", I_MIN, I_MAX);
	repl_printf("Discharge: %d %d %d\r\n", DISCHARGE_PWM, DISCHARGE_DAC, DISCHARGE_BASE);
	repl_printf("FAN: %d %lg %lg %lg %lg %lg\r\n", FAN_PWM, (double)FAN_KP, (double)FAN_KA, (double)FAN_TI, (double)FAN_TD, (double)FAN_TF);
	repl_printf("Storage: %s %s\r\n", FS_STORAGE_PAR, FS_STORAGE_MNT);
	repl_printf("Resource: %s %s\r\n", FS_RESOURCE_PAR, FS_RESOURCE_MNT);

    return ESP_OK;
}

static esp_err_t cmd_memory(int argc, char **argv)
{
	repl_printf("Free MEM : %d, DRAM : %d\r\n"
		, heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_32BIT)
		, heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));

	return ESP_OK;
}

static esp_err_t cmd_task(int argc, char **argv)
{
	UBaseType_t x, uxArraySize=uxTaskGetNumberOfTasks();
	TaskStatus_t *pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );
	ESP_RETURN_ON_FALSE(pxTaskStatusArray
			, ESP_ERR_NO_MEM, TAG, "Failed to alloc memory for listing tasks");

	uint32_t ulTotalRunTime;
	uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

	repl_printf("Total runtime : %u\r\n", ulTotalRunTime);

	repl_printf("#\tName        \tStatus\tPrio\tStackHWM"
#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
			"\tCore"
#endif
			"\tTime\r\n");

	ulTotalRunTime /= 100;
	if (ulTotalRunTime<1) ulTotalRunTime=1;
	uint32_t ulStatsAsPercentage, half=ulTotalRunTime/2;
	char s_core[8], s_percent[8];
	for( x = 0; x < uxArraySize; x++ ) {
		 ulStatsAsPercentage = (pxTaskStatusArray[x].ulRunTimeCounter + half)/ulTotalRunTime;
		 if (ulStatsAsPercentage==0)
			 strcpy(s_percent, "<1");
		 else
			 sprintf(s_percent, "%2u", ulStatsAsPercentage);

#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
		 if (pxTaskStatusArray[x].xCoreID==tskNO_AFFINITY)
			 strcpy(s_core, "N/A");
		 else
			 sprintf(s_core, "%3u", pxTaskStatusArray[x].xCoreID);
#endif

		 repl_printf("%u\t%-12s\t%6u\t%u/%u\t%8u"
#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
			"\t%4s"
#endif
				 "\t%3s%%\r\n",
				 pxTaskStatusArray[x].xTaskNumber,
				 pxTaskStatusArray[x].pcTaskName,
				 pxTaskStatusArray[x].eCurrentState,
				 pxTaskStatusArray[x].uxCurrentPriority, pxTaskStatusArray[x].uxBasePriority,
				 pxTaskStatusArray[x].usStackHighWaterMark,
#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
				 s_core,
#endif
				 s_percent
		 );
	}
	vPortFree( pxTaskStatusArray );

	return ESP_OK;
}

static esp_err_t cmd_timer(int argc, char **argv)
{
	return esp_timer_dump(stdout);
}

static esp_err_t cmd_file(int argc, char **argv)
{
 	if (argc>1 && !strcmp(argv[1], "format")) {
		esp_err_t ret=app_format();
		if (ret==ESP_OK)
			repl_printf("Storage is formated, all file is discarded\r\n");

		return ret;
 	}

	FRESULT res;
    FATFS *fs;
    size_t free_clusters;
    res=f_getfree(storage_drv, &free_clusters, &fs);
    if (res==FR_OK) {
		size_t total_sectors = (fs->n_fatent - 2) * fs->csize;
		size_t free_sectors = free_clusters * fs->csize;
		repl_printf("In internal storage, total bytes: %d, free bytes: %d\r\n",
				    total_sectors * CONFIG_WL_SECTOR_SIZE, free_sectors * CONFIG_WL_SECTOR_SIZE);
    } else
    	repl_printf("Failed to get storage infomation (%d)\r\n", res);

    FF_DIR d;
    FILINFO finfo;
    res=f_opendir(&d, storage_drv);
    if (res==FR_OK) {
    	while (f_readdir(&d, &finfo)==FR_OK) {
    		if (!finfo.fname[0]) break;
    		if (finfo.fname[0]=='.') continue;
     		repl_printf("%s: %d bytes\r\n", finfo.fname, finfo.fsize);
    	}
    } else
    	repl_printf("Failed to get root directory infomation (%d)\r\n", res);

   return ESP_OK;
}

static esp_err_t cmd_log(int argc, char **argv)
{
	int n=0;
	int clear=0;
	char buff[256];

	while (argc>1) {
		if (!strcmp(argv[argc-1], "clear"))
			clear=1;
		else if (sscanf(argv[argc-1], "%d", &n)!=1)
			n=0;
		argc--;
	}

	if (clear) {
		int i;
		for (i=n; i<FS_LOG_FILE_NUM; i++) {
			snprintf(buff, sizeof(buff), FS_LOG_FILE, i);
		    remove(buff);
		}
		repl_printf("Deleted logs from #%d\r\n", n);
		return ESP_OK;
	}

	snprintf(buff, sizeof(buff), FS_LOG_FILE, n);

	FILE *f=fopen(buff, "r");
	if (!f)
		repl_printf("Log file #%d does not exist.\r\n", n);
	else{
		repl_printf("Log file #%d\r\n", n);
		do {
			n=fread(buff, 1 , sizeof(buff), f);
			fwrite(buff, 1, n, stdout);
		} while (n==sizeof(buff));
		fclose(f);
		repl_printf("\r\n");
	}

	return ESP_OK;
}

static esp_err_t cmd_reset(int argc, char **argv)
{
	repl_printf("...Reset\r\n");
	app_reset();

	return ESP_OK;
}

static esp_err_t register_repl_cmd()
{
	static const esp_console_cmd_t cmd [] = {
		{
	        .command = "version",
	        .help = "Show version infomation",
	        .hint = NULL,
	        .func = &cmd_version,
	    }, {
	        .command = "memory",
	        .help = "Show free memory",
	        .hint = NULL,
	        .func = &cmd_memory,
	    }, {
			.command = "task",
			.help = "Show current task running state",
			.hint = NULL,
			.func = &cmd_task,
	    }, {
			.command = "file",
			.help = "Show file system",
			.hint = "[format - to format internal file system]",
			.func = &cmd_file,
		}, {
			.command = "timer",
			.help = "Show timer state",
			.hint = NULL,
			.func = &cmd_timer,
		},{
			.command = "log",
			.help = "Show current and history logs stored in flash",
			.hint = "[log number, 0:this boot, 1:last ...] [clear, to delete logs from the number]",
			.func = &cmd_log,
		},{
			.command = "reset",
			.help = "Reset the system",
			.hint = NULL,
			.func = &cmd_reset,
		},
	};

	int i;
	for (i=0; i<sizeof(cmd)/sizeof(esp_console_cmd_t); i++)
		esp_console_cmd_register(&cmd[i]);

	return ESP_OK;
}

static int log_to_file(const char * fmt, va_list args)
{
	if (wl_storage!=WL_INVALID_HANDLE && log_file[0]) {
		FILE *f=fopen(log_file, "a");
		if (f) {
			vfprintf(f, fmt, args);
			fclose(f);
		}
	}

	if (log_original)
		return log_original(fmt, args);
	else
		return vprintf(fmt, args);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing ...");

    esp_err_t ret=ESP_OK, ret2;

    /* prepare reset clean-up function */
    ESP_GOTO_ON_ERROR(ret=esp_register_shutdown_handler(&app_on_shutdown)
			, reset, TAG, "Failed to register shutdown handler (%d:%s)", ret, esp_err_to_name(ret));

    /* config watch dog */
    ESP_GOTO_ON_ERROR(ret=esp_task_wdt_init(WATCH_DOG_TIME, 0)
    		, reset, TAG, "Failed to initialize watch dog (%d:%s)", ret, esp_err_to_name(ret));

    /* init flash file system with WL support first, driver "0:" */
    esp_vfs_fat_mount_config_t mntcfg1 = {
		.max_files = FS_STORAGE_OPENMAX,
		.format_if_mount_failed = 1,
		.allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };
    ESP_GOTO_ON_ERROR(ret=esp_vfs_fat_spiflash_mount(FS_STORAGE_MNT, FS_STORAGE_PAR, &mntcfg1, &wl_storage)
			, reset, TAG, "Failed to mount flash storage file system (%d:%s)", ret, esp_err_to_name(ret));

    esp_vfs_fat_mount_config_t mntcfg2 = {
		.max_files = FS_RESOURCE_OPENMAX,
		.format_if_mount_failed = 0,
		.allocation_unit_size = 4096 /* default generated by fatfs_create_rawflash_image() */
    };
    ESP_GOTO_ON_ERROR(ret=esp_vfs_fat_rawflash_mount(FS_RESOURCE_MNT, FS_RESOURCE_PAR, &mntcfg2)
			, reset, TAG, "Failed to mount flash resource file system (%d:%s)", ret, esp_err_to_name(ret));

    /* duplicate log stream to log file */
    int i;
    char fname[256];
    snprintf(fname, sizeof(fname), FS_LOG_FILE, FS_LOG_FILE_NUM-1);
    remove(fname);
    for (i=FS_LOG_FILE_NUM-1; i>0; i--) {
        snprintf(log_file, sizeof(log_file), FS_LOG_FILE, i-1);
        snprintf(fname, sizeof(fname), FS_LOG_FILE, i);
    	rename(log_file, fname);
    }
	log_original=esp_log_set_vprintf(log_to_file);

    /* init NVS */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_GOTO_ON_ERROR(ret=nvs_flash_erase()
        		, reset, TAG, "Failed to clean NVS for recovering (%d:%s)", ret, esp_err_to_name(ret));

        ESP_GOTO_ON_ERROR(ret=nvs_flash_init()
        		, reset, TAG, "Failed to init NVS (%d:%s)", ret, esp_err_to_name(ret));
    } else if (ret!=ESP_OK)
        ESP_GOTO_ON_FALSE(0
        		, ret, reset, TAG, "Failed to init NVS (%d:%s)", ret, esp_err_to_name(ret));

    /* create event group for signal bits usage */
    ESP_GOTO_ON_FALSE((app_egroup=xEventGroupCreateStatic(&egroup))!=NULL
			,ESP_FAIL, reset, TAG, "Failed to create event group");

    /* create default event loop for system usage */
    ESP_GOTO_ON_ERROR(ret=esp_event_loop_create_default()
			, reset, TAG, "Failed to create default event loop (%d:%s)", ret, esp_err_to_name(ret));

    /* init CORE/Stack customized event loop
     * default event loop is pinned to CORE0, core usage is not balance */
    esp_event_loop_args_t ecfg = {
		.queue_size = 64,
		.task_name = "APP_ELOOP",
		.task_priority = 3,
		.task_stack_size = ELOOP_STACK,
		.task_core_id = ELOOP_CORE
	};
    ESP_GOTO_ON_ERROR(ret=esp_event_loop_create(&ecfg, &app_eloop)
			, reset, TAG, "Failed to create customized event loop (%d:%s)", ret, esp_err_to_name(ret));

    /* init GPIO ISR service for button and rotary encoder */
    ESP_GOTO_ON_ERROR(ret=gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3)
			, reset, TAG, "Failed to install GPIO ISR service (%d:%s)", ret, esp_err_to_name(ret));

    /* init REPL console and register some app command */
    ESP_GOTO_ON_ERROR(ret=repl_init()
			, reset, TAG, "Failed to initialize REPL console (%d:%s)", ret, esp_err_to_name(ret));
    ESP_GOTO_ON_ERROR(ret=register_repl_cmd()
			, reset, TAG, "Failed to register REPL command (%d:%s)", ret, esp_err_to_name(ret));

    /* init and load config, config should be initialized first */
    ESP_GOTO_ON_ERROR(ret=conf_init()
			, reset, TAG, "Failed to initialize configuration (%d:%s)", ret, esp_err_to_name(ret));

    /* init main system, it contains key parameters, should be initialied early */
    ESP_GOTO_ON_ERROR(ret=main_init()
		, reset, TAG, "Failed to initialize main system (%d:%s)", ret, esp_err_to_name(ret));

    /* init sampling */
    ESP_GOTO_ON_ERROR(ret=samp_init()
    	, reset, TAG, "Failed to initialize sampling system (%d:%s)", ret, esp_err_to_name(ret));

    /* init output */
    ESP_GOTO_ON_ERROR(ret=output_init()
        , reset, TAG, "Failed to initialize output system (%d:%s)", ret, esp_err_to_name(ret));

    /* init stat */
    ESP_GOTO_ON_ERROR(ret=stat_init()
        , reset, TAG, "Failed to initialize statistics system (%d:%s)", ret, esp_err_to_name(ret));

    /* init ui system */
    ESP_GOTO_ON_ERROR(ret=ui_init()
		, reset, TAG, "Failed to initialize user interface (%d:%s)", ret, esp_err_to_name(ret));

    /* wait for stability of all sub-systems, eg. set-up time for of CS1237 */
    vTaskDelay(200/portTICK_PERIOD_MS);

    /* creat main task to run */
    ESP_GOTO_ON_FALSE(xTaskCreatePinnedToCore(main_task, "MAIN_TASK", MAIN_STACK, NULL, MAIN_PRIORITY, NULL, MAIN_CORE)==pdPASS
    		, ESP_FAIL, reset, TAG, "Failed to create main task");

    ESP_GOTO_ON_FALSE(xEventGroupSetBits(app_egroup, APP_EG_READY) & APP_EG_READY
    		, ESP_FAIL, reset, TAG, "Failed to set READY bit on event group");
    ESP_GOTO_ON_ERROR((ret=esp_event_post_to(app_eloop, APP_E_BASE, APP_E_READY, NULL, 0, portMAX_DELAY))
    		, reset, TAG, "Failed to post READY event (%d:%s)", ret, esp_err_to_name(ret));

    ESP_LOGI(TAG, "System ready");

    /* let WIFI/web work, and it's not fatal */
    if ((ret2=wifi_init())!=ESP_OK
     || (ret2=wifi_reset())!=ESP_OK)
    	ESP_LOGW(TAG, "Failed to start WIFI (%d:%s)", ret2, esp_err_to_name(ret2));
   else if ((ret2=web_init())!=ESP_OK)
    	ESP_LOGW(TAG, "Failed to start web server (%d:%s)", ret2, esp_err_to_name(ret2));

    return;

reset:
	ESP_LOGE(TAG, "Failed to init system, will reset after 10 seconds");
    vTaskDelay(10000/portTICK_PERIOD_MS);
	app_reset();
}


