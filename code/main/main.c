/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "esp_task_wdt.h"
#include "esp_console.h"

#include "app.h"
#include "samp.h"
#include "output.h"
#include "stat.h"
#include "ui.h"
#include "repl.h"
#include "pidc.h"
#include "main.h"

/*
static const char * const TAG = "MAIN";
*/

/**
 * key running parameters
 */
main_vars_t main_vars = {
	.vs=V_MIN,
	.is=I_MIN,
	.on=1, /* default on */
};

esp_err_t main_message(const char *s)
{
	if (s)
		strlcpy(main_vars.msg, s, sizeof(main_vars.msg));
	else
		main_vars.msg[0]=0;

	return ESP_OK;
}

static esp_err_t cmd_show(int argc, char **argv)
{
	repl_printf("Ouputing Status : %s\r\n", main_vars.on?"On":"Off");
	repl_printf("Setting Voltage : %d mV (%08X)\r\n", main_vars.vs, main_vars.raw_vs);
	repl_printf("Present Voltage : %d mV (%08X)\r\n", main_vars.vc, main_vars.raw_vc);
	repl_printf("Setting Current : %d mA (%08X)\r\n", main_vars.is, main_vars.raw_is);
	repl_printf("Present Current : %d mA (%08X)\r\n", main_vars.ic, main_vars.raw_ic);
	repl_printf("Power  : %d mW\r\n", main_vars.p);
	repl_printf("Energy : %lld J\r\n", main_vars.e/1000000);
	repl_printf("Temperature : %lg celsius degree\r\n", main_vars.temp/1000.0);
	repl_printf("Fan output : %d/255\r\n", main_vars.fan);
	repl_printf("Discharge output : %d/255\r\n", main_vars.discharge);

    return ESP_OK;
}

static esp_err_t cmd_getv(int argc, char **argv)
{
	repl_printf("Voltage : %d mV\r\n", main_vars.vc);

    return ESP_OK;
}

static esp_err_t cmd_geti(int argc, char **argv)
{
	repl_printf("Current : %d mA\r\n", main_vars.ic);

    return ESP_OK;
}

static esp_err_t cmd_getp(int argc, char **argv)
{
	repl_printf("Power : %d mW\r\n", main_vars.p);

    return ESP_OK;
}

static esp_err_t cmd_gete(int argc, char **argv)
{
	repl_printf("Energy : %lld J\r\n", (main_vars.e+500000)/1000000);

    return ESP_OK;
}

static esp_err_t cmd_gett(int argc, char **argv)
{
	repl_printf("Temperature : %lg celsius degree\r\n", main_vars.temp/1000.0);

    return ESP_OK;
}

static esp_err_t cmd_setv(int argc, char **argv)
{
	int to_set = main_vars.vs;

	if (argc > 1 && argv[1] && sscanf(argv[1], "%d", &to_set)==1) {
		if (to_set < V_MIN) to_set=V_MIN;
		else if (to_set > V_MAX) to_set=V_MAX;
		main_vars.vs=to_set;
	}

	repl_printf("Voltage Set : %d mV\r\n", main_vars.vs);

    return ESP_OK;
}

static esp_err_t cmd_seti(int argc, char **argv)
{
	int to_set = main_vars.is;

	if (argc > 1 && argv[1] && sscanf(argv[1], "%d", &to_set)==1) {
		if (to_set < I_MIN) to_set=I_MIN;
		else if (to_set > I_MAX) to_set=I_MAX;
		main_vars.is=to_set;
	}

	repl_printf("Current Set: %d mA\r\n", main_vars.is);

    return ESP_OK;
}

static esp_err_t cmd_sete(int argc, char **argv)
{
	int64_t to_set = main_vars.e;

	if (argc > 1 && argv[1] && sscanf(argv[1], "%lld", &to_set)==1) {
		if (to_set < 0) to_set=0;
		main_vars.e=to_set*1000000;
	}

	repl_printf("Energy : %lld J\r\n", main_vars.e/1000000);

    return ESP_OK;
}

static esp_err_t cmd_output(int argc, char **argv)
{
	if (argc>1 && argv[1]) {
		if (!strcmp("on", argv[1]))
			main_vars.on=1;
		else if (!strcmp("off", argv[1]))
			main_vars.on=0;
	}

	repl_printf("Ouputing : %s\r\n", main_vars.on?"on":"off");

    return ESP_OK;
}

static esp_err_t register_repl_cmd()
{
	static const esp_console_cmd_t cmd [] = {
		{
			.command = "show",
			.help = "Show all parameters",
			.hint = NULL,
			.func = &cmd_show,
		}, {
	        .command = "getv",
	        .help = "Get actual voltage",
	        .hint = NULL,
	        .func = &cmd_getv,
	    }, {
	        .command = "geti",
	        .help = "Get actual current",
	        .hint = NULL,
	        .func = &cmd_geti,
	    }, {
	        .command = "getp",
	        .help = "Get actual power",
	        .hint = NULL,
	        .func = &cmd_getp,
	    }, {
	        .command = "gete",
	        .help = "Get integral energy",
	        .hint = NULL,
	        .func = &cmd_gete,
	    }, {
	        .command = "gett",
	        .help = "Get temperature",
	        .hint = NULL,
	        .func = &cmd_gett,
	    }, {
			.command = "setv",
			.help = "Get and set target of voltage",
			.hint = "[new voltage setting, if specified, in mV]",
			.func = &cmd_setv,
		}, {
			.command = "seti",
			.help = "Get and set target of current",
			.hint = "[new current setting, if specified, in mA]",
			.func = &cmd_seti,
		}, {
			.command = "sete",
			.help = "Set new integral energy value",
			.hint = "[new integral energy value, 0 to reset totally]",
			.func = &cmd_sete,
		}, {
			.command = "output",
			.help = "Show/Set outputing status",
			.hint = "[on/off: set outputing on/off]",
			.func = &cmd_output,
		},
	};

	int i;
	for (i=0; i<sizeof(cmd)/sizeof(esp_console_cmd_t); i++)
		esp_console_cmd_register(&cmd[i]);

	return ESP_OK;
}

esp_err_t main_init()
{
	main_vars.vs=conf_vars.quick.v[0];
	if (main_vars.vs<V_MIN) main_vars.vs=V_MIN;
	else if (main_vars.vs>V_MAX) main_vars.vs=V_MAX;

	main_vars.is=conf_vars.quick.i[0];
	if (main_vars.is<I_MIN) main_vars.is=I_MIN;
	else if (main_vars.is>I_MAX) main_vars.is=I_MAX;

	main_vars.on=1;

	return register_repl_cmd();
}

void main_task()
{
	esp_task_wdt_add(NULL);

	int waitTicks=MAIN_PERIOD_MS>portTICK_PERIOD_MS?MAIN_PERIOD_MS/portTICK_PERIOD_MS:1;
	int waitMs=waitTicks*portTICK_PERIOD_MS;
	int fan_time=0;
	int t;

	pidc_t fan_pid;
	pidc_init(&fan_pid);
	pidc_param(&fan_pid, FAN_KP, FAN_TI, FAN_TD, FAN_TF); /* will do a process every second */
	pidc_deadzone(&fan_pid, -0.7, 0.7);
	pidc_ilimit(&fan_pid, -0.1, 1.1);

	TickType_t xLastWakeTime=xTaskGetTickCount();

    while(1) {
    	/* check watchdog */
    	esp_task_wdt_reset();

    	/* wait process time */
    	xTaskDelayUntil(&xLastWakeTime, waitTicks);

    	/* sample data */
        samp_samp();

#if DISCHARGE_PWM || DISCHARGE_DAC
        /* discharge algorithm for PWM or DAC*/
        t=main_vars.vs;
        /* minus 1V, avoid jitter of main_vars.vc and calibration error */
        if (t<main_vars.vc-1000) t=main_vars.vc;
        /* constant load */
        t=t<DISCHARGE_BASE?255:(255*DISCHARGE_BASE/t);

        if (t<main_vars.discharge-4 || t>main_vars.discharge+4)
        	main_vars.discharge=t;
#else
        /* discharge algorithm for GPIO level with hysteretic feature, not tested
         * start discharging when vs<DISCHARGE_BASE */
        main_vars.discharge=((main_vars.vs-main_vars.discharge-main_vars.discharge)<DISCHARGE_BASE)?255:0;
        /* this discharge circiut should cooperate with a slow-dischaging 2k/3W resistor tied in voltage output
         * when vc>vs+1V, discharging start, and stop when vc<vs+0.5V, resistor slowly discharges remains */
        main_vars.discharge=((main_vars.vc+main_vars.discharge+main_vars.discharge)>main_vars.vs+1000)?255:0;
#endif

#if FAN_PWM
        /* fan PWM control algorithm using PID feat. main_vars.ic */
	    fan_time+=waitMs;
        if (fan_time>=1000) { /* process period is 1s */
        	fan_time-=1000;
    		pidc_target(&fan_pid, conf_vars.fan_temp);
    		t=pidc_run(&fan_pid, main_vars.temp/1000.0, (double)main_vars.ic/I_MAX*FAN_KA)*255;
    		main_vars.fan=t<0?0:(t>255?255:t);
        }
#else
        /* fan control algorithm for GPIO level with hysteretic feature, not tested */
        fan_time+=waitMs;
        if (fan_time>=1000) { /* process period is 1s */
        	fan_time-=1000;
        	t=(main_vars.temp+500)/1000;
            main_vars.fan=(t>conf_vars.fan_temp-(main_vars.fan>127?3:-3))?255:0;
        }
#endif

       	/* ouput control */
        output_out();

    	/* gather statistics */
        stat_stat();
    }
}
