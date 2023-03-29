#include "lcdkit_bias_bl_utility.h"
#include "lcdkit_bias_ic_common.h"
#include "lcdkit_dbg.h"
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
extern struct dsm_client *lcd_dclient;
#endif
static struct lcdkit_bias_ic_device * plcdkit_bias_ic = NULL;
static char chip_name[LCD_BIAS_IC_NAME_LEN] = "default";
static struct lcd_bias_voltage_info g_bias_config = {0};
struct class *bias_class = NULL;


static int lcdkit_bias_ic_write_byte(struct lcdkit_bias_ic_device *pbias_ic, unsigned char reg, unsigned char data)
{
    int ret = 0;
    ret = i2c_smbus_write_byte_data(pbias_ic->client, reg, data);
	
    if(ret < 0)
    {
        dev_err(&pbias_ic->client->dev, "failed to write 0x%.2x\n", reg);
    }
	
    return ret;
}

int lcdkit_bias_set_voltage(void)
{
    int ret = 0;
#if defined (CONFIG_HUAWEI_DSM)
    static uint32_t s_set_voltage_vsp_count = 0;
    static uint32_t s_set_voltage_vsn_count = 0;
#endif

    if(plcdkit_bias_ic == NULL)
    {
        LCDKIT_ERR("no bias ic is found!\n");
        return -1;
    }
    ret = lcdkit_bias_ic_write_byte(plcdkit_bias_ic, plcdkit_bias_ic->bias_config.vpos_reg, plcdkit_bias_ic->bias_config.vpos_val);
    if(ret < 0)
    {
        LCDKIT_ERR("set bias ic vsp failed!\n");
#if defined (CONFIG_HUAWEI_DSM)
        s_set_voltage_vsp_count++;
        if (s_set_voltage_vsp_count >= DMD_SET_VOLTAGE_VSP_FALI_EXPIRE_COUNT)
        {
            if (lcd_dclient && !dsm_client_ocuppy(lcd_dclient))
            {
                dsm_client_record(lcd_dclient, "set bias ic vsp failed! vpos_reg = 0x%x, vpos_val = 0x%x, chip_name = %s.\n",
                    plcdkit_bias_ic->bias_config.vpos_reg, plcdkit_bias_ic->bias_config.vpos_val, chip_name);
                dsm_client_notify(lcd_dclient, DSM_LCD_BIAS_I2C_ERROR_NO);
                s_set_voltage_vsp_count = 0;
            }
        }
#endif
        return ret;
    }
#if defined (CONFIG_HUAWEI_DSM)
    s_set_voltage_vsp_count = 0;
#endif
    ret = lcdkit_bias_ic_write_byte(plcdkit_bias_ic, plcdkit_bias_ic->bias_config.vneg_reg, plcdkit_bias_ic->bias_config.vneg_val);
    if(ret < 0)
    {
        LCDKIT_ERR("set bias ic vsn failed!\n");
#if defined (CONFIG_HUAWEI_DSM)
        s_set_voltage_vsn_count++;
        if (s_set_voltage_vsn_count >= DMD_SET_VOLTAGE_VSN_FALI_EXPIRE_COUNT)
        {
            if (lcd_dclient && !dsm_client_ocuppy(lcd_dclient))
            {
                dsm_client_record(lcd_dclient, "set bias ic vsn failed! vneg_reg = 0x%x, vneg_val = 0x%x, chip_name = %s.\n",
                    plcdkit_bias_ic->bias_config.vneg_reg, plcdkit_bias_ic->bias_config.vneg_val, chip_name);
                dsm_client_notify(lcd_dclient, DSM_LCD_BIAS_I2C_ERROR_NO);
                s_set_voltage_vsn_count = 0;
            }
        }
#endif
        return ret;
    }
#if defined (CONFIG_HUAWEI_DSM)
    s_set_voltage_vsn_count = 0;
#endif

    if(plcdkit_bias_ic->bias_config.state_mask != 0)
    {
        ret = lcdkit_bias_ic_write_byte(plcdkit_bias_ic, plcdkit_bias_ic->bias_config.state_reg, plcdkit_bias_ic->bias_config.state_val);
        if(ret < 0)
        {
            LCDKIT_ERR("set bias ic state register failed!\n");
            return ret;
        }
    }
	return 0;
}

struct lcd_bias_voltage_info * lcdkit_get_lcd_bias_ic_info(void)
{
    if(NULL == plcdkit_bias_ic)
    {
        return NULL;
    }
    return &plcdkit_bias_ic->bias_config;
}


void lcdkit_bias_ic_propname_cat(char*pdest, char*psrc, int len)
{
    if(NULL == pdest || NULL == psrc)
    {
        return;
    }
    memset(pdest,0,len);
    snprintf(pdest, len, "%s,%s", chip_name, psrc);

    return;
}

int lcdkit_parse_bias_ic_config(struct device_node *np)
{
    char tmp_buf[128] = {0};
    int ret = 0;
    uint32_t tmp_val = 0;

    if(!strlen(chip_name) || !strcmp(chip_name,"default"))
    {
        LCDKIT_ERR("ret is %d  ic_type is %d\n",ret,g_bias_config.ic_type);
        return -1;
    }

    lcdkit_bias_ic_propname_cat(tmp_buf,"lcdkit-bias-type",sizeof(tmp_buf));
    ret = of_property_read_u32(np,tmp_buf,&tmp_val);
    g_bias_config.ic_type = (!ret? (unsigned char)tmp_val : 0);


    lcdkit_bias_ic_propname_cat(tmp_buf,"lcdkit-bias-vpos-reg",sizeof(tmp_buf));
    ret = of_property_read_u32(np,tmp_buf,&tmp_val);
    g_bias_config.vpos_reg = (!ret? (unsigned char)tmp_val : 0);
	
    lcdkit_bias_ic_propname_cat(tmp_buf,"lcdkit-bias-vneg-reg",sizeof(tmp_buf));
    ret = of_property_read_u32(np,tmp_buf,&tmp_val);
    g_bias_config.vneg_reg = (!ret? (unsigned char)tmp_val : 0);

    lcdkit_bias_ic_propname_cat(tmp_buf,"lcdkit-bias-vpos-val",sizeof(tmp_buf));
    ret = of_property_read_u32(np,tmp_buf,&tmp_val);
    g_bias_config.vpos_val = (!ret? (unsigned char)tmp_val : 0);
	
    lcdkit_bias_ic_propname_cat(tmp_buf,"lcdkit-bias-vneg-val",sizeof(tmp_buf));
    ret = of_property_read_u32(np,tmp_buf,&tmp_val);
    g_bias_config.vneg_val = (!ret? (unsigned char)tmp_val : 0);

    lcdkit_bias_ic_propname_cat(tmp_buf,"lcdkit-bias-vpos-mask",sizeof(tmp_buf));
    ret = of_property_read_u32(np,tmp_buf,&tmp_val);
    g_bias_config.vpos_mask = (!ret? (unsigned char)tmp_val : 0);
	
    lcdkit_bias_ic_propname_cat(tmp_buf,"lcdkit-bias-vneg-mask",sizeof(tmp_buf));
    ret = of_property_read_u32(np,tmp_buf,&tmp_val);
    g_bias_config.vneg_mask = (!ret? (unsigned char)tmp_val : 0);

    lcdkit_bias_ic_propname_cat(tmp_buf,"lcdkit-bias-state-reg",sizeof(tmp_buf));
    ret = of_property_read_u32(np,tmp_buf,&tmp_val);
    g_bias_config.state_reg = (!ret? (unsigned char)tmp_val : 0);
	
    lcdkit_bias_ic_propname_cat(tmp_buf,"lcdkit-bias-state-val",sizeof(tmp_buf));
    ret = of_property_read_u32(np,tmp_buf,&tmp_val);
    g_bias_config.state_val = (!ret? (unsigned char)tmp_val : 0);

    lcdkit_bias_ic_propname_cat(tmp_buf,"lcdkit-bias-state-mask",sizeof(tmp_buf));
    ret = of_property_read_u32(np,tmp_buf,&tmp_val);
    g_bias_config.state_mask = (!ret? (unsigned char)tmp_val : 0);

    return 0;
}

static ssize_t bias_vsp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct lcdkit_bias_ic_device *pchip = NULL;
    int ret = 0;
    unsigned char val = 0;

    if(!dev)
    {
        return snprintf(buf, PAGE_SIZE, "dev is null\n");
    }

    pchip = dev_get_drvdata(dev);
    if(!pchip)
    {
        return snprintf(buf, PAGE_SIZE, "data is null\n");
    }
    
    if(pchip->bias_config.ic_type & BIAS_IC_READ_INHIBITION)
    {
        return snprintf(buf, PAGE_SIZE, "read inhibition\n");    
    }

    ret = i2c_smbus_read_byte_data(pchip->client, pchip->bias_config.vpos_reg);
    if(ret < 0)
    {
        goto i2c_error;
    }
	val = (unsigned char)ret;
    return snprintf(buf, PAGE_SIZE, "vsp = 0x%x\n",val);
	
i2c_error:
    return snprintf(buf, PAGE_SIZE, "bias i2c read vsp error\n");
}

static ssize_t bias_vsp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct lcdkit_bias_ic_device *pchip = NULL;
    unsigned char val = 0;
    unsigned int input_val = 0;
	int ret = 0;

    ret = sscanf(buf, "vsp=0x%x", &input_val);
    if(ret < 0)
    {
        LCDKIT_ERR("check input!\n");
        return -EINVAL;
    }
    val = (unsigned char)input_val;
    pchip = dev_get_drvdata(dev);
	if(!pchip)
    {
        return -EINVAL;
    }
	ret = lcdkit_bias_ic_write_byte(pchip, pchip->bias_config.vpos_reg, val);
    if(ret < 0)
    {
        LCDKIT_ERR("bias i2c write vsp register error\n");
        return -EINVAL;
	}
    return size;
}
static DEVICE_ATTR(vsp, S_IRUGO|S_IWUSR, bias_vsp_show, bias_vsp_store);


static ssize_t bias_vsn_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct lcdkit_bias_ic_device *pchip = NULL;
    int ret = 0;
    unsigned char val = 0;

    if(!dev)
    {
        return snprintf(buf, PAGE_SIZE, "dev is null\n");
    }

    pchip = dev_get_drvdata(dev);
    if(!pchip)
    {
        return snprintf(buf, PAGE_SIZE, "data is null\n");
    }

    if(pchip->bias_config.ic_type & BIAS_IC_READ_INHIBITION)
    {
        return snprintf(buf, PAGE_SIZE, "read inhibition\n");    
    }

    ret = i2c_smbus_read_byte_data(pchip->client, pchip->bias_config.vneg_reg);
    if(ret < 0)
    {
        goto i2c_error;
    }
	val = (unsigned char)ret;
    return snprintf(buf, PAGE_SIZE, "vsn = 0x%x\n",val);
	
i2c_error:
    return snprintf(buf, PAGE_SIZE, "bias i2c read vsn error\n");
}

static ssize_t bias_vsn_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct lcdkit_bias_ic_device *pchip = NULL;
    unsigned int input_val = 0;
    unsigned char val = 0;
	int ret = 0;

    ret = sscanf(buf, "vsn=0x%x",&input_val);
    if(ret < 0)
    {
        LCDKIT_ERR("check input!\n");
        return -EINVAL;
    }
	val = (unsigned char)input_val;
    pchip = dev_get_drvdata(dev);
	if(!pchip)
    {
        return -EINVAL;
    }
	ret = lcdkit_bias_ic_write_byte(pchip, pchip->bias_config.vneg_reg, val);
    if(ret < 0)
    {
        LCDKIT_ERR("bias i2c write vsn register error\n");
        return -EINVAL;
	}
    return size;
}
static DEVICE_ATTR(vsn, S_IRUGO|S_IWUSR, bias_vsn_show, bias_vsn_store);

static struct attribute *bias_attributes[] = {
    &dev_attr_vsp.attr,
    &dev_attr_vsn.attr,
    NULL,
};

static const struct attribute_group bias_group = {
    .attrs = bias_attributes,
};

static int lcdkit_bias_ic_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    struct device_node* np = NULL;

    LCDKIT_ERR("enter lcdkit_bias_ic_probe");
    plcdkit_bias_ic = devm_kzalloc(&client->dev, sizeof(struct lcdkit_bias_ic_device) , GFP_KERNEL);

    if (!plcdkit_bias_ic)
    {
        return -EINVAL;
    }

    plcdkit_bias_ic->client = client;
    np = lcdkit_get_lcd_node();
    if(!np)
    {
        devm_kfree(&client->dev, plcdkit_bias_ic);
        plcdkit_bias_ic = NULL;
        return -EINVAL;
    }
    lcdkit_parse_bias_ic_config(np);
    plcdkit_bias_ic->bias_config = g_bias_config;

    if(bias_class)
    {
        plcdkit_bias_ic->dev = device_create(bias_class, NULL, 0, "%s", "lcd_bias");
        if (IS_ERR(plcdkit_bias_ic->dev))
        {
            LCDKIT_ERR("Unable to create device; errno = %ld\n", PTR_ERR(plcdkit_bias_ic->dev));
            plcdkit_bias_ic->dev = NULL;
        } 
        else
        {
            dev_set_drvdata(plcdkit_bias_ic->dev, plcdkit_bias_ic);
            ret = sysfs_create_group(&plcdkit_bias_ic->dev->kobj, &bias_group);
            if (ret)
            {
                LCDKIT_ERR("Create bias sysfs group node failed!\n");
                device_destroy(bias_class,0);
            }
        }
    }
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    set_lcd_bias_dev_flag();
#endif

    return 0;
}

static int lcdkit_bias_ic_remove(struct i2c_client *client)
{
    sysfs_remove_group(&plcdkit_bias_ic->dev->kobj, &bias_group);
    device_destroy(bias_class, 0);
    devm_kfree(&client->dev, plcdkit_bias_ic);
    return 0;
}

static struct i2c_device_id bias_ic_common_id[] = {
    {"bias_ic_common", 0},
    {},
};

MODULE_DEVICE_TABLE(i2c, bias_ic_common_id);

static struct of_device_id lcdkit_bias_ic_match_table[] = {
    { .compatible = "bias_ic_common",},
    {},
};

static struct i2c_driver lcdkit_bias_ic_driver = {
	.probe = lcdkit_bias_ic_probe,
	.remove = lcdkit_bias_ic_remove,
	.driver = {
		.name = "bias_ic_common",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(lcdkit_bias_ic_match_table),
	},
	.id_table = bias_ic_common_id,
};

static int __init lcdkit_bias_ic_module_init(void)
{
    int ret = 0;
    char tmp_name[LCD_BIAS_IC_NAME_LEN] = {0};
	char compatible_name[128] = {0};

	ret = lcdkit_get_bias_ic_name(tmp_name,sizeof(tmp_name));
	if(0 != ret)
    {
        return 0;
    }
    LCDKIT_ERR("lcdkit_bias_ic_module_init chip_name is %s\n",tmp_name);
    if(!strcmp(tmp_name,"default"))
    {
        return 0;
    }

    memset(chip_name,0,LCD_BIAS_IC_NAME_LEN);
    memcpy(chip_name,tmp_name,LCD_BIAS_IC_NAME_LEN);
    chip_name[LCD_BIAS_IC_NAME_LEN-1] = 0;
    memcpy(bias_ic_common_id[0].name, chip_name, LCD_BIAS_IC_NAME_LEN);
    memcpy(lcdkit_bias_ic_match_table[0].compatible, chip_name, LCD_BIAS_IC_NAME_LEN);
    lcdkit_bias_ic_driver.driver.name = chip_name;
    snprintf(compatible_name, sizeof(compatible_name), "huawei,%s", chip_name);
    memcpy(lcdkit_bias_ic_match_table[0].compatible, compatible_name, 128);
    lcdkit_bias_ic_match_table[0].compatible[127] = 0;

    bias_class = class_create(THIS_MODULE, "lcd_bias");
    if (IS_ERR(bias_class))
    {
        LCDKIT_ERR("Unable to create bias class; errno = %ld\n", PTR_ERR(bias_class));
        bias_class = NULL;
    }

    return i2c_add_driver(&lcdkit_bias_ic_driver);
}

static void __exit lcdkit_bias_ic_module_exit(void)
{
    if(NULL != bias_class)
    {
        class_destroy(bias_class);
    }
	i2c_del_driver(&lcdkit_bias_ic_driver);
}

module_init(lcdkit_bias_ic_module_init);
module_exit(lcdkit_bias_ic_module_exit);
MODULE_DESCRIPTION("AWINIC bias ic common driver");
MODULE_LICENSE("GPL v2");
