/*
 * Hisilicon Printk.
 *
 * Copyright (c) 2016 HUAWEI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>

#ifdef CONFIG_HUAWEI_PRINTK_CTRL
#include <log/log_usertype/log-usertype.h>
#include <linux/hisi/hw_cmdline_parse.h> /*for runmode_is_factory*/
#endif

static unsigned int scbbpdrxstat1;
static unsigned int scbbpdrxstat2;
static unsigned int fpga_flag;
static void __iomem	*addr;
static int init_time_addr;

u64 hisi_getcurtime(void)
{
	u64 timervalue[4] = {0};
	u64 pcurtime = 0;
	u64 ts;

	if (!init_time_addr)
		return 0;
	timervalue[0] = readl(addr + scbbpdrxstat1);
	timervalue[1] = readl(addr + scbbpdrxstat2);
	timervalue[2] = readl(addr + scbbpdrxstat1);
	timervalue[3] = readl(addr + scbbpdrxstat2);

	if (timervalue[2] < timervalue[0])
		pcurtime = (((timervalue[3] - 1) << 32) | timervalue[0]);
	else
		pcurtime = ((timervalue[1] << 32) | timervalue[0]);

	if (fpga_flag == 1) {
		ts = do_div(pcurtime, 19200000);
		ts = ts * 625;
		do_div(ts, 12);
		pcurtime = pcurtime * 1000000000 + ts;
	} else {
		ts = do_div(pcurtime, 32764);
		ts = ts * 1953125;
		do_div(ts, 64);
		pcurtime = pcurtime * 1000000000 + ts;
	}

	return pcurtime;
}
size_t print_time(u64 ts, char *buf)
{
	int temp = 0;
	unsigned long rem_nsec;

	rem_nsec = do_div(ts, 1000000000);

	if (!printk_time)
		return 0;
	if (!buf) {
		return (unsigned int)snprintf(NULL, 0, "[%5lu.000000s]",
				(unsigned long)ts
				);/* [false alarm]:buffer will not overflow  */
	}

	// cppcheck-suppress *
	/*lint -save -e421 */
	temp = sprintf(buf, "[%5lu.%06lus]",
			(unsigned long)ts, rem_nsec/1000
			);/* [false alarm]:buffer will not overflow  */
	/*lint -restore */
	if (temp >= 0) {
		return (unsigned int)temp;
	} else {
		return 0;
	}
}

static int __init uniformity_timer_init(void)
{
	struct device_node *np = NULL;
	int ret;
	np = of_find_compatible_node(NULL, NULL, "hisilicon,prktimer");
	if (!np) {
	        printk("NOT FOUND device node 'hisilicon,prktimer'!\n");
	        return -ENXIO;
	}

	if (addr) {
		init_time_addr = 0;
		early_iounmap(addr, SZ_4K);
		addr = NULL;
	}

	addr = of_iomap(np, 0);
	if (!addr) {
	        printk("failed to get prktimer resource. lisc addr init fail\n");
	        return -ENXIO;
	}
	ret = of_property_read_u32(np, "fpga_flag", &fpga_flag);
	if (ret) {
	        printk("failed to get fpga_flag resource.\n");
	        return -ENXIO;
	}
	if (fpga_flag == 1) {
		scbbpdrxstat1 = 0x1008;
		scbbpdrxstat2 = 0x100c;
	} else {
		scbbpdrxstat1 = 0x534;
		scbbpdrxstat2 = 0x538;
	}
	init_time_addr = 1;
	return 0;
}
#ifdef CONFIG_HISI_APANIC
void panic_print_msg(struct printk_log *msg)
{
	char *text = (char *)msg + sizeof(struct printk_log);
	size_t text_size = msg->text_len;
	char time_log[80] = "";
	size_t tlen = 0;
	char *ptime_log;
	ptime_log = time_log;

	do {
			const char *next = memchr(text, '\n', text_size);
			size_t text_len;

			if (next) {
				text_len = next - text;
				next++;
				text_size -= next - text;
			} else {
				text_len = text_size;
			}
			tlen = print_time(msg->ts_nsec, ptime_log);
			apanic_console_write(ptime_log, tlen);
			apanic_console_write(text, text_len);
			apanic_console_write("\n", 1);

			text = (char *) next;
	} while (text);

}
#endif
void hisi_log_store_add_time(char *hisi_char, u32 sizeof_hisi_char, u16 *hisi_len) {
	struct tm tm_rtc;
	unsigned long cur_secs = 0;
	static unsigned long prev_jffy;
	static unsigned int prejf_init_flag;
	if (prejf_init_flag == 0) {
		prejf_init_flag = 1;
		prev_jffy = jiffies;
	}
	cur_secs = get_seconds();
	cur_secs -= sys_tz.tz_minuteswest * 60; /*lint !e647*/
	time_to_tm(cur_secs, 0, &tm_rtc);
	if (time_after(jiffies, prev_jffy + 1 * HZ)) {
		prev_jffy = jiffies;
		*hisi_len += snprintf(hisi_char, sizeof_hisi_char, "[%lu:%.2d:%.2d %.2d:%.2d:%.2d]",
					1900 + tm_rtc.tm_year, tm_rtc.tm_mon + 1, tm_rtc.tm_mday, tm_rtc.tm_hour, tm_rtc.tm_min, tm_rtc.tm_sec
					);
	}

#if defined(CONFIG_HISI_PRINTK_EXTENSION)
	*hisi_len += snprintf(hisi_char + *hisi_len, sizeof_hisi_char-*hisi_len, "[pid:%d,cpu%d,%s]",
						current->pid, smp_processor_id(), in_irq() ? "in irq" : current->comm);
#endif
	return;
}
pure_initcall(uniformity_timer_init);

#ifdef CONFIG_HISI_AMBA_PL011
int get_console_index(void)
{
	if ((selected_console != -1) && (selected_console < MAX_CMDLINECONSOLES))
		return console_cmdline[selected_console].index;

	return -1;
}

int get_console_name(char *name, int name_buf_len)
{
	if ((selected_console != -1) && (selected_console < MAX_CMDLINECONSOLES)) {
		strncpy(name, console_cmdline[selected_console].name, min(sizeof(console_cmdline[selected_console].name), name_buf_len)); /*lint !e574 !e58*/
		return 0;
	}

	return -1;
}
#endif

#ifdef CONFIG_HUAWEI_PRINTK_CTRL
int printk_level = LOGLEVEL_DEBUG;
int sysctl_printk_level = LOGLEVEL_DEBUG;
extern raw_spinlock_t *g_logbuf_level_lock_ex;
/*
 * if loglevel > level, the log will not be saved to memory in no log load
 * log load and factory mode load will not be affected
 */
void printk_level_setup(int level)
{
	unsigned int type = get_logusertype_flag();

	if (type != COMMERCIAL_USER || runmode_is_factory())
		return;

	pr_alert("printk_level_setup: %d\n", level);
	raw_spin_lock(g_logbuf_level_lock_ex);
	if (level >= LOGLEVEL_EMERG && level <= LOGLEVEL_DEBUG)
		printk_level = level;
	raw_spin_unlock(g_logbuf_level_lock_ex);
}
#endif

#define REG_NUMS (3)
static int __init early_printk_timer_setup(char *str)
{
	int i = 0;
	int ret;
	char *token;
	unsigned long long regs[REG_NUMS] = {};

	while ((token = strsep(&str, ",")) != NULL) {
		token = strim(token);
		if (!*token)
			continue;

		if (i >= REG_NUMS)
			goto fmt_err;

		ret = kstrtoull(token, 0, &regs[i++]);
		if (ret < 0)
			goto fmt_err;
	}

	if (i != REG_NUMS)
		goto fmt_err;

	if (regs[0] == 0)
		goto fmt_err;

	addr = early_ioremap(regs[0], SZ_4K);
	if (!addr) {
		pr_err("%s fail to ioremap\n", __func__);
		return -ENOMEM;
	}

	scbbpdrxstat1 = regs[1];
	scbbpdrxstat2 = regs[2];
	init_time_addr = 1;

	return 0;

fmt_err:
	pr_err("should be printktimer=<base>,<offset>,<offset>\n");
	return -EINVAL;
}

early_param("printktimer", early_printk_timer_setup);
