/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define VERSION         1
#define REVISION        "Zero"

#define VFAN            15.5   /* Vfan for calculate temperature, in Volt, can be double */

#define ADC1_CHAN_TEMP  7 /* ADC1.7, GPIO pin 35 */

#define PIN_FAN        14
#define PIN_DISCHARGE  26
#define DAC_DISCHARGE  DAC_CHANNEL_2

#define PIN_SPICLK    5
#define PIN_MOSI     17
#define PIN_SPIAUX   18
#define PIN_LCDCS    27
#define PIN_RESETLCD 13

#define PIN_IROTA 16
#define PIN_IROTB  4
#define PIN_IBTN   0

#define PIN_VROTA 25
#define PIN_VROTB 33
#define PIN_VBTN  32

#define PIN_ISAMPK 23
#define PIN_ISAMPD 22

#define PIN_VSAMPK 21
#define PIN_VSAMPD 19

#define PIN_IPWM  15
#define PIN_VPWM   2

#define PIN_UART_TX 1
#define PIN_UART_RX 3

#define ROTARY_TYPE  0 /* 0: one pulse per step, 1: half pulse per step, 2:as 0 but cw/ccw reversed, 3:as 1 but cw/ccw reversed */
#define BTN_LONGTIME 1200000 /* 1.2s in micro-second */

#define V_MAX 50000  /* in mV */
#define V_MIN 0      /* in mV */
#define V_OFF 0      /* in mV */
#define I_MAX 50000  /* in mA */
#define I_MIN 0      /* in mA */
#define I_OFF 0      /* in mA */

#define STAT_SIZE     128  /* statistics point number of every second, minute, hour */
#define QUICK_POINTS   27  /* store favorite v/i points to load */

/* default calibration between mV/mA and 0-2^31 scope int32 */
#define CAL_POINTS             16   /* how many calibration points can be set */
#define CAL_ISAMP_MIN           0   /* default I sampling calibration point for I_MIN */
#define CAL_ISAMP_MAX  1709139286   /* default I sampling calibration point for I_MAX */
#define CAL_VSAMP_MIN           0   /* default V sampling calibration point for V_MIN */
#define CAL_VSAMP_MAX  1684300900   /* default V sampling calibration point for V_MIN */
#define CAL_IOUT_MIN    428403863   /* default I output calibration point for I_MIN */
#define CAL_IOUT_MAX   1752958662   /* default I output calibration point for I_MIN */
#define CAL_VOUT_MIN    428403863   /* default V output calibration point for V_MIN */
#define CAL_VOUT_MAX   1458758867   /* default V output calibration point for V_MIN */

#define DISCHARGE_PWM           0  /* use PWM discharge output */
#define DISCHARGE_DAC           1  /* use PWM discharge output */
#define DISCHARGE_BASE       3000  /* if output voltage is below this, discharge fully at nearly 1.3A, now my PWM circuit can only hold 4W */

/*
 *  KP/KA is weights for PID feat. Current Controlling FAN, can be double
 *  KP=1 means full fan output when temperature is nearly 1 degree higher
 *  KA=1 means full fan output when current is nearly I_MAX
 *  unit of TI/TD/TF is second, can be double
 */
#define FAN_KP 0.15
#define FAN_KA 0.3
#define FAN_TI 150
#define FAN_TD   2
#define FAN_TF  10
#define FAN_PWM                 1  /* use PWM fan output */

#define MAX_RECOVERABLE_FAILURES 50 /* continuous recoverable failures will cause reset */
#define WATCH_DOG_TIME     5

#define MAIN_STACK      4096
#define MAIN_CORE       1
#define MAIN_PRIORITY   5
#define MAIN_PERIOD_MS  30  /* do a sampling/output/statistics every period */

#define REPL_STACK      4096
#define REPL_CORE       tskNO_AFFINITY
#define ELOOP_QUEUE     64
#define ELOOP_STACK     4096
#define ELOOP_CORE      tskNO_AFFINITY
#define HTTPD_STACK     6144
#define HTTPD_CORE      tskNO_AFFINITY
#define HTTPD_HANDLERS  8
#define HTTPD_PORT      80
#define HTTPD_DEF_FILE  "index.htm"  /* wl fatfsgen support only 8.3 file name */

#define FS_PATH_SIZE        256 /* include '\0' at tail */
#define FS_STORAGE_OPENMAX  6
#define FS_STORAGE_PAR      "storage"
#define FS_STORAGE_MNT      "/storage"
#define FS_CONF_FILE        FS_STORAGE_MNT "/app.cfg"
#define FS_REPL_HISTORY     FS_STORAGE_MNT "/repl.his"
#define FS_LOG_FILE         FS_STORAGE_MNT "/app%d.log"
#define FS_LOG_FILE_NUM     10  /* will use 0.log - 9.log */

#define FS_RESOURCE_OPENMAX 20/* mostly for web server */
#define FS_RESOURCE_PAR    "resource"
#define FS_RESOURCE_MNT    "/resource"
#define FS_DIR_WEB          FS_RESOURCE_MNT "/web"

/* Terminator colors used by REPL, not to modify*/
#define TERM_NO_SETTING       0
#define TERM_HIGHT_LIGHT      1
#define TERM_UNDERLINE        4
#define TERM_BLINK            5
#define TERM_INVERT           7
#define TERM_FADE             8
#define TERM_COLOR_BLACK     30
#define TERM_COLOR_RED       31
#define TERM_COLOR_GREEN     32
#define TERM_COLOR_YELLOW    33
#define TERM_COLOR_BLUE      34
#define TERM_COLOR_PURPLE    35
#define TERM_COLOR_CYAN      36
#define TERM_COLOR_WHITE     37
#define TERM_BGCOLOR_BLACK   40
#define TERM_BGCOLOR_RED     41
#define TERM_BGCOLOR_GREEN   42
#define TERM_BGCOLOR_YELLOW  43
#define TERM_BGCOLOR_BLUE    44
#define TERM_BGCOLOR_PURPLE  45
#define TERM_BGCOLOR_CYAN    46
#define TERM_BGCOLOR_WHITE   47

#define REPL_PROMPT          "\033[32mzxd2400>\033[0m "
#define REPL_PROMPT_NO_ESC   "zxd2400> "
#define REPL_HINT_COLOR      TERM_COLOR_YELLOW
#define REPL_LINE_MAX        256
#define REPL_ARGS_MAX        16
#define REPL_HISTORY_LINES   0

#define COLOR_BG          0x000000
#define COLOR_IC          0xFF0000
#define COLOR_IS          0xFF8080
#define COLOR_VC          0x00FF00
#define COLOR_VS          0x80FF80
#define COLOR_OFF         0x303030
#define COLOR_IT          0xB80000
#define COLOR_VT          0x00B800
#define COLOR_PT          0xB8B800
#define COLOR_ET          0x00B8B8
#define COLOR_TEMP        0x0000DF
#define COLOR_MESSAGE     0xC0C0C0
#define COLOR_SIGN        0xC0C000
#define COLOR_MENU        0xC0C0C0
#define COLOR_MENUHL      0xFFFF00
#define COLOR_MENUHINT    0x00D0D0

#ifdef __cplusplus
}
#endif
