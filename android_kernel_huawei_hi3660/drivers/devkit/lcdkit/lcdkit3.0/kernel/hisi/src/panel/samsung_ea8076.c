/*
*panel adapter
*product:elle
*panel:samsung_ea8076
*/

#define EA8076_BARCODE_2D_LEN  16

static int samsung_ea8076_2d_barcode(char *oem_data, struct hisi_fb_data_type *hisifd)
{
	char read_value[OEM_INFO_SIZE_MAX+1] = {0};
	int ret = 0;
	int i=0;

	if (disp_info->oeminfo.barcode_2d.support) {
		ret = lcd_kit_dsi_cmds_rx(hisifd, read_value, &disp_info->oeminfo.barcode_2d.cmds);
		oem_data[0] = BARCODE_2D_TYPE;
		oem_data[1] = BARCODE_BLOCK_NUM;
		for (i = 0; i < EA8076_BARCODE_2D_LEN; i++){
			oem_data[i + 2] = read_value[i];
		}
	}
	return ret;
}

static struct lcd_kit_panel_ops samsung_ea8076 = {
	.lcd_get_2d_barcode = samsung_ea8076_2d_barcode,
};

int samsung_ea8076_probe(void)
{
	int ret = LCD_KIT_OK;

	ret = lcd_kit_panel_ops_register(&samsung_ea8076);
	if (ret) {
		LCD_KIT_ERR("failed\n");
		return LCD_KIT_FAIL;
	}
	return LCD_KIT_OK;
}
