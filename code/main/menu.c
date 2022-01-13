/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include <math.h>
#include "esp_netif.h"

#include "app.h"
#include "main.h"
#include "menu.h"

#define INPUT_SIZE 25

//extern const uint8_t LIST_TOPPIC[] asm("_binary_listtop_pic_start");
//extern const uint8_t LIST_BTMPIC[] asm("_binary_listbtm_pic_start");

static void list_show (menu_list_t * list)
{
/*
 * drawing Y positions:
 * 3-18
 * 22-37
 * 41-56
 * 60-75
 * 79-94
 * 98-113
 * 117-132
 */
	int i,y,t;
	for (i=0,y=3; i<7; i++,y+=19) {
		t=i+list->showfrom;
		if (t>=list->num) break;
		ui_text(0, y, UI_LCD_WIDTH,
				list->items[t].text,
				t==list->focused?color_menuhl:color_menu,
				font_menu);
	}
}

static void list_focus(menu_list_t * list, int nf)
{
	nf=nf%list->num;
	if (nf<0) nf+=list->num;

	int sf=nf-3;
	if (sf>list->num-7) sf=list->num-7;
	if (sf<0) sf=0;

	if (sf!=list->showfrom) {
		list->showfrom=sf;
		list->focused=nf;
		list_show(list);
	} else {
		ui_text(0, (list->focused-sf)*19+3, UI_LCD_WIDTH,
				list->items[list->focused].text, color_menu, font_menu);

		list->focused=nf;

		ui_text(0, (list->focused-sf)*19+3, UI_LCD_WIDTH,
				list->items[list->focused].text, color_menuhl, font_menu);
	}
}

/* handler to process menu list: menu_list_t */
int32_t menu_list_handler(void * arg, int32_t event, void *data)
{
	menu_list_t * list=(menu_list_t *)arg;

	if (event!=UI_E_ENTER && list->entered>0)
		event=(*list->items[list->entered-1].handler)(list->items[list->entered-1].arg, event, data);

list_process:
	switch(event) {
	case UI_E_CLOSE:
		list->entered=0;
		return UI_E_CLOSE;
	case UI_E_LEAVE:
		list->entered=0;
		return UI_E_RETURN;
	case UI_E_ENTER:
		list->data=data;
		list->showfrom=0;
		list->focused=0;
		list->entered=0;
		ui_clear();
		list_show(list);
		break;
	case UI_E_DONE:
		list->entered=0;
		break;
	case UI_E_RETURN:
		list->entered=0;
		ui_clear();
		list_show(list);
		break;
	case UI_E_IROTL:
	case UI_E_IROTL_FAST:
	case UI_E_VROTL:
	case UI_E_VROTL_FAST:
		list_focus(list, list->focused-1);
		break;
	case UI_E_IROTR:
	case UI_E_IROTR_FAST:
	case UI_E_VROTR:
	case UI_E_VROTR_FAST:
		list_focus(list, list->focused+1);
		break;
	case UI_E_ICLICK:
	case UI_E_VCLICK:
		if (list->items[list->focused].handler) {
			list->entered=list->focused+1;
			event=(*list->items[list->focused].handler)(list->items[list->focused].arg, UI_E_ENTER, list->data);
			goto list_process;
		}
		break;
	}

	return UI_E_NONE;
}

/* handler for just returning */
int32_t menu_return_handler(void * arg, int32_t event, void *data)
{
	if (event==UI_E_CLOSE) return UI_E_CLOSE;
	return UI_E_LEAVE;
}

static int option_show_from(menu_option_t * option, int focused)
{
	int sf=focused/option->cols - 3;
	int rows=(option->num+1+option->cols-1)/option->cols;
	if (sf>rows-7) sf=rows-7;
	if (sf<0) sf=0;
	sf*=option->cols;

	return sf;
}

static void option_show (menu_option_t * option)
{
/*
 * drawing Y positions:
 * 3-18 :
 * 22-37
 * 41-56
 * 60-75
 * 79-94
 * 98-113
 * 117-132
 */
	int i, j, x, y;
	int colw=UI_LCD_WIDTH/option->cols;

	int t=option->showfrom;
	for (i=0,y=3; i<7; i++,y+=19) {
		for (j=0,x=0; j<option->cols; j++,t++,x+=colw) {
			if (t>option->num) break;
			ui_text(x, y, colw,
					t==0?BUNDLE.menu.back:option->items[t-1],
					t==option->focused?color_menuhl:color_menu,
					font_menu);
		}
	}
}

static void option_focus(menu_option_t * option, int nf)
{
	nf=nf%(option->num+1);
	if (nf<0) nf+=option->num+1;

	int sf=option_show_from(option, nf);
	if (sf!=option->showfrom) {
		option->showfrom=sf;
		option->focused=nf;
		option_show(option);
	} else {
		int i=option->focused-sf;
		int colw=UI_LCD_WIDTH/option->cols;
		ui_text((i%option->cols)*colw, (i/option->cols)*19+3, colw,
				option->focused==0?BUNDLE.menu.back:option->items[option->focused-1],
				color_menu,
				font_menu);

		option->focused=nf;

		i=option->focused-sf;
		ui_text((i%option->cols)*colw, (i/option->cols)*19+3, colw,
				option->focused==0?BUNDLE.menu.back:option->items[option->focused-1],
				color_menuhl,
				font_menu);
	}
}

int32_t menu_option_handler(void * arg, int32_t event, void *data)
{
	menu_option_t * option=(menu_option_t *)arg;

	if (event!=UI_E_ENTER && option->entered>0) {
		event=(*option->handler)(option->arg, event, data);
	}

option_process:
	switch(event) {
	case UI_E_CLOSE:
		option->entered=0;
		return UI_E_CLOSE;
	case UI_E_LEAVE:
		option->entered=0;
		return UI_E_RETURN;
	case UI_E_ENTER:
		option->focused=0;
		option->entered=0;
		if (option->init) {
			int nt=(*option->init)(option->init_arg, &option->items, option->num, &option->focused);
			if (nt>=0) option->num=nt;
		}
		ui_clear();
		option->showfrom=option_show_from(option, option->focused);
		option_show(option);
		break;
	case UI_E_DONE:
		option->entered=0;
		break;
	case UI_E_RETURN:
		option->entered=0;
		if (option->init) {
			int nt=(*option->init)(option->init_arg, &option->items, option->num, &option->focused);
			if (nt>=0 && nt!=option->num) {
				option->focused=0;
				option->num=nt;
				option->showfrom=0;
			}
		}
		ui_clear();
		option_show(option);
		break;
	case UI_E_IROTL:
	case UI_E_IROTL_FAST:
	case UI_E_VROTL:
	case UI_E_VROTL_FAST:
		option_focus(option, option->focused-1);
		break;
	case UI_E_IROTR:
	case UI_E_IROTR_FAST:
	case UI_E_VROTR:
	case UI_E_VROTR_FAST:
		option_focus(option, option->focused+1);
		break;
	case UI_E_ICLICK:
	case UI_E_VCLICK:
		if (option->focused<1)
			return UI_E_RETURN;
		if (option->handler) {
			option->entered=option->focused;
			event=(*option->handler)(option->arg, UI_E_ENTER, (void*)(option->entered-1));
			goto option_process;
		}
		break;
	}

	return UI_E_NONE;
}

int menu_option_init_index (void *arg, const char * const ** items, int num, int * focused)
{
	*focused=*(int *)arg +1;
	return -1;
}

int32_t menu_option_set_index (void *arg, int32_t event, void *data)
{
	if (event==UI_E_CLOSE)
		return UI_E_CLOSE;
	else if (event==UI_E_ENTER) {
		if (arg) *(int *)arg=(int)data;
	}

	return UI_E_LEAVE;
}

int menu_option_init_int (void *arg, const char * const ** items, int num, int * focused)
{
	menu_option_int_arg * p=(menu_option_int_arg *)arg;

	int i;
	for (i=0;i<num;i++) {
		if (*p->value==p->items[i]) {
			*focused=i+1;
			break;
		}
	}

	return -1;
}

int32_t menu_option_set_int (void *arg, int32_t event, void *data)
{
	if (event==UI_E_CLOSE)
		return UI_E_CLOSE;
	else if (event==UI_E_ENTER) {
		menu_option_int_arg * p=(menu_option_int_arg *)arg;
		*p->value=p->items[(int)data];
	}

	return UI_E_LEAVE;
}

static void confirm_show (menu_confirm_t * confirm)
{
/*
 * 0-100: show hint
 * 110-125: show choices, x:30, 150
 */
	ui_text_wrap(0, 0, UI_LCD_WIDTH, 100, confirm->hint,	color_menuhint,	font_menu);
	ui_text(20, 110, 100, BUNDLE.menu.confirm_cancel, confirm->focused?color_menu:color_menuhl, font_menu);
	ui_text(140, 110, 100, BUNDLE.menu.confirm_ok, confirm->focused?color_menuhl:color_menu, font_menu);
}

static void confirm_focus (menu_confirm_t * confirm)
{
	ui_text(20, 110, 100, BUNDLE.menu.confirm_cancel, confirm->focused?color_menu:color_menuhl, font_menu);
	ui_text(140, 110, 100, BUNDLE.menu.confirm_ok, confirm->focused?color_menuhl:color_menu, font_menu);
}

int32_t menu_confirm_handler(void * arg, int32_t event, void *data)
{
	menu_confirm_t * confirm=(menu_confirm_t *)arg;

	if (event!=UI_E_ENTER && confirm->entered>0) {
		event=(*confirm->handler)(confirm->arg, event, data);
	}

confirm_process:
	switch(event) {
	case UI_E_CLOSE:
		confirm->entered=0;
		return UI_E_CLOSE;
	case UI_E_DONE:
	case UI_E_RETURN:
	case UI_E_LEAVE:
		confirm->entered=0;
		return UI_E_RETURN;
	case UI_E_ENTER:
		confirm->focused=0;
		confirm->entered=0;
		confirm->data=data;
		ui_clear();
		confirm_show(confirm);
		break;
	case UI_E_IROTL:
	case UI_E_IROTL_FAST:
	case UI_E_VROTL:
	case UI_E_VROTL_FAST:
	case UI_E_IROTR:
	case UI_E_IROTR_FAST:
	case UI_E_VROTR:
	case UI_E_VROTR_FAST:
		confirm->focused=confirm->focused?0:1;
		confirm_focus(confirm);
		break;
	case UI_E_ICLICK:
	case UI_E_VCLICK:
		if (confirm->focused<1)
			return UI_E_RETURN;
		if (confirm->handler) {
			confirm->entered=1;
			event=(*confirm->handler)(confirm->arg, UI_E_ENTER, confirm->data);
			goto confirm_process;
		}
		break;
	}

	return UI_E_NONE;
}

/**
 * a simple handler just to call esp_err_t (*arg)() when entered, then to return
 */
int32_t menu_caller_simple (void *arg, int32_t event, void *data)
{
	if (event==UI_E_CLOSE)
		return UI_E_CLOSE;
	else if (event==UI_E_ENTER) {
		((esp_err_t(*)()) arg)();
	}

	return UI_E_LEAVE;
}

static void input_show (menu_input_t * input)
{
/*
 * line 3-18: is the inputting string, x:0-200, 25 chracters, following "√×" sign
 * line 20: show the cursor
 * Y position for characters to be elected:
 * 30+y*20 y=0-4 :
 *
 * X position for characters to be elected:
 * 2+12*x x=0-19  223-230
 */
	ui_text(0, 3, UI_LCD_WIDTH-40, input->buff, color_menu, font_menu);
	if (input->cursor>=0)
		ST7789_rect(ui_lcd, input->cursor*8, 20, 8, 2, color_menuhl+2);
	ui_text(UI_LCD_WIDTH-32, 3, 16, "√", input->cursor<-1?color_menuhl:color_menu, font_menu);
	ui_text(UI_LCD_WIDTH-16, 3, 16, "×", input->cursor==-1?color_menuhl:color_menu, font_menu);

	int x, y, z;
	const char * p=input->chars;
	char c[2]={0, 0};
	for (y=30, z=0; y<UI_LCD_HEIGHT-16; y+=20)
		for (x=2; x<UI_LCD_WIDTH-8; x+=12, z++)
			if (!(c[0]=*p++))
				return;
			else if (z==input->focused) {
				ui_text(x, y, 8, c, color_menuhl, font_menu);
				ST7789_frame(ui_lcd, x-2, y-2, 12, 20, 1, color_menuhl+2);
			} else {
				ui_text(x, y, 8, c, color_menu, font_menu);
			}
}

static void input_focus (menu_input_t * input, int nf)
{
	int l=strlen(input->chars);
	nf=nf%l;
	if (nf<0) nf+=l;

	int x=(input->focused%20)*12+2;
	int y=(input->focused/20)*20+30;
	char c[2]={input->chars[input->focused], 0};
	ui_text(x, y, 8, c, color_menu, font_menu);
	ST7789_frame(ui_lcd, x-2, y-2, 12, 20, 1, color_bg);

	input->focused=nf;

	x=(input->focused%20)*12+2;
	y=(input->focused/20)*20+30;
	c[0]=input->chars[input->focused];
	ui_text(x, y, 8, c, color_menuhl, font_menu);
	ST7789_frame(ui_lcd, x-2, y-2, 12, 20, 1, color_menuhl+2);
}

static void input_cursor (menu_input_t * input, int nc)
{
	int l, n=((input->size>INPUT_SIZE+1)?(INPUT_SIZE-1):(input->size-2));
	for (l=n; l>=0 && (input->buff[l]==' ' || input->buff[l]==0); l--);
	l++;
	if (l>n) l=n;
	l+=3;
	nc=(nc+2)%l;
	if (nc<0) nc+=l;
	nc-=2;

	if (input->cursor>=0)
		ST7789_rect(ui_lcd, input->cursor*8, 20, 8, 2, color_bg);
	else if (input->cursor==-1)
		ui_text(UI_LCD_WIDTH-16, 3, 16, "×", color_menu, font_menu);
	else
		ui_text(UI_LCD_WIDTH-32, 3, 16, "√", color_menu, font_menu);

	input->cursor=nc;

	if (input->cursor>=0)
		ST7789_rect(ui_lcd, input->cursor*8, 20, 8, 1, color_menuhl+2);
	else if (input->cursor==-1)
		ui_text(UI_LCD_WIDTH-16, 3, 16, "×", color_menuhl, font_menu);
	else
		ui_text(UI_LCD_WIDTH-32, 3, 16, "√", color_menuhl, font_menu);
}

static void input_set(menu_input_t * input)
{
	if (input->cursor<0) return;

	char c[2]={input->chars[input->focused], 0};
	if (*c==0x08) { /* back space */
		if (input->cursor==0) return;
		*c=0x7f;
		ST7789_rect(ui_lcd, input->cursor*8, 20, 8, 2, color_bg);
		input->cursor--;
	}
	if (*c==0x7f) { /* del */
		int i;
		for (i=input->cursor; i<INPUT_SIZE; i++)
			input->buff[i]=input->buff[i+1];
		input->buff[INPUT_SIZE]=' ';
		ui_text(0, 3, UI_LCD_WIDTH-40, input->buff, color_menu, font_menu);
		ST7789_rect(ui_lcd, input->cursor*8, 20, 8, 2, color_menuhl+2);
	} else {
		input->buff[input->cursor]=*c;
		ui_text(input->cursor*8, 3, 8, c, color_menu, font_menu);
		input_cursor(input, input->cursor+1);
	}
}

int32_t menu_input_handler(void * arg, int32_t event, void *data)
{
	menu_input_t * input=(menu_input_t *)arg;
	char *p;

	switch(event) {
	case UI_E_CLOSE:
		return UI_E_CLOSE;
	case UI_E_LEAVE:
		return UI_E_RETURN;
	case UI_E_DONE:
	case UI_E_RETURN:
	case UI_E_ENTER:
		input->focused=0;
		input->cursor=-1;
		input->data=data;
		ui_clear();
		if (input->getter) {
			(*input->getter)(input->getter_arg, input->data, input->buff, ((input->size>INPUT_SIZE+1)?(INPUT_SIZE+1):input->size));
			/* change all '\0' to space ' ' */
			for (int i=strlen(input->buff); i<INPUT_SIZE; i++)
				input->buff[i]=' ';
			input->buff[INPUT_SIZE]=0;
		}
		else {
			memset(input->buff, ' ', INPUT_SIZE);
			input->buff[INPUT_SIZE]=0;
		}
		input_show(input);
		break;
	case UI_E_IROTL:
		input_cursor(input, input->cursor-1);
		break;
	case UI_E_IROTL_FAST:
		input_cursor(input, input->cursor-5);
		break;
	case UI_E_IROTR:
		input_cursor(input, input->cursor+1);
		break;
	case UI_E_IROTR_FAST:
		input_cursor(input, input->cursor+5);
		break;
	case UI_E_VROTL:
		input_focus(input, input->focused-1);
		break;
	case UI_E_VROTL_FAST:
		input_focus(input, input->focused-5);
		break;
	case UI_E_VROTR:
		input_focus(input, input->focused+1);
		break;
	case UI_E_VROTR_FAST:
		input_focus(input, input->focused+5);
		break;
	case UI_E_VCLICK:
		if (input->cursor>=0) input_set(input);
		break;
	case UI_E_ICLICK:
		if (input->cursor==-1)
			return UI_E_RETURN;
		else if (input->cursor<-1) {
			if (input->setter) {
				/* trim space */
				p=input->buff+((input->size>INPUT_SIZE+1)?INPUT_SIZE:(input->size-1));
				do { *(p--)=0; } while (p>input->buff && *p==' ');
				for (p=input->buff; *p==' '; p++);
				(*input->setter)(input->setter_arg, input->data, p);
			}
			return UI_E_RETURN;
		}
		break;
	}

	return UI_E_NONE;
}

void menu_input_getter_str (void * arg, void *data, char *buff, int size)
{
	strlcpy(buff, (const char *)arg, size);
}

void menu_input_setter_str (void * arg, void *data, const char *str)
{
	strcpy((char *)arg, str);
}

void menu_input_getter_ip (void * arg, void *data, char *buff, int size)
{
	snprintf(buff, size, IPSTR, IP2STR((esp_ip4_addr_t *)arg));
}

void menu_input_setter_ip (void * arg, void *data, const char *str)
{
	((esp_ip4_addr_t *)arg)->addr=esp_ip4addr_aton(str);
}

void menu_input_getter_iv (void * arg, void *data, char *buff, int size)
{
	snprintf(buff, size, "%-6.3lf", ((double)*(int *)arg)/1000);
}

void menu_input_setter_iv (void * arg, void *data, const char *str)
{
	double a;
	if (sscanf(str, "%lf", &a)==1) {
		*(int *)arg=(int)round(a*1000);
	}
}

static void prompt_show(void * arg)
{
	if (arg)
		ui_text_wrap(0, 0, UI_LCD_WIDTH, 100, arg, color_menuhint, font_menu);
	ui_text(100, 110, 100, BUNDLE.menu.confirm_ok, color_menuhl, font_menu);
}

int32_t menu_prompt_handler(void * arg, int32_t event, void *data)
{
	if (data) arg=data;

	switch(event) {
	case UI_E_CLOSE:
		return UI_E_CLOSE;
	case UI_E_LEAVE:
		return UI_E_RETURN;
	case UI_E_ENTER:
	case UI_E_RETURN:
		ui_clear();
		prompt_show(arg);
		break;
	case UI_E_DONE:
		break;
	case UI_E_ICLICK:
	case UI_E_VCLICK:
		return UI_E_RETURN;
	}

	return UI_E_NONE;
}

