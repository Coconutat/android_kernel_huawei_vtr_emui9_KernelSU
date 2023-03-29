#ifndef _IAN231_H_
#define _INA231_H_

#ifndef BIT
#define BIT(x)    (1 << (x))
#endif

/* CONFIG reg=0x00, default=0x4127, RW */
#define INA231_CONFIG_REG                0x00

#define INA231_CONFIG_RST_MASK           (BIT(15)) /* reset bit */
#define INA231_CONFIG_RST_SHIFT          (15)
#define INA231_CONFIG_AVG_MASK           (BIT(9) | BIT(10) | BIT(11)) /* averaging mode */
#define INA231_CONFIG_AVG_SHIFT          (9)
#define INA231_CONFIG_VBUSCT_MASK        (BIT(6) | BIT(7) | BIT(8)) /* bus voltage conversion time */
#define INA231_CONFIG_VBUSCT_SHIFT       (6)
#define INA231_CONFIG_VSHUNTCT_MASK      (BIT(3) | BIT(4) | BIT(5)) /* shunt voltage conversion time */
#define INA231_CONFIG_VSHUNTCT_SHIFT     (3)
#define INA231_CONFIG_MODE_MASK          (BIT(0) | BIT(1) | BIT(2)) /* operating mode */
#define INA231_CONFIG_MODE_SHIFT         (0)

#define INA231_CONFIG_RST_ENABLE         (1)

#define INA231_CONFIG_AVG_NUM_1          (0x0)
#define INA231_CONFIG_AVG_NUM_4          (0x1)
#define INA231_CONFIG_AVG_NUM_16         (0x2)
#define INA231_CONFIG_AVG_NUM_64         (0x3)
#define INA231_CONFIG_AVG_NUM_128        (0x4)
#define INA231_CONFIG_AVG_NUM_256        (0x5)
#define INA231_CONFIG_AVG_NUM_512        (0x6)
#define INA231_CONFIG_AVG_NUM_1024       (0x7)

#define INA231_CONFIG_CT_140US           (0x0)
#define INA231_CONFIG_CT_204US           (0x1)
#define INA231_CONFIG_CT_332US           (0x2)
#define INA231_CONFIG_CT_588US           (0x3)
#define INA231_CONFIG_CT_1100US          (0x4)
#define INA231_CONFIG_CT_2116US          (0x5)
#define INA231_CONFIG_CT_4156US          (0x6)
#define INA231_CONFIG_CT_8244US          (0x7)

#define INA231_CONFIG_MODE_PD0           (0x0) /* power-down */
#define INA231_CONFIG_MODE_SV_TRIG       (0x1) /* shunt voltage, triggered */
#define INA231_CONFIG_MODE_BV_TRIG       (0x2) /* bus voltage, triggered */
#define INA231_CONFIG_MODE_SB_TRIG       (0x3) /* shunt and bus, triggered */
#define INA231_CONFIG_MODE_PD1           (0x4) /* power-down */
#define INA231_CONFIG_MODE_SV_CONS       (0x5) /* shunt voltage, continuous */
#define INA231_CONFIG_MODE_BV_CONS       (0x6) /* bus voltage, continuous */
#define INA231_CONFIG_MODE_SB_CONS       (0x7) /* shunt and bus, continuous */

/* SHUNT_VOLTAGE reg=0x01, default=0x0000, R */
#define INA231_SHUNT_VOLTAGE_REG         0x01

/* BUS_VOLTAGE reg=0x02, default=0x0000, R */
#define INA231_BUS_VOLTAGE_REG           0x02

#define INA231_BUS_VOLTAGE_STEP          (125) /* 25mv */

/* POWER reg=0x03, default=0x0000, R */
#define INA231_POWER_REG                 0x03

/* CURRENT reg=0x04, default=0x0000, R */
#define INA231_CURRENT_REG               0x04

/* CALIBRATION reg=0x05, default=0x0000, RW */
#define INA231_CALIBRATION_REG           0x05

/* MASK_ENABLE reg=0x06, default=0x0000, RW */
#define INA231_MASK_ENABLE_REG           0x06

#define INA231_MASK_ENABLE_SOL_MASK      (BIT(15)) /* shunt voltage over-voltage */
#define INA231_MASK_ENABLE_SOL_SHIFT     (15)
#define INA231_MASK_ENABLE_SUL_MASK      (BIT(14)) /* shunt voltage under-voltage */
#define INA231_MASK_ENABLE_SUL_SHIFT     (14)
#define INA231_MASK_ENABLE_BOL_MASK      (BIT(13)) /* bus voltage over-voltage */
#define INA231_MASK_ENABLE_BOL_SHIFT     (13)
#define INA231_MASK_ENABLE_BUL_MASK      (BIT(12)) /* bus voltage under-voltage */
#define INA231_MASK_ENABLE_BUL_SHIFT     (12)
#define INA231_MASK_ENABLE_POL_MASK      (BIT(11)) /* over-limit power */
#define INA231_MASK_ENABLE_POL_SHIFT     (11)
#define INA231_MASK_ENABLE_CNVR_MASK     (BIT(10)) /* conversion ready */
#define INA231_MASK_ENABLE_CNVR_SHIFT    (10)
#define INA231_MASK_ENABLE_AFF_MASK      (BIT(4)) /* alert function flag */
#define INA231_MASK_ENABLE_AFF_SHIFT     (4)
#define INA231_MASK_ENABLE_CVRF_MASK     (BIT(3)) /* conversion ready flag */
#define INA231_MASK_ENABLE_CVRF_SHIFT    (3)
#define INA231_MASK_ENABLE_OVF_MASK      (BIT(2)) /* math overflow flag */
#define INA231_MASK_ENABLE_OVF_SHIFT     (2)
#define INA231_MASK_ENABLE_APOL_MASK     (BIT(1)) /* alert polarity */
#define INA231_MASK_ENABLE_APOL_SHIFT    (1)
#define INA231_MASK_ENABLE_LEN_MASK      (BIT(0)) /* alert altch enable */
#define INA231_MASK_ENABLE_LEN_SHIFT     (0)

/* ALERT_LIMIT reg=0x07, default=0x0000, RW */
#define INA231_ALERT_LIMIT_REG           0x07

/* register count */
#define INA231_MAX_REGS                  (8)

#define INA231_UA_TO_MA                  (1000)
#define INA231_UV_TO_MV                  (1000)
#define INA231_NV_TO_MV                  (1000000)

#define INA231_DEVICE_DEFAULT_TEMP       (25)

#define INA231_INIT_FINISH               (1)
#define INA231_NOT_INIT                  (0)
#define INA231_ENABLE_INTERRUPT_NOTIFY   (1)
#define INA231_DISABLE_INTERRUPT_NOTIFY  (0)

struct ina231_config_data {
	u16 config_sleep_in;
	u16 config_reset;
	u16 config_work;
	u16 calibrate_content;
	u16 mask_enable_content;
	u16 alert_limit_content;

	int shunt_voltage_lsb; /* nV/bit */
	int shunt_voltage_max;
	int bus_voltage_lsb; /* uV/bit */
	int bus_voltage_max;
	int current_lsb; /* uA/bit */
	int current_max;
	int power_lsb; /* uW/bit */
	int power_max;
};

struct ina231_device_info {
	struct device *dev;
	struct i2c_client *client;
	struct work_struct irq_work;
	struct nty_data nty_data;
	int gpio_int;
	int irq_int;
	int chip_already_init;

	struct ina231_config_data *config;
};

#endif /* end of _INA231_H_ */
