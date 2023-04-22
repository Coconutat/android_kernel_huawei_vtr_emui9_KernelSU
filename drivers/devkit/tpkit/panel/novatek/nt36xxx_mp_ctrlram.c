/* drivers/input/touchscreen/nt36772/nt36772_mp_ctrlram.c
 *
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 5629 $
 * $Date: 2016-07-15 11:24:48 +0800 (星期五, 15 七月 2016) $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/syscalls.h>


#include <linux/regulator/consumer.h>
#include <huawei_platform/log/log_jank.h>
#include "../../huawei_ts_kit.h"
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif

#include "nt36xxx.h"
#include "nt36xxx_mp_ctrlram.h"
//---For Debug : Test Time, Mallon 20160907---
#include <linux/jiffies.h>


#define NORMAL_MODE 0x00
#define TEST_MODE_1 0x21
#define TEST_MODE_2 0x22
#define MP_MODE_CC 0x41
#define FREQ_HOP_DISABLE 0x66
#define FREQ_HOP_ENABLE 0x65

static uint8_t RecordResult_Short[40 * 40] = {0};
static uint8_t RecordResult_Open[40 * 40] = {0};
static uint8_t RecordResult_FWMutual[40 * 40] = {0};
static uint8_t RecordResult_FW_CC[40 * 40] = {0};
static uint8_t RecordResult_FW_Diff[40 * 40] = {0};
static int32_t RecordResult_FWMutual_X_Delta[40 * 40] = {0};
static int32_t RecordResult_FWMutual_Y_Delta[40 * 40] = {0};

static int32_t TestResult_Short = 0;
static int32_t TestResult_Open = 0;
static int32_t TestResult_FWMutual = 0;
static int32_t TestResult_FW_CC = 0;
static int32_t TestResult_Noise = 0;
static int32_t TestResult_FW_Diff = 0;
static int32_t TestResult_FWMutual_Delta = 0;

static int32_t RawData_Short[40 * 40] = {0};
static int32_t RawData_Open[40 * 40] = {0};
static int32_t RawData_Diff[40 * 40] = {0};
static int32_t RawData_FWMutual[40 * 40] = {0};
static int32_t RawData_FW_CC[40 * 40] = {0};

#define PASS 0
#define FAIL -1
#define TEST_LMT_SIZE 1
#define CSV_PRODUCT_SYSTEM 1
#define CSV_ODM_SYSTEM 2

#define	CSV_TP_CAP_CC_MAX "PS_Config_Lmt_FW_CC_P"
#define	CSV_TP_CAP_CC_MIN "PS_Config_Lmt_FW_CC_N"
#define	CSV_TP_CAP_DIFF_MAX "PS_Config_Lmt_FW_Diff_P"
#define	CSV_TP_CAP_DIFF_MIN "PS_Config_Lmt_FW_Diff_N"
#define	CSV_TP_CAP_SHORT_MAX "PS_Config_Lmt_Short_Rawdata_P"
#define	CSV_TP_CAP_SHORT_MIN "PS_Config_Lmt_Short_Rawdata_N"
#define	CSV_TP_CAP_RAW_MAX "PS_Config_Lmt_FW_Rawdata_P"
#define	CSV_TP_CAP_RAW_MIN "PS_Config_Lmt_FW_Rawdata_N"
#define	CSV_TP_CAP_OPEN_MAX "PS_Config_Lmt_Open_Rawdata_P_A"
#define	CSV_TP_CAP_OPEN_MIN "PS_Config_Lmt_Open_Rawdata_N_A"
#define	CSV_TP_CAP_RAW_DELTA_X "PS_Config_Lmt_FW_Rawdata_X_Delta"
#define	CSV_TP_CAP_RAW_DELTA_Y "PS_Config_Lmt_FW_Rawdata_Y_Delta"

static char mmitest_result[TS_RAWDATA_RESULT_MAX] = {0};/*store mmi test result*/

#define CHANNEL_NUM 2//tx and rx
#define TARGET_NAME_SIZE	128
static char parse_target_name_up[TARGET_NAME_SIZE] = {0};
static char parse_target_name_low[TARGET_NAME_SIZE] = {0};
static bool nvt_parse_csvifle_ready = 0;
struct test_cmd {
	uint32_t addr;
	uint8_t len;
	uint8_t data[64];
};
struct get_csv_data {
	uint64_t size;
	int32_t csv_data[];
};
static struct test_cmd *CtrlRAM_test_cmd = NULL;
static int32_t CtrlRAM_test_cmd_num = 0;
static struct test_cmd *SignalGen_test_cmd = NULL;
static int32_t SignalGen_test_cmd_num = 0;
static struct test_cmd *SignalGen2_test_cmd = NULL;
static int32_t SignalGen2_test_cmd_num = 0;

// control ram parameter from short_open.ini
static uint32_t System_Init_CTRLRAMTableStartAddress = 0;
static uint32_t TableType0_GLOBAL0_RAW_BASE_ADDR = 0;
static uint32_t TableType1_GLOBAL0_RAW_BASE_ADDR = 0;
static uint32_t TableType0_GLOBAL_Addr = 0;
static uint32_t TableType1_GLOBAL_Addr = 0;

static int8_t nvt_mp_isInitialed = 0;


extern struct nvt_ts_data *nvt_ts;
extern int32_t novatek_ts_kit_read(uint16_t i2c_addr, uint8_t *buf, uint16_t len);
extern int32_t novatek_ts_kit_dummy_read(uint16_t i2c_addr);
extern int32_t novatek_ts_kit_write(uint16_t i2c_addr, uint8_t *buf, uint16_t len);
extern void nvt_kit_sw_reset_idle(void);
extern void nvt_kit_bootloader_reset(void);
extern int32_t nvt_kit_check_fw_reset_state(RST_COMPLETE_STATE check_reset_state);
extern int8_t nvt_kit_switch_noPD(uint8_t noPD);
extern int32_t nvt_kit_clear_fw_status(void);
extern int32_t nvt_kit_check_fw_status(void);
extern void nvt_kit_change_mode(uint8_t mode);
extern int8_t nvt_kit_get_fw_info(void);
extern uint8_t nvt_kit_get_fw_pipe(void);
extern void nvt_kit_read_mdata(uint32_t xdata_addr, uint32_t xdata_btn_addr);
extern void nvt_kit_get_mdata(int32_t *buf, uint8_t *m_x_num, uint8_t *m_y_num);
extern int32_t novatek_kit_read_projectid(void);
extern char novatek_kit_project_id[PROJECT_ID_LEN+1];
extern uint8_t nvt_fw_ver;

typedef enum mp_criteria_item {
	E_Raw_Lim_Open_Pos,
	E_Raw_Lim_Open_Neg,
	E_Raw_Lim_Short_Pos,
	E_Raw_Lim_Short_Neg,
	E_Lim_FW_Raw_Pos,
	E_Lim_FW_Raw_Neg,
	E_Lim_FW_CC_Pos,
	E_Lim_FW_CC_Neg,
	E_Lim_FW_Diff_Pos,
	E_Lim_FW_Diff_Neg,
	E_FW_Raw_Delta_Pos,
	E_FW_Raw_Delta_Neg,
	E_Open_rawdata,
	E_ADCOper_Cnt,
	E_MP_Cri_Item_Last
} mp_criteria_item_e;

typedef enum ctrl_ram_item {
	E_System_Init_CTRLRAMTableStartAddress,
	E_TableType0_GLOBAL_Addr,
	E_TableType1_GLOBAL_Addr,
	E_TableType0_GLOBAL0_RAW_BASE_ADDR,
	E_TableType1_GLOBAL0_RAW_BASE_ADDR,
	E_CtrlRam_Item_Last
} ctrl_ram_item_e;

typedef enum signal_gen_item {
	E_SignalGen,
	E_SignalGen2,
	E_SignalGen_Item_Last
} signal_gen_item_e;

typedef enum signal_gen_cmd_item {
	E_SignalGen_Cmd_No,
	E_SignalGen_Cmd_Name,
	E_SignalGen_Cmd_Addr,
	E_SignalGen_Cmd_Val,
	E_SignalGen_Cmd_Last
} signal_gen_cmd_item_e;

typedef enum rawdata_type {
	E_RawdataType_Short,
	E_RawdataType_Open,
	E_RawdataType_Last
} rawdata_type_e;

static void goto_next_line(char **ptr)
{
	do {
		*ptr = *ptr + 1;
	} while (**ptr != '\n');
	*ptr = *ptr + 1;
}

static void copy_this_line(char *dest, char *src)
{
	char *copy_from;
	char *copy_to;

	copy_from = src;
	copy_to = dest;
	do {
		*copy_to = *copy_from;
		copy_from++;
		copy_to++;
	} while((*copy_from != '\n') && (*copy_from != '\r'));
	*copy_to = '\0';
}

static void str_low(char *str)
{
	int i;

	for (i = 0; i < strlen(str); i++)
		if ((str[i] >= 65) && (str[i] <= 90))
			str[i] += 32;
}

static unsigned long str_to_hex(char *p)
{
	unsigned long hex = 0;
	unsigned long length = strlen(p), shift = 0;
	unsigned char dig = 0;

	str_low(p);
	length = strlen(p);

	if (length == 0)
		return 0;

	do {
		dig = p[--length];
		dig = dig < 'a' ? (dig - '0') : (dig - 'a' + 0xa);
		hex |= (dig << shift);
		shift += 4;
	} while (length);
	return hex;
}

/*******************************************************
Description:
	Novatek touchscreen check ASR error function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_check_asr_error(void)
{
	uint8_t buf[4] = {0};

	//---write i2c cmds to ASR error flag---
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0xF6;
	novatek_ts_kit_write(I2C_FW_Address, buf, 3);

	//---read ASR error flag---
	buf[0] = 0x96;
	novatek_ts_kit_read(I2C_FW_Address, buf, 3);

	if((buf[1] & 0x01) || (buf[2] & 0x01)) {
		TS_LOG_ERR("%s: Error!, buf[1]=0x%02X, buf[2]=0x%02X\n", __func__, buf[1], buf[2]);
		return -1;
	} else {
		return 0;
	}
}

/*******************************************************
Description:
	Novatek touchscreen clear ASR error flag function.

return:
	n.a
*******************************************************/
static void nvt_clean_asr_error_flag(void)
{
	uint8_t buf[4] = {0};

	//---write i2c cmds to ASR error flag---
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0xF6;
	novatek_ts_kit_write(I2C_FW_Address, buf, 3);

	//---clean ASR error flag---
	buf[0] = 0x96;
	buf[1] = 0x00;
	buf[2] = 0x00;
	novatek_ts_kit_write(I2C_FW_Address, buf, 3);
}

static int32_t nvt_load_mp_ctrlram_ini(void)
{
	int32_t retval = 0;
	struct file *fp = NULL;
	char *fbufp = NULL; // buffer for content of file
	mm_segment_t org_fs;
	char file_path[64];
	struct kstat stat;
	loff_t pos = 0;
	int32_t read_ret = 0;
	char *ptr = NULL;
	ctrl_ram_item_e ctrl_ram_item = E_System_Init_CTRLRAMTableStartAddress;
	char ctrlram_data_buf[128] = {0};
	char ctrlram_item_str[128] = {0};

	TS_LOG_INFO("%s:++\n", __func__);
#ifdef BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE
	snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/%s_short_open.ini", novatek_kit_project_id);
#else
	snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s_short_open.ini", novatek_kit_project_id);
#endif
		
	org_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(file_path, O_RDONLY, 0);
	if (fp == NULL || IS_ERR(fp)) {
		TS_LOG_ERR("%s: open %s failed\n", __func__, file_path);
		set_fs(org_fs);
		retval = -1;
		return retval;
	}

	retval = vfs_stat(file_path, &stat);
	if (!retval) {
		fbufp = (char *)vmalloc(stat.size + 1);
		if (!fbufp) {
			TS_LOG_ERR("%s: vmalloc %lld bytes failed.\n", __func__, stat.size);
			retval = -2;
			set_fs(org_fs);
			filp_close(fp, NULL);
			return retval;
		}else{
			memset(fbufp, 0, stat.size + 1);
		}

		read_ret = vfs_read(fp, (char __user *)fbufp, stat.size, &pos);
		if (read_ret > 0) {
			//pr_info("%s: File Size:%lld\n", __func__, stat.size);
			//pr_info("---------------------------------------------------\n");
			//printk("fbufp:\n");
			//for(i = 0; i < stat.size; i++) {
			//  printk("%c", fbufp[i]);
			//}
			//pr_info("---------------------------------------------------\n");

			fbufp[stat.size] = 0;
			ptr = fbufp;

			while ( ptr && (ptr < (fbufp + stat.size))) {
				if (ctrl_ram_item == E_System_Init_CTRLRAMTableStartAddress) {
					ptr = strstr(ptr, "[System_Init]");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: [System_Init] not found!\n", __func__);
						retval = -5;
						goto exit_free;
					}
					ptr = strstr(ptr, "CTRLRAMTableStartAddress");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: CTRLRAMTableStartAddress not found!\n", __func__);
						retval = -6;
						goto exit_free;
					}
					copy_this_line(ctrlram_data_buf, ptr);
					sscanf(ctrlram_data_buf, "CTRLRAMTableStartAddress=%127s", ctrlram_item_str);
					System_Init_CTRLRAMTableStartAddress = str_to_hex(ctrlram_item_str + 2); // skip "0x"
					TS_LOG_INFO("System_Init_CTRLRAMTableStartAddress = 0x%08X\n", System_Init_CTRLRAMTableStartAddress);
				} else if (ctrl_ram_item == E_TableType0_GLOBAL_Addr) {
					ptr = strstr(ptr, "[TableType0]");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: [TableType0] not found!\n", __func__);
						retval = -7;
						goto exit_free;
					}
					ptr = strstr(ptr, "GLOBAL_Addr=");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: \"[TableType0] GLOBAL_Addr\" not found!\n", __func__);
						retval = -8;
						goto exit_free;
					}
					copy_this_line(ctrlram_data_buf, ptr);
					sscanf(ctrlram_data_buf, "GLOBAL_Addr=%127s", ctrlram_item_str);
					TableType0_GLOBAL_Addr = str_to_hex(ctrlram_item_str + 2); // skip "0x"
					TS_LOG_INFO("TableType0_GLOBAL_Addr = 0x%08X\n", TableType0_GLOBAL_Addr);
					TableType0_GLOBAL_Addr = TableType0_GLOBAL_Addr - 0x10000;
				} else if (ctrl_ram_item == E_TableType1_GLOBAL_Addr) {
					ptr = strstr(ptr, "[TableType1]");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: [TableType1] not found!\n", __func__);
						retval = -9;
						goto exit_free;
					}
					ptr = strstr(ptr, "GLOBAL_Addr=");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: \"[TableType1] GLOBAL_Addr\" not found!\n", __func__);
						retval = -10;
						goto exit_free;
					}
					copy_this_line(ctrlram_data_buf, ptr);
					sscanf(ctrlram_data_buf, "GLOBAL_Addr=%127s", ctrlram_item_str);
					TableType1_GLOBAL_Addr = str_to_hex(ctrlram_item_str + 2); // skip "0x"
					TS_LOG_INFO("TableType1_GLOBAL_Addr = 0x%08X\n", TableType1_GLOBAL_Addr);
					TableType1_GLOBAL_Addr = TableType1_GLOBAL_Addr - 0x10000;
				} else if (ctrl_ram_item == E_TableType0_GLOBAL0_RAW_BASE_ADDR) {
					ptr = strstr(ptr, "[TableType0_GLOBAL0]");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: [TableType0_GLOBAL0] not found!\n", __func__);
						retval = -11;
						goto exit_free;
					}
					ptr = strstr(ptr, "\nRAW_BASE_ADDR=");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: \"[TableType0_GLOBAL0] RAW_BASE_ADDR\" not found!\n", __func__);
						retval = -12;
						goto exit_free;
					}
					ptr++; // skip first byte '\n'
					copy_this_line(ctrlram_data_buf, ptr);
					sscanf(ctrlram_data_buf, "RAW_BASE_ADDR=%127s", ctrlram_item_str);
					TableType0_GLOBAL0_RAW_BASE_ADDR = str_to_hex(ctrlram_item_str + 2); // skip "0x"
					TS_LOG_INFO("TableType0_GLOBAL0_RAW_BASE_ADDR = 0x%08X\n", TableType0_GLOBAL0_RAW_BASE_ADDR);
					TableType0_GLOBAL0_RAW_BASE_ADDR = TableType0_GLOBAL0_RAW_BASE_ADDR | 0x10000;
				} else if (ctrl_ram_item == E_TableType1_GLOBAL0_RAW_BASE_ADDR) {
					ptr = strstr(ptr, "[TableType1_GLOBAL0]");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: [TableType1_GLOBAL0] not found!\n", __func__);
						retval = -13;
						goto exit_free;
					}
					ptr = strstr(ptr, "\nRAW_BASE_ADDR=");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: \"[TableType1_GLOBAL0] RAW_BASE_ADDR\" not found!\n", __func__);
						retval = -14;
						goto exit_free;
					}
					ptr++; // skip first byte '\n'
					copy_this_line(ctrlram_data_buf, ptr);
					sscanf(ctrlram_data_buf, "RAW_BASE_ADDR=%127s", ctrlram_item_str);
					TableType1_GLOBAL0_RAW_BASE_ADDR = str_to_hex(ctrlram_item_str + 2); // skip "0x"
					TS_LOG_INFO("TableType1_GLOBAL0_RAW_BASE_ADDR = 0x%08X\n", TableType1_GLOBAL0_RAW_BASE_ADDR);
					TableType1_GLOBAL0_RAW_BASE_ADDR = TableType1_GLOBAL0_RAW_BASE_ADDR | 0x10000;
				}

				ctrl_ram_item++;
				if (ctrl_ram_item == E_CtrlRam_Item_Last) {
					TS_LOG_INFO("%s: Load control ram items finished\n", __func__);
					retval = 0;
					break;
				}
			}

        } else {
            TS_LOG_ERR("%s: retval=%d, read_ret=%d, fbufp=%p, stat.size=%lld\n", __func__, retval, read_ret, fbufp, stat.size);
            retval = -3;
            goto exit_free;
        }
    } else {
        TS_LOG_ERR("%s: failed to get file stat, retval = %d\n", __func__, retval);
        retval = -4;
        goto exit_free;
    }

exit_free:
	set_fs(org_fs);
	if (fbufp) {
		vfree(fbufp);
		fbufp = NULL;
	}
	if (fp) {
		filp_close(fp, NULL);
		fp = NULL;
	}

	TS_LOG_INFO("%s:--, retval=%d\n", __func__, retval);
	return retval;
}

static int32_t nvt_load_mp_ctrlram_bin(void)
{
	int32_t retval = 0;
	struct file *fp = NULL;
	char *fbufp = NULL; // buffer for content of file
	mm_segment_t org_fs;
	char file_path[64];
	struct kstat stat;
	loff_t pos = 0;
	int32_t read_ret = 0;
	char *ptr = NULL;
	uint32_t i = 0;
	uint32_t ctrlram_cur_addr = 0;

	TS_LOG_INFO("%s:++\n", __func__);
#ifdef BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE
	snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/%s_short_open.bin", novatek_kit_project_id);	
#else
	snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s_short_open.bin", novatek_kit_project_id);	
#endif
		
	org_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(file_path, O_RDONLY, 0);
	if (fp == NULL || IS_ERR(fp)) {
		TS_LOG_ERR("%s: open %s failed\n", __func__, file_path);
		set_fs(org_fs);
		retval = -1;
		return retval;
	}

	retval = vfs_stat(file_path, &stat);
	if (!retval) {
		fbufp = (char *)vmalloc(stat.size + 1);
		if (!fbufp) {
			TS_LOG_ERR("%s: vmalloc %lld bytes failed.\n", __func__, stat.size);
			retval = -2;
			set_fs(org_fs);
			filp_close(fp, NULL);
			return retval;
		}else{
			memset(fbufp, 0, stat.size + 1);
		}

		read_ret = vfs_read(fp, (char __user *)fbufp, stat.size, &pos);
		if (read_ret > 0) {
			//pr_info("%s: File Size:%lld\n", __func__, stat.size);
			//pr_info("---------------------------------------------------\n");
			//printk("fbufp:\n");
			//for(i = 0; i < stat.size; i++) {
			//  printk("%c", fbufp[i]);
			//}
			//pr_info("---------------------------------------------------\n");
			fbufp[stat.size] = 0;
			printk("ctrlram bin file size is %lld\n", stat.size);
			ptr = fbufp;

			CtrlRAM_test_cmd_num = (int32_t)((int32_t)stat.size / 4);
			if (System_Init_CTRLRAMTableStartAddress != 0) {
				ctrlram_cur_addr = System_Init_CTRLRAMTableStartAddress;
			} else {
				TS_LOG_ERR("%s: System_Init_CTRLRAMTableStartAddress is not initialized!\n", __func__);
				retval = -5;
				goto exit_free;
			}
			if (CtrlRAM_test_cmd != NULL) {
				vfree(CtrlRAM_test_cmd);
				CtrlRAM_test_cmd = NULL;
			}
			CtrlRAM_test_cmd = (struct test_cmd *)vmalloc(CtrlRAM_test_cmd_num * sizeof(struct test_cmd));
			if (!CtrlRAM_test_cmd) {
				TS_LOG_ERR("%s: vmalloc for CtrlRAM_test_cmd failed.\n", __func__);
				retval = -ENOMEM;
				goto exit_free;
			}

			for (i = 0; i < CtrlRAM_test_cmd_num; i++) {
				CtrlRAM_test_cmd[i].addr = ctrlram_cur_addr;
				CtrlRAM_test_cmd[i].len = 4;
				CtrlRAM_test_cmd[i].data[0] = *(ptr + (4 * i));
				CtrlRAM_test_cmd[i].data[1] = *(ptr + (4 * i) + 1); 
				CtrlRAM_test_cmd[i].data[2] = *(ptr + (4 * i) + 2);
				CtrlRAM_test_cmd[i].data[3] = *(ptr + (4 * i) + 3);
				//if (i < 10) {
				//	printk("%02X %02X %02X %02X\n", CtrlRAM_test_cmd[i].data[0], CtrlRAM_test_cmd[i].data[1], CtrlRAM_test_cmd[i].data[2], CtrlRAM_test_cmd[i].data[3]);
				//}
				ctrlram_cur_addr = ctrlram_cur_addr + 4;
			}

		} else {
			TS_LOG_ERR("%s: retval=%d, read_ret=%d, fbufp=%p, stat.size=%lld\n", __func__, retval, read_ret, fbufp, stat.size);
			retval = -3;
			goto exit_free;
		}
	} else {
		TS_LOG_ERR("%s: failed to get file stat, retval = %d\n", __func__, retval);
		retval = -4;
		goto exit_free;
	}

exit_free:
	set_fs(org_fs);
	if (fbufp) {
		vfree(fbufp);
		fbufp = NULL;
	}
	if (fp) {
		filp_close(fp, NULL);
		fp = NULL;
	}

	TS_LOG_INFO("%s:--, retval=%d\n", __func__, retval);

	return retval;
}

static int32_t nvt_load_mp_signal_gen_setting(void)
{
	int32_t retval = 0;
	struct file *fp = NULL;
	char *fbufp = NULL; // buffer for content of file
	mm_segment_t org_fs;
	char file_path[64];
	struct kstat stat;
	loff_t pos = 0;
	int32_t read_ret = 0;
	char *ptr = NULL;
	uint32_t i = 0;
	signal_gen_item_e signal_gen_item = E_SignalGen;
	char signal_gen_buf[128] = {0};
	signal_gen_cmd_item_e signal_gen_cmd_item = E_SignalGen_Cmd_Name;
	char *token = NULL;
	char *tok_ptr = NULL;

	TS_LOG_INFO("%s:++\n", __func__);
#ifdef BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE
	snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/%s_default.ini", novatek_kit_project_id);
#else
	snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s_default.ini", novatek_kit_project_id);
#endif
	org_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(file_path, O_RDONLY, 0);
	if (fp == NULL || IS_ERR(fp)) {
		TS_LOG_ERR("%s: open %s failed\n", __func__, file_path);
		set_fs(org_fs);
		retval = -1;
		return retval;
	}

	retval = vfs_stat(file_path, &stat);
	if (!retval) {
		fbufp = (char *)vmalloc(stat.size + 1);
		if (!fbufp) {
			TS_LOG_ERR("%s: vmalloc %lld bytes failed.\n", __func__, stat.size);
			retval = -2;
			set_fs(org_fs);
			filp_close(fp, NULL);
			return retval;
		}else{
			memset(fbufp, 0, stat.size + 1);
		}

		read_ret = vfs_read(fp, (char __user *)fbufp, stat.size, &pos);
		if (read_ret > 0) {
			//pr_info("%s: File Size:%lld\n", __func__, stat.size);
			//pr_info("---------------------------------------------------\n");
			//printk("fbufp:\n");
			//for(i = 0; i < stat.size; i++) {
			//  printk("%c", fbufp[i]);
			//}
			//pr_info("---------------------------------------------------\n");

			fbufp[stat.size] = 0;
			ptr = fbufp;

			while ( ptr && (ptr < (fbufp + stat.size))) {
				if (signal_gen_item == E_SignalGen) {
					ptr = strstr(ptr, "[SignalGen]");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: [SignalGen] not found!\n", __func__);
						retval = -5;
						goto exit_free;
					}
					ptr = strstr(ptr, "SignalNum=");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: [SignalGen] SignalNum  not found!\n", __func__);
						retval = -6;
						goto exit_free;
					}
					copy_this_line(signal_gen_buf, ptr);
					sscanf(signal_gen_buf, "SignalNum=%d", &SignalGen_test_cmd_num);
					TS_LOG_INFO("SignalGen_test_cmd_num = %d\n", SignalGen_test_cmd_num);
					if (SignalGen_test_cmd) {
						vfree(SignalGen_test_cmd);
						SignalGen_test_cmd = NULL;
					}
					SignalGen_test_cmd = vmalloc(SignalGen_test_cmd_num * sizeof(struct test_cmd));
					if (!SignalGen_test_cmd) {
						TS_LOG_ERR("%s: vmalloc for SignalGen_test_cmd failed!\n", __func__);
						retval = -ENOMEM;
						goto exit_free;
					}
					for (i = 0; i < SignalGen_test_cmd_num; i++) {
						goto_next_line(&ptr);
						copy_this_line(signal_gen_buf, ptr);
						signal_gen_cmd_item = E_SignalGen_Cmd_No;
						tok_ptr = signal_gen_buf;
						while((token = strsep(&tok_ptr,", =\t\0"))) {
							if (strlen(token) == 0) {
								continue;
							}
							if (signal_gen_cmd_item == E_SignalGen_Cmd_No) {
								signal_gen_cmd_item++;
							} else if (signal_gen_cmd_item == E_SignalGen_Cmd_Name) {
								signal_gen_cmd_item++;
							} else if (signal_gen_cmd_item == E_SignalGen_Cmd_Addr) {
								SignalGen_test_cmd[i].addr = str_to_hex(token + 2); // skip "0x"
								SignalGen_test_cmd[i].len = 1;
								signal_gen_cmd_item++;
							} else if (signal_gen_cmd_item == E_SignalGen_Cmd_Val) {
								SignalGen_test_cmd[i].data[0] = (uint8_t)str_to_hex(token + 2); // skip "0x"
								signal_gen_cmd_item++;
							} else if (signal_gen_cmd_item == E_SignalGen_Cmd_Last) {
								break;
							}
						}
					}
				} else if (signal_gen_item == E_SignalGen2) {
					ptr = strstr(ptr, "[SignalGen2]");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: [SignalGen2] not found!\n", __func__);
						retval = -7;
						goto exit_free;
					}
					ptr = strstr(ptr, "SignalNum=");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: [SignalGen2] SignalNum not found!\n", __func__);
						retval = -8;
						goto exit_free;
					}
					copy_this_line(signal_gen_buf, ptr);
					sscanf(signal_gen_buf, "SignalNum=%d", &SignalGen2_test_cmd_num);
					TS_LOG_INFO("SignalGen2_test_cmd_num = %d\n", SignalGen2_test_cmd_num);
					if (SignalGen2_test_cmd) {
						vfree(SignalGen2_test_cmd);
						SignalGen2_test_cmd = NULL;
					}
					SignalGen2_test_cmd = vmalloc(SignalGen2_test_cmd_num * sizeof(struct test_cmd));
					if (!SignalGen2_test_cmd) {
						TS_LOG_ERR("%s: vmalloc for SignalGen2_test_cmd failed!\n", __func__);
						retval = -ENOMEM;
						goto exit_free;
					}
					for (i = 0; i < SignalGen2_test_cmd_num; i++) {
						goto_next_line(&ptr);
						copy_this_line(signal_gen_buf, ptr);
						signal_gen_cmd_item = E_SignalGen_Cmd_No;
						tok_ptr = signal_gen_buf;
						while((token = strsep(&tok_ptr,", =\t\0"))) {
							if (strlen(token) == 0) {
								continue;
							}
							if (signal_gen_cmd_item == E_SignalGen_Cmd_No) {
								signal_gen_cmd_item++;
							} else if (signal_gen_cmd_item == E_SignalGen_Cmd_Name) {
								signal_gen_cmd_item++;
							} else if (signal_gen_cmd_item == E_SignalGen_Cmd_Addr) {
								SignalGen2_test_cmd[i].addr = str_to_hex(token + 2); // skip "0x"
								SignalGen2_test_cmd[i].len = 1;
								signal_gen_cmd_item++;
							} else if (signal_gen_cmd_item == E_SignalGen_Cmd_Val) {
								SignalGen2_test_cmd[i].data[0] = (uint8_t)str_to_hex(token + 2); // skip "0x"
								signal_gen_cmd_item++;
							} else if (signal_gen_cmd_item == E_SignalGen_Cmd_Last) {
								break;
							}
						}
					}
				}

				signal_gen_item++;
				if (signal_gen_item == E_SignalGen_Item_Last) {
					TS_LOG_INFO("%s: Load signal gen items finished\n", __func__);
					retval = 0;
					break;
				}
			}

		} else {
			TS_LOG_ERR("%s: retval=%d, read_ret=%d, fbufp=%p, stat.size=%lld\n", __func__, retval, read_ret, fbufp, stat.size);
			retval = -3;
			goto exit_free;
		}
	} else {
		TS_LOG_ERR("%s: failed to get file stat, retval = %d\n", __func__, retval);
		retval = -4;
		goto exit_free;
	}

exit_free:
	set_fs(org_fs);
	if (fbufp) {
		vfree(fbufp);
		fbufp = NULL;
	}
	if (fp) {
		filp_close(fp, NULL);
		fp = NULL;
	}

	TS_LOG_INFO("%s:--, retval=%d\n", __func__, retval);
	return retval;
}

/*******************************************************
Description:
	Novatek touchscreen set ADC operation function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_set_adc_oper(void)
{
	uint8_t buf[4] = {0};
	int32_t i, j;
	const int32_t retry_adc_oper = 10;
	const int32_t retry_adc_status = 10;

	for (i = 0; i < retry_adc_oper; i++) {
		//---write i2c cmds to set ADC operation---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0xF4;
		novatek_ts_kit_write(I2C_FW_Address, buf, 3);

		//---write i2c cmds to clear ADC operation---
		buf[0] = 0x4C;
		buf[1] = 0x00;
		novatek_ts_kit_write(I2C_FW_Address, buf, 2);

		msleep(10);
		//---write i2c cmds to set ADC operation---
		buf[0] = 0x4C;
		buf[1] = 0x01;
		novatek_ts_kit_write(I2C_FW_Address, buf, 2);

		for (j = 0; j < retry_adc_status; j++) {
			//---read ADC status---
			buf[0] = 0x4C;
			buf[1] = 0x00;
			novatek_ts_kit_read(I2C_FW_Address, buf, 2);

			if (buf[1] == 0x00)
				break;

			msleep(10);
		}

		if ((j >= retry_adc_status) || (nvt_check_asr_error() != 0)) {
			TS_LOG_ERR("%s: Failed!, buf[1]=0x%02X, i=%d\n", __func__, buf[1], i);
			nvt_clean_asr_error_flag();
		} else {
			break;
		}
	}

	if (i >= retry_adc_oper) {
		TS_LOG_ERR("%s: Failed!\n", __func__);
		return -1;
	} else {
		return 0;
	}
}

/*******************************************************
Description:
	Novatek touchscreen write test commands function.

return:
	n.a.
*******************************************************/
static void nvt_write_test_cmd(struct test_cmd *cmds, int32_t cmd_num)
{
	int32_t i = 0;
	int32_t j = 0;
	uint8_t buf[64];

	for (i = 0; i < cmd_num; i++) {
		//---set xdata index---
		nvt_set_page(cmds[i].addr);

		//---write test cmds---
		buf[0] = (cmds[i].addr & 0xFF);
		for (j = 0; j < cmds[i].len; j++) {
			buf[1 + j] = cmds[i].data[j];
		}
		novatek_ts_kit_write(I2C_FW_Address, buf, 1 + cmds[i].len);

/*
		//---read test cmds (debug)---
		buf[0] = (cmds[i].addr & 0xFF);
		novatek_ts_kit_read(I2C_FW_Address, buf, 1 + cmds[i].len);
		printk("0x%08X, ", cmds[i].addr);
		for (j = 0; j < cmds[i].len; j++) {
			printk("0x%02X, ", buf[j + 1]);
		}
		printk("\n");
*/
	}
}

static int32_t nvt_set_memory(uint32_t addr, uint8_t data)
{
	int32_t ret = 0;
	uint8_t buf[64] = {0};

	//---set xdata index---
	ret = nvt_set_page(addr);
	if (ret < 0) {
		TS_LOG_ERR("%s: write xdata index failed!(%d)\n", __func__, ret);
		return ret;
	}

	//---write data---
	buf[0] = addr & 0xFF;
	buf[1] = data;
	ret = novatek_ts_kit_write(I2C_FW_Address, buf, 2);
	if (ret < 0) {
		TS_LOG_ERR("%s: write data failed!(%d)\n", __func__, ret);
		return ret;
	}

/*
	//---read data (debug)---
	buf[0] = addr & 0xFF;
	buf[1] = 0x00;
	ret = novatek_ts_kit_read(I2C_FW_Address, buf, 2);
	printk("0x%08X, 0x%02X\n", addr, buf[1]);
*/

	return 0;
}

static int32_t nvt_get_memory(uint32_t addr, uint8_t *data)
{
	int32_t ret = 0;
	uint8_t buf[64] = {0};

	//---set xdata index---
	ret = nvt_set_page(addr);
	if (ret < 0) {
		TS_LOG_ERR("%s: write xdata index failed!(%d)\n", __func__, ret);
		return ret;
	}

	//---read data---
	buf[0] = addr & 0xFF;
	buf[1] = 0;
	ret = novatek_ts_kit_read(I2C_FW_Address, buf, 2);
	if (ret < 0) {
		TS_LOG_ERR("%s: read data failed!(%d)\n", __func__, ret);
		return ret;
	}
	*data = buf[1];

	return 0;
}

static void nvt_ASR_PowerOnMode(void)
{
	nvt_set_memory(0x1F690, 0x00);
	nvt_set_memory(0x1F693, 0x00);
	nvt_set_memory(0x1F68C, 0x01);
	nvt_set_memory(0x1F691, 0x01);
}

static void nvt_ASR_FrameMode(void)
{
	nvt_set_memory(0x1F690, 0x01);
	nvt_set_memory(0x1F693, 0x01);
	nvt_set_memory(0x1F68C, 0x01);
	nvt_set_memory(0x1F691, 0x01);
}

//---fix high current issue, Taylor 20160908---
static void nvt_TCON_PowerOnInit(void)
{
	uint8_t tmp_val = 0;

	// Step1
	nvt_set_memory(0x1F4BD, 0x01);
	nvt_set_memory(0x1F60F, 0x03);
	// ASR(Power On Mode)
	nvt_ASR_PowerOnMode();

	// Step2
	// 0x1F5EC,bit1 = 1 bit5 = 1, (| 0x22)
	nvt_get_memory(0x1F5EC, &tmp_val);
	tmp_val |= 0x22;
	nvt_set_memory(0x1F5EC, tmp_val);
	// 0x1F5EE,bit1 = 1 bit5 = 1, (| 0x22)
	tmp_val = 0;
	nvt_get_memory(0x1F5EE, &tmp_val);
	tmp_val |= 0x22;
	nvt_set_memory(0x1F5EE, tmp_val);
	// ASR(Frame On Mode)
	nvt_ASR_FrameMode();
	msleep(2);

	// Step3
	// 0x1F5F0, bit0 = 1, (| 0x01)
	tmp_val = 0;
	nvt_get_memory(0x1F5F0, &tmp_val);
	tmp_val |= 0x01;
	nvt_set_memory(0x1F5F0, tmp_val);
	// 20160908 0x1F5F1 = 0
	nvt_set_memory(0x1F5F1, 0x00);
	// 0x1F5F2, bit0 = 1, (| 0x01)
	tmp_val = 0;
	nvt_get_memory(0x1F5F2, &tmp_val);
	tmp_val |= 0x01;
	nvt_set_memory(0x1F5F2, tmp_val);

	// 20160908 sync to tuning tool
	// 0x1F5F3 = 0
	nvt_set_memory(0x1F5F3, 0x00);
	//0x1F5F4 = 0
	nvt_set_memory(0x1F5F4, 0x00);
	//0x1F5F5 = 0
	nvt_set_memory(0x1F5F5, 0x00);
	//0x1F5F6 = 0
	nvt_set_memory(0x1F5F6, 0x00);
	//0x1F5F7 = 0
	nvt_set_memory(0x1F5F7, 0x00);
	//0x1F5F8 = 0x24
	nvt_set_memory(0x1F5F8, 0x24);
	//0x1F5F9 = 0x1b
	nvt_set_memory(0x1F5F9, 0x1B);
	//0x1F5FA = 0
	nvt_set_memory(0x1F5FA, 0x00);
	//0x1F5FB = 0
	nvt_set_memory(0x1F5FB, 0x00);
	//0x1F5FC = 1
	nvt_set_memory(0x1F5FC, 0x01);
	//0x1F5FD = 0
	nvt_set_memory(0x1F5FD, 0x00);
	//0x1F5FE = 0
	nvt_set_memory(0x1F5FE, 0x00);
	//0x1F478 = 1
	nvt_set_memory(0x1F478, 0x01);
	// 20160908 sync to tuning tool (end)

	// ASR(Power On Mode)
	nvt_ASR_PowerOnMode();

	// Step4
	// 0x1F5EC, bit2 = 1 bit3 = 1, (| 0x0C)
	tmp_val = 0;
	nvt_get_memory(0x1F5EC, &tmp_val);
	tmp_val |= 0x0C;
	nvt_set_memory(0x1F5EC, tmp_val);
	// 0x1F5EE, bit2 = 1 bit3 = 1, (| 0x0C)
	tmp_val = 0;
	nvt_get_memory(0x1F5EE, &tmp_val);
	tmp_val |= 0x0C;
	nvt_set_memory(0x1F5EE, tmp_val);
	// ASR(Frame On Mode)
	nvt_ASR_FrameMode();
	msleep(1);
}

static void nvt_TCON_PowerOn(void)
{
	uint8_t tmp_val = 0;

	// Step1
	nvt_set_memory(0x1F4BD, 0x01);
	nvt_set_memory(0x1F60F, 0x03);
	// ASR(Power On Mode)
	nvt_ASR_PowerOnMode();
	// Step2
	// 0x1F5EC,bit4 = 0, (& 0xEF)
	nvt_get_memory(0x1F5EC, &tmp_val);
	tmp_val &= 0xEF;
	nvt_set_memory(0x1F5EC, tmp_val);
	// 0x1F5ED,bit0 = 0, (& 0xFE)
	tmp_val = 0;
	nvt_get_memory(0x1F5ED, &tmp_val);
	tmp_val &= 0xFE;
	nvt_set_memory(0x1F5ED, tmp_val);
	// 0x1F5EE,bit4 = 0, (& 0xEF)
	tmp_val = 0;
	nvt_get_memory(0x1F5EE, &tmp_val);
	tmp_val &= 0xEF;
	nvt_set_memory(0x1F5EE, tmp_val);
	// 0x1F5EF,bit0 = 0, (& 0xFE)
	tmp_val = 0;
	nvt_get_memory(0x1F5EF, &tmp_val);
	tmp_val &= 0xFE;
	nvt_set_memory(0x1F5EF, tmp_val);
	// ASR(Frame Mode)
	nvt_ASR_FrameMode();
	// Step3
	// 0x1F5EC, bit1 = 1 bit2 = 1 bit3 = 1 bit5 = 1, (| 0x2E)
	tmp_val = 0;
	nvt_get_memory(0x1F5EC, &tmp_val);
	tmp_val |= 0x2E;
	nvt_set_memory(0x1F5EC, tmp_val);
	// 0x1F5EE, bit1 = 1 bit2 = 1 bit3 = 1 bit5 = 1, (| 0x2E)
	tmp_val = 0;
	nvt_get_memory(0x1F5EE, &tmp_val);
	tmp_val |= 0x2E;
	nvt_set_memory(0x1F5EE, tmp_val);
	// ASR(Frame Mode)
	nvt_ASR_FrameMode();
	// Step4
	// 0x1F5F0,bit0 = 1, (| 0x01)
	tmp_val = 0;
	nvt_get_memory(0x1F5F0, &tmp_val);
	tmp_val |= 0x01;
	nvt_set_memory(0x1F5F0, tmp_val);
	// 0x1F5F2,bit0 = 1, (| 0x01)
	tmp_val = 0;
	nvt_get_memory(0x1F5F2, &tmp_val);
	tmp_val |= 0x01;
	nvt_set_memory(0x1F5F2, tmp_val);
	// ASR(Power On Mode)
	nvt_ASR_PowerOnMode();
	//Delay 100us
	msleep(1);
}

static void nvt_Active(void)
{
	uint8_t tmp_val = 0;

	// RESET2 toggle high
	// 0x1F066, bit2 = 1, (| 0x04)
	nvt_get_memory(0x1F066, &tmp_val);
	tmp_val |= 0x04;
	nvt_set_memory(0x1F066, tmp_val);
	// DSV_EN toggle high
	// 0x1F06E, 0x1, (| 0x01)
	tmp_val = 0;
	nvt_get_memory(0x1F06E, &tmp_val);
	tmp_val |= 0x01;
	nvt_set_memory(0x1F06E, tmp_val);
	// Wait MTP download ready IRQ
	msleep(100);
	// Wait TSSTB high
	// RO, 0x1F067, bit2 == 0 & bit7 == 1, (& 0xFB | 0x80)
	tmp_val = 0;
	nvt_get_memory(0x1F067, &tmp_val);
	tmp_val &= 0xFB;
	tmp_val |= 0x80;
	nvt_set_memory(0x1F067, tmp_val);
	msleep(17);
	// TCON Power on
	nvt_TCON_PowerOn();
}

static void nvt_Set_SignalGen(void)
{
	// [SignalGen]
	nvt_write_test_cmd(SignalGen_test_cmd, SignalGen_test_cmd_num);

	// [SignalGen2]
	nvt_write_test_cmd(SignalGen2_test_cmd, SignalGen2_test_cmd_num);
}

static void nvt_SwitchGlobalTable(rawdata_type_e rawdata_type)
{
	if (rawdata_type == E_RawdataType_Short) {
		nvt_set_memory(System_Init_CTRLRAMTableStartAddress, (uint8_t)(TableType0_GLOBAL_Addr & 0xFF));
		nvt_set_memory(System_Init_CTRLRAMTableStartAddress + 1, (uint8_t)((TableType0_GLOBAL_Addr & 0xFF00) >> 8));
	} else if (rawdata_type == E_RawdataType_Open) {
		nvt_set_memory(System_Init_CTRLRAMTableStartAddress, (uint8_t)(TableType1_GLOBAL_Addr & 0xFF));
		nvt_set_memory(System_Init_CTRLRAMTableStartAddress + 1, (uint8_t)((TableType1_GLOBAL_Addr & 0xFF00) >> 8));
	} else {
		// do nothing
	}
}

static int32_t nvt_mp_Initial(rawdata_type_e rawdata_type)
{
	TableType0_GLOBAL_Addr = 0;
	TableType1_GLOBAL_Addr = 0;
	TableType0_GLOBAL0_RAW_BASE_ADDR = 0;
	TableType1_GLOBAL0_RAW_BASE_ADDR = 0;

	// 0. SW reset + idle
	nvt_kit_sw_reset_idle();

	// 1. Stop WDT
	nvt_set_memory(0x1F050, 0x07);
	nvt_set_memory(0x1F051, 0x55);

	// 2. Switch Long HV
	// TSVD_pol Positive (register)
	//nvt_set_memory(0x1F44D, 0x01);
	// TSHD_pol Positive (register)
	nvt_set_memory(0x1F44E, 0x01);
	// TSVD_en (register)
	nvt_set_memory(0x1F44F, 0x01);
	// TSVD_pol Positive (register)
	nvt_set_memory(0x1F44D, 0x01);

	// 3. Set CtrlRAM from INI and BIN
	if (nvt_load_mp_ctrlram_ini()) {
		TS_LOG_ERR("%s: load MP CtrlRAM ini failed!\n", __func__);
		return -EAGAIN;
	}
	if (nvt_load_mp_ctrlram_bin()) {
		TS_LOG_ERR("%s: load MP CtrlRAM bin failed!\n", __func__);
		return -EAGAIN;
	}
	nvt_write_test_cmd(CtrlRAM_test_cmd, CtrlRAM_test_cmd_num);

	// 4. TCON Power on initial class AB flow
	nvt_TCON_PowerOnInit();

	// 5. TCON Power on class AB flow
	nvt_TCON_PowerOn();

	// 6. Active
	nvt_Active();

	// ADC Oper
	// Step1
	// write [CTRLRAMTableStartAddress] at 0x1f448, 0x1f449 (0x00, 0x00)
	nvt_set_memory(0x1F448, (uint8_t)((System_Init_CTRLRAMTableStartAddress - 0x10000) & 0xFF));
	nvt_set_memory(0x1F449, (uint8_t)(((System_Init_CTRLRAMTableStartAddress - 0x10000) >> 8) & 0xFF));
	// write GlobalTableAddress at &CTRLRAMTableStartAddress (0x08, 0x00)
	
	// Step2
	// write 1 at 0x1f447
	nvt_set_memory(0x1F447, 0x01);
	// Step3, 4 - Signal Gen
	nvt_set_memory(0x1F690, 0x00);
	nvt_set_memory(0x1F693, 0x00);
	// write all registry of [SignalGen]&[SignalGen2] from "default.ini" to register
	if (nvt_load_mp_signal_gen_setting()) {
		TS_LOG_ERR("%s: load MP signal gen setting failed!\n", __func__);
		return -EAGAIN;
	}
	nvt_Set_SignalGen();
	nvt_set_memory(0x1F68C, 0x01);
	nvt_set_memory(0x1F691, 0x01);
	// Step5 - TADC (not used)
	// Step6 - ADC oper
	// nvt_set_memory(0x1F44C, 0x01);
	// nvt_set_memory(0x1F44D, 0x00);

	//RAW_RDY_SH_NUM[5:0]
	nvt_set_memory(0x1F450, 0x04);
	//for CUT2
	nvt_set_memory(0x1F50B, 0x00);
	nvt_set_memory(0x1F6D6, 0x01);
	nvt_set_memory(0x1F6DF, 0x01);

	nvt_SwitchGlobalTable(rawdata_type);

	nvt_mp_isInitialed = 1;

	return 0;
}

int32_t Nova_OffsetToReg(uint32_t addr, uint32_t *offset_data, uint32_t afe_cnt)
{
	const uint32_t OFFSET_TABLE_SIZE = 88;
	uint8_t *reg_data = NULL;
	int32_t col = 0;
	int32_t i = 0;
	struct test_cmd RegData_test_cmd;

	reg_data = (uint8_t *)vmalloc(OFFSET_TABLE_SIZE * 9);
	if (!reg_data) {
		TS_LOG_ERR("%s: vmalloc for reg_data failed.\n", __func__);
		return -ENOMEM;
	}

	for (col = 0; col < 9; col++) {
		int32_t rawdata_cnt = col * 66;
		for (i = 0; i < (OFFSET_TABLE_SIZE / 4); i++) {
			reg_data[col * OFFSET_TABLE_SIZE + i * 4 + 0] = (uint8_t)((offset_data[rawdata_cnt + 0] >> 0) & 0xFF);
			reg_data[col * OFFSET_TABLE_SIZE + i * 4 + 1] = (uint8_t)(((offset_data[rawdata_cnt + 0] >> 8) & 0x03) | ((offset_data[rawdata_cnt + 1] << 2) & 0xFC));
			reg_data[col * OFFSET_TABLE_SIZE + i * 4 + 2] = (uint8_t)(((offset_data[rawdata_cnt + 1] >> 6) & 0x0F) | ((offset_data[rawdata_cnt + 2] << 4) & 0xF0));
			reg_data[col * OFFSET_TABLE_SIZE + i * 4 + 3] = (uint8_t)(((offset_data[rawdata_cnt + 2] >> 4) & 0x3F));
			rawdata_cnt += 3;
		}

		// write (OFFSET_TABLE_SIZE / 2) each time
		RegData_test_cmd.addr = addr + col * OFFSET_TABLE_SIZE;
		RegData_test_cmd.len = OFFSET_TABLE_SIZE / 2;
		memcpy(RegData_test_cmd.data, reg_data + col * OFFSET_TABLE_SIZE, OFFSET_TABLE_SIZE / 2);
		nvt_write_test_cmd(&RegData_test_cmd, 1);

		RegData_test_cmd.addr = addr + col * OFFSET_TABLE_SIZE + (OFFSET_TABLE_SIZE / 2);
		RegData_test_cmd.len = OFFSET_TABLE_SIZE / 2;
		memcpy(RegData_test_cmd.data, reg_data + col * OFFSET_TABLE_SIZE + (OFFSET_TABLE_SIZE / 2), OFFSET_TABLE_SIZE / 2);
		nvt_write_test_cmd(&RegData_test_cmd, 1);
	}

	if (reg_data) {
		vfree(reg_data);
		reg_data = NULL;
	}

	return 0;
}

static int32_t OpenRawToCS_pF(int32_t rawdata)
{
	int64_t CS = 0;
	int64_t Raw = 0;

	Raw = (int64_t)rawdata;
	Raw = Raw * 1000;
	//CS = ((Raw / 1228) - 5000) * 16;
	//CS = ((Raw / 1208) - 5083) * 32;
	CS = ((div_s64(Raw, 1208)) - 5083)*32;

	return (int32_t)CS;
}

/*******************************************************
Description:
	Novatek touchscreen read open test raw data function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_read_open(void)
{
	int32_t i = 0;
	int32_t x = 0;
	int32_t y = 0;
	uint8_t buf[128] = {0};
	uint8_t *rawdata_buf = NULL;

	TS_LOG_INFO("%s:++\n", __func__);

	rawdata_buf = (uint8_t *) vmalloc(IC_X_CFG_SIZE * IC_Y_CFG_SIZE * 2);
	if (!rawdata_buf) {
		TS_LOG_ERR("%s: vmalloc for rawdata_buf failed!\n", __func__);
		return -ENOMEM;
	}

	if (nvt_mp_isInitialed == 0) {
		if (nvt_mp_Initial(E_RawdataType_Open)) {
			TS_LOG_ERR("%s: MP Initial failed!\n", __func__);
			if (rawdata_buf) {
				vfree(rawdata_buf);
				rawdata_buf = NULL;
			}
			return -EAGAIN;
		}
	}

	nvt_SwitchGlobalTable(E_RawdataType_Open);

	for (i = 0; i < mADCOper_Cnt; i++) {
		if (nvt_set_adc_oper() < 0) {
			if (rawdata_buf) {
				vfree(rawdata_buf);
				rawdata_buf = NULL;
			}
			return -EAGAIN;
		}
	}

	for (y = 0; y < IC_Y_CFG_SIZE; y++) {
		//---change xdata index---
		if(nvt_ts->btype == TS_BUS_I2C) {
			buf[0] = 0xFF;
			buf[1] = 0x01;
			buf[2] = (uint8_t)(((TableType1_GLOBAL0_RAW_BASE_ADDR + y * IC_X_CFG_SIZE * 2) & 0xFF00) >> 8);
			novatek_ts_kit_write(I2C_FW_Address, buf, 3);
		} else {
			nvt_set_page(TableType1_GLOBAL0_RAW_BASE_ADDR + y * IC_X_CFG_SIZE * 2);
		}
		//---read data---
		buf[0] = (uint8_t)((TableType1_GLOBAL0_RAW_BASE_ADDR + y * IC_X_CFG_SIZE * 2) & 0xFF);
		novatek_ts_kit_read(I2C_FW_Address, buf, IC_X_CFG_SIZE * 2 + 1);
		memcpy(rawdata_buf + y * IC_X_CFG_SIZE * 2, buf + 1, IC_X_CFG_SIZE * 2);
	}

	for (y = 0; y < IC_Y_CFG_SIZE; y++) {
		for (x = 0; x < IC_X_CFG_SIZE; x++) {
			if ((AIN_Y[y] != 0xFF) && (AIN_X[x] != 0xFF)) {
				RawData_Open[AIN_Y[y] * X_Channel + AIN_X[x]] = ((rawdata_buf[(y * IC_X_CFG_SIZE + x) * 2] + 256 * rawdata_buf[(y * IC_X_CFG_SIZE + x) * 2 + 1]));
			}
		}
	}

	if (rawdata_buf) {
		vfree(rawdata_buf);
		rawdata_buf = NULL;
	}

	// convert rawdata to CS
	for (y = 0; y < Y_Channel; y++) {
		for (x = 0; x < X_Channel; x++) {
			printk("%5d, ", RawData_Open[y * X_Channel + x]);
			RawData_Open[y * X_Channel + x] = OpenRawToCS_pF(RawData_Open[y * X_Channel + x]);
		}
		printk("\n");
	}
	printk("\n");

	printk("%s:\n", __func__);
	for (y = 0; y < Y_Channel; y++) {
		for (x = 0; x < X_Channel; x++) {
			printk("%5d, ", RawData_Open[y * X_Channel + x]);
		}
		printk("\n");
	}
	printk("\n");

	TS_LOG_INFO("%s:--\n", __func__);

	return 0;
}

static int8_t nvt_switch_FreqHopEnDis(uint8_t FreqHopEnDis)
{
	uint8_t buf[8] = {0};
	uint8_t retry = 0;
	int8_t ret = 0;

	TS_LOG_INFO("%s:++\n", __func__);

	for (retry = 0; retry < 20; retry++) {
		//---set xdata index to EVENT BUF ADDR---
		nvt_set_page(nvt_ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

		//---switch FreqHopEnDis---
		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = FreqHopEnDis;
		novatek_ts_kit_write(I2C_FW_Address, buf, 2);

		msleep(35);

		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = 0xFF;
		novatek_ts_kit_read(I2C_FW_Address, buf, 2);

		if (buf[1] == 0x00)
			break;
	}

	if (unlikely(retry == 20)) {
		TS_LOG_ERR("%s: switch FreqHopEnDis 0x%02X failed, buf[1]=0x%02X\n", __func__, FreqHopEnDis, buf[1]);
		ret = -1;
	}

	TS_LOG_INFO("%s:--\n", __func__);

	return ret;
}

static int32_t nvt_read_raw(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;

	TS_LOG_INFO("%s:++\n", __func__);

/*
	if (nvt_kit_clear_fw_status()) {
		return -EAGAIN;
	}

	nvt_kit_change_mode(TEST_MODE_2);

	if (nvt_kit_check_fw_status()) {
		return -EAGAIN;
	}

	if (nvt_kit_get_fw_info()) {
		return -EAGAIN;
	}
*/
	if (nvt_kit_get_fw_pipe() == 0)
		nvt_kit_read_mdata(nvt_ts->mmap->RAW_PIPE0_ADDR, nvt_ts->mmap->RAW_BTN_PIPE0_ADDR);
	else
		nvt_kit_read_mdata(nvt_ts->mmap->RAW_PIPE1_ADDR, nvt_ts->mmap->RAW_BTN_PIPE1_ADDR);

	nvt_kit_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			xdata[y * x_num + x] =  (int16_t)xdata[y * x_num + x];
		}
	}

	//nvt_kit_change_mode(NORMAL_MODE);

	printk("%s:\n", __func__);
	for (y = 0; y < Y_Channel; y++) {
		for (x = 0; x < X_Channel; x++) {
			iArrayIndex = y * X_Channel + x;
			printk("%5d ", xdata[iArrayIndex]);
		}
		printk("\n");
	}
	printk("\n");

	TS_LOG_INFO("%s:--\n", __func__);
	return 0;
}

static int32_t nvt_read_CC(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;

	TS_LOG_INFO("%s:++\n", __func__);

/*
	if (nvt_kit_clear_fw_status()) {
		return -EAGAIN;
	}

	nvt_kit_change_mode(MP_MODE_CC);

	if (nvt_kit_check_fw_status()) {
		return -EAGAIN;
	}

	nvt_kit_get_fw_info();
*/
	if (nvt_kit_get_fw_pipe() == 0)
		nvt_kit_read_mdata(nvt_ts->mmap->DIFF_PIPE1_ADDR, nvt_ts->mmap->DIFF_BTN_PIPE1_ADDR);
	else
		nvt_kit_read_mdata(nvt_ts->mmap->DIFF_PIPE0_ADDR, nvt_ts->mmap->DIFF_BTN_PIPE0_ADDR);

	nvt_kit_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			xdata[y * x_num + x] =  (int16_t)xdata[y * x_num + x];
		}
	}

	//nvt_kit_change_mode(NORMAL_MODE);

	printk("%s:\n", __func__);
	for (y = 0; y < Y_Channel; y++) {
		for (x = 0; x < X_Channel; x++) {
			iArrayIndex = y * X_Channel + x;
			printk("%5d ", xdata[iArrayIndex]);
		}
		printk("\n");
	}
	printk("\n");

	TS_LOG_INFO("%s:--\n", __func__);
	return 0;
}

static int32_t nvt_read_diff(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;

	TS_LOG_INFO("%s:++\n", __func__);
/*
	if (nvt_kit_clear_fw_status()) {
		return -EAGAIN;
	}

	nvt_kit_change_mode(TEST_MODE_2);

	if (nvt_kit_check_fw_status()) {
		return -EAGAIN;
	}

	nvt_kit_get_fw_info();
*/
	if (nvt_kit_get_fw_pipe() == 0)
		nvt_kit_read_mdata(nvt_ts->mmap->DIFF_PIPE0_ADDR, nvt_ts->mmap->DIFF_BTN_PIPE0_ADDR);
	else
		nvt_kit_read_mdata(nvt_ts->mmap->DIFF_PIPE1_ADDR, nvt_ts->mmap->DIFF_BTN_PIPE1_ADDR);

	nvt_kit_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			xdata[y * x_num + x] =  (int16_t)xdata[y * x_num + x];
		}
	}

	//nvt_kit_change_mode(NORMAL_MODE);

	TS_LOG_INFO("%s:--\n", __func__);

	return 0;
}

static int32_t nvt_read_noise(void)
{
	int32_t x = 0;
	int32_t y = 0;
	int32_t iArrayIndex = 0;

	if (nvt_read_diff(RawData_Diff)) {
		return 1; // read data failed
	}

	printk("%s:RawData_Diff:\n", __func__);
	for (y = 0; y < Y_Channel; y++) {
		for (x = 0; x < X_Channel; x++) {
			iArrayIndex = y * X_Channel + x;
			printk("%5d ", RawData_Diff[iArrayIndex]);
		}
		printk("\n");
	}
	printk("\n");

	return 0;
}
static void nvt_enable_short_test(void)
{
	uint8_t buf[8] = {0};

	//---set xdata index to EVENT BUF ADDR---
	nvt_set_page(nvt_ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

	//---enable short test---
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = 0x43;
	buf[2] = 0xAA;
	buf[3] = 0x02;
	buf[4] = 0x00;
	novatek_ts_kit_write(I2C_FW_Address, buf, 5);
}

static int32_t nvt_polling_hand_shake_status(void)
{
	uint8_t buf[8] = {0};
	int32_t i = 0;
	const int32_t retry = 200;

	for (i = 0; i < retry; i++) {
		//---set xdata index to EVENT BUF ADDR---
		nvt_set_page(nvt_ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE);

		//---read fw status---
		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		buf[1] = 0x00;
		novatek_ts_kit_read(I2C_FW_Address, buf, 2);

		if ((buf[1] == 0xA0) || (buf[1] == 0xA1))
			break;

		msleep(10);
	}

	if (i >= retry) {
		TS_LOG_ERR("%s: polling hand shake status failed, buf[1]=0x%02X\n", __func__, buf[1]);
		// Read back 5 bytes from offset EVENT_MAP_HOST_CMD for debug check
		if(nvt_ts->btype == TS_BUS_SPI) {
			nvt_set_page(nvt_ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);
			buf[0] = EVENT_MAP_HOST_CMD;
			buf[1] = 0x00;
			buf[2] = 0x00;
			buf[3] = 0x00;
			buf[4] = 0x00;
			buf[5] = 0x00;
			novatek_ts_kit_read(I2C_FW_Address, buf, 6);
			TS_LOG_ERR("Read back 5 bytes from offset EVENT_MAP_HOST_CMD: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", buf[1], buf[2], buf[3], buf[4], buf[5]);
		}
		return -1;
	} else {
		return 0;
	}
}

static void nvt_tddi_enable_noise_collect(int32_t frame_num)
{
	uint8_t buf[8] = {0};

	//---set xdata index to EVENT BUF ADDR---
	if(nvt_ts->btype == TS_BUS_I2C) {
		buf[0] = NVTTDDI_DOUBLE_F_CMD;
		buf[1] = (nvt_ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
		buf[2] = (nvt_ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
		novatek_ts_kit_write(I2C_FW_Address, buf, NVTTDDI_THREE_BYTES_LENGTH);
	} else {
		nvt_set_page(nvt_ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);
	}

	//---enable noise collect test---
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = NVTTDDI_FOUR_SEVEN_CMD;
	buf[2] = NVTTDDI_DOUBLE_A_CMD;
	buf[3] = frame_num;
	buf[4] = NVTTDDI_DOUBLE_ZERO_CMD;
	novatek_ts_kit_write(I2C_FW_Address, buf, NVTTDDI_FIVE_BYTES_LENGTH);
}

static void nvt_tddi_enable_open_test(void)
{
	uint8_t buf[8] = {0};

	//---set xdata index to EVENT BUF ADDR---
	if(nvt_ts->btype == TS_BUS_I2C) {
		buf[0] = NVTTDDI_DOUBLE_F_CMD;
		buf[1] = (nvt_ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
		buf[2] = (nvt_ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
		novatek_ts_kit_write(I2C_FW_Address, buf, NVTTDDI_THREE_BYTES_LENGTH);
	} else {
		nvt_set_page(nvt_ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);
	}

	//---enable open test---
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = NVTTDDI_FOUR_FIVE_CMD;
	buf[2] = NVTTDDI_DOUBLE_A_CMD;
	buf[3] = NVTTDDI_ZERO_TWO_CMD;
	buf[4] = NVTTDDI_DOUBLE_ZERO_CMD;
	novatek_ts_kit_write(I2C_FW_Address, buf, NVTTDDI_FIVE_BYTES_LENGTH);
}

#define NVT_TDDI_ABS(x) (x >= 0 ? x : (-x))
static int32_t nvt_tddi_read_fw_noise(int32_t *xdata)
{
	uint8_t x_num = NO_ERR;
	uint8_t y_num = NO_ERR;
	uint32_t x = NO_ERR;
	uint32_t y = NO_ERR;
	int32_t iArrayIndex = NO_ERR;
	int32_t frame_num = NO_ERR;
	int32_t RawData_Diff_Min = NO_ERR;
	int32_t RawData_Diff_Max = NO_ERR;

	TS_LOG_INFO("%s:++\n", __func__);

	//---Enter Test Mode---
	if (nvt_kit_clear_fw_status()) {
		return -EAGAIN;
	}
	frame_num = nvt_ts->noise_test_frame / NVTTDDI_TEST_FRAME_DIVIDE_NUM;
	if (frame_num <= NO_ERR) {
		frame_num = NVTTDDI_FRAME_NUMBER;
	}
	TS_LOG_INFO("%s: frame_num=%d\n", __func__, frame_num);
	nvt_tddi_enable_noise_collect(frame_num);
	//need wait PS_Config_Diff_TEST_Frame*8.3ms
	msleep(frame_num*83);

	if (nvt_polling_hand_shake_status()) {
		return -EAGAIN;
	}

	if (nvt_kit_get_fw_info()) {
		return -EAGAIN;
	}

	if (nvt_kit_get_fw_pipe() == NO_ERR) {
		nvt_kit_read_mdata(nvt_ts->mmap->DIFF_PIPE0_ADDR, nvt_ts->mmap->DIFF_BTN_PIPE0_ADDR);
	} else {
		nvt_kit_read_mdata(nvt_ts->mmap->DIFF_PIPE1_ADDR, nvt_ts->mmap->DIFF_BTN_PIPE1_ADDR);
	}
	nvt_kit_get_mdata(xdata, &x_num, &y_num);
	for (y = NO_ERR; y < y_num; y++) {
		for (x = NO_ERR; x < x_num; x++) {
			iArrayIndex = y * x_num + x;
			RawData_Diff_Max = (int8_t)(MIDDLE_EIGHT_BITS(xdata[iArrayIndex]));
			RawData_Diff_Min = (int8_t)(LOW_EIGHT_BITS(xdata[iArrayIndex]));
			if (NVT_TDDI_ABS(RawData_Diff_Max) > NVT_TDDI_ABS(RawData_Diff_Min)) {
				RawData_Diff[iArrayIndex] = RawData_Diff_Max;
			} else {
				RawData_Diff[iArrayIndex] = RawData_Diff_Min;
			}
		}
	}

	//---Leave Test Mode---
	nvt_kit_change_mode(NORMAL_MODE);

	printk("%s:RawData_Diff:\n", __func__);
	for (y = NO_ERR; y < Y_Channel; y++) {
		for (x = NO_ERR; x < X_Channel; x++) {
			iArrayIndex = y * X_Channel + x;
			printk("%5d ", RawData_Diff[iArrayIndex]);
		}
		printk("\n");
	}
	printk("\n");

	TS_LOG_INFO("%s:--\n", __func__);

	return NO_ERR;
}

static int32_t nvt_read_fw_short(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;

	TS_LOG_INFO("%s:++\n", __func__);

	if (nvt_kit_clear_fw_status()) {
		return -EAGAIN;
	}

	nvt_enable_short_test();

	if (nvt_polling_hand_shake_status()) {
		return -EAGAIN;
	}

	if (nvt_kit_get_fw_info()) {
		return -EAGAIN;
	}

	if (nvt_kit_get_fw_pipe() == 0)
		nvt_kit_read_mdata(nvt_ts->mmap->RAW_PIPE0_ADDR, nvt_ts->mmap->RAW_BTN_PIPE0_ADDR);
	else
		nvt_kit_read_mdata(nvt_ts->mmap->RAW_PIPE1_ADDR, nvt_ts->mmap->RAW_BTN_PIPE1_ADDR);

	nvt_kit_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			xdata[y * x_num + x] =  (int16_t)xdata[y * x_num + x];
		}
	}

	nvt_kit_change_mode(NORMAL_MODE);

	printk("%s:\n", __func__);
	for (y = 0; y < Y_Channel; y++) {
		for (x = 0; x < X_Channel; x++) {
			iArrayIndex = y * X_Channel + x;
			printk("%5d ", xdata[iArrayIndex]);
		}
		printk("\n");
	}
	printk("\n");

	TS_LOG_INFO("%s:--\n", __func__);

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen read fw open test raw data function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_tddi_read_fw_open(void)
{
	uint32_t raw_pipe_addr = NO_ERR;
	int32_t x = NO_ERR;
	int32_t y = NO_ERR;
	uint8_t buf[128] = {0};
	uint8_t *rawdata_buf = NULL;
	uint8_t x_channel_size = 0;
	uint8_t y_channel_size = 0;

	if (NULL == nvt_ts) {
		TS_LOG_ERR("%s: param error\n", __FUNCTION__);
		return -EINVAL;
	}

	if (NULL == nvt_ts->NvtTddi_X_Channel || NULL == nvt_ts->NvtTddi_Y_Channel) {
		TS_LOG_ERR("%s: param error\n", __FUNCTION__);
		return -EINVAL;
	}

	if (FLAG_EXIST == nvt_ts->nvttddi_channel_flag) {
		if (*(nvt_ts->NvtTddi_X_Channel) > U8_MAX || *(nvt_ts->NvtTddi_Y_Channel) > U8_MAX ) {
			TS_LOG_ERR("%s: data conversion failed!\n", __func__);
			return -EINVAL;
		} else {
			x_channel_size = (uint8_t)*(nvt_ts->NvtTddi_X_Channel);
			y_channel_size = (uint8_t)*(nvt_ts->NvtTddi_Y_Channel);
		}
	} else {
		x_channel_size = IC_X_CFG_SIZE;
		y_channel_size = IC_Y_CFG_SIZE;
	}

	TS_LOG_INFO("%s:++\n", __func__);

	//---Enter Test Mode---
	if (nvt_kit_clear_fw_status()) {
		return -EAGAIN;
	}

	nvt_tddi_enable_open_test();

	if (nvt_polling_hand_shake_status()) {
		return -EAGAIN;
	}

	rawdata_buf = (uint8_t *) vmalloc(IC_X_CFG_SIZE * IC_Y_CFG_SIZE * NVTTDDI_MULTIPLY_2_NUM);
	if (!rawdata_buf) {
		TS_LOG_ERR("%s: vmalloc for rawdata_buf failed!\n", __func__);
		return -ENOMEM;
	}

	if (nvt_kit_get_fw_pipe() == NO_ERR) {
		raw_pipe_addr = nvt_ts->mmap->RAW_PIPE0_ADDR;
	} else {
		raw_pipe_addr = nvt_ts->mmap->RAW_PIPE1_ADDR;
	}

	for (y = NO_ERR; y < IC_Y_CFG_SIZE; y++) {
		//---change xdata index---
		if(nvt_ts->btype == TS_BUS_I2C) {
			buf[0] = NVTTDDI_DOUBLE_F_CMD;
			buf[1] = (uint8_t)(HIGHT_EIGHT_BITS(raw_pipe_addr + y * IC_X_CFG_SIZE * NVTTDDI_MULTIPLY_2_NUM));
			buf[2] = (uint8_t)(MIDDLE_EIGHT_BITS(raw_pipe_addr + y * IC_X_CFG_SIZE * NVTTDDI_MULTIPLY_2_NUM));
			novatek_ts_kit_write(I2C_FW_Address, buf, NVTTDDI_THREE_BYTES_LENGTH);
		} else {
			nvt_set_page(raw_pipe_addr + y * IC_X_CFG_SIZE * NVTTDDI_MULTIPLY_2_NUM);
		}
		//---read data---
		buf[0] = (uint8_t)(LOW_EIGHT_BITS(raw_pipe_addr + y * IC_X_CFG_SIZE * NVTTDDI_MULTIPLY_2_NUM));
		novatek_ts_kit_read(I2C_FW_Address, buf, IC_X_CFG_SIZE * NVTTDDI_MULTIPLY_2_NUM + NVTTDDI_PLUS_ONE);
		memcpy(rawdata_buf + y * IC_X_CFG_SIZE * NVTTDDI_MULTIPLY_2_NUM, buf + NVTTDDI_PLUS_ONE, IC_X_CFG_SIZE * NVTTDDI_MULTIPLY_2_NUM);
	}

	for (y = NO_ERR; y < IC_Y_CFG_SIZE; y++) {
		for (x = NO_ERR; x < IC_X_CFG_SIZE; x++) {
			if ((AIN_Y[y] != NVTTDDI_DOUBLE_F_CMD) && (AIN_X[x] != NVTTDDI_DOUBLE_F_CMD)) {
				RawData_Open[AIN_Y[y] * X_Channel + AIN_X[x]] = (int16_t)((rawdata_buf[(y * IC_X_CFG_SIZE + x) * NVTTDDI_MULTIPLY_2_NUM] + NVTTDDI_MULTIPLY_256_NUM * rawdata_buf[(y * IC_X_CFG_SIZE + x) * NVTTDDI_MULTIPLY_2_NUM + NVTTDDI_PLUS_ONE]));
			}
		}
	}

	if (rawdata_buf) {
		vfree(rawdata_buf);
		rawdata_buf = NULL;
	}

	//---Leave Test Mode--
	nvt_kit_change_mode(NORMAL_MODE);

	printk("%s:\n", __func__);
	for (y = NO_ERR; y < y_channel_size; y++) {
		for (x = NO_ERR; x < x_channel_size; x++) {
			printk("%5d, ", RawData_Open[y * x_channel_size + x]);
		}
		printk("\n");
	}
	printk("\n");

	TS_LOG_INFO("%s:--\n", __func__);

	return NO_ERR;
}

static int nvt_get_threshold_from_csvfile(int columns, int rows, char* target_name, struct get_csv_data *data)
{
	char file_path[100] = {0};
	char file_name[64] = {0};
	int ret = 0;
	int result = 0;

	TS_LOG_INFO("%s called\n", __func__);

	if (!data || !target_name || columns*rows > data->size) {
		TS_LOG_ERR("parse csvfile failed: data or target_name is NULL\n");
		return FAIL;
	}

	snprintf(file_name, sizeof(file_name), "%s_%s_%s_%s_raw.csv",
			nvt_ts->chip_data->ts_platform_data->product_name,
			nvt_ts->chip_data->chip_name,
			novatek_kit_project_id,
			nvt_ts->chip_data->module_name);
	TS_LOG_INFO("%s: file_name=%s\n", __func__, file_name);

	if (CSV_PRODUCT_SYSTEM == nvt_ts->csvfile_use_system) {
		snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/%s", file_name);
	}
	else if (CSV_ODM_SYSTEM == nvt_ts->csvfile_use_system) {
		snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s", file_name);
	}
	else{
	    return FAIL;
	}
	TS_LOG_INFO("threshold file name:%s, rows_size=%d, columns_size=%d, target_name = %s\n", file_path, rows, columns, target_name);

	result =  ts_kit_parse_csvfile(file_path, target_name, data->csv_data, rows, columns);
	if (PASS == result){
		ret = PASS;
		TS_LOG_INFO("Get threshold successed form csvfile\n");
	} else {
		TS_LOG_INFO("csv file parse fail:%s\n", file_path);
		ret = FAIL;
	}
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen raw data delta test function.

return:
	Executive outcomes. 0---passed. negative---failed.
*******************************************************/
static int32_t nvt_rawdata_up_low(int32_t rawdata[], uint8_t RecordResult[], uint8_t x_len, uint8_t y_len, int32_t Upper_Lmt[], int32_t Lower_Lmt[])
{
	int32_t retval = 0;
	int32_t i = 0;
	int32_t j = 0;
	uint64_t rawdata_size = x_len*y_len;

	struct get_csv_data *rawdata_up = NULL;
	struct get_csv_data *rawdata_low = NULL;

	TS_LOG_INFO("%s:++\n", __func__);

	if (FLAG_EXIST == nvt_ts->test_capacitance_via_csvfile && PASS == nvt_parse_csvifle_ready) {
		if (rawdata_size <= 0) {
			TS_LOG_ERR("%s, tx or rx is zero\n", __func__);
			retval = -1;
			goto exit;
		}
		rawdata_up = kzalloc(rawdata_size*sizeof(int32_t)+sizeof(struct get_csv_data), GFP_KERNEL);
		rawdata_low = kzalloc(rawdata_size*sizeof(int32_t)+sizeof(struct get_csv_data), GFP_KERNEL);

		if (!rawdata_up || !rawdata_low) {
			TS_LOG_ERR("%s: malloc rawdata up or low failed\n", __func__);
			retval = -1;
			goto exit;
		}
		rawdata_up->size = rawdata_size;
		rawdata_low->size = rawdata_size;
		if (FAIL == nvt_get_threshold_from_csvfile(x_len, y_len, parse_target_name_up, rawdata_up) ||
			FAIL == nvt_get_threshold_from_csvfile(x_len, y_len, parse_target_name_low, rawdata_low)) {
			TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
			retval = -1;
			goto exit;
		} else {
			Upper_Lmt = rawdata_up->csv_data;
			Lower_Lmt = rawdata_low->csv_data;
		}
	}

	//---Check Lower & Upper Limit---
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < x_len; i++) {
			RecordResult[j * x_len + i] = NO_ERR;
			if(rawdata[j * x_len + i] > Upper_Lmt[j * x_len + i]) {
				RecordResult[j * x_len + i] |= 0x01;
				retval = -1;
				TS_LOG_ERR("%s, [%2d, %2d] Val is [%5d] ,limit is[%5d, %5d]", parse_target_name_up, i, j, rawdata[j * x_len + i],
					Lower_Lmt[j * x_len + i], Upper_Lmt[j * x_len + i]);
			}

			if(rawdata[j * x_len + i] < Lower_Lmt[j * x_len + i]) {
				RecordResult[j * x_len + i] |= 0x02;
				retval = -1;
				TS_LOG_ERR("%s, [%2d, %2d] Val is [%5d] ,limit is[%5d, %5d]", parse_target_name_up, i, j, rawdata[j * x_len + i],
					Lower_Lmt[j * x_len + i], Upper_Lmt[j * x_len + i]);
			}
		}
	}

	TS_LOG_INFO("%s:--\n", __func__);
exit:
	if (rawdata_up)
		kfree(rawdata_up);
	if (rawdata_low)
		kfree(rawdata_low);

	//---Return Result---
	return retval;
}

/*******************************************************
Description:
	Novatek touchscreen calculate G Ratio and Normal
	function.

return:
	Executive outcomes. 0---succeed. 1---failed.
*******************************************************/
static int32_t nvt_rawdata_delta(int32_t rawdata[], uint8_t x_len, uint8_t y_len)
{
	int32_t retval = 0;
	int32_t i = 0;
	int32_t j = 0;
	int32_t delta = 0;
	uint64_t rawdata_size = x_len*y_len;
	struct get_csv_data *rawdata_up = NULL;
	struct get_csv_data *rawdata_low = NULL;

	TS_LOG_INFO("%s:++\n", __func__);

	if (FLAG_EXIST == nvt_ts->test_capacitance_via_csvfile) {
		if (rawdata_size <= 0) {
			TS_LOG_ERR("%s, tx or rx is zero\n", __func__);
			retval = -1;
			goto exit;
		}
		rawdata_up = kzalloc(rawdata_size*sizeof(int32_t) + sizeof(struct get_csv_data), GFP_KERNEL);
		rawdata_low = kzalloc(rawdata_size*sizeof(int32_t) + sizeof(struct get_csv_data), GFP_KERNEL);

		if (!rawdata_up || !rawdata_low) {
			TS_LOG_ERR("%s: malloc rawdata up or low failed\n", __func__);
			retval = -1;
			goto exit;
		}
		rawdata_up->size = rawdata_size;
		rawdata_low->size = rawdata_size;

		if (FAIL == nvt_get_threshold_from_csvfile(x_len-1, y_len, CSV_TP_CAP_RAW_DELTA_X, rawdata_up) ||
			FAIL == nvt_get_threshold_from_csvfile(x_len, y_len-1, CSV_TP_CAP_RAW_DELTA_Y, rawdata_low)) {
			TS_LOG_ERR("%s:get threshold from csvfile failed\n", __func__);
			retval = -1;
			goto exit;
		} else {
			memcpy(PS_Config_Lmt_FW_Rawdata_X_Delta, rawdata_up->csv_data, rawdata_size*sizeof(int32_t));
			memcpy(PS_Config_Lmt_FW_Rawdata_Y_Delta, rawdata_low->csv_data, rawdata_size*sizeof(int32_t));
		}
	}

	//---Check X Delta---
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < (x_len-1); i++) {
			RecordResult_FWMutual_X_Delta[j * (x_len-1) + i] = (rawdata[j * x_len + i] - rawdata[j * x_len + (i+1)]);
		}
	}

	printk("%s:RawData_X_Delta:\n", __func__);
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < (x_len-1); i++) {
			if(RecordResult_FWMutual_X_Delta[j * (x_len-1) + i] > 0) {
				delta = RecordResult_FWMutual_X_Delta[j * (x_len-1) + i];
			} else {
				delta = (0 - RecordResult_FWMutual_X_Delta[j * (x_len-1) + i]);
			}

			if (delta > PS_Config_Lmt_FW_Rawdata_X_Delta[j * (x_len-1) + i]) {
				TS_LOG_ERR("%s, [%2d, %2d] Val is [%5d] ,limit is[0, %5d]", CSV_TP_CAP_RAW_DELTA_X, i, j, delta, PS_Config_Lmt_FW_Rawdata_X_Delta[j * (x_len-1) + i]);
				retval = -1;
			}
			printk("%5d, ", delta);
		}
		printk("\n");
	}
	printk("\n");


	//---Check Y Delta---
	for (j = 0; j < x_len; j++) {
		for (i = 0; i < (y_len-1); i++) {
			RecordResult_FWMutual_Y_Delta[i * x_len + j] = (rawdata[i * x_len + j] - rawdata[(i+1) * x_len + j]);
		}
	}

	printk("%s:RawData_Y_Delta:\n", __func__);
	for (j = 0; j < (y_len-1); j++) {
		for (i = 0; i < x_len; i++) {
			if(RecordResult_FWMutual_Y_Delta[j * x_len + i] > 0) {
				delta = RecordResult_FWMutual_Y_Delta[j * x_len + i];
			} else {
				delta = (0 - RecordResult_FWMutual_Y_Delta[j * x_len + i]);
			}

			if (delta > PS_Config_Lmt_FW_Rawdata_Y_Delta[j * x_len + i]) {
				TS_LOG_ERR("%s, [%2d, %2d] Val is [%5d] ,limit is[0, %5d]", CSV_TP_CAP_RAW_DELTA_Y, i, j, delta, PS_Config_Lmt_FW_Rawdata_Y_Delta[j * (x_len-1) + i]);
				retval = -1;
			}
			printk("%5d, ", delta);
		}
		printk("\n");
	}
	printk("\n");

exit:
	if (rawdata_up)
		kfree(rawdata_up);
	if (rawdata_low)
		kfree(rawdata_low);

	TS_LOG_INFO("%s:--\n", __func__);

	//---Return Result---
	return retval;
}

/*******************************************************
Description:
	Novatek touchscreen raw data test function.

return:
	Executive outcomes. 0---passed. negative---failed.
*******************************************************/
static int32_t RawDataTest_Sub(int32_t rawdata[], uint8_t RecordResult[], uint8_t x_ch, uint8_t y_ch, int32_t Rawdata_Limit_Postive, int32_t Rawdata_Limit_Negative)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t iArrayIndex = 0;
	int32_t iTolLowBound = 0;
	int32_t iTolHighBound = 0;
	bool isAbsCriteria = false;
	bool isPass = true;
	uint64_t rawdata_size = TEST_LMT_SIZE;

	struct get_csv_data *rawdata_up = NULL;
	struct get_csv_data *rawdata_low = NULL;

	TS_LOG_INFO("%s:++\n", __func__);

	if (FLAG_EXIST == nvt_ts->test_capacitance_via_csvfile && PASS == nvt_parse_csvifle_ready) {
		if (rawdata_size <= 0) {
			TS_LOG_ERR("%s, tx or rx is zero\n", __func__);
			isPass = false;
			goto exit;
		}
		rawdata_up = kzalloc(rawdata_size*sizeof(int32_t)+sizeof(struct get_csv_data), GFP_KERNEL);
		rawdata_low = kzalloc(rawdata_size*sizeof(int32_t)+sizeof(struct get_csv_data), GFP_KERNEL);

		if (!rawdata_up || !rawdata_low) {
			TS_LOG_ERR("%s: malloc rawdata up or low failed\n", __func__);
			isPass = false;
			goto exit;
		}
		rawdata_up->size = rawdata_size;
		rawdata_low->size = rawdata_size;
		if (FAIL == nvt_get_threshold_from_csvfile(TEST_LMT_SIZE, TEST_LMT_SIZE, parse_target_name_up, rawdata_up) ||
			FAIL == nvt_get_threshold_from_csvfile(TEST_LMT_SIZE, TEST_LMT_SIZE, parse_target_name_low, rawdata_low)) {
			TS_LOG_ERR("%s: get threshold from csvfile failed\n", __func__);
			isPass = false;
			goto exit;
		} else {
			Rawdata_Limit_Postive = *(rawdata_up->csv_data);
			Rawdata_Limit_Negative = *(rawdata_low->csv_data);
		}
	}

	if ((Rawdata_Limit_Postive != 0) || (Rawdata_Limit_Negative != 0))
		isAbsCriteria = true;

	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			iArrayIndex = j * x_ch + i;

			RecordResult[iArrayIndex] = 0x00; // default value for PASS

			if (isAbsCriteria) {
				iTolLowBound = Rawdata_Limit_Negative;
				iTolHighBound = Rawdata_Limit_Postive;
			}

			if(rawdata[iArrayIndex] > iTolHighBound) {
				RecordResult[iArrayIndex] |= 0x01;
				TS_LOG_ERR("%s, [%2d, %2d] Val is [%5d] ,limit is[%5d, %5d]", parse_target_name_up, i, j, rawdata[iArrayIndex], iTolLowBound, iTolHighBound);
			}

			if(rawdata[iArrayIndex] < iTolLowBound) {
				RecordResult[iArrayIndex] |= 0x02;
				TS_LOG_ERR("%s, [%2d, %2d] Val is [%5d] ,limit is[%5d, %5d]", parse_target_name_up, i, j, rawdata[iArrayIndex], iTolLowBound, iTolHighBound);
			}
		}
	}

	//---Check RecordResult---
	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			if (RecordResult[j * x_ch + i] != 0) {
				isPass = false;
				break;
			}
		}
	}

exit:
	if (rawdata_up)
		kfree(rawdata_up);
	if (rawdata_low)
		kfree(rawdata_low);

	if (isPass == false) {
		return -1; // FAIL
	} else {
		return 0; // PASS
	}
}

static int16_t nvt_get_avg(int32_t *p)
{
	int32_t sum=0;
	int i;
	if(!X_Channel || !Y_Channel) {
		TS_LOG_INFO("X_Channel or Y_Channel is 0\n");
	}
	for(i=0; i < (X_Channel*Y_Channel); i++){
	sum +=p[i];
	}

	return (int16_t) (sum / (X_Channel*Y_Channel));
}

static int16_t nvt_get_max(int32_t *p)
{
	int32_t max=INT_MIN;
	int i;

	for (i = 0; i <  (X_Channel*Y_Channel); i++) {
		max = max < p[i] ? p[i] : max;
	}

	return  (int16_t) max;
}

static int16_t nvt_get_min(int32_t *p)
{
	int32_t min=INT_MAX;
	int i;

	for (i = 0; i <  (X_Channel*Y_Channel); i++) {
		min = min >  p[i] ? p[i] : min;
	}

	return  (int16_t) min;
}
static int mmi_add_static_data(void)
{
 	int i;

	i=  strlen (mmitest_result);
	if  (i >=TS_RAWDATA_RESULT_MAX) {
		return -EINVAL;
	}
	snprintf((mmitest_result+i), TS_RAWDATA_RESULT_MAX - i,"[%5d,%5d,%5d]",
   	 nvt_get_avg(RawData_FWMutual), nvt_get_max(RawData_FWMutual),nvt_get_min(RawData_FWMutual));

	i= strlen(mmitest_result);
	if  (i >= TS_RAWDATA_RESULT_MAX) {
		return -EINVAL;
	}
	snprintf((mmitest_result+i), TS_RAWDATA_RESULT_MAX - i,"[%5d,%5d,%5d]",
   	 nvt_get_avg(RawData_Diff), nvt_get_max(RawData_Diff),nvt_get_min(RawData_Diff));

	return 0;

}
/*******************************************************
Description:
	Novatek touchscreen self-test criteria print
	function.

return:
	n.a.
*******************************************************/
static void nvt_print_lmt_array(int32_t *array, int32_t x_ch, int32_t y_ch)
{
	int32_t i = 0;
	int32_t j = 0;

	for (j = 0; j < y_ch; j++) {
		for(i = 0; i < x_ch; i++) {
			printk("%5d ", array[j * x_ch + i]);
		}
		printk("\n");
	}
}

static void nvt_print_criteria(void)
{
	uint32_t y = 0;

	TS_LOG_INFO("%s:++\n", __func__);

	//---PS_Config_Lmt_FW_Rawdata---
	printk("PS_Config_Lmt_FW_Rawdata_P:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_Rawdata_P, X_Channel, Y_Channel);
	printk("PS_Config_Lmt_FW_Rawdata_N:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_Rawdata_N, X_Channel, Y_Channel);

	//---PS_Config_Lmt_FW_CC---
	if(nvt_ts->rawdate_pointer_to_pointer) {
		printk("PS_Config_Lmt_FW_CC_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_FW_CC_P_A, X_Channel, Y_Channel);
		printk("PS_Config_Lmt_FW_CC_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_FW_CC_N_A, X_Channel, Y_Channel);
	} else {
		printk("PS_Config_Lmt_FW_CC_P: %5d\n", PS_Config_Lmt_FW_CC_P);
		printk("PS_Config_Lmt_FW_CC_N: %5d\n", PS_Config_Lmt_FW_CC_N);
	}
	//---PS_Config_Lmt_FW_Diff---
	if(nvt_ts->rawdate_pointer_to_pointer) {
		printk("PS_Config_Lmt_FW_Diff_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_FW_Diff_P_A, X_Channel, Y_Channel);
		printk("PS_Config_Lmt_FW_Diff_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_FW_Diff_N_A, X_Channel, Y_Channel);
	} else {
		printk("PS_Config_Lmt_FW_Diff_P: %5d\n", PS_Config_Lmt_FW_Diff_P);
		printk("PS_Config_Lmt_FW_Diff_N: %5d\n", PS_Config_Lmt_FW_Diff_N);
	}

	//---PS_Config_Lmt_FW_Rawdata_Delta---
	printk("PS_Config_Lmt_FW_Rawdata_X_Delta:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_Rawdata_X_Delta, (X_Channel - 1), Y_Channel);
	printk("PS_Config_Lmt_FW_Rawdata_Y_Delta:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_Rawdata_Y_Delta, X_Channel, (Y_Channel - 1));

	//---PS_Config_Lmt_Short_Rawdata---
	if(nvt_ts->rawdate_pointer_to_pointer) {
		printk("PS_Config_Lmt_Short_Rawdata_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_Short_Rawdata_P_A, X_Channel, Y_Channel);
		printk("PS_Config_Lmt_Short_Rawdata_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_Short_Rawdata_N_A, X_Channel, Y_Channel);
	} else {
		printk("PS_Config_Lmt_Short_Rawdata_P: %5d\n", PS_Config_Lmt_Short_Rawdata_P);
		printk("PS_Config_Lmt_Short_Rawdata_N: %5d\n", PS_Config_Lmt_Short_Rawdata_N);
	}

	//---PS_Config_Lmt_Open_Rawdata---
	if (nvt_ts->open_test_by_fw > NO_ERR) {
		printk("PS_Config_Lmt_Open_Rawdata_P_A:\n");
		nvt_print_lmt_array(PS_Config_Lmt_Open_Rawdata_P_A, X_Channel, Y_Channel);
		printk("PS_Config_Lmt_Open_Rawdata_N_A:\n");
		nvt_print_lmt_array(PS_Config_Lmt_Open_Rawdata_N_A, X_Channel, Y_Channel);
	} else {
		printk("PS_Config_Lmt_Open_Rawdata_P: %5d\n", PS_Config_Lmt_Open_Rawdata_P);
		printk("PS_Config_Lmt_Open_Rawdata_N: %5d\n", PS_Config_Lmt_Open_Rawdata_N);
	}
	printk("AIN_X:\n");
	for (y = NO_ERR; y < IC_X_CFG_SIZE; y++) {
		printk("%5u ", AIN_X[y]);
	}
	printk("\n");
	printk("AIN_Y:\n");
	for (y = NO_ERR; y < IC_Y_CFG_SIZE; y++) {
		printk("%5u ", AIN_Y[y]);
	}
	printk("\n");
	//---mADCOper_Cnt---
	printk("mADCOper_Cnt: %5d\n", mADCOper_Cnt);

	TS_LOG_INFO("%s:--\n", __func__);
}

static int set_parse_target_name(const char *up, const char* low)
{
	int len_up = 0;
	int len_low = 0;

	if (!up || !low) {
		TS_LOG_ERR("target name null, set failed\n");
		return FAIL;
	}
	len_up = strlen(up);
	len_low = strlen(low);
	if (len_up >= TARGET_NAME_SIZE || len_low >= TARGET_NAME_SIZE) {
		TS_LOG_ERR("target name is error, to length\n");
		return FAIL;
	}
	memset(parse_target_name_up, '\0', TARGET_NAME_SIZE);
	memset(parse_target_name_low, '\0', TARGET_NAME_SIZE);
	strncpy(parse_target_name_up, up, strlen(up));
	strncpy(parse_target_name_low, low, strlen(low));

	TS_LOG_INFO("%s: parse_target_name_up=%s ,parse_target_name_low=%s \n", __func__, parse_target_name_up, parse_target_name_low);

	return PASS;
}
static void nvt_kit_rawdata_test(struct ts_rawdata_info *info)
{
	if (nvt_read_raw(RawData_FWMutual) != 0) {
		TestResult_FWMutual = 1;
		TS_LOG_INFO("%s: nvt_read_raw ERROR! TestResult_FWMutual=%d\n", __func__, TestResult_FWMutual);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		if (FLAG_EXIST == nvt_ts->test_capacitance_via_csvfile) {
			nvt_parse_csvifle_ready = set_parse_target_name(CSV_TP_CAP_RAW_MAX, CSV_TP_CAP_RAW_MIN);
		}
		TestResult_FWMutual = nvt_rawdata_up_low(RawData_FWMutual, RecordResult_FWMutual, X_Channel, Y_Channel,
													PS_Config_Lmt_FW_Rawdata_P, PS_Config_Lmt_FW_Rawdata_N);
		if (TestResult_FWMutual == -1){
			TS_LOG_INFO("%s: FW RAWDATA TEST FAIL! TestResult_FWMutual=%d\n", __func__, TestResult_FWMutual);
		} else {
			TS_LOG_INFO("%s: FW RAWDATA TEST PASS! TestResult_FWMutual=%d\n", __func__, TestResult_FWMutual);
		}
	}
}

static void nvt_kit_CC_test(struct ts_rawdata_info *info)
{
	if (nvt_read_CC(RawData_FW_CC) != 0) {
		TestResult_FW_CC = 1;
		TS_LOG_INFO("%s: nvt_read_CC ERROR! TestResult_FW_CC=%d\n", __func__, TestResult_FW_CC);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		if (FLAG_EXIST == nvt_ts->test_capacitance_via_csvfile) {
			nvt_parse_csvifle_ready = set_parse_target_name(CSV_TP_CAP_CC_MAX, CSV_TP_CAP_CC_MIN);
		}
		if(nvt_ts->rawdate_pointer_to_pointer) {
			TestResult_FW_CC = nvt_rawdata_up_low(RawData_FW_CC, RecordResult_FW_CC, X_Channel, Y_Channel,
												PS_Config_Lmt_FW_CC_P_A, PS_Config_Lmt_FW_CC_N_A);
		} else {
			TestResult_FW_CC = RawDataTest_Sub(RawData_FW_CC, RecordResult_FW_CC, X_Channel, Y_Channel,
												PS_Config_Lmt_FW_CC_P, PS_Config_Lmt_FW_CC_N);
		}
		if (TestResult_FW_CC == -1){
			TS_LOG_INFO("%s: FW CC TEST FAIL! TestResult_FW_CC=%d\n", __func__, TestResult_FW_CC);
		} else {
			TS_LOG_INFO("%s: FW CC TEST PASS! TestResult_FW_CC=%d\n", __func__, TestResult_FW_CC);
		}
	}
}

static void nvt_kit_delta_test(struct ts_rawdata_info *info)
{
	if (nvt_rawdata_delta(RawData_FWMutual, X_Channel, Y_Channel) != 0) {
		TestResult_FWMutual_Delta = -1;	// -1: FAIL
		TS_LOG_INFO("%s: XY Nearby Delta Test FAIL! TestResult_FWMutual_Delta=%d\n", __func__, TestResult_FWMutual_Delta);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		TestResult_FWMutual_Delta = 0;
		TS_LOG_INFO("%s: XY Nearby Delta Test PASS! TestResult_FWMutual_Delta=%d\n", __func__, TestResult_FWMutual_Delta);
	}
}
static void  nvt_kit_noise_test(struct ts_rawdata_info *info)
{
	int32_t noise_ret = NO_ERR;

	if (nvt_ts->noise_test_frame > NO_ERR) {
		noise_ret = nvt_tddi_read_fw_noise(RawData_Diff);
	} else {
		noise_ret = nvt_read_noise();
	}
	if (noise_ret != NO_ERR) {
		TestResult_Noise = 1;	// 1: ERROR
		TestResult_FW_Diff = 1;
		TS_LOG_INFO("%s: nvt_read_noise ERROR! TestResult_Noise=%d\n", __func__, TestResult_Noise);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		if (FLAG_EXIST == nvt_ts->test_capacitance_via_csvfile) {
			nvt_parse_csvifle_ready = set_parse_target_name(CSV_TP_CAP_DIFF_MAX, CSV_TP_CAP_DIFF_MIN);
		}
		if(nvt_ts->rawdate_pointer_to_pointer) {
			TestResult_FW_Diff = nvt_rawdata_up_low(RawData_Diff, RecordResult_FW_Diff, X_Channel, Y_Channel,
												PS_Config_Lmt_FW_Diff_P_A, PS_Config_Lmt_FW_Diff_N_A);
		} else {
			TestResult_FW_Diff = RawDataTest_Sub(RawData_Diff, RecordResult_FW_Diff, X_Channel, Y_Channel,
												PS_Config_Lmt_FW_Diff_P, PS_Config_Lmt_FW_Diff_N);
		}
		if (TestResult_FW_Diff == -1) {
			TestResult_Noise = -1;
			TS_LOG_INFO("%s: NOISE TEST FAIL! TestResult_Noise=%d\n", __func__, TestResult_Noise);
		} else {
			TestResult_Noise = 0;
			TS_LOG_INFO("%s: NOISE TEST PASS! TestResult_Noise=%d\n", __func__, TestResult_Noise);
		}
	}
}

static void nvt_kit_short_test(struct ts_rawdata_info *info)
{
	if (nvt_read_fw_short(RawData_Short) != 0) {
		TestResult_Short = 1; // 1:ERROR
		TS_LOG_INFO("%s: nvt_read_short ERROR! TestResult_Short=%d\n", __func__, TestResult_Short);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		if (FLAG_EXIST == nvt_ts->test_capacitance_via_csvfile) {
			nvt_parse_csvifle_ready = set_parse_target_name(CSV_TP_CAP_SHORT_MAX, CSV_TP_CAP_SHORT_MIN);
		}
		//---Self Test Check --- // 0:PASS, -1:FAIL
		if(nvt_ts->rawdate_pointer_to_pointer) {
			TestResult_Short = nvt_rawdata_up_low(RawData_Short, RecordResult_Short, X_Channel, Y_Channel,
												PS_Config_Lmt_Short_Rawdata_P_A, PS_Config_Lmt_Short_Rawdata_N_A);
		} else {
			TestResult_Short = RawDataTest_Sub(RawData_Short, RecordResult_Short, X_Channel, Y_Channel,
												PS_Config_Lmt_Short_Rawdata_P, PS_Config_Lmt_Short_Rawdata_N);
		}
		if (TestResult_Short == -1){
			TS_LOG_INFO("%s: SHORT TEST FAIL! TestResult_Short=%d\n", __func__, TestResult_Short);
		} else {
			TS_LOG_INFO("%s: SHORT TEST PASS! TestResult_Short=%d\n", __func__, TestResult_Short);
		}
	}
}

static void nvt_kit_open_test(struct ts_rawdata_info *info, uint8_t x_channel_size, uint8_t y_channel_size)
{
	int32_t open_ret = NO_ERR;

	if (nvt_ts->open_test_by_fw > NO_ERR) {
		open_ret = nvt_tddi_read_fw_open();
	} else {
		open_ret = nvt_read_open();
	}
	//---Open Test---
	if (open_ret != NO_ERR) {
		TestResult_Open = 1;	// 1:ERROR
		TS_LOG_INFO("%s: nvt_read_open ERROR! TestResult_Open=%d\n", __func__, TestResult_Open);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		//---Self Test Check --- // 0:PASS, -1:FAIL
		if (nvt_ts->open_test_by_fw > NO_ERR) {
			if (FLAG_EXIST == nvt_ts->test_capacitance_via_csvfile) {
				nvt_parse_csvifle_ready = set_parse_target_name(CSV_TP_CAP_OPEN_MAX, CSV_TP_CAP_OPEN_MIN);
			}
			TestResult_Open = nvt_rawdata_up_low(RawData_Open, RecordResult_Open, x_channel_size, y_channel_size,
											PS_Config_Lmt_Open_Rawdata_P_A, PS_Config_Lmt_Open_Rawdata_N_A);
		} else {
			TestResult_Open = RawDataTest_Sub(RawData_Open, RecordResult_Open, X_Channel, Y_Channel,
											PS_Config_Lmt_Open_Rawdata_P, PS_Config_Lmt_Open_Rawdata_N);
		}
		if (TestResult_Open == -1){
			TS_LOG_INFO("%s: OPEN TEST FAIL! TestResult_Open=%d\n", __func__, TestResult_Open);
		} else {
			TS_LOG_INFO("%s: OPEN TEST PASS! TestResult_Open=%d\n", __func__, TestResult_Open);
		}
	}
}
/*******************************************************
Description:
	Novatek touchscreen selftest function for huawei_touchscreen.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
#define TP_TEST_FAILED_REASON_LEN 20
static char selftest_failed_reason[TP_TEST_FAILED_REASON_LEN] = { "-software_reason" };
int32_t nvt_kit_selftest(struct ts_rawdata_info *info)
{
	uint8_t buf[8] = {0};
	char test_0_result[4]={0};
	char test_1_result[4]={0};
	char test_2_result[4]={0};
	char test_3_result[4]={0};
	char test_4_result[4]={0};
	int32_t noise_ret = NO_ERR;
	int32_t open_ret = NO_ERR;
	uint8_t x_channel_size = 0;
	uint8_t y_channel_size = 0;

	if (NULL == info || NULL == nvt_ts) {
		TS_LOG_ERR("%s: param error\n", __FUNCTION__);
		return -EINVAL;
	}

	if (NULL == nvt_ts->NvtTddi_X_Channel || NULL == nvt_ts->NvtTddi_Y_Channel) {
		TS_LOG_ERR("%s: param error\n", __FUNCTION__);
		return -EINVAL;
	}

	TS_LOG_INFO("%s: nvt_ts->nvttddi_channel_flag=%d\n", __func__, nvt_ts->nvttddi_channel_flag);
	if (FLAG_EXIST == nvt_ts->nvttddi_channel_flag) {
		if (*(nvt_ts->NvtTddi_X_Channel) > U8_MAX || *(nvt_ts->NvtTddi_Y_Channel) > U8_MAX ) {
			TS_LOG_ERR("%s: data conversion failed!\n", __func__);
			return -EINVAL;
		}else {
			x_channel_size = (uint8_t)*(nvt_ts->NvtTddi_X_Channel);
			y_channel_size = (uint8_t)*(nvt_ts->NvtTddi_Y_Channel);
		}
	} else {
		x_channel_size = IC_X_CFG_SIZE;
		y_channel_size = IC_Y_CFG_SIZE;
	}
	//--Mutex Lock---
	if (mutex_lock_interruptible(&nvt_ts->lock)) {
		TS_LOG_ERR("%s: acquire nvt_ts mutex lock FAIL!", __func__);
		return -ERESTARTSYS;
	}
	//---For Debug : Test Time, Mallon 20160907---
	unsigned long timer_start=0,timer_end=0;
	timer_start=jiffies;
	//---print criteria ,mallon 20161012-----
	if(nvt_ts->print_criteria == true) {
		nvt_print_criteria();
		nvt_ts->print_criteria = false;
	}
	nvt_mp_isInitialed = 0;
	TestResult_Short = 0;
	TestResult_Open = 0;

	//---Get ProjectID to match input & output files---
	//novatek_read_projectid();

	if (nvt_ts->btype == TS_BUS_I2C) {
		//---Test I2C Transfer---
		buf[0] = 0x00;
		if (novatek_ts_kit_read(I2C_FW_Address, buf, 2) < 0) {
			TS_LOG_ERR("%s: I2C READ FAIL!", __func__);
			strcpy(test_0_result, "0F-");
			strcpy(test_1_result, "1F-");
			strcpy(test_2_result, "2F-");
			strcpy(test_3_result, "3F-");
			strcpy(test_4_result, "4F");
			goto err_nvt_i2c_read;
		} else {
			strcpy(test_0_result, "0P-");
		}
	} else if (nvt_ts->btype == TS_BUS_SPI) {
		//---Test SPI Transfer---
		buf[0] = 0x00;
		if (novatek_ts_kit_read(I2C_FW_Address, buf, 2) < 0) {
			TS_LOG_ERR("%s: SPI READ FAIL!", __func__);
			strcpy(test_0_result, "0F-");
			strcpy(test_1_result, "1F-");
			strcpy(test_2_result, "2F-");
			strcpy(test_3_result, "3F-");
			strcpy(test_4_result, "4F");
			goto err_nvt_spi_read;
		} else {
			strcpy(test_0_result, "0P-");
		}
	}

	if (nvt_ts->btype == TS_BUS_SPI) {
		//---Download MP FW---
		if (nvt_kit_fw_update_boot_spi(nvt_ts->fw_name_mp)) {
			TS_LOG_ERR("%s: load MP firmware FAIL!", __func__);
			strcpy(test_1_result, "1F-");
			strcpy(test_2_result, "2F-");
			strcpy(test_3_result, "3F-");
			strcpy(test_4_result, "4F");
			goto err_load_mp_fw;
		}
	}

	//---Reset IC---
	//nvt_bootloader_reset();
	//nvt_check_fw_reset_state(RESET_STATE_REK_FINISH);

	//---Disable FW Frequence Hopping---
	nvt_switch_FreqHopEnDis(FREQ_HOP_DISABLE);
	if (nvt_kit_check_fw_reset_state(RESET_STATE_NORMAL_RUN)) {
		TS_LOG_ERR("%s: nvt_check_fw_reset_state  FAIL!", __func__);
		goto err_nvt_check_fw_reset_state;
	}

	msleep(100);

	//---Enter test mode  ,Mallon 20160928---
        if (nvt_kit_clear_fw_status()) {
		TS_LOG_ERR("%s: nvt_clear_fw_status  FAIL!", __func__);
		goto err_nvt_clear_fw_status;
	}

	nvt_kit_change_mode(MP_MODE_CC);

	if (nvt_kit_check_fw_status()) {
		TS_LOG_ERR("%s: nvt_check_fw_status  FAIL!", __func__);
		goto err_nvt_check_fw_status;
	}

	if (nvt_kit_get_fw_info()) {
		TS_LOG_ERR("%s: nvt_get_fw_info  FAIL!", __func__);
		goto err_nvt_get_fw_info;
	}
	TS_LOG_INFO("%s: FW Version: %d\n", __func__, nvt_fw_ver);

	//---FW Rawdata Test---
	nvt_kit_rawdata_test(info);

	//---FW CC Test---	
	nvt_kit_CC_test(info);

	//--- Result for FW Rawdata & CC Test---
	if ((TestResult_FWMutual != 0) || (TestResult_FW_CC != 0)) {
		strcpy(test_1_result, "1F-");
	} else {
		strcpy(test_1_result, "1P-");
	}

	//---Offset between Columns and Raws---
	//---XY Nearby Delta Test---
	nvt_kit_delta_test(info);

	//--- Result for XY Nearby Delta Test---
	if (TestResult_FWMutual_Delta != 0) {
		strcpy(test_2_result, "2F-");
	} else {
		strcpy(test_2_result, "2P-");
	}
	if (nvt_ts->noise_test_frame > NO_ERR) {
		//---Leave Test Mode before Noise Test---
		nvt_kit_change_mode(NORMAL_MODE);
	}

	TS_LOG_INFO("%s: nvt_ts->noise_test_frame = %d\n", __func__, nvt_ts->noise_test_frame);

	//---Noise Test---
	nvt_kit_noise_test(info);

	//--- Result for Noise Test---
	if (TestResult_Noise != 0) {
		strcpy(test_3_result, "3F-");
	} else {
		strcpy(test_3_result, "3P-");
	}
	if (nvt_ts->noise_test_frame == NO_ERR) {
	//---set FW to Normal Mode for short Test, 20160928----
		nvt_kit_change_mode(NORMAL_MODE);
	}
	//---Short Test (in FW)---
	nvt_kit_short_test(info);

	//---Enable FW Frequence Hopping---
	//invt_switch_FreqHopEnDis(FREQ_HOP_ENABLE);

	TS_LOG_INFO("%s: nvt_ts->open_test_by_fw = %d\n", __func__, nvt_ts->open_test_by_fw);
	//---Open Test---
	nvt_kit_open_test(info,x_channel_size, y_channel_size);

	//--- Result for Open & Short Test---
	if ((TestResult_Short != 0) || (TestResult_Open != 0)) {
		strcpy(test_4_result, "4F");
	} else {
		strcpy(test_4_result, "4P");
	}

	//---Reset IC---
	if (nvt_ts->btype == TS_BUS_I2C) {
		nvt_kit_bootloader_reset();
	} else if (nvt_ts->btype == TS_BUS_SPI) {
		//---Download Normal FW---
		nvt_kit_fw_update_boot_spi(nvt_ts->fw_name);
	}

	uint32_t bytes_of_array;
	//---Copy Data to info->buff---
	if(nvt_ts->criteria_threshold_flag){

		info->buff[0] = x_channel_size;
		info->buff[1] = y_channel_size;
		info->used_size = x_channel_size * y_channel_size * 5  + CHANNEL_NUM;//4:rawdata+diff+short+open

		if(info->used_size < TS_RAWDATA_BUFF_MAX){//buff is enough for rawdata, diff, short and open

			bytes_of_array = x_channel_size*y_channel_size*sizeof(int);
			memcpy(&info->buff[CHANNEL_NUM], RawData_FWMutual, bytes_of_array);

			uint32_t start_p = x_channel_size * y_channel_size + CHANNEL_NUM;
			memcpy(&info->buff[start_p], RawData_Diff, bytes_of_array);

			start_p = x_channel_size * y_channel_size*2 + CHANNEL_NUM;
			memcpy(&info->buff[start_p],RawData_Short,bytes_of_array);

			start_p = x_channel_size * y_channel_size*3 + CHANNEL_NUM;
			memcpy(&info->buff[start_p],RawData_Open,bytes_of_array);

			start_p = x_channel_size * y_channel_size*4 + CHANNEL_NUM;
			memcpy(&info->buff[start_p],RawData_FW_CC,bytes_of_array);
		} else {
			TS_LOG_ERR("%s: info->buff's size is not enough for rawdata, diff, short and open\n",__func__);
		}
	}else{
		info->buff[0] = X_Channel;
		info->buff[1] = Y_Channel;
		info->used_size = X_Channel * Y_Channel * 5  + CHANNEL_NUM;//2:rawdata + diff

		if(info->used_size < TS_RAWDATA_BUFF_MAX){//buff is enough for rawdata, diff, short and open
			bytes_of_array = (X_Channel*(uint64_t)Y_Channel*sizeof(int));
			memcpy(&info->buff[CHANNEL_NUM], RawData_FWMutual, bytes_of_array);

			uint32_t start_p = X_Channel * Y_Channel + CHANNEL_NUM;
			memcpy(&info->buff[start_p], RawData_Diff, bytes_of_array);

			start_p = X_Channel * Y_Channel*2 + CHANNEL_NUM;
			memcpy(&info->buff[start_p],RawData_Short,bytes_of_array);

			start_p = X_Channel * Y_Channel*3 + CHANNEL_NUM;
			memcpy(&info->buff[start_p],RawData_Open,bytes_of_array);

			start_p = X_Channel * Y_Channel*4 + CHANNEL_NUM;
			memcpy(&info->buff[start_p],RawData_FW_CC,bytes_of_array);
		} else {
			TS_LOG_ERR("%s: info->buff's size is not enough for rawdata, diff, short and open\n",__func__);
		}
	}

err_nvt_get_fw_info:
err_nvt_check_fw_status:
err_nvt_clear_fw_status:
err_nvt_check_fw_reset_state:
err_load_mp_fw:
err_nvt_i2c_read:
err_nvt_spi_read:
	//---Mutex Unlock---
	mutex_unlock(&nvt_ts->lock);
	//---Check Fail Reason---
	if((TestResult_Short == -1) || (TestResult_Open == -1) ||
		(TestResult_FWMutual_Delta == -1) || (TestResult_Noise ==-1)||
		TestResult_FW_CC==-1||TestResult_FWMutual==-1)
		strncpy(selftest_failed_reason, "-panel_reason", TP_TEST_FAILED_REASON_LEN);

	//---String Copy---
	memset(mmitest_result, 0, sizeof(mmitest_result));
	strncat(mmitest_result, test_0_result, strlen(test_0_result));
	strncat(mmitest_result, test_1_result, strlen(test_1_result));
	strncat(mmitest_result, test_2_result, strlen(test_2_result));
	strncat(mmitest_result, test_3_result, strlen(test_3_result));
	strncat(mmitest_result, test_4_result, strlen(test_4_result));
	//----add static data in SH ,Mallon 20160928---
	mmi_add_static_data();
	strncat(mmitest_result, ";", strlen(";"));
	if (0 == strlen(mmitest_result) || strstr(mmitest_result, "F")) {
		strncat(mmitest_result, selftest_failed_reason, strlen(selftest_failed_reason));
	}

	 //------change project_id in SH, Mallon 20160928---
	//strncat(mmitest_result, "-novatek-nt36772", strlen("-novatek-nt36772"));
	strncat(mmitest_result, "-novatek_", strlen("-novatek_"));
	strncat(mmitest_result, novatek_kit_project_id, PROJECT_ID_LEN); 
	  
	//---Copy String to Result---
	memcpy(info->result, mmitest_result, strlen(mmitest_result));
	
	//---For Debug : Test Time, Mallon 20160907---
	timer_end = jiffies;
	TS_LOG_INFO("%s: self test time:%d\n", __func__, jiffies_to_msecs(timer_end-timer_start));

	return NO_ERR;
}
