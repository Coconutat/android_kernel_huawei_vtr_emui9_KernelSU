#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/usb/class-dual-role.h>

#ifdef CONFIG_DUAL_ROLE_USB_INTF

struct otg_dual_role_data {
    struct device *dev;
    struct dual_role_phy_instance *dual_role_ins;
    int dual_state;
};

static enum dual_role_property otg_dual_role_props[] = {
    DUAL_ROLE_PROP_MODE,
    DUAL_ROLE_PROP_PR,
    DUAL_ROLE_PROP_DR,
};

static int otg_dual_role_prop_is_writeable(struct dual_role_phy_instance *drp,
                                enum dual_role_property prop)
{
    if (prop == DUAL_ROLE_PROP_MODE)
        return 1;
    else
        return 0;
}

static int otg_dual_role_get_prop(struct dual_role_phy_instance *dual_role,
                        enum dual_role_property prop, unsigned int *val)
{
    struct otg_dual_role_data *dual_data = dual_role_get_drvdata(dual_role);
    int state = 0;

    if (!dual_data) {
        return -EINVAL;
    }

    state = dual_data->dual_state;

    if (prop == DUAL_ROLE_PROP_MODE) {
        *val = state;
    } else if (prop == DUAL_ROLE_PROP_PR) {
        if (state == DUAL_ROLE_PROP_MODE_DFP)
            *val = DUAL_ROLE_PROP_PR_SRC;
        else if (state == DUAL_ROLE_PROP_MODE_UFP)
            *val = DUAL_ROLE_PROP_PR_SNK;
        else
            *val = DUAL_ROLE_PROP_PR_NONE;
    } else if (prop == DUAL_ROLE_PROP_DR) {
        if (state == DUAL_ROLE_PROP_MODE_DFP)
            *val = DUAL_ROLE_PROP_DR_HOST;
        else if (state == DUAL_ROLE_PROP_MODE_UFP)
            *val = DUAL_ROLE_PROP_DR_DEVICE;
        else
            *val = DUAL_ROLE_PROP_DR_NONE;
    } else {
        return -EINVAL;
    }

    pr_info("%s: get prop %d as %d\n", __func__, prop, *val);
    return 0;
}

static int otg_dual_role_set_prop(struct dual_role_phy_instance *dual_role,
                        enum dual_role_property prop, const unsigned int *val)
{
    struct otg_dual_role_data *dual_data = dual_role_get_drvdata(dual_role);
    int state = 0;

    if (!dual_data) {
        return -EINVAL;
    }

    if (prop != DUAL_ROLE_PROP_MODE) {
        return -EINVAL;
    }

    if (*val != DUAL_ROLE_PROP_MODE_DFP &&
                *val != DUAL_ROLE_PROP_MODE_UFP) {
        return -EINVAL;
    }

    dual_data->dual_state = *val;
    dual_role_instance_changed(dual_role);

    pr_info("%s: set prop %d as %d success\n", __func__, prop, *val);

    return 0;
}

static int otg_dual_role_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct dual_role_phy_desc *dual_desc = NULL;
    struct dual_role_phy_instance *dual_role_ins = NULL;
    struct otg_dual_role_data *dual_data= NULL;

    pr_info("%s: enter dual role driver probe\n", __func__);

    dual_data = devm_kzalloc(&(pdev->dev), sizeof(*dual_data), GFP_KERNEL);
    if (!dual_data) {
        ret = -ENOMEM;
        goto ALLOC_DATA_FAIL;
    }

    dual_desc = devm_kzalloc(&(pdev->dev), sizeof(*dual_desc), GFP_KERNEL);
    if (!dual_desc) {
        ret = -ENOMEM;
        goto ALLOC_DESC_FAIL;
    }

    dual_desc->name = "otg_default";
    dual_desc->supported_modes = DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
    dual_desc->properties = otg_dual_role_props;
    dual_desc->num_properties = ARRAY_SIZE(otg_dual_role_props);
    dual_desc->get_property = otg_dual_role_get_prop;
    dual_desc->set_property = otg_dual_role_set_prop;
    dual_desc->property_is_writeable = otg_dual_role_prop_is_writeable;
    dual_role_ins = devm_dual_role_instance_register(&(pdev->dev), dual_desc);
    if (IS_ERR(dual_role_ins)) {
        pr_err("%s: otg dual role fail to register dual role instance\n",
__func__);
        ret = -EINVAL;
        goto REG_ROLE_FAIL;
    }

    dual_data->dev = &(pdev->dev);
    dual_data->dual_role_ins = dual_role_ins;
    dual_data->dual_state = DUAL_ROLE_PROP_MODE_NONE;
    dual_role_ins->drv_data = dual_data;

    dev_set_drvdata(&(pdev->dev), dual_data);

    pr_info("%s: dual role driver probe success\n", __func__);

    return 0;

REG_ROLE_FAIL:
    devm_kfree(&(pdev->dev), dual_desc);
ALLOC_DESC_FAIL:
    devm_kfree(&(pdev->dev), dual_data);
ALLOC_DATA_FAIL:
    pr_err("%s: dual role driver probe fail, ret %d\n", __func__, ret);
    return ret;
}

static int otg_dual_role_remove(struct platform_device *pdev)
{
    struct otg_dual_role_data *dual_data = dev_get_drvdata(&(pdev->dev));

    if (!dual_data) {
        return -EINVAL;
    }

    devm_dual_role_instance_unregister(&(pdev->dev), dual_data->dual_role_ins);
    dev_set_drvdata(&(pdev->dev), NULL);
    return 0;
}

static struct of_device_id otg_dual_role_of_match[] = {
    {.compatible = "huawei,otg-dual-role", },
    {},
};

static struct platform_driver otg_dual_role_drv = {
    .probe = otg_dual_role_probe,
    .remove = otg_dual_role_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "otg_dual_role",
        .of_match_table = otg_dual_role_of_match,
    },
};

static int __init otg_dual_role_init(void)
{
    int ret = 0;
    ret = platform_driver_register(&otg_dual_role_drv);
    pr_err("%s: otg dual role driver register, ret %d\n", __func__, ret);
    return ret;
}

static void __exit otg_dual_role_exit(void)
{
    platform_driver_unregister(&otg_dual_role_drv);
    return;
}

module_init(otg_dual_role_init);
module_exit(otg_dual_role_exit);

MODULE_AUTHOR("huawei");
MODULE_DESCRIPTION("This module register otg dual role dummy");
MODULE_LICENSE("GPL v2");
#endif
