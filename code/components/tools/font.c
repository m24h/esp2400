/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"

#include "font.h"

static const char * const TAG = "font";

static uint32_t font_from_utf8(const char **c)
{
/*
 * 	Bytes Bits Hex Min  Hex Max  Byte Sequence in Binary
	  1     7  00000000 0000007f 0vvvvvvv
	  2    11  00000080 000007FF 110vvvvv 10vvvvvv   ?vvv vvvv vvvv
	  3    16  00000800 0000FFFF 1110vvvv 10vvvvvv 10vvvvvv  0011 0000 0000 0010
	  4    21  00010000 001FFFFF 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv
	  5    26  00200000 03FFFFFF 111110vv 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv
	  6    31  04000000 7FFFFFFF 1111110v 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv
 */
	if (!c || !*c || !**c) return 0;

	uint32_t ret=(uint32_t)*((*c)++) & 0xff;
	if ((ret & 0xc0) == 0xc0 && (**c & 0xc0) == 0x80) {
		ret=(ret & 0x3f)<<6 | ((uint32_t)*((*c)++) & 0x3f);
		if ((ret & 0x800) && (**c & 0xc0) == 0x80) {
			ret=(ret & 0x7ff)<<6 | ((uint32_t)*((*c)++) & 0x3f);
			if ((ret & 0x10000) && (**c & 0xc0) == 0x80) {
				ret=(ret & 0xffff)<<6 | ((uint32_t)*((*c)++) & 0x3f);
				if ((ret & 0x200000) && (**c & 0xc0) == 0x80) {
					ret=(ret & 0x1f0000)<<6 | ((uint32_t)*((*c)++) & 0x3f);
					if ((ret & 0x4000000) && (**c & 0xc0) == 0x80) {
						ret=(ret & 0x3ffffff)<<6 | ((uint32_t)*((*c)++) & 0x3f);
					}
				}
			}
		}
	}

	return ret;
}

static uint32_t font_from_gb2312(const char **c)
{
	if (!c || !*c || !**c) return 0;

	uint32_t ret=(uint32_t)*((*c)++) & 0xff;
	if (ret<0xA1 || ret>0xFE) return ret;
	uint8_t t=(uint8_t) **c;
	if (t<0xA1 || t>0xFE) return ret;
	(*c)++;
	return (ret<<8)|t;
}

uint32_t font_from(const char **c, font_coding_e coding)
{
	if (coding==FONT_UTF8)
		return font_from_utf8(c);
	else if (coding==FONT_GB2312)
		return font_from_gb2312(c);

	return (uint32_t)*((*c)++);
}

static int cmp_codes(const void *a, const void *b)
{
	if (*(uint32_t*)a > *(uint32_t*)b) return 1;
	if (*(uint32_t*)a < *(uint32_t*)b) return -1;
	return 0;
}

esp_err_t font_find(font_found_t * found, const font_lib_t *lib, uint32_t code)
{
	ESP_RETURN_ON_FALSE(found
			, ESP_ERR_INVALID_ARG, TAG, "Null argument");

	found->data=NULL;
	if (code==0) return ESP_OK;

	while (lib) {
		if ((lib->flag & FONT_SORTED) && lib->size>32 ) {
			const uint32_t *p=bsearch (&code, lib->codes, lib->size, sizeof (uint32_t), cmp_codes);
			if (p) {
				found->data=lib->fonts+(p-lib->codes)*lib->bytes;
				found->height=lib->height;
				found->width=lib->width;
				found->skip=lib->skip;
				return ESP_OK;
			}
		} else {
			for (int i=0; i<lib->size; i++) {
				if (!lib->codes[i])
					break;
				if (lib->codes[i] == code) {
					found->data=lib->fonts+(i*lib->bytes);
					found->height=lib->height;
					found->width=lib->width;
					found->skip=lib->skip;
					return ESP_OK;
				}
			}
		}
		lib=lib->next;
	}

	return ESP_OK;
}

esp_err_t font_lib_load (font_lib_t **lib, int width, int height, int skip, int bytes,
		       const uint8_t *fonts, const char *index, font_coding_e coding,
			   int size, uint32_t index_end, uint32_t flag)
{
	ESP_RETURN_ON_FALSE(lib && fonts && index
			, ESP_ERR_INVALID_ARG, TAG, "Invalid argument (%p, %p, %p)", lib, fonts, index);

	int i;
	uint32_t c;

	const char *s=index;
	for (i=0; i<size; i++) {
		c=font_from(&s, coding);
		if (!c || c==index_end) break;
	}
	size=i;

	font_lib_t *l=(font_lib_t *)malloc(sizeof(font_lib_t)+sizeof(uint32_t)*size+8);
	ESP_RETURN_ON_FALSE(l
			, ESP_ERR_NO_MEM , TAG, "Failed to allocate font lib memory");

	l->size=size;
	l->bytes=bytes;
	l->flag=flag;
	l->fonts=fonts;
	l->codes=(const uint32_t *)(l+1);
	l->width=width;
	l->height=height;
	l->skip=skip;
	l->next=NULL;

	for (i=0; i<size; i++) {
		c=font_from(&index, coding);
		if (!c || c==index_end) {
			size=i;
			break;
		}
		((uint32_t *)l->codes)[i]=c;
	}

	*lib=l;

	return ESP_OK;
}

esp_err_t font_lib_wrap (font_lib_t **lib, int width, int height, int skip, int bytes,
		       const uint8_t *fonts, const uint32_t *codes, int size, uint32_t flag)
{
	ESP_RETURN_ON_FALSE(lib && fonts && codes
			, ESP_ERR_INVALID_ARG, TAG, "Invalid argument (%p, %p, %p)", lib, fonts, codes);

	int i;
	for (i=0; i<size; i++)
		if (!codes[i]) break;
	size=i;

	font_lib_t *l=(font_lib_t *)malloc(sizeof(font_lib_t));
	ESP_RETURN_ON_FALSE(l
			, ESP_ERR_NO_MEM , TAG, "Failed to allocate font lib memory");

	l->size=size;
	l->bytes=bytes;
	l->fonts=fonts;
	l->flag=flag;
	l->codes=codes;
	l->width=width;
	l->height=height;
	l->skip=skip;
	l->next=NULL;

	*lib=l;

	return ESP_OK;
}

esp_err_t font_lib_free (font_lib_t **lib)
{
	ESP_RETURN_ON_FALSE(lib
			, ESP_ERR_INVALID_ARG, TAG, "NULL argument");

	if (*lib) {
		free(*lib);
		*lib=NULL;
	}

	return ESP_OK;
}

esp_err_t font_lib_free_chain (font_lib_t **lib)
{
	ESP_RETURN_ON_FALSE(lib
			, ESP_ERR_INVALID_ARG, TAG, "NULL argument");

	while (*lib) {
		font_lib_t * next=(*lib)->next;
		free(*lib);
		*lib=next;
	}

	return ESP_OK;
}
