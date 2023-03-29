#include <linux/notifier.h>
#include <linux/export.h>
#include <linux/power/hisi/coul/hisi_coul_event.h>

static ATOMIC_NOTIFIER_HEAD(hisi_coul_atomic_notifier_list);
static BLOCKING_NOTIFIER_HEAD(hisi_coul_blocking_notifier_list);


int hisi_coul_register_atomic_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&hisi_coul_atomic_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(hisi_coul_register_atomic_notifier);


int hisi_coul_unregister_atomic_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&hisi_coul_atomic_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(hisi_coul_unregister_atomic_notifier);


int hisi_call_coul_atomic_notifiers(int val,void *v)
{
	return atomic_notifier_call_chain(&hisi_coul_atomic_notifier_list,(unsigned long)val, v);/*lint !e571*/
}
EXPORT_SYMBOL_GPL(hisi_call_coul_atomic_notifiers);

int hisi_coul_register_blocking_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&hisi_coul_blocking_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(hisi_coul_register_blocking_notifier);


int hisi_coul_unregister_blocking_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&hisi_coul_blocking_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(hisi_coul_unregister_blocking_notifier);


int hisi_call_coul_blocking_notifiers(int val,void *v)
{
	return blocking_notifier_call_chain(&hisi_coul_blocking_notifier_list,(unsigned long)val, v);/*lint !e571*/
}
EXPORT_SYMBOL_GPL(hisi_call_coul_blocking_notifiers);






