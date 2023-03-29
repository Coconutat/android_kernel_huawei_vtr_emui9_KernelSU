#ifndef _DIRECT_CHARGER_POWER_SUPPLY_H
#define _DIRECT_CHARGER_POWER_SUPPLY_H

int direct_charge_set_bst_ctrl(int enable);
int direct_charge_ps_probe(struct platform_device *pdev);
#endif
