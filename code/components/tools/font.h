/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FONT_SORTED  0x01  /* font lib is incremently sorted for fast search */

typedef enum {
	FONT_BYTE=0,
	FONT_UTF8,
	FONT_GB2312
} font_coding_e;

typedef struct {
	int 	       width;
	int 	       height;
	int		       skip;
	const uint8_t *data;
} font_found_t;

/**
 * font has at least [width]*[height] bits
 * [size] is number of font in lib
 * [skip] is for every line
 * [bytes] is length of every font
 * [next] to point another lib in chain
 */
typedef struct font_lib_s {
	int 			   size;
	uint32_t           flag;
	int 	           width;
	int 	           height;
	int		           skip;
	int                bytes;

	struct font_lib_s  *next;
	const uint8_t      *fonts;
	const uint32_t     *codes;
} font_lib_t;

/**
 * transfer chars to some uint32 code using [coding]
 * and move pointer *<c> to next char
 */
uint32_t font_from(const char **c, font_coding_e coding);

/**
 * search font lib and return font data with matched [code]
 * result stored in [found], its [data] is not NULL if found
 */
esp_err_t font_find(font_found_t * found, const font_lib_t *lib, uint32_t code);

/**
 * make font lib using specified font [data] and [index] string coded by [coding]
 * [lib] is a pointer to return allocated struct
 * [index_end] coded character to indicate ending of index string
 * note: \0 always end index string
 * [size] also limit the lib size
 * [flag] is some feature of font lib, like FONT_SORTED
 * this will allocate memory for storing internal codes
 * but use input [fonts] directly
 * use font_lib_free to free created font lib
 */
esp_err_t font_lib_load (font_lib_t **lib, int width, int height, int skip, int bytes,
		       const uint8_t *fonts, const char *index, font_coding_e coding,
			   int size, uint32_t index_end, uint32_t flag);

/**
 * make font lib using specified font [data] and [index]
 * [font] is array of font data, and data of each font is bits of pixels
 * [index] is array of uint32_t code of each font, code 0 ends the index
 * [lib] is a pointer to return allocated struct
 * [size] also limit the lib size
 * [flag] is some feature of font lib, like FONT_SORTED
 * this use [fonts] and [index] directly, not allocate memory for internal codes
 * use font_lib_free to free created font lib
 */
esp_err_t font_lib_wrap (font_lib_t **lib, int width, int height, int skip, int bytes,
		       const uint8_t *fonts, const uint32_t *codes, int size, uint32_t flag);

/**
 * free allocated lib and store *[lib] to NULL
 * note: chained up next lib and data loaded from outside will not be released
 */
esp_err_t font_lib_free (font_lib_t **lib);

/**
 * free allocated lib including its chained other libs and store *[lib] to NULL
 */
esp_err_t font_lib_free_chain (font_lib_t **lib);

#ifdef __cplusplus
}
#endif


