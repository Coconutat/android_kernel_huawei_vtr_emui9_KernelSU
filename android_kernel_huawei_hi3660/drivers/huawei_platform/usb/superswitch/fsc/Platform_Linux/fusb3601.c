#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/param.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <asm/irq.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
//#include <linux/hisi/usb/hisi_usb.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
//#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#include <linux/wakelock.h>
#include <huawei_platform/log/hw_log.h>
//#include <huawei_platform/power/huawei_charger.h>
#ifdef CONFIG_DIRECT_CHARGER
//#include <huawei_platform/power/direct_charger.h>
#endif
#include "fusb3601.h"
#include "FSCTypes.h"
#include "platform_helpers.h"

#ifndef HWLOG_TAG
#define HWLOG_TAG fusb3601_scp
HWLOG_REGIST();
#endif
//FSC_BOOL fusb_I2C_WriteData(FSC_U8 address, FSC_U8 length, FSC_U8* data);

//FSC_BOOL fusb_I2C_ReadData(FSC_U8 address, FSC_U8* data);
//int fusb_i2c_write_mask(u8 reg, u8 MASK, u8 SHIFT, u8 value)
struct work_struct g_scp_work;
#define FUSB3601_ACK_CRCRX  (1<<1)
#define FUSB3601_ACK_PARRX  (1<<0)

int fusb3601_scp_init(void)
{
	fusb_i2c_write_mask(0x80, 1<<4, 5, 0);
	fusb_i2c_write_mask(0x80, 1<<2, 3, 0);
	fusb_i2c_write_mask(0x80, 1<<5, 5, 0);
}
int fusb3601_scp_cmd_transfer_check(void)
{
	u8 reg_val1 = 0,reg_val2 = 0,reg_val3 = 0,i = 0;
	int ret;
	/*read accp interrupt registers until value is not zero */
	do {
		msleep(10);
		ret = fusb_I2C_ReadData(FUSB3601_SCP_EVENT_1,&reg_val1);
		ret = fusb_I2C_ReadData(FUSB3601_SCP_EVENT_2,&reg_val2);
		ret = fusb_I2C_ReadData(FUSB3601_SCP_EVENT_3,&reg_val3);
		i++;
	} while(i < 10 && reg_val1 == 0 && reg_val2 == 0 && reg_val3 == 0);

	if(reg_val1== 0 && reg_val2 == 0 && reg_val3 == 0)
	{
		hwlog_info("%s : read accp interrupt time out,total time is %d ms\n",__func__,i*10);
	}
	if(reg_val1 < 0 || reg_val2 < 0 || reg_val3 < 0)
	{
		hwlog_err("%s: read  error!!! reg_val1=%d,reg_val2=%d reg_val2=%d \n", __func__, reg_val1,reg_val2,reg_val3);
		return -1;
	}
	hwlog_info("%s:reg_val1=%d,reg_val2=%d reg_val2=%d \n", __func__, reg_val1,reg_val2,reg_val3);

	/*if something  changed print reg info */
	//if(reg_val2 & (FAS9685_PROSTAT | FAS9685_DSDVCSTAT))
	//{
		//hwlog_info("%s : ACCP state changed  ,reg[0x86]=0x%x,reg[0x87]=0x%x,reg[0x88]=0x%x\n",__func__,reg_val1,reg_val2,reg_val3);
	//}

	/* judge if cmd transfer success */
	if(!((reg_val3 & FUSB3601_ACK_CRCRX)||(reg_val3 & FUSB3601_ACK_PARRX)))
	{
		return 0;
	}
	else
	{
		hwlog_err("%s: read  error!!! reg_val1=%d,reg_val2=%d reg_val2=%d \n", __func__, reg_val1,reg_val2,reg_val3);
		return -1;
	}
}
#define FUSB3601_REG_ACCP_CMD   0xa0
#define FUSB3601_REG_ACCP_ADDR  0xa1
#define FUSB3601_SEND_DATA 0xa2
#define FUSB3601_CMD_SBRRD  0x0c
#define FUSB3601_CMD_SBRWR  0x0b
#define FUSB3601_REG_ACCP_DATA  0xc1

int fusb3601_accp_adapter_reg_read(int* val, int reg)
{
	int reg_val1 = 0,reg_val2 = 0,reg_val3 = 0,val1;
	int i=0,ret = 0;
	//mutex_lock(&accp_adaptor_reg_lock);
	//for(i=0;i< FCP_RETRY_MAX_TIMES;i++)
	for(i = 0; i < 3; i++)
	{
		/*before send cmd, read and clear accp interrupt registers */
		ret = fusb_I2C_ReadData(FUSB3601_SCP_EVENT_1,&reg_val1);
		ret = fusb_I2C_ReadData(FUSB3601_SCP_EVENT_2,&reg_val2);
		ret = fusb_I2C_ReadData(FUSB3601_SCP_EVENT_3,&reg_val3);

		ret = fusb_I2C_WriteData(FUSB3601_REG_ACCP_ADDR, 1, reg);
		ret = fusb_I2C_ReadData(FUSB3601_REG_ACCP_ADDR,val1);
		hwlog_err("%s:FUSB3601_REG_ACCP_ADDR = 0x%x,ret = %d\n",__func__,val1,ret);
		ret = fusb_I2C_WriteData(FUSB3601_REG_ACCP_CMD, 1, FUSB3601_CMD_SBRRD);
		ret = fusb_I2C_ReadData(FUSB3601_REG_ACCP_CMD,val1);
		hwlog_err("%s:FUSB3601_REG_ACCP_CMD = 0x%x\n",__func__,val1);
		//ret |= fusb3601_write_reg_mask(FUSB3601_REG_ACCP_CNTL, FUSB3601_ACCP_IS_ENABLE | FAS9685_ACCP_SENDCMD,FAS9685_ACCP_CNTL_MASK);
		//if(ret)
		//{
			//hwlog_err("%s: write error ret is %d \n",__func__,ret);
			//mutex_unlock(&accp_adaptor_reg_lock);
			//return -1;
		//}
		hwlog_err("%s : adapter register read fail times=%d ,register=0x%x,data=0x%x,reg[0x86]=0x%x,reg[0x87]=0x%x,reg[0x88]=0x%x \n",__func__,i,reg,*val,reg_val1,reg_val2,reg_val3);
		/* check cmd transfer success or fail */
		if(0 == fusb3601_scp_cmd_transfer_check())
		{
			/* recived data from adapter */
			ret = fusb_I2C_ReadData(FUSB3601_REG_ACCP_DATA,val);
			hwlog_err("%s:read_val = %d\n",__func__,*val);
			break;
		}

		/* if transfer failed, restart accp protocol */
		//fcp_protocol_restart();
		hwlog_err("%s : adapter register read fail times=%d ,register=0x%x,data=0x%x,reg[0x86]=0x%x,reg[0x87]=0x%x,reg[0x88]=0x%x \n",__func__,i,reg,*val,reg_val1,reg_val2,reg_val3);
	}
	//hwlog_debug("%s : adapter register retry times=%d ,register=0x%x,data=0x%x,reg[0x59]=0x%x,reg[0x5A]=0x%x \n",__func__,i,reg,*val,reg_val1,reg_val2);
	//if(FCP_RETRY_MAX_TIMES == i)
	//{
		//hwlog_err("%s : ack error,retry %d times \n",__func__,i);
		//ret = -1;
	//}
	//else
	//{
		//ret = 0;
	//}
	//mutex_unlock(&accp_adaptor_reg_lock);
	return ret;
}

int fusb3601_accp_adapter_reg_write(int val, int reg)
{
	int reg_val1 = 0,reg_val2 = 0,reg_val3 = 0;
	int i = 0,ret = 0;
	//mutex_lock(&accp_adaptor_reg_lock);
	//for(i=0;i< FCP_RETRY_MAX_TIMES;i++)
	//for(i=0;i< 3;i++)
	{
		/*before send cmd, clear accp interrupt registers */
		ret = fusb_I2C_ReadData(FUSB3601_SCP_EVENT_1,&reg_val1);
		ret = fusb_I2C_ReadData(FUSB3601_SCP_EVENT_2,&reg_val2);
		ret = fusb_I2C_ReadData(FUSB3601_SCP_EVENT_3,&reg_val3);

		//ret |=fusb3601_write_reg(FUSB3601_REG_ACCP_CMD,FUSB3601_CMD_SBRWR);
		//ret |=fusb3601_write_reg(FUSB3601_REG_ACCP_ADDR, reg);
		//ret |=fusb3601_write_reg(FUSB3601_REG_ACCP_DATA, val);
		//ret |=fusb3601_write_reg_mask(FUSB3601_REG_ACCP_CNTL, FUSB3601_ACCP_IS_ENABLE | FAS9685_ACCP_SENDCMD,FAS9685_ACCP_CNTL_MASK);
		//if(ret < 0)
		//{
			//hwlog_err("%s: write error ret is %d \n",__func__,ret);
			//mutex_unlock(&accp_adaptor_reg_lock);
			//return -1;
		//}

		/* check cmd transfer success or fail */
		//if(0 ==fcp_cmd_transfer_check())
		{
			//break;
		}

		/* if transfer failed, restart accp protocol */
		//fcp_protocol_restart();
		hwlog_err("%s : adapter register write fail times=%d ,register=0x%x,data=0x%x,reg[0x59]=0x%x,reg[0x5A]=0x%x \n",__func__,i,reg,val,reg_val1,reg_val2);
	}
	hwlog_debug("%s : adapter register retry times=%d ,register=0x%x,data=0x%x,reg[0x59]=0x%x,reg[0x5A]=0x%x \n",__func__,i,reg,val,reg_val1,reg_val2);

	//if(FCP_RETRY_MAX_TIMES == i)
	//{
		//hwlog_err("%s : ack error,retry %d times \n",__func__,i);
		//ret = -1;
	//}
	//else
	//{
		ret = 0;
	//}
	//mutex_unlock(&accp_adaptor_reg_lock);
	return ret;
}

int test(struct work_struct *work)
{
	int i = 0;
	int val = 0xff;
	int reg_val1;
	int reg_val2;
	int reg_val3;
	int reg_val4;
	hwlog_info("%s\n",__func__);

	//while(1)
	//{
		//msleep(100);
		//reg_val1 = fusb3601_read_reg(0xf7);
		//hwlog_info("%s:0xf7 = 0x%x\n",__func__,reg_val1);
		//reg_val1 = fusb3601_read_reg(0x86);
		//hwlog_info("%s:0x86 = 0x%x\n",__func__,reg_val1);
		//if (reg_val1 & 0xa1)
		//{
			//break;
		//}
	//}
	while(1)
	{
		for (i = 0;i < 10; i++)
		{
			msleep(100);
			hwlog_info("%s:i = %d\n",__func__,i);
		}
		fusb_I2C_ReadData(0x86,reg_val1);
		hwlog_info("%s:0x86 = 0x%x\n",__func__,reg_val1);
		fusb_I2C_ReadData(0x87,reg_val2);
		hwlog_info("%s:0x87 = 0x%x\n",__func__,reg_val2);
		fusb_I2C_ReadData(0x99,reg_val3);
		hwlog_info("%s:0x99 = 0x%x\n",__func__,reg_val3);
		fusb_I2C_ReadData(0x9a,reg_val4);
		hwlog_info("%s:0x9a = 0x%x\n",__func__,reg_val4);
		if (reg_val1 != 0 || reg_val2 != 0 || reg_val3 != 0 || reg_val4 != 0)
		{
			break;
		}
		fusb3601_accp_adapter_reg_read(&val,0xb0);
		hwlog_info("%s:0xb0 = 0x%x\n",__func__,val);
		fusb3601_accp_adapter_reg_read(&val,0xb1);
		hwlog_info("%s:0xb1 = 0x%x\n",__func__,val);
		fusb3601_accp_adapter_reg_read(&val,0xb2);
		hwlog_info("%s:0xb2 = 0x%x\n",__func__,val);
		fusb3601_accp_adapter_reg_read(&val,0xb3);
		hwlog_info("%s:0xb3 = 0x%x\n",__func__,val);
	}
	hwlog_info("%s:0x86 = 0x%x 0x87 = 0x%x 0x99 = 0x%x 0x9a = 0x%x\n",__func__,reg_val1,reg_val2,reg_val3,reg_val4);
}


static int fusb3601_scp_probe(struct platform_device *pdev)
{
	hwlog_info("%s\n",__func__);
	int reg_val1;

	INIT_WORK(&g_scp_work,test);
	schedule_work(&g_scp_work);
	return 0;
}

static int scp_remove(struct platform_device *pdev)
{
	return 0;
}

static void scp_shutdown(struct platform_device *pdev)
{
	return;
}


static int scp_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}



static struct of_device_id scp_match_table[] = {
	{
	 .compatible = "fusb3601,scp",
	 .data = NULL,
	 },
	{
	 },
};

static struct platform_driver scp_driver = {
	.probe = fusb3601_scp_probe,
	.remove = scp_remove,

	.shutdown = scp_shutdown,
	.driver = {
		   .name = "fusb3601_scp",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(scp_match_table),
		   },
};
static int __init scp_init(void)
{
	return platform_driver_register(&scp_driver);
}

static void __exit scp_exit(void)
{
	platform_driver_unregister(&scp_driver);
}

late_initcall(scp_init);
module_exit(scp_exit);
/*lint -restore*/

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(" scp module driver");
MODULE_AUTHOR("HUAWEI Inc");



