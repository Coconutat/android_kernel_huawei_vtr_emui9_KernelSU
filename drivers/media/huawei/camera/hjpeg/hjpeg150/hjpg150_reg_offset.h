


#ifndef __HJPG150_REG_OFFSET_H__
#define __HJPG150_REG_OFFSET_H__

/******************************************************************************/
/*                      HiStarISP JPGENC Registers' Definitions                            */
/******************************************************************************/

#define JPGENC_JPE_ENCODE_REG            0x4   /* Encode enable */
#define JPGENC_JPE_INIT_REG              0x8   /* SW force update update */
#define JPGENC_JPE_ENC_HRIGHT1_REG       0x18  /* JPEG codec horizontal image size for encoding */
#define JPGENC_JPE_ENC_VBOTTOM_REG       0x1C  /* JPEG codec vertical image size for encoding */
#define JPGENC_JPE_PIC_FORMAT_REG        0x20  /* JPEG picture encoding format */
#define JPGENC_JPE_RESTART_INTERVAL_REG  0x24  /* restart marker insertion register */
#define JPGENC_JPE_TQ_Y_SELECT_REG       0x28  /* Q-table selector 0, quant table for Y component */
#define JPGENC_JPE_TQ_U_SELECT_REG       0x2C  /* Q-table selector 1, quant table for U component */
#define JPGENC_JPE_TQ_V_SELECT_REG       0x30  /* Q-table selector 2, quant table for V component */
#define JPGENC_JPE_DC_TABLE_SELECT_REG   0x34  /* Huffman table selector for DC values */
#define JPGENC_JPE_AC_TABLE_SELECT_REG   0x38  /* Huffman table selector for AC value */
#define JPGENC_JPE_TABLE_DATA_REG        0x3C  /* table programming register */
#define JPGENC_JPE_TABLE_ID_REG          0x40  /* table programming select register */
#define JPGENC_JPE_TAC0_LEN_REG          0x44  /* Huffman AC table 0 length */
#define JPGENC_JPE_TDC0_LEN_REG          0x48  /* Huffman DC table 0 length */
#define JPGENC_JPE_TAC1_LEN_REG          0x4C  /* Huffman AC table 1 length */
#define JPGENC_JPE_TDC1_LEN_REG          0x50  /* Huffman DC table 1 length */
#define JPGENC_JPE_ENCODER_MODE_REG      0x60  /* encode mode */
#define JPGENC_JPE_DEBUG_REG             0x64  /* debug information register */
#define JPGENC_JPE_ERROR_IMR_REG         0x68  /* JPEG error interrupt mask register */
#define JPGENC_JPE_ERROR_RIS_REG         0x6C  /* JPEG error raw interrupt status register */
#define JPGENC_JPE_ERROR_MIS_REG         0x70  /* JPEG error masked interrupt status register */
#define JPGENC_JPE_ERROR_ICR_REG         0x74  /* JPEG error interrupt clear regisger */
#define JPGENC_JPE_ERROR_ISR_REG         0x78  /* JPEG error interrupt set register */
#define JPGENC_JPE_STATUS_IMR_REG        0x7C  /* JPEG status interrupt mask register */
#define JPGENC_JPE_STATUS_RIS_REG        0x80  /* JPEG status raw interrupt status register */
#define JPGENC_JPE_STATUS_MIS_REG        0x84  /* JPEG status masked interrupt status register */
#define JPGENC_JPE_STATUS_ICR_REG        0x88  /* JPEG status interrupt clear register */
#define JPGENC_JPE_STATUS_ISR_REG        0x8C  /* JPEG status interrrupt set register */
#define JPGENC_JPE_CONFIG_REG            0x90  /* JPEG configuration register */
#define JPGENC_ADDRESS_Y_REG             0x94  /* Y Buffer address */
#define JPGENC_ADDRESS_UV_REG            0x98  /* UV Buffer addresss */
#define JPGENC_STRIDE_REG                0x9C  /* Address stride in bytes */
#define JPGENC_SYNCCFG_REG               0x100 /* Source Synchronization configuration */
#define JPGENC_JPE_ENC_HRIGHT2_REG       0x104 /* Picture from pipe2 Hsize */
#define JPGENC_JPG_BYTE_CNT_REG          0x108 /* Number of encoded bytes sent to video port */
#define JPGENC_PREFETCH_REG              0x10C /* Prefetch configuration */
#define JPGENC_PREFETCH_IDY0_REG         0x110 /* Prefetch ID configuration (Y Buffer */
#define JPGENC_PREFETCH_IDY1_REG         0x114 /* Prefetch ID configuration (Y Buffer */
#define JPGENC_PREFETCH_IDUV0_REG        0x118 /* Prefetch ID configuration (UV Buffer */
#define JPGENC_PREFETCH_IDUVY_REG        0x11C /* Prefetch ID configuration (UV Buffer */
#define JPGENC_PREREAD_REG               0x120 /* Number of preread MCU configuration */
#define JPGENC_INPUT_SWAP_REG           0x124 /* swap pixel component at input. */
#define JPGENC_FORCE_CLK_ON_CFG_REG      0x130 /* used to force the clock which is generally controlled by HW */
#define JPGENC_DBG_0_REG                 0x200
#define JPGENC_DBG_1_REG                 0x204
#define JPGENC_DBG_2_REG                 0x208
#define JPGENC_DBG_3_REG                 0x20C
#define JPGENC_DBG_4_REG                 0x210
#define JPGENC_DBG_5_REG                 0x214
#define JPGENC_DBG_6_REG                 0x218
#define JPGENC_DBG_7_REG                 0x21C
#define JPGENC_DBG_8_REG                 0x220
#define JPGENC_DBG_9_REG                 0x224
#define JPGENC_DBG_10_REG                0x228
#define JPGENC_DBG_11_REG                0x22C
#define JPGENC_DBG_12_REG                0x230
#define JPGENC_DBG_13_REG                0x234

#endif // __HJPG150_REG_OFFSET_H__
