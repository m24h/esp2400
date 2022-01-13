/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "app.h"
#include "main.h"
#include "stat.h"
#include "web.h"

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

#define CONTENT_TYPE_CHARSET "; charset=utf-8"

const static char * const TAG="WEB";

httpd_handle_t web_server=NULL;

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "application/octet-stream" CONTENT_TYPE_CHARSET;
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".htm")) {
        type = "text/html" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".xml")) {
        type = "application/xml" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".txt")) {
        type = "text/plain" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".json")) {
        type = "application/json" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".class")) {
        type = "application/octet-stream" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".gif")) {
        type = "image/gif" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".bmp")) {
        type = "image/bmp" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".jpg")) {
        type = "image/jpg" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".mp4")) {
        type = "video/mp4" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".pdf")) {
        type = "application/pdf" CONTENT_TYPE_CHARSET;
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "image/svg+xml" CONTENT_TYPE_CHARSET;
    }

    return httpd_resp_set_type(req, type);
}

static esp_err_t handler_common_get(httpd_req_t *req)
{
	if (!(xEventGroupGetBits(app_egroup) & APP_EG_READY)) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "System not ready");
		return ESP_FAIL;
	}

    char path[FS_PATH_SIZE];
    char buff[512];

    strlcpy(buff, req->uri, sizeof(buff));
    char * uriend=strchr(buff, '?');
    if (uriend) *uriend=0;

    strlcpy(path, FS_DIR_WEB, sizeof(path));
    int urilen=strlen(buff);
    if (urilen==0)
    	strlcat(path, "/" HTTPD_DEF_FILE, sizeof(path));
    else {
    	if (req->uri[0]!='/')
    		strlcat(path, "/", sizeof(path));
    	strlcat(path, buff, sizeof(path));
    	if (path[strlen(path)-1]=='/')
        	strlcat(path, HTTPD_DEF_FILE, sizeof(path));
    }

    int fd = open(path, O_RDONLY, 0);
    if (fd == -1) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_OK;
    }

    set_content_type_from_file(req, path);

    ssize_t n;
    do {
        n = read(fd, buff, sizeof(buff));
        if (n == -1) {
            ESP_LOGW(TAG, "Failed to read file : %s", path);
        } else if (n > 0) {
            if (httpd_resp_send_chunk(req, buff, n) != ESP_OK) {
                close(fd);
                httpd_resp_send_chunk(req, NULL, 0);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (n > 0);

    close(fd);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static void uri_decode(char *s)
{
	char c;
	int i;
	char *q=s;
	while ((c=*(s++))) {
		if (c=='+') c=' ';
		else if (c=='%' && *s && *(s+1)) {
			if (sscanf(s, "%2x", &i)==1) {
				c=i;
				s+=2;
			}
		}
		*(q++)=c;
	}
	*q=0;
}

static esp_err_t handler_data_get(httpd_req_t *req)
{
	if (!(xEventGroupGetBits(app_egroup) & APP_EG_READY)) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "System not ready");
		return ESP_FAIL;
	}

	char  buff[512];
	char  vstr[64];
	int   i, n, off, value;
	int64_t v64;

	int uri_off=(int)req->user_ctx;
    strlcpy(vstr, req->uri+uri_off, sizeof(vstr));
    char * uriend=strchr(vstr, '?');
    if (uriend) *uriend=0;

	if (httpd_req_get_url_query_str(req, buff, sizeof(buff))!=ESP_OK)
		buff[0]=0; /* maybe no query string */

	httpd_resp_set_type(req, "application/json" CONTENT_TYPE_CHARSET);
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); /* allowing cross-domain AJAX */

	if (!strcmp(vstr, "get")) {
		if (conf_vars.pass[0]
         && (httpd_query_key_value(buff, "pass", vstr, sizeof(vstr))!=ESP_OK ||
		     (strcmp(conf_vars.pass, vstr) && strcmp(conf_vars.admpass, vstr)))
		) {
			httpd_resp_sendstr(req, "{\"ret\":\"bad password\"}");
			return ESP_OK;
		}

		sprintf(buff,"{\"ret\":\"ok\",\"on\":%d,\"vc\":%d,\"ic\":%d,\"vs\":%d,\"is\":%d,"
				     "\"p\":%d,\"e\":%lld,\"temp\":%d,\"vmax\":%d,\"vmin\":%d,\"imax\":%d,\"imin\":%d}",
					 main_vars.on,main_vars.vc, main_vars.ic, main_vars.vs, main_vars.is,
					 main_vars.p, (main_vars.e+500000)/1000000, (main_vars.temp+500)/1000,
					 V_MAX, V_MIN, I_MAX, I_MIN);
		httpd_resp_sendstr(req, buff);
		return ESP_OK;
	}

	if (!strcmp(vstr, "set")) {
		if (conf_vars.admpass[0]
		 && (httpd_query_key_value(buff, "pass", vstr, sizeof(vstr))!=ESP_OK ||
			 strcmp(conf_vars.admpass, vstr))) {
			httpd_resp_sendstr(req, "{\"ret\":\"bad password\"}");
			return ESP_OK;
		}
		if (httpd_query_key_value(buff, "on", vstr, sizeof(vstr))==ESP_OK) {
			if (sscanf(vstr,"%d",&value)==1) {
				main_vars.on=value?1:0;
			}
		}
		if (httpd_query_key_value(buff, "vs", vstr, sizeof(vstr))==ESP_OK) {
			if (sscanf(vstr,"%d",&value)==1) {
				if (value>V_MAX) value=V_MAX;
				else if (value<V_MIN) value=V_MIN;
				main_vars.vs=value;
			}
		}
		if (httpd_query_key_value(buff, "is", vstr, sizeof(vstr))==ESP_OK) {
			if (sscanf(vstr,"%d",&value)==1) {
				if (value>I_MAX) value=I_MAX;
				else if (value<I_MIN) value=I_MIN;
				main_vars.is=value;
			}
		}
		if (httpd_query_key_value(buff, "e", vstr, sizeof(vstr))==ESP_OK) {
			if (sscanf(vstr,"%lld",&v64)==1) {
				if (v64<0) v64=0;
				main_vars.e=v64*1000000; /*J to uJ */
			}
		}
		if (httpd_query_key_value(buff, "msg", vstr, sizeof(vstr))==ESP_OK) {
			uri_decode(vstr);
			if (main_message(vstr)!=ESP_OK) {
				httpd_resp_sendstr(req, "{\"ret\":\"Failed to set message\"}");
				return ESP_OK;
			}
		}

		sprintf(buff,"{\"ret\":\"ok\",\"on\":%d,\"vc\":%d,\"ic\":%d,\"vs\":%d,\"is\":%d,"
				     "\"p\":%d,\"e\":%lld,\"temp\":%d,\"vmax\":%d,\"vmin\":%d,\"imax\":%d,\"imin\":%d}",
					main_vars.on, main_vars.vc, main_vars.ic, main_vars.vs, main_vars.is,
					main_vars.p, (main_vars.e+500000)/1000000, (main_vars.temp+500)/1000,
					V_MAX, V_MIN, I_MAX, I_MIN);
		httpd_resp_sendstr(req, buff);
		return ESP_OK;
	}

	if (!strcmp(vstr, "stat")) {
		if (conf_vars.pass[0]
         && (httpd_query_key_value(buff, "pass", vstr, sizeof(vstr))!=ESP_OK ||
		     (strcmp(conf_vars.pass, vstr) && strcmp(conf_vars.admpass, vstr)))
         ) {
			httpd_resp_sendstr(req, "{\"ret\":\"bad password\"}");
			return ESP_OK;
		}

		stat_ring_t *ringsel=&stat_data.s;
		if (httpd_query_key_value(buff, "period", vstr, sizeof(vstr))==ESP_OK) {
			if (!strcmp("m", vstr))
				ringsel=&stat_data.m;
			else if (!strcmp("h", vstr))
				ringsel=&stat_data.h;
		}

		stat_ring_t *ring=malloc(sizeof(stat_ring_t));
		if (!ring) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No memory to copy statistic data");
            return ESP_FAIL;
		}

		STAT_LOCK_R();
		memcpy(ring, ringsel, sizeof(stat_ring_t));
		STAT_UNLOCK_R();

		sprintf(buff,"{\"ret\":\"ok\",\"on\":%d,\"vc\":%d,\"ic\":%d,\"vs\":%d,\"is\":%d,"
				     "\"p\":%d,\"e\":%lld,\"temp\":%d,\"vmax\":%d,\"vmin\":%d,\"imax\":%d,\"imin\":%d,"
				     "\"stat\":[",
					main_vars.on, main_vars.vc, main_vars.ic, main_vars.vs, main_vars.is,
					main_vars.p, (main_vars.e+500000)/1000000, (main_vars.temp+500)/1000,
					V_MAX, V_MIN, I_MAX, I_MIN);
		if (httpd_resp_send_chunk(req, buff, strlen(buff))!=ESP_OK) {
			free(ring);
            httpd_resp_send_chunk(req, NULL, 0);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send data");
            return ESP_FAIL;
		}

		off=ring->offset;
		n=ring->num;
		if (n>120) n=120;
		stat_item_t *item;
		buff[0]=0;
		int len;
		for (i=0;i<n;i++) {
			item = &(ring->items[(i+off)%STAT_SIZE]);
			if (i>0) strlcat(buff, ",", sizeof(buff));
			len=strlen(buff);
			snprintf(buff+len, sizeof(buff)-len, "{\"v\":[%d,%d,%d],\"i\":[%d,%d,%d],\"p\":[%d,%d,%d]}",
					item->v.min, item->v.avg, item->v.max,
					item->i.min, item->i.avg, item->i.max,
					item->p.min, item->p.avg, item->p.max);
			if (i%4==3) {
				if (httpd_resp_send_chunk(req, buff, strlen(buff))!=ESP_OK) {
					free(ring);
		            httpd_resp_send_chunk(req, NULL, 0);
		            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send data");
		            return ESP_FAIL;
				}
				buff[0]=0;
			}
		}
		strlcat(buff, "]}", sizeof(buff));
		if (httpd_resp_send_chunk(req, buff, strlen(buff))!=ESP_OK) {
			free(ring);
            httpd_resp_send_chunk(req, NULL, 0);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send data");
            return ESP_FAIL;
		}
		free(ring);
		httpd_resp_send_chunk(req, NULL, 0);
		return ESP_OK;
	}

	httpd_resp_sendstr(req, "{\"ret\":\"unknown request\"}");
	return ESP_OK;
}

esp_err_t web_init()
{
	esp_err_t ret;

	httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
	cfg.server_port=HTTPD_PORT;
	cfg.uri_match_fn = httpd_uri_match_wildcard;
	cfg.lru_purge_enable=true;
	cfg.max_open_sockets=CONFIG_LWIP_MAX_SOCKETS-4;
	cfg.core_id=HTTPD_CORE;
	cfg.stack_size=HTTPD_STACK;
	cfg.max_uri_handlers=HTTPD_HANDLERS;
    cfg.max_resp_headers=HTTPD_HANDLERS;
	ESP_RETURN_ON_ERROR(ret=httpd_start(&web_server, &cfg)
			, TAG, "Failed to start HTTPD server (%d:%s)", ret, esp_err_to_name(ret));

	ESP_RETURN_ON_FALSE(xEventGroupSetBits(app_egroup, APP_EG_WEB_OK) & APP_EG_WEB_OK
    		, ESP_FAIL, TAG, "Failed to set WEB_OK bit on event group");
    ESP_RETURN_ON_ERROR(ret=esp_event_post_to(app_eloop, APP_E_BASE, APP_E_WEB_OK, NULL, 0, portMAX_DELAY)
    		, TAG, "Failed to post WEB_OK event (%d:%s)", ret, esp_err_to_name(ret));

    httpd_uri_t data_get_uri = {
		.uri = "/data/*",
		.method = HTTP_GET,
		.handler = handler_data_get,
		.user_ctx = (void*)6 /* offset URI to get rid of prefix of URI like "/data/" */
	};
    ESP_RETURN_ON_ERROR(ret=httpd_register_uri_handler(web_server, &data_get_uri)
    		, TAG, "Failed to register web data handler (%d:%s)", ret, esp_err_to_name(ret));

    /* other handlers should be add before this, cause this match every thing */
    httpd_uri_t common_get_uri = {
		.uri = "*",
		.method = HTTP_GET,
		.handler = handler_common_get,
		.user_ctx = NULL
	};
    ESP_RETURN_ON_ERROR(ret=httpd_register_uri_handler(web_server, &common_get_uri)
    		, TAG, "Failed to register web file handler (%d:%s)", ret, esp_err_to_name(ret));

    return ESP_OK;
}
