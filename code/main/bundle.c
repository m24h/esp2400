/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */

#include "def.h"
#include "bundle.h"

/** THIS FILE MUST BE UTF-8 ENCODED **/

const char * bundle_langs[BUNDLE_SIZE]={"中文", "English"};

static const bundle_t data[BUNDLE_SIZE]={
	{
		.msg={
			.ready="系统就绪",

		},
		.menu={
			.back="-返回-",
			.back2="返回",
			.option="选项",
			.option_lang="语言",
			.option_fan_temp="控温等级",
			.option_v_rise="电压升速限制",
			.option_v_fall="电压降速限制",
			.option_i_rise="电流升速限制",
			.option_i_fall="电流降速限制",
			.option_fade_nolimit="不限制",
			.conf="配置",
			.conf_save="保存配置",
			.conf_save_hint="保存当前配置, 不然重启会失效, 请注意配置修改不会自动保存",
			.conf_load="载入配置",
			.conf_load_hint="载入已保存的配置",
			.conf_default="恢复原始配置",
			.conf_default_hint="恢复设计时的原始配置(但并不自动保存), 现有的设置将会失效",
			.conf_format="完全清除",
			.conf_format_hint="完全清除所有设置和任何存储, 重启后设备将如同原始",
			.conf_reset="重新启动",
			.conf_reset_hint="重启设备, 设备重启后将使用已经保存的配置",
			.quick="预设",
			.quick_v="电压",
			.quick_i="电流",
			.cal="校正",
			.cal_err_noroom="操作失败: 空闲的校正点已经用完, 请删除至少一个校正点才能增加新的",
			.cal_err_2points="操作失败: 至少要有两个校正点, 系统才能正常工作",
			.cal_err_monoinc="操作失败: 不能保持校正数据的单调递增性, 请删除可能冲突的校正点后再试",
			.cal_addv="增加电压点",
			.cal_addv_hint="调整电压至大概要矫正的位置, 在输出稳定后, 输入测量到的准确电压",
			.cal_delv="查看和删除电压点",
			.cal_delv_hint="是否确定删除这个电压校正点",
			.cal_addi="增加电流点",
			.cal_addi_hint="调整电流至大概要矫正的位置, 在输出稳定后, 输入测量到的准确电流",
			.cal_deli="查看和删除电流点",
			.cal_deli_hint="是否确定删除这个电流校正点",
			.wifi="网络",
			.wifi_status="状态",
			.wifi_status_prompt="接入:%d.%d.%d.%d\n掩码:%d.%d.%d.%d\n网关:%d.%d.%d.%d\n热点:%d.%d.%d.%d\n掩码:%d.%d.%d.%d",
			.wifi_status_err="无法获取网络状态 (%d)",
			.wifi_name="主机名",
			.wifi_sta="接入",
			.wifi_sta_on="是否启用",
			.wifi_sta_ssid="网络标识",
			.wifi_sta_auth="认证方式",
			.wifi_sta_pass="口令",
			.wifi_sta_ip="地址",
			.wifi_sta_mask="掩码",
			.wifi_sta_gw="网关",
			.wifi_ap="热点",
			.wifi_ap_on="是否启用",
			.wifi_ap_ssid="网络标识",
			.wifi_ap_auth="认证方式",
			.wifi_ap_pass="口令",
			.wifi_ap_ip="地址",
			.wifi_ap_mask="掩码",
			.wifi_pass="读取口令",
			.wifi_admpass="设置口令",
			.wifi_reset="重启网络",
			.wifi_reset_hint="重启网络以符合当前配置 (并不意味着保存配置)",
			.stat="统计",
			.stat_v="电压",
			.stat_i="电流",
			.stat_p="功率",
			.stat_e="能量",
			.stat_t="总体",
			.stat_s="秒数",
			.stat_m="分钟",
			.stat_h="小时",
			.stat_cur="当前",
			.stat_max="最大",
			.stat_min="最小",
			.stat_avg="平均",
			.stat_exit="返回",
			.stat_clickback="按键返回",
			.stat_zero="清零",
			.stat_zero_hint="将清除统计信息, 包括积累的最大最小和平均值, 以及能量值",
			.confirm_ok="确定",
			.confirm_cancel="取消",
			.onoff_on="启用",
			.onoff_off="关闭",
		},
	}, {
		.msg={
			.ready="System ready",
		},
		.menu={
			.back="-BACK-",
			.back2="BACK",
			.option="Options",
			.option_lang="Language",
			.option_fan_temp="Temperature control",
			.option_v_rise="Voltage rising speed",
			.option_v_fall="Voltage falling speed",
			.option_i_rise="Current rising speed",
			.option_i_fall="Current falling speed",
			.option_fade_nolimit="No limit",
			.conf="Configuration",
			.conf_save="Save",
			.conf_save_hint="Save configuration, otherwise it will be discarded after reboot",
			.conf_load="Load",
			.conf_load_hint="Load stored configuration",
			.conf_default="Restore to Default",
			.conf_default_hint="Restore default configuration (but not to save automatically), all configuration in stock will be discarded",
			.conf_format="Full Clean",
			.conf_format_hint="Clean all configuration and storage, device will become whole new after reboot",
			.conf_reset="Reset",
			.conf_reset_hint="Reset device for a new start",
			.quick="Presets",
			.quick_v="Voltage",
			.quick_i="Current",
			.cal="Calibrating",
			.cal_err_noroom="Failed: no room for new calibration points, before retrying, delete some old points",
			.cal_err_2points="Failed: at least 2 calibration points is needed",
			.cal_err_monoinc="Failed: calibration data is not monotone increasing, try again after deleting some conflicting points",
			.cal_addv="Add Voltage",
			.cal_addv_hint="Adjust output to approximate voltage, after output becomes stable, input the mesured voltage value",
			.cal_delv="Show | Delete Voltage",
			.cal_delv_hint="Really want to delete this voltage calibration point?",
			.cal_addi="Add Current",
			.cal_addi_hint="Adjust output to approximate current, after output becomes stable, input the mesured current value",
			.cal_deli="Show | Delete Current",
			.cal_deli_hint="Really want to delete this current calibration point?",
			.wifi="Network",
			.wifi_status="Status",
			.wifi_status_prompt=" STA:%d.%d.%d.%d\nMASK:%d.%d.%d.%d\nGWAY:%d.%d.%d.%d\n  AP:%d.%d.%d.%d\nMASK:%d.%d.%d.%d",
			.wifi_status_err="Unable to retrieve WIFI status (%d)",
			.wifi_name="Host Name",
			.wifi_sta="Station",
			.wifi_sta_on="On/Off",
			.wifi_sta_ssid="SSID",
			.wifi_sta_auth="Authentication",
			.wifi_sta_pass="Password",
			.wifi_sta_ip="IP",
			.wifi_sta_mask="Netmask",
			.wifi_sta_gw="Gateway",
			.wifi_ap="AP",
			.wifi_ap_on="On/Off",
			.wifi_ap_ssid="SSID",
			.wifi_ap_auth="Authentication",
			.wifi_ap_pass="Password",
			.wifi_ap_ip="IP",
			.wifi_ap_mask="Netmask",
			.wifi_pass="Password for Getting",
			.wifi_admpass="Password for Setting",
			.wifi_reset="Restart Network",
			.wifi_reset_hint="Restart network to make present configuration take effect (this does not mean saving configuration)",
			.stat="Statistics",
			.stat_v="Voltage",
			.stat_i="Current",
			.stat_p="Power",
			.stat_e="Energy",
			.stat_t="Total",
			.stat_s="Second",
			.stat_m="Minute",
			.stat_h="Hour",
			.stat_cur="Now",
			.stat_max="Max",
			.stat_min="Min",
			.stat_avg="Avg",
			.stat_clickback="Click to Exit",
			.stat_exit="Exit",
			.stat_zero="Zero",
			.stat_zero_hint="This will clear the statistics data, including accumulated Energy value",
			.confirm_ok="OK",
			.confirm_cancel="CANCEL",
			.onoff_on="On",
			.onoff_off="Off",
		},
	},
};

const bundle_t * const bundle_data = data;