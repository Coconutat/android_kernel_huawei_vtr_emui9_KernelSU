#ifndef __LCDKIT_PARSE_H_
#define __LCDKIT_PARSE_H_

#define LCDKIT_CMD_REQ_MAX                  4
#define LCDKIT_CMD_REQ_RX                   0x0001
#define LCDKIT_CMD_REQ_COMMIT               0x0002
#define LCDKIT_CMD_CLK_CTRL                 0x0004
#define LCDKIT_CMD_REQ_UNICAST              0x0008
#define LCDKIT_CMD_REQ_DMA_TPG              0x0040
#define LCDKIT_CMD_REQ_NO_MAX_PKT_SIZE      0x0008
#define LCDKIT_CMD_REQ_LP_MODE              0x0010
#define LCDKIT_CMD_REQ_HS_MODE              0x0020

enum lcdkit_ctrl_op_mode
{
    LCDKIT_DSI_LP_MODE,
    LCDKIT_DSI_HS_MODE,
};

int buf_trans(const char* inbuf, int inlen, char** outbuf, int* outlen);
int buf_int_trans(const char* inbuf, int inlen, uint32_t** outbuf, int* outlen);
void lcdkit_parse_panel_dts(struct device_node* np);
void lcdkit_parse_platform_dts(struct device_node* np,  void* pdata);

#endif
