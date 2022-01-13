/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "esp_console.h"

#include "app.h"
#include "main.h"
#include "repl.h"
#include "stat.h"

const static char * const TAG="STAT";

stat_data_t stat_data={};

int         stat_lock_readers=0;

static int64_t last_time=0;
static int64_t sum_v=0;
static int64_t sum_i=0;
static int64_t sum_p=0;
static int     sum_n=0;

static void enclose(stat_ring_t *ring)
{
	if (ring->last_avg_cnt<1) return;

	int end=(ring->offset+ring->num) % STAT_SIZE;
	int t=ring->last_avg_cnt>>1;
	ring->items[end].i.avg = (ring->items[end].i.avg + t) / ring->last_avg_cnt;
	ring->items[end].v.avg = (ring->items[end].v.avg + t) / ring->last_avg_cnt;
	ring->items[end].p.avg = (ring->items[end].p.avg + t) / ring->last_avg_cnt;

	if (ring->num<STAT_SIZE-1) ring->num++;
	else ring->offset=(ring->offset+1) % STAT_SIZE;

	ring->last_avg_cnt=0;
}

static void update_value(stat_ring_t *ring, int i, int v, int p)
{
	int end=(ring->offset+ring->num) % STAT_SIZE;

	if (ring->last_avg_cnt<1) {
		ring->items[end].i.avg = i;
		ring->items[end].i.max = i;
		ring->items[end].i.min = i;

		ring->items[end].v.avg = v;
		ring->items[end].v.max = v;
		ring->items[end].v.min = v;

		ring->items[end].p.avg = p;
		ring->items[end].p.max = p;
		ring->items[end].p.min = p;
	} else {
		ring->items[end].i.avg += i;
		if (ring->items[end].i.max < i) ring->items[end].i.max = i;
		if (ring->items[end].i.min > i) ring->items[end].i.min = i;

		ring->items[end].v.avg += v;
		if (ring->items[end].v.max < v) ring->items[end].v.max = v;
		if (ring->items[end].v.min > v) ring->items[end].v.min = v;

		ring->items[end].p.avg += p;
		if (ring->items[end].p.max < p) ring->items[end].p.max = p;
		if (ring->items[end].p.min > p) ring->items[end].p.min = p;
	}

	ring->last_avg_cnt++;
}

static void update_item(stat_ring_t *ring, stat_item_t *item)
{
	int end=(ring->offset+ring->num) % STAT_SIZE;

	if (ring->last_avg_cnt<1) {
		ring->items[end].i.avg = item->i.avg;
		ring->items[end].i.max = item->i.max;
		ring->items[end].i.min = item->i.min;

		ring->items[end].v.avg = item->v.avg;
		ring->items[end].v.max = item->v.max;
		ring->items[end].v.min = item->v.min;

		ring->items[end].p.avg = item->p.avg;
		ring->items[end].p.max = item->p.max;
		ring->items[end].p.min = item->p.min;
	} else {
		ring->items[end].i.avg += item->i.avg;
		if (ring->items[end].i.max<item->i.max) ring->items[end].i.max=item->i.max;
		if (ring->items[end].i.min>item->i.min) ring->items[end].i.min=item->i.min;

		ring->items[end].v.avg += item->v.avg;
		if (ring->items[end].v.max<item->v.max) ring->items[end].v.max=item->v.max;
		if (ring->items[end].v.min>item->v.min) ring->items[end].v.min=item->v.min;

		ring->items[end].p.avg += item->p.avg;
		if (ring->items[end].p.max<item->p.max) ring->items[end].p.max=item->p.max;
		if (ring->items[end].p.min>item->p.min) ring->items[end].p.min=item->p.min;
	}

	ring->last_avg_cnt++;
}

static esp_err_t cmd_statv(int argc, char **argv)
{
	stat_ring_t * ring=&stat_data.s;
	const char * s="seconds";
	if (argc>1) {
		if (!strcmp(argv[1],"m")) {
			ring=&stat_data.m;
			s="minutes";
		}
		else if (!strcmp(argv[1],"h")) {
			ring=&stat_data.h;
			s="hours";
		}
	}

	repl_printf("Voltage (mV) in last %d %s\r\n", ring->num, s);
	repl_printf("#\tAverage     \tMinimum     \tMaxium\r\n");

	int i;
	for (i=0;i<ring->num;i++)
		repl_printf("%d\t%-12d\t%-12d\t%-12d\r\n",i,
				ring->items[(i+ring->offset)%STAT_SIZE].v.avg,
				ring->items[(i+ring->offset)%STAT_SIZE].v.min,
				ring->items[(i+ring->offset)%STAT_SIZE].v.max);

    return ESP_OK;
}

static esp_err_t cmd_stati(int argc, char **argv)
{
	stat_ring_t * ring=&stat_data.s;
	const char * s="seconds";
	if (argc>1) {
		if (!strcmp(argv[1],"m")) {
			ring=&stat_data.m;
			s="minutes";
		}
		else if (!strcmp(argv[1],"h")) {
			ring=&stat_data.h;
			s="hours";
		}
	}

	repl_printf("Current (mA) in last %d %s\r\n", ring->num, s);
	repl_printf("#\tAverage     \tMinimum     \tMaxium\r\n");

	int i;
	for (i=0;i<ring->num;i++)
		repl_printf("%d\t%-12d\t%-12d\t%-12d\r\n",i,
				ring->items[(i+ring->offset)%STAT_SIZE].i.avg,
				ring->items[(i+ring->offset)%STAT_SIZE].i.min,
				ring->items[(i+ring->offset)%STAT_SIZE].i.max);

    return ESP_OK;
}

static esp_err_t cmd_statp(int argc, char **argv)
{
	stat_ring_t * ring=&stat_data.s;
	const char * s="seconds";
	if (argc>1) {
		if (!strcmp(argv[1],"m")) {
			ring=&stat_data.m;
			s="minutes";
		}
		else if (!strcmp(argv[1],"h")) {
			ring=&stat_data.h;
			s="hours";
		}
	}

	repl_printf("Power (mW) in last %d %s\r\n", ring->num, s);
	repl_printf("#\tAverage     \tMinimum     \tMaxium\r\n");

	int i;
	for (i=0;i<ring->num;i++)
		repl_printf("%d\t%-12d\t%-12d\t%-12d\r\n",i,
				ring->items[(i+ring->offset)%STAT_SIZE].p.avg,
				ring->items[(i+ring->offset)%STAT_SIZE].p.min,
				ring->items[(i+ring->offset)%STAT_SIZE].p.max);

    return ESP_OK;
}

static esp_err_t cmd_stat(int argc, char **argv)
{
	if (argc>1 && !strcmp(argv[1],"reset")) {
		stat_reset_ts();
	}

	repl_printf("            \tAverage     \tMinimum     \tMaxium\r\n");
	repl_printf("Voltage (mV)\t%-12d\t%-12d\t%-12d\r\n", stat_data.t.v.avg, stat_data.t.v.min, stat_data.t.v.max);
	repl_printf("Current (mA)\t%-12d\t%-12d\t%-12d\r\n", stat_data.t.i.avg, stat_data.t.i.min, stat_data.t.i.max);
	repl_printf("Power   (mW)\t%-12d\t%-12d\t%-12d\r\n", stat_data.t.p.avg, stat_data.t.p.min, stat_data.t.p.max);

	return ESP_OK;
}

static esp_err_t register_repl_cmd()
{
	static const esp_console_cmd_t cmd [] = {
		{
			.command = "stat",
			.help = "Show gathered whole-time statistics data, from last boot or last reset",
			.hint = "[reset: to reset whole-time statistics data]",
			.func = &cmd_stat,
		}, {
			.command = "statv",
			.help = "Show gathered statistics of voltage",
			.hint = "[s:last seconds, m:last seconds, h:last hours]",
			.func = &cmd_statv,
		}, {
	        .command = "stati",
			.help = "Show gathered statistics of current",
			.hint = "[s:last seconds, m:last seconds, h:last hours]",
	        .func = &cmd_stati,
	    }, {
	        .command = "statp",
			.help = "Show gathered statistics of power",
			.hint = "[s:last seconds, m:last seconds, h:last hours]",
	        .func = &cmd_statp,
	    },
	};

	int i;
	for (i=0; i<sizeof(cmd)/sizeof(esp_console_cmd_t); i++)
		esp_console_cmd_register(&cmd[i]);

	return ESP_OK;
}

esp_err_t stat_init()
{
	esp_err_t ret;

	ESP_RETURN_ON_ERROR(ret=rwlock_init(app_egroup, APP_EG_STAT_W, APP_EG_STAT_R, &stat_lock_readers)
			, TAG, "Failed to init stat r/w lock (%d:%s)", ret, esp_err_to_name(ret));

	last_time=0;
	memset(&stat_data, 0, sizeof(stat_data_t));
	stat_data.t.v.max=INT_MIN;
	stat_data.t.v.min=INT_MIN;
	stat_data.t.v.avg=INT_MIN;
	stat_data.t.i.max=INT_MIN;
	stat_data.t.i.min=INT_MIN;
	stat_data.t.i.avg=INT_MIN;
	stat_data.t.p.max=INT_MIN;
	stat_data.t.p.min=INT_MIN;
	stat_data.t.p.avg=INT_MIN;

	return register_repl_cmd();
}

void stat_stat ()
{
	int64_t now=esp_timer_get_time();

	int ic=main_vars.ic;
	int vc=main_vars.vc;
	int64_t t64=(int64_t)ic*vc;
	if (t64<0) t64=-t64;

	int pc=(int)((t64+500)/1000);
	main_vars.p=pc;
	if (last_time>0) /* means last_time is real */
		main_vars.e += (t64*(now-last_time)+500000)/1000000;

	rwlock_lock_w(app_egroup, APP_EG_STAT_W, portMAX_DELAY);

	if (stat_data.t.v.max==INT_MIN || stat_data.t.v.max<vc) stat_data.t.v.max=vc;
	if (stat_data.t.v.min==INT_MIN || stat_data.t.v.min>vc) stat_data.t.v.min=vc;
	if (stat_data.t.i.max==INT_MIN || stat_data.t.i.max<ic) stat_data.t.i.max=ic;
	if (stat_data.t.i.min==INT_MIN || stat_data.t.i.min>ic) stat_data.t.i.min=ic;
	if (stat_data.t.p.max==INT_MIN || stat_data.t.p.max<pc) stat_data.t.p.max=pc;
	if (stat_data.t.p.min==INT_MIN || stat_data.t.p.min>pc) stat_data.t.p.min=pc;
	sum_v+=vc;
	sum_i+=ic;
	sum_p+=pc;
	sum_n++;
	stat_data.t.v.avg=(int)((sum_v+sum_n/2)/sum_n);
	stat_data.t.i.avg=(int)((sum_i+sum_n/2)/sum_n);
	stat_data.t.p.avg=(int)((sum_p+sum_n/2)/sum_n);
	if (sum_n>1073741821) { /* if too many count */
		sum_v>>=1;
		sum_i>>=1;
		sum_p>>=1;
		sum_n>>=1;
	}

	if (last_time>0 && now/1000000!=last_time/1000000) { /* different second */
		stat_data.s.last_enclose=last_time;
		enclose(&stat_data.s);
		if (stat_data.s.num>0)
			update_item(&stat_data.m, stat_data.s.items+(stat_data.s.offset+stat_data.s.num-1)%STAT_SIZE);

		if (now/60000000!=last_time/60000000) { /* diffrent minute */
			stat_data.m.last_enclose=last_time;
			enclose(&stat_data.m);
			if (stat_data.m.num>0)
				update_item(&stat_data.h, stat_data.m.items+(stat_data.m.offset+stat_data.m.num-1)%STAT_SIZE);

			if (now/3600000000!=last_time/3600000000) { /* diffrent hour */
				stat_data.h.last_enclose=last_time;
				enclose(&stat_data.h);
			}
		}
	}

	update_value(&stat_data.s, ic, vc, main_vars.p);

	rwlock_unlock_w(app_egroup, APP_EG_STAT_W);

	last_time=now;
}

esp_err_t stat_reset_ts ()
{
	int ic=main_vars.ic;
	int vc=main_vars.vc;
	int64_t t64=(int64_t)ic*vc;
	if (t64<0) t64=-t64;
	int pc = (int)((t64+500)/1000);

	rwlock_lock_w(app_egroup, APP_EG_STAT_W, portMAX_DELAY);

	memset(&stat_data, 0, sizeof(stat_data_t));
	last_time=0;

	stat_data.t.v.max=vc;
	stat_data.t.v.min=vc;
	stat_data.t.v.avg=vc;
	stat_data.t.i.max=ic;
	stat_data.t.i.min=ic;
	stat_data.t.i.avg=ic;
	stat_data.t.p.max=pc;
	stat_data.t.p.min=pc;
	stat_data.t.p.avg=pc;
	sum_v=vc;
	sum_i=ic;
	sum_p=pc;
	sum_n=1;

	rwlock_unlock_w(app_egroup, APP_EG_STAT_W);

	return ESP_OK;
}
