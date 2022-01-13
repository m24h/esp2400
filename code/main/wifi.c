/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "esp_console.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
#include "ping/ping_sock.h"

#include "app.h"
#include "repl.h"
#include "wifi.h"

const static char * const TAG="WIFI";

int  wifi_state_sta=0;
int  wifi_state_ap=0;

esp_netif_t * wifi_intf_sta=NULL;
esp_netif_t * wifi_intf_ap=NULL;

static const char* state_str[5]={"off", "starting", "connecting", "connected", "on"};

/* for maybe unregister in future */
static esp_event_handler_instance_t ehi_wifi=NULL;
static esp_event_handler_instance_t ehi_ip=NULL;

static const wifi_auth_mode_t auth_ap_from_conf [4] = {WIFI_AUTH_OPEN, WIFI_AUTH_WPA_PSK, WIFI_AUTH_WPA2_PSK, WIFI_AUTH_WPA_WPA2_PSK};
static const wifi_auth_mode_t auth_sta_from_conf [5] = {WIFI_AUTH_OPEN, WIFI_AUTH_WEP, WIFI_AUTH_WPA_PSK, WIFI_AUTH_WPA2_PSK, WIFI_AUTH_WPA_WPA2_PSK};

static esp_ping_handle_t hdl_ping=NULL;

static void on_wifi(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	esp_err_t ret2;
	wifi_event_ap_staconnected_t* event_conn;
	wifi_event_ap_stadisconnected_t * event_disconn;
	esp_netif_dhcp_status_t dhcpst;
	wifi_mode_t wifimode;

	switch(id) {
	case WIFI_EVENT_STA_START:
		wifi_state_sta=1;
		if (esp_wifi_get_mode(&wifimode)!=ESP_OK
		 || wifimode!=WIFI_MODE_APSTA
		 || wifi_state_ap>0
	    ) { /* else need to wait untill ap is also started*/
			if ((ret2=esp_wifi_connect())==ESP_OK)
				wifi_state_sta=2;
			else
				ESP_LOGE(TAG, "Failed to start WIFI connecting (%d:%s)", ret2, esp_err_to_name(ret2));
	    }
		break;

	case WIFI_EVENT_STA_STOP:
		wifi_state_sta=0;

		break;

	case WIFI_EVENT_AP_START:
		wifi_state_ap=4;
		if (wifi_state_sta==1
		 && esp_wifi_get_mode(&wifimode)==ESP_OK
		 && wifimode==WIFI_MODE_APSTA
	    ) { /* else need to wait untill ap is also started*/
			if ((ret2=esp_wifi_connect())==ESP_OK)
				wifi_state_sta=2;
			else
				ESP_LOGE(TAG, "Failed to start WIFI connecting (%d:%s)", ret2, esp_err_to_name(ret2));
	    }

		break;

	case WIFI_EVENT_AP_STOP:
		wifi_state_ap=0;
		break;

	case WIFI_EVENT_STA_CONNECTED:
		wifi_state_sta=3;
    	if (conf_vars.sta.ip.addr) {
    		if ((ret2=esp_netif_dhcpc_get_status(wifi_intf_sta, &dhcpst))==ESP_OK) {
	    		if (dhcpst!=ESP_NETIF_DHCP_STOPPED)
	    			esp_netif_dhcpc_stop(wifi_intf_sta);
	    		esp_netif_ip_info_t info = {
	    			.ip = { .addr = conf_vars.sta.ip.addr },
	    			.netmask = { .addr = conf_vars.sta.mask.addr },
	    			.gw = { .addr = conf_vars.sta.gw.addr },
	    		};
	    		if ((ret2=esp_netif_set_ip_info(wifi_intf_sta, &info))==ESP_OK)
	    			wifi_state_sta=4;
	    		else
	    			ESP_LOGE(TAG, "Failed to set IP address (%d:%s)", ret2, esp_err_to_name(ret2));
	    	} else
	    		ESP_LOGW(TAG, "Failed to get DHCP-C status (%d:%s)", ret2, esp_err_to_name(ret2));
    	}
		break;

	case WIFI_EVENT_STA_DISCONNECTED:
		wifi_state_sta=1;
		if (conf_vars.sta.on && (xEventGroupWaitBits(app_egroup, APP_EG_READY, 0, 0, 10) & APP_EG_READY)) {
			ESP_LOGW(TAG, "WIFI disconnected, reconnecting");
			if ((ret2=esp_wifi_connect())==ESP_OK)
				wifi_state_sta=2;
			else
				ESP_LOGE(TAG, "Failed to start WIFI connecting (%d:%s)", ret2, esp_err_to_name(ret2));
		}
		break;

	case WIFI_EVENT_AP_STACONNECTED:
		event_conn = (wifi_event_ap_staconnected_t*) data;
		ESP_LOGI(TAG, "WIFI station " MACSTR " join, AID=%d", MAC2STR(event_conn->mac), event_conn->aid);
		break;

	case WIFI_EVENT_AP_STADISCONNECTED:
		event_disconn = (wifi_event_ap_stadisconnected_t*) data;
		ESP_LOGI(TAG, "WIFI station " MACSTR " leave, AID=%d", MAC2STR(event_disconn->mac), event_disconn->aid);
		break;
	}
}

static void on_ip(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	ip_event_got_ip_t * ipevt;
	switch(id) {
	case IP_EVENT_STA_GOT_IP:
		ipevt=(ip_event_got_ip_t *) data;
		if (ipevt->esp_netif==wifi_intf_sta) wifi_state_sta=4;
		else if (ipevt->esp_netif==wifi_intf_ap) wifi_state_ap=4;
		break;
	case IP_EVENT_AP_STAIPASSIGNED:
		break;
	}
}

esp_err_t wifi_reset()
{
	esp_err_t ret, ret2;
    esp_netif_dhcp_status_t dhcpst;

    /* stop WIFI first */
	esp_wifi_stop();
	esp_wifi_set_mode(WIFI_MODE_NULL);
	wifi_state_sta=wifi_state_ap=0;

    /* set mDNS/NETBIOS name */
	netbiosns_set_name(conf_vars.name);
	if ((ret2=mdns_hostname_set(conf_vars.name))!=ESP_OK
	 || (ret2=mdns_instance_name_set(conf_vars.name))!=ESP_OK
  	) {
		ESP_LOGW(TAG, "Failed to set mDNS name (%d:%s)", ret2, esp_err_to_name(ret2));
	}

	if (conf_vars.ap.on) {
		if (conf_vars.sta.on) {
			ret=esp_wifi_set_mode(WIFI_MODE_APSTA);
		} else {
			ret=esp_wifi_set_mode(WIFI_MODE_AP);
		}
	} else if (conf_vars.sta.on) {
		ret=esp_wifi_set_mode(WIFI_MODE_STA);
	} else {
		/* not to open WIFI */
		return ESP_OK;
	}
	ESP_RETURN_ON_FALSE(ret==ESP_OK
			,ret , TAG, "Failed to config WIFI mode (%d:%s)", ret, esp_err_to_name(ret));

	if (conf_vars.sta.on) {
	    wifi_config_t cfg = {
	        .sta = {
				.pmf_cfg = {
					.capable = true,
					.required = false
				},
	        },
	    };
	    strlcpy((char *)cfg.sta.ssid, conf_vars.sta.ssid, sizeof(cfg.sta.ssid));
	    strlcpy((char *)cfg.sta.password, conf_vars.sta.pass, sizeof(cfg.sta.password));
	    cfg.sta.threshold.authmode=auth_sta_from_conf[conf_vars.sta.auth<0?0:(conf_vars.sta.auth>4?4:conf_vars.sta.auth)];

	    ESP_RETURN_ON_ERROR(ret=esp_wifi_set_config(WIFI_IF_STA, &cfg)
				, TAG, "Failed to config WIFI Station (%d:%s)", ret, esp_err_to_name(ret));

	    if ((ret2=esp_netif_dhcpc_get_status(wifi_intf_sta, &dhcpst))==ESP_OK) {
	    	if (conf_vars.sta.ip.addr && dhcpst!=ESP_NETIF_DHCP_STOPPED)
	    		esp_netif_dhcpc_stop(wifi_intf_sta);
	    	else if (!conf_vars.sta.ip.addr && dhcpst==ESP_NETIF_DHCP_STOPPED)
	    		esp_netif_dhcpc_start(wifi_intf_sta);
	    } else
	    	ESP_LOGW(TAG, "Failed to get DHCP-C status (%d:%s)", ret2, esp_err_to_name(ret2));
	}

	if (conf_vars.ap.on) {
	    wifi_config_t cfg = {
	        .ap = {
	        	.max_connection = 4,
				.beacon_interval = 500,
				.channel=6
	        },
	    };
	    strlcpy((char *)cfg.ap.ssid, conf_vars.ap.ssid, sizeof(cfg.ap.ssid));
	    strlcpy((char *)cfg.ap.password, conf_vars.ap.pass, sizeof(cfg.ap.password));
	    cfg.ap.authmode=auth_ap_from_conf[conf_vars.ap.auth<0?0:(conf_vars.ap.auth>3?3:conf_vars.ap.auth)];

		ESP_RETURN_ON_ERROR(ret=esp_wifi_set_config(WIFI_IF_AP, &cfg)
				, TAG, "Failed to config WIFI AP (%d:%s)", ret, esp_err_to_name(ret));

	    if ((ret2=esp_netif_dhcps_get_status(wifi_intf_ap, &dhcpst))==ESP_OK) {
	    	if (conf_vars.ap.ip.addr) {
	    		if (dhcpst!=ESP_NETIF_DHCP_STOPPED)
	    			esp_netif_dhcps_stop(wifi_intf_ap);

	    		esp_netif_ip_info_t info = {
	    			.ip = { .addr = conf_vars.ap.ip.addr },
	    			.netmask = { .addr = conf_vars.ap.mask.addr },
	    		};
	    		if ((ret2=esp_netif_set_ip_info(wifi_intf_ap, &info))!=ESP_OK)
	    			ESP_LOGE(TAG, "Failed to set IP address (%d:%s)", ret2, esp_err_to_name(ret2));

	    		esp_netif_dhcps_start(wifi_intf_ap); /* start again to give DHCP service */

	    	} else if (dhcpst==ESP_NETIF_DHCP_STOPPED)
	    		esp_netif_dhcps_start(wifi_intf_ap);
	    } else
	    	ESP_LOGW(TAG, "Failed to get DHCP-S status (%d:%s)", ret2, esp_err_to_name(ret2));
	}

	ESP_RETURN_ON_ERROR(ret=esp_wifi_start()
			, TAG, "Failed to start WIFI (%d:%s)", ret, esp_err_to_name(ret));

	return ESP_OK;
}

static esp_err_t cmd_net(int argc, char **argv)
{
	esp_err_t ret;

	if (!wifi_intf_sta || !wifi_intf_ap) {
		repl_printf("Network is not initialized, retry few monments later\r\n");
		return ESP_OK;
	}

	if (argc>1 && !strcmp(argv[1], "reset")) {
		repl_printf("Network will reset now, see the result few monments later\r\n");
		return wifi_reset();
	}

	esp_netif_ip_info_t sta, ap;
	ESP_RETURN_ON_ERROR(ret=esp_netif_get_ip_info(wifi_intf_sta, &sta),
			TAG, "Failed to get infomation of WIFI station interface (%d:%s)", ret, esp_err_to_name(ret));
	ESP_RETURN_ON_ERROR(ret=esp_netif_get_ip_info(wifi_intf_ap, &ap),
			TAG, "Failed to get infomation of WIFI AP interface (%d:%s)", ret, esp_err_to_name(ret));

	repl_printf("WIFI station : %s, ip=" IPSTR ", netmask=" IPSTR ", gateway=" IPSTR "\r\n",
			state_str[wifi_state_sta], IP2STR(&sta.ip), IP2STR(&sta.netmask), IP2STR(&sta.gw));
	repl_printf("WIFI AP : %s, ip=" IPSTR ", netmask=" IPSTR "\r\n",
			state_str[wifi_state_ap], IP2STR(&ap.ip),	IP2STR(&ap.netmask));

	if (wifi_state_ap>=4) {
		wifi_sta_list_t sta_list;
		ESP_RETURN_ON_ERROR(ret=esp_wifi_ap_get_sta_list(&sta_list),
				TAG, "Failed to get list of WIFI stations connected (%d:%s)", ret, esp_err_to_name(ret));
		int i;
		repl_printf("Connected stations :");
		for (i=0; i<sta_list.num;i++) {
			repl_printf(" " MACSTR, MAC2STR(sta_list.sta[i].mac));
		}
		repl_printf("\r\n");
	}

	return ESP_OK;
}

static void on_ping_success(esp_ping_handle_t hdl, void *args)
{
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_err_t ret2;

    if ((ret2=esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno)))==ESP_OK
     && (ret2=esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl)))==ESP_OK
     && (ret2=esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr)))==ESP_OK
     && (ret2=esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len)))==ESP_OK
     && (ret2=esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time)))==ESP_OK)
		repl_printf("\r\nFrom " IPSTR " icmp_seq=%d bytes=%d ttl=%d time=%d ms",
				IP2STR(&target_addr), seqno, recv_len, ttl, elapsed_time);
	else
		ESP_LOGW(TAG, "Failed to get ping profile (%d:%s)", ret2, esp_err_to_name(ret2));
}

static void on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    uint16_t seqno;
    ip_addr_t target_addr;
    esp_err_t ret2;

    if ((ret2=esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno)))==ESP_OK
     && (ret2=esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr)))==ESP_OK)
    	repl_printf("\r\nFrom " IPSTR " icmp_seq=%d timeout", IP2STR(&target_addr), seqno);
    else
    	ESP_LOGW(TAG, "Failed to get ping profile (%d:%s)", ret2, esp_err_to_name(ret2));
}

static void on_ping_end(esp_ping_handle_t hdl, void *args)
{
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;

    esp_err_t ret2;

    if ((ret2=esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted)))==ESP_OK
     && (ret2=esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received)))==ESP_OK
     && (ret2=esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms)))==ESP_OK)
    	repl_printf("\r\n%d packets transmitted, %d received, time %dms\r\n", transmitted, received, total_time_ms);
    else
    	ESP_LOGW(TAG, "Failed to get ping profile (%d:%s)", ret2, esp_err_to_name(ret2));

    esp_ping_stop(hdl_ping);
    if ((ret2=esp_ping_delete_session(hdl_ping))==ESP_OK)
    	hdl_ping=NULL;
    else
    	ESP_LOGW(TAG, "Failed to delete ping session (%d:%s)", ret2, esp_err_to_name(ret2));

 	xEventGroupSetBits(app_egroup, APP_EG_PING);
}

static esp_err_t cmd_ping(int argc, char **argv)
{
	ip_addr_t ip;
	int count=3;

	if (hdl_ping!=NULL) {
		repl_printf("There is a PING session still working, need to wait its completion\r\n");
		return ESP_OK;
	}

	if (argc>1) {
		ip.addr=esp_ip4addr_aton(argv[1]);
		if (argc>2) {
			if (sscanf(argv[2], "%d", &count)!=1)
				count=3;
		}
	} else {
		esp_netif_ip_info_t info;
		if (esp_netif_get_ip_info(wifi_intf_sta, &info)==ESP_OK
			&& info.gw.addr)
			ip.addr=info.gw.addr;
		else {
			repl_printf("Can't determine IP destination\r\n");
			return ESP_OK;
		}
	}

	if ((xEventGroupWaitBits(app_egroup, APP_EG_PING, pdTRUE, pdFALSE, 3000/portTICK_PERIOD_MS) & APP_EG_PING) !=APP_EG_PING) {
		repl_printf("PING session is busy\r\n");
		return ESP_OK;
	}

    esp_ping_config_t cfg = ESP_PING_DEFAULT_CONFIG();
    cfg.timeout_ms = 2000;
    cfg.interval_ms = 1000;
    cfg.target_addr.addr = ip.addr;
    cfg.count = count;

    /* set callback functions */
    esp_ping_callbacks_t cbs={
    	.on_ping_success = on_ping_success,
		.on_ping_timeout = on_ping_timeout,
		.on_ping_end = on_ping_end,
    };

    esp_err_t ret2, ret3;
    if ((ret2=esp_ping_new_session(&cfg, &cbs, &hdl_ping))==ESP_OK) {
    	if ((ret2=esp_ping_start(hdl_ping))==ESP_OK) {
    	    repl_printf("Pinging " IPSTR " %d times, started ...\r\n", IP2STR(&ip), count);
    	} else {
         	ESP_LOGW(TAG, "Failed to start ping (%d:%s)", ret2, esp_err_to_name(ret2));
    	    if ((ret3=esp_ping_delete_session(hdl_ping))==ESP_OK)
    	    	hdl_ping=NULL;
    	    else
    	    	ESP_LOGW(TAG, "Failed to delete ping session (%d:%s)", ret3, esp_err_to_name(ret3));

    	    xEventGroupSetBits(app_egroup, APP_EG_PING);
    	}
    } else {
    	hdl_ping=NULL;
     	ESP_LOGW(TAG, "Failed to create ping session (%d:%s)", ret2, esp_err_to_name(ret2));
     	xEventGroupSetBits(app_egroup, APP_EG_PING);
    }

    /* wait untill PINGing completes */
    xEventGroupWaitBits(app_egroup, APP_EG_PING, pdFALSE, pdFALSE, 30000/portTICK_PERIOD_MS);
    return ret2;
}

static esp_err_t register_repl_cmd()
{
	static const esp_console_cmd_t cmd [] = {
		{
			.command = "net",
			.help = "Show network interfaces, if [reset] is appended, network will restart for new configuration",
			.hint = "[reset] ",
			.func = &cmd_net,
		},{
			.command = "ping",
			.help = "ping ip address, if [ip destination] is not set, WIFI station gateway is used",
			.hint = "[ip destination like xx.xx.xx.xx] [repeat times]",
			.func = &cmd_ping,
		},
	};

	int i;
	for (i=0; i<sizeof(cmd)/sizeof(esp_console_cmd_t); i++)
		esp_console_cmd_register(&cmd[i]);

	ESP_RETURN_ON_FALSE(xEventGroupSetBits(app_egroup, APP_EG_PING) & APP_EG_PING
    		, ESP_FAIL, TAG, "Failed to set PING bit on event group");

	return ESP_OK;
}

esp_err_t wifi_init()
{
	esp_err_t ret, ret2;

	wifi_state_sta=0;
	wifi_state_ap=0;

	ESP_RETURN_ON_FALSE((ret=esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi, NULL, &ehi_wifi))==ESP_OK
					 && (ret=esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip, NULL, &ehi_ip))==ESP_OK
			, ret, TAG, "Failed to register WIFI/IP event handler (%d:%s)", ret, esp_err_to_name(ret));

	ESP_RETURN_ON_ERROR(ret=esp_netif_init()
			, TAG, "Failed to init underlying TCP/IP stack (%d:%s)", ret, esp_err_to_name(ret));

	ESP_RETURN_ON_FALSE((wifi_intf_sta=esp_netif_create_default_wifi_sta())!=NULL
			         && (wifi_intf_ap=esp_netif_create_default_wifi_ap())!=NULL
			, ESP_FAIL, TAG, "Failed to create WIFI network interfaces (%p, %p)", wifi_intf_sta, wifi_intf_ap);

	ESP_RETURN_ON_FALSE(xEventGroupSetBits(app_egroup, APP_EG_NET_OK) & APP_EG_NET_OK
    		, ESP_FAIL, TAG, "Failed to set NET_OK bit on event group");
    ESP_RETURN_ON_ERROR(ret=esp_event_post_to(app_eloop, APP_E_BASE, APP_E_NET_OK, NULL, 0, portMAX_DELAY)
    		, TAG, "Failed to post NET_OK event (%d:%s)", ret, esp_err_to_name(ret));

	netbiosns_init();

	if ((ret2=mdns_init())!=ESP_OK
	 || (ret2=mdns_hostname_set(conf_vars.name))!=ESP_OK
	 || (ret2=mdns_instance_name_set(conf_vars.name))!=ESP_OK
     || (ret2=mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0))!=ESP_OK
	) {
		ESP_LOGW(TAG, "Failed to init mDNS service (%d:%s)", ret2, esp_err_to_name(ret2));
	}

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_RETURN_ON_FALSE((ret=esp_wifi_init(&cfg))==ESP_OK
			         && (ret=esp_wifi_set_storage(WIFI_STORAGE_RAM))==ESP_OK
			, ret, TAG, "Failed to init WIFI (%d:%s)", ret, esp_err_to_name(ret));

	return register_repl_cmd();
}

