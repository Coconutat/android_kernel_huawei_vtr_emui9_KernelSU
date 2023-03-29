#include "onewire_phy_pl061_gpio.h"

#define HWLOG_TAG HW_ONEWIRE_PHY
HWLOG_REGIST();

static struct platform_device *phy_pdev;

static onewire_gpio_des pl061_gpio;

static onewire_time_request *ow_trq;

static char gpio_name[] = "onewire_phy_pl061_gpio";

/* private delay -- same as kernel 4.4 __delay -- using it carefully */
static void __hw_delay(unsigned long cycles)
{
	cycles_t start = get_cycles();

	while ((get_cycles() - start) < cycles) ;
}

/*
 *It will set whole bank gpio to a state without checking current gpio bank direction state.
 *Caution:
 *It's only can be use in the safe environment!
 */
static void gpio_direction_output_unsafe(unsigned char value)
{
    writeb(pl061_gpio.gpio_dir_out_data, pl061_gpio.gpio_dir_addr);
    writeb(!!value << (pl061_gpio.offset), pl061_gpio.gpio_data_addr);
}

/*
 *It will set whole bank gpio to a state without checking current gpio bank direction state.
 *Caution:
 *It's only can be use in the safe environment!
 */
static void gpio_direction_input_unsafe(void)
{
    writeb(pl061_gpio.gpio_dir_in_data, pl061_gpio.gpio_dir_addr);
}

/*
 *Set value to gpio value register without checking current gpio direction state.
 *Caution:
 *It's only can be use in the safe environment!
 */
static void gpio_set_value_unsafe(unsigned char value)
{
    writeb(!!value << (pl061_gpio.offset), pl061_gpio.gpio_data_addr);
}

/*
 *Read value from gpio value register without checking current gpio direction state.
 *Caution:
 *It's only can be use in the safe environment!
 */
static int gpio_get_value_unsafe(void)
{
    return !!readb(pl061_gpio.gpio_data_addr);
}

static inline void get_current_gpio_bank_dir(void)
{
    unsigned char current_gpio_bank_dir;

    /* get current gpio bank in/out status */
    current_gpio_bank_dir = readb(pl061_gpio.gpio_dir_addr);

    /* get onewire gpio direction in/out data */
    pl061_gpio.gpio_dir_in_data = current_gpio_bank_dir & (~BIT(pl061_gpio.offset));
    pl061_gpio.gpio_dir_out_data = current_gpio_bank_dir | BIT(pl061_gpio.offset);
}

/* onewire reset operation */
static unsigned char onewire_reset(void)
{
    unsigned char presence;
    unsigned long flags;
    spin_lock_irqsave(pl061_gpio.lock, flags);
    get_current_gpio_bank_dir();
    gpio_direction_output_unsafe(LOW_VOLTAGE);
    __hw_delay(ow_trq->reset_init_low_cycles);
    gpio_set_value_unsafe(HIGH_VOLTAGE);
    gpio_direction_input_unsafe();
    __hw_delay(ow_trq->reset_slave_response_delay_cycles);
    presence = gpio_get_value_unsafe();
    __hw_delay(ow_trq->reset_hold_high_cycles);
    spin_unlock_irqrestore(pl061_gpio.lock, flags);
    return(presence);
}

/* onewire bit operation */
static inline void onewire_read_bit(unsigned char *value)
{
    gpio_direction_output_unsafe(LOW_VOLTAGE);
    __hw_delay(ow_trq->read_init_low_cycles);
    gpio_direction_input_unsafe();
    *value =  (*value) | (gpio_get_value_unsafe() << SHIFT_7);
    __hw_delay(ow_trq->read_residual_cycles);
}

static inline void onewire_write_bit(const unsigned char bitval)
{
    gpio_direction_output_unsafe(LOW_VOLTAGE);
    __hw_delay(ow_trq->write_init_low_cycles);
    gpio_set_value_unsafe(bitval);
    __hw_delay(ow_trq->write_hold_cycles);
    gpio_set_value_unsafe(HIGH_VOLTAGE);
    __hw_delay(ow_trq->write_residual_cycles);
}

/* onewire byte operation */
static unsigned char onewire_read_byte(void)
{
    unsigned char value = 0;
    unsigned long flags;
    spin_lock_irqsave(pl061_gpio.lock, flags);
    get_current_gpio_bank_dir();
    onewire_read_bit(&value);
    value >>= SHIFT_1;
    onewire_read_bit(&value);
    value >>= SHIFT_1;
    onewire_read_bit(&value);
    value >>= SHIFT_1;
    onewire_read_bit(&value);
    value >>= SHIFT_1;
    onewire_read_bit(&value);
    value >>= SHIFT_1;
    onewire_read_bit(&value);
    value >>= SHIFT_1;
    onewire_read_bit(&value);
    value >>= SHIFT_1;
    onewire_read_bit(&value);
    /* set gpio output high for powering when eeprom programing or hmac computation */
    gpio_direction_output_unsafe(HIGH_VOLTAGE);
    spin_unlock_irqrestore(pl061_gpio.lock, flags);
    return(value);
}

static void onewire_write_byte(const unsigned char val)
{
    unsigned long flags;
    spin_lock_irqsave(pl061_gpio.lock, flags);
    get_current_gpio_bank_dir();
    onewire_write_bit(val & BIT_0);
    onewire_write_bit(val & BIT_1);
    onewire_write_bit(val & BIT_2);
    onewire_write_bit(val & BIT_3);
    onewire_write_bit(val & BIT_4);
    onewire_write_bit(val & BIT_5);
    onewire_write_bit(val & BIT_6);
    onewire_write_bit(val & BIT_7);
    spin_unlock_irqrestore(pl061_gpio.lock, flags);
    return;
}

/* wait for onewire IC process  */
static void onewire_wait_for_ic(unsigned int ms)
{
    msleep(ms);
}

/* set up onewire time request */
static void onewire_set_time_request(onewire_time_request *ic_ow_trq)
{
    if(ic_ow_trq) {
        ow_trq = ic_ow_trq;
        hwlog_info("Timer Frequence is %u, Hz is %u, Loops per jiffy is %lu\n",
                   arch_timer_get_cntfrq(), HZ, loops_per_jiffy);
        hwlog_info("reset_init_low_cycles = %ucycles(%uns),"
                   "reset_slave_response_delay_cycles = %ucycles(%uns),"
                   "reset_hold_high_cycles = %ucycles(%uns)\n",
                   ow_trq->reset_init_low_cycles, ow_trq->reset_init_low_ns,
                   ow_trq->reset_slave_response_delay_cycles, ow_trq->reset_slave_response_delay_ns,
                   ow_trq->reset_hold_high_cycles, ow_trq->reset_hold_high_ns);
        hwlog_info("write_init_low_cycles = %ucycles(%uns),"
                   "write_hold_cycles = %ucycles(%uns),"
                   "write_residual_cycles = %ucycles(%uns)\n",
                   ow_trq->write_init_low_cycles, ow_trq->write_init_low_ns,
                   ow_trq->write_hold_cycles, ow_trq->write_hold_ns,
                   ow_trq->write_residual_cycles, ow_trq->write_residual_ns);
        hwlog_info("read_init_low_cycles = %ucycles(%uns),"
                   "read_residual_cycles = %ucycles(%uns)\n",
                   ow_trq->read_init_low_cycles, ow_trq->read_init_low_ns,
                   ow_trq->read_residual_cycles, ow_trq->read_residual_ns);
    }else {
        hwlog_err("NULL point ic_ow_trq passed to %s.\n", __func__);
    }
}

static int pl061_gpio_register(onewire_phy_ops * ow_phy_ops, unsigned int id)
{
    unsigned int myid;
    int ret;

    ret = of_property_read_u32(phy_pdev->dev.of_node, "phandle", &myid);
    if(ret) {
        hwlog_err("This physic controller is loaded, but never used!(phandle:%d, func:%s)",
                  ret, __func__);
        return ONEWIRE_PHY_MATCH_FAIL;
    }

    if( myid == id ) {
        ow_phy_ops->reset            = onewire_reset;
        ow_phy_ops->read_byte        = onewire_read_byte;
        ow_phy_ops->write_byte       = onewire_write_byte;
        ow_phy_ops->set_time_request = onewire_set_time_request;
        ow_phy_ops->wait_for_ic      = onewire_wait_for_ic;
        return ONEWIRE_PHY_SUCCESS;
    }

    return ONEWIRE_PHY_MATCH_FAIL;
}

static ow_phy_reg_list pl061_gpio_reg_node;

static int onewire_phy_pl061_gpio_driver_probe(struct platform_device *pdev)
{
    int ret;
    int gpio_id;
    struct device_node *gpio_chip_np;
    struct gpio_desc *desc;
    struct gpio_chip *chip;
    struct pl061_gpio *pl061_chip;
    u32 gpio_reg_property[GPIO_REG_PROPERTY_SIZE];
    volatile void *gpio_virtual_base_addr;

    hwlog_info("onewire_phy: pl061 gpio is probing...\n");

    phy_pdev = pdev;

    /* get onewire gpio description from device tree */
    gpio_chip_np = of_parse_phandle(pdev->dev.of_node, "onewire-gpio", GPIO_CHIP_PHANDLE_INDEX);
    ONEWIRE_PHY_NULL_POINT(gpio_chip_np);
    ret = of_property_read_u32_array(gpio_chip_np, "reg", gpio_reg_property,
                                     GPIO_REG_PROPERTY_SIZE);
    ONEWIRE_PHY_DTS_READ_ERROR("gpio_reg_property");
    pl061_gpio.gpio_phy_base_addr = gpio_reg_property[ADDRESS_HIGH32BIT];
    pl061_gpio.gpio_phy_base_addr <<= SHIFT_32;
    pl061_gpio.gpio_phy_base_addr += gpio_reg_property[ADDRESS_LOW32BIT];
    pl061_gpio.length = gpio_reg_property[LENGTH_HIGH32BIT];
    pl061_gpio.length <<= SHIFT_32;
    pl061_gpio.length += gpio_reg_property[LENGTH_LOW32BIT];
    ret = of_property_read_u32_index(pdev->dev.of_node, "onewire-gpio", ONEWIRE_GPIO_OFFSET_INDEX,
                                     &pl061_gpio.offset);
    ONEWIRE_PHY_DTS_READ_ERROR("offset");

    /* get virtual address from physic address */
    gpio_virtual_base_addr = devm_ioremap(&pdev->dev, pl061_gpio.gpio_phy_base_addr,
                                          pl061_gpio.length);
    ONEWIRE_PHY_NULL_POINT(gpio_virtual_base_addr);
    pl061_gpio.gpio_data_addr = gpio_virtual_base_addr + BIT(pl061_gpio.offset + PL061_DATA_OFFSET);
    pl061_gpio.gpio_dir_addr = gpio_virtual_base_addr + PL061_DIR_OFFSET;

    /* get gpio id */
    gpio_id = of_get_named_gpio(pdev->dev.of_node, "onewire-gpio", GPIO_INDEX);
    if (gpio_id < 0) {
        hwlog_err("No gpio named onewire-gpio required by %s.",  __func__);
        return ONEWIRE_GPIO_FAIL;
    }

    /*
     *Cation:
     *Kernel version sensitive!
     *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     *!!!!!!!!!!!!!!!!!!!!!!!!Kernel Version Sensitive!!!!!!!!!!!!!!!!!!!!!!!!
     *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     */
    desc = gpio_to_desc(gpio_id);
    ONEWIRE_PHY_NULL_POINT(desc);
    chip = gpiod_to_chip(desc);
    ONEWIRE_PHY_NULL_POINT(chip);
    pl061_chip = container_of(chip, struct pl061_gpio, gc);
    ONEWIRE_PHY_NULL_POINT(pl061_chip);
    pl061_gpio.lock = &pl061_chip->lock;
    ONEWIRE_PHY_NULL_POINT(pl061_gpio.lock);
    ret = devm_gpio_request(&pdev->dev, gpio_id, gpio_name);
    if(ret) {
        hwlog_err("Gpio request failed(%d) in %s.", gpio_id, __func__);
        return ONEWIRE_GPIO_FAIL;
    }

    pl061_gpio_reg_node.onewire_phy_register = pl061_gpio_register;
    list_add_tail(&pl061_gpio_reg_node.node, &ow_phy_reg_head);

    hwlog_info("onewire_phy: pl061 gpio probed success.\n");

    return ONEWIRE_PHY_SUCCESS;
}

static int  onewire_phy_pl061_gpio_driver_remove(struct platform_device *pdev)
{
    return ONEWIRE_PHY_SUCCESS;
}

static struct of_device_id onewire_phy_pl061_gpio_match_table[] = {
    {
        .compatible = "onewire-phy,pl061,gpio",
    },
    { /*end*/},
};

static struct platform_driver onewire_phy_pl061_gpio_driver = {
    .probe		= onewire_phy_pl061_gpio_driver_probe,
    .remove		= onewire_phy_pl061_gpio_driver_remove,
    .driver		= {
        .name = "onewire_phy_pl061_gpio",
        .owner = THIS_MODULE,
        .of_match_table = onewire_phy_pl061_gpio_match_table,
    },
};

int __init onewire_phy_pl061_gpio_driver_init(void)
{
    hwlog_info("onewire_phy: pl061 gpio driver init...\n");
    return platform_driver_register(&onewire_phy_pl061_gpio_driver);
}

void __exit onewire_phy_pl061_gpio_driver_exit(void)
{
    hwlog_info("onewire_phy: pl061 gpio driver exit...\n");
    platform_driver_unregister(&onewire_phy_pl061_gpio_driver);
}

subsys_initcall_sync(onewire_phy_pl061_gpio_driver_init);
module_exit(onewire_phy_pl061_gpio_driver_exit);

MODULE_LICENSE("GPL");
