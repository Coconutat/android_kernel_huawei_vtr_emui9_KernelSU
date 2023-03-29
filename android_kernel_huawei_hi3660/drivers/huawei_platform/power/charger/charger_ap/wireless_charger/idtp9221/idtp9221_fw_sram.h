#ifndef _IDT9221_FW_SRAM_H
#define _IDT9221_FW_SRAM_H

#include <idtp9221_fw_sram_020b.h>
#include <idtp9221_txfw_sram_0314.h>


#include <huawei_platform/power/wireless_charger.h>

struct fw_update_info {
	const enum wireless_mode fw_sram_mode; //TX_SRAM or RX_SRAM
	const char *name_fw_update_from; //SRAM update, from which OTP firmware version
	const char *name_fw_update_to; //SRAM update, to which OTP firmware version
	const unsigned char *fw_sram; //SRAM
	const unsigned int fw_sram_size;
	const u16 fw_sram_update_addr;
};
const struct fw_update_info fw_update[] = {
	{
		.fw_sram_mode		 = WIRELESS_RX_MODE,
		.name_fw_update_from = IDT9221_OTP_FW_VERSION_020BH,
		.name_fw_update_to   = IDT9221_OTP_FW_VERSION_020BH,
		.fw_sram             = idt_fw_sram_020bh,
		.fw_sram_size        = ARRAY_SIZE(idt_fw_sram_020bh),
		.fw_sram_update_addr = IDT9221_RX_SRAMUPDATE_ADDR,
	},
	{
		.fw_sram_mode        = WIRELESS_TX_MODE,
		.name_fw_update_from = IDT9221_OTP_FW_VERSION_030FH,
		.name_fw_update_to   = IDT9221_OTP_FW_VERSION_030FH,
		.fw_sram             = idt_txfw_sram_0314h,
		.fw_sram_size        = ARRAY_SIZE(idt_txfw_sram_0314h),
		.fw_sram_update_addr = IDT9221_TX_SRAMUPDATE_ADDR,
	},
};

#endif
