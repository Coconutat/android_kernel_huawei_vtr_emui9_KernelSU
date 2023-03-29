/* MD5: af4f6fbb9e960dd44fd56278b249f4ec*/
#if !defined(__PRODUCT_CONFIG_GUCPHY_H__)
#define __PRODUCT_CONFIG_GUCPHY_H__

#ifndef ZSP_DSP_CHIP_BB_TYPE		
#define ZSP_DSP_CHIP_BB_TYPE		 12 
#endif 

#ifndef NV_VERSION
#define NV_VERSION nv_history 
#endif 

#ifndef ZSP_DSP_PRODUCT_FORM		
#define ZSP_DSP_PRODUCT_FORM		 4 
#endif 

#ifndef BOARD
#define BOARD ASIC 
#endif 

#ifndef UPHY_BOARD_TYPE
#define UPHY_BOARD_TYPE 2 
#endif 

#ifndef FEATURE_TEMP_MULTI_MODE_LP
#define FEATURE_TEMP_MULTI_MODE_LP FEATURE_ON 
#endif 

#ifndef FEATURE_SRAM_400K
#define FEATURE_SRAM_400K FEATURE_OFF 
#endif 

#ifndef FEATURE_EXTERNAL_32K_CLK		
#define FEATURE_EXTERNAL_32K_CLK		 FEATURE_ON 
#endif 

#ifndef FEATURE_SOCP_ON_DEMAND		
#define FEATURE_SOCP_ON_DEMAND		 FEATURE_OFF 
#endif 

#ifndef FEATURE_TAS				
#define FEATURE_TAS				 FEATURE_ON 
#endif 

#ifndef FEATURE_DC_DPA			
#define FEATURE_DC_DPA			 FEATURE_ON 
#endif 

#ifndef FEATURE_RFIC_RESET_GPIO_ON		
#define FEATURE_RFIC_RESET_GPIO_ON		 FEATURE_OFF 
#endif 

#ifndef FEATURE_VIRTUAL_BAND		
#define FEATURE_VIRTUAL_BAND		 FEATURE_ON 
#endif 

#ifndef FEATURE_HI6363                	
#define FEATURE_HI6363                	 FEATURE_OFF 
#endif 

#ifndef FEATURE_RTT_TEST
#define FEATURE_RTT_TEST FEATURE_ON 
#endif 

#ifndef FEATURE_RTT_RANDOM_TEST
#define FEATURE_RTT_RANDOM_TEST FEATURE_OFF 
#endif 

#ifndef FEATURE_GUTLC_ONE_DSP
#define FEATURE_GUTLC_ONE_DSP FEATURE_OFF 
#endif 

#ifndef FEATURE_NX_CORE_OPEN
#define FEATURE_NX_CORE_OPEN FEATURE_OFF 
#endif 

#ifndef FEATURE_CSDR
#define FEATURE_CSDR FEATURE_ON 
#endif 

#ifndef FEATURE_GSM_SDR			
#define FEATURE_GSM_SDR			 FEATURE_ON 
#endif 

#ifndef FEATURE_GSM_SDR_DAIC		
#define FEATURE_GSM_SDR_DAIC		 FEATURE_OFF 
#endif 

#ifndef FEATURE_XBBE16_NEW_MAIL		
#define FEATURE_XBBE16_NEW_MAIL		 FEATURE_ON 
#endif 

#ifndef CPHY_PUB_DTCM_BASE	
#define CPHY_PUB_DTCM_BASE	 0x72900000 
#endif 

#ifndef CPHY_PUB_ITCM_BASE	
#define CPHY_PUB_ITCM_BASE	 0x72980000 
#endif 

#ifndef CPHY_PRV_DTCM_BASE	
#define CPHY_PRV_DTCM_BASE	 0x49480000 
#endif 

#ifndef CPHY_PRV_ITCM_BASE	
#define CPHY_PRV_ITCM_BASE	 0x49580000 
#endif 

#ifndef CPHY_PUB_DTCM_SIZE		
#define CPHY_PUB_DTCM_SIZE		 0x80000 
#endif 

#ifndef CPHY_PUB_ITCM_SIZE		
#define CPHY_PUB_ITCM_SIZE		 0x80000 
#endif 

#ifndef CPHY_PRV_DTCM_SIZE		
#define CPHY_PRV_DTCM_SIZE		 0x80000 
#endif 

#ifndef CPHY_PRV_ITCM_SIZE		
#define CPHY_PRV_ITCM_SIZE		 0x80000 
#endif 

#ifndef CPHY_PUB_DTCM_GLB_MINUS_LOCAL	
#define CPHY_PUB_DTCM_GLB_MINUS_LOCAL	 (0xE2900000 - 0x72900000) 
#endif 

#ifndef XTENSA_CORE_X_CACHE
#define XTENSA_CORE_X_CACHE v8r5_dallas_cbbe16 
#endif 

#ifndef LD_MAP_PATH             		
#define LD_MAP_PATH             		 hi3660-cphy-asic-bbe16-lsp_ChicagoSFt 
#endif 

#ifndef XTENSA_CORE_X_SYSTEM
#define XTENSA_CORE_X_SYSTEM RD-2012.5 
#endif 

#endif /*__PRODUCT_CONFIG_H__*/ 
