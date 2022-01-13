/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * do nothing and just return to higher level handler
 */
int32_t menu_return_handler(void * arg, int32_t event, void *data);

/*
 * list style menu
 */
typedef struct {
	const char *   text;    /* item title, can be bundle reference */
	ui_handler_t   handler; /* if handler is NULL, this cannot be focused */
	void         * arg;     /* argument for calling [handler] */
} menu_list_item_t;

typedef struct {
	const menu_list_item_t * items;
	int                      num;       /* number of items */
	void * data; /* data got when entered, will be delivered to handler when entering handler */
    int    focused;   /* which item is focused */
    int	   entered;   /* which item is entered, indexing from 1 */
    int    showfrom;  /* which item is showed at top */
} menu_list_t;

int32_t menu_list_handler(void * arg, int32_t event, void *data);

/* option initializer
 * return number of items, -1 means not to modify original number
 * set items through [itmes] if needed, or leave it untouched
 * set focused (indexing from 1) if needed, or leave it untouched
 */
typedef int (*menu_option_init_t)(void *arg, const char * const ** items, int num, int * focused);

/*
 * option picking style menu, there's always a "return" option at first, internally indexed as 0
 */
typedef struct {
	const char * const * items; /* a (const char *)/bundle reference array of option titles */
	int                  num;   /* number of items */
	int                  cols;  /* number of column to show */
	menu_option_init_t   init;       /* if it is not-null, it will be called to provide [items] when entered */
	void *               init_arg;   /* argument for [handler] */
	ui_handler_t         handler; /* when option picked, the index (start from 0) is send to this through [data] in UI_E_ENTER event*/
	void *               arg;     /* argument for [handler] */
    int                  focused;   /* which item is focused, 0 is "back" */
    int			         entered;   /* which item is entered, starting from 1 */
    int                  showfrom;  /* which item is showed at top */
} menu_option_t;

int32_t menu_option_handler(void * arg, int32_t event, void *data);

/**
 * handler for initialied focus at index value (indexing from 1) in (int*)[arg] (indexing from 0)
 */
int menu_option_init_index (void *arg, const char * const ** items, int num, int * focused);

/**
 * handler for selected index value (indexing from 0) to an (int*) [arg]
 */
int32_t menu_option_set_index (void *arg, int32_t event, void *data);

/**
 * type of [arg] for int option
 */
typedef struct {
	int * value;  /* the pointer to value for getting/setting */
	const int * items;  /* the int version of option items for pick */
} menu_option_int_arg;

/**
 * handler for initialied focus at int-equal item, *[arg] is a menu_option_int_arg
 */
int menu_option_init_int (void *arg, const char * const ** items, int num, int * focused);

/**
 * handler for set picked value in (menu_option_int_arg *)[arg]
 */
int32_t menu_option_set_int (void *arg, int32_t event, void *data);

/**
 * confirm an operation or cancel
 */
typedef struct {
	const char *            hint;    /* hint text showed at top, can be bundle reference also */
	ui_handler_t            handler; /* when "OK" is clicked, this handler is entered */
	void *                  arg;     /* arg for [handler] */
	void *                  data;    /* data got when entered, will be delivered to handler when entering handler */
	int                     focused; /* 0: cancel 1:ok */
    int			            entered; /* 1 if handler is entered */
} menu_confirm_t;

int32_t menu_confirm_handler(void * arg, int32_t event, void *data);

/**
 * a simple handler just to call esp_err_t (*arg)() when entered, then to return
 */
int32_t menu_caller_simple (void *arg, int32_t event, void *data);

/**
 * string inputing
 */
typedef void (*menu_input_getter_t) (void * arg, void *data, char *buff, int size);

typedef void (*menu_input_setter_t) (void * arg, void *data, const char *str);

typedef struct {
	const char *             chars;       /* characters can be input */
	char                     buff[28];    /* at most 25 characters, and tailing '\0' */
	int                      size;        /* maxium number of characters can input, including tailing '\0' */
	menu_input_getter_t      getter;      /* initializer for setting buffer before start */
	void *                   getter_arg;
	menu_input_setter_t      setter;      /* when inputing is done, call this */
	void *                   setter_arg;
	void *                   data;        /* data got when entered, will be delivered to getter/setter when entering handler */
	int                      cursor;      /* position in inputing string */
	int                      focused;     /* position in characters be elected */
} menu_input_t;

/* input handler, using menu_input_t [arg] */
int32_t menu_input_handler(void * arg, int32_t event, void *data);

/* a menu_input_getter_t function get string from (char *)arg */
void menu_input_getter_str (void * arg, void *data, char *buff, int size);

/* a menu_input_setter_t function set string to (char *)arg, which's size should be at least 26 bytes long*/
void menu_input_setter_str (void * arg, void *data, const char *str);

/* a menu_input_getter_t function get IP from (esp_ip4_addr_t *)arg */
void menu_input_getter_ip (void * arg, void *data, char *buff, int size);

/* a menu_input_setter_t function set IP to (esp_ip4_addr_t *)arg, which's size should be at least 26 bytes long*/
void menu_input_setter_ip (void * arg, void *data, const char *str);

/* a menu_input_getter_t function get I/V from (int *)arg */
void menu_input_getter_iv (void * arg, void *data, char *buff, int size);

/* a menu_input_setter_t function set I/V to (int *)arg, which's size should be at least 26 bytes long*/
void menu_input_setter_iv (void * arg, void *data, const char *str);

/* prompt handler: when entered, show the text in (const char* or bundle reference) [data] or [arg], click to return */
int32_t menu_prompt_handler(void * arg, int32_t event, void *data);

#ifdef __cplusplus
}
#endif
