
#include <linux/notifier.h>
#include <linux/export.h>

static BLOCKING_NOTIFIER_HEAD(lcd_notifier_list);
static BLOCKING_NOTIFIER_HEAD(tpd_notifier_list);

/* ####################################################*/
int lcd_notifer_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&lcd_notifier_list, nb);
}
EXPORT_SYMBOL(lcd_notifer_register_client);


int lcd_notifer_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&lcd_notifier_list, nb);
}
EXPORT_SYMBOL(lcd_notifer_unregister_client);


int lcd_notifier_call_chain(unsigned long val)
{
	return blocking_notifier_call_chain(&lcd_notifier_list, val, NULL);
}


/* ####################################################*/
int tpd_notifer_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&tpd_notifier_list, nb);
}

int tpd_notifer_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&tpd_notifier_list, nb);
}

int tpd_notifier_call_chain(unsigned long val)
{
	return blocking_notifier_call_chain(&tpd_notifier_list, val, NULL);
}
EXPORT_SYMBOL_GPL(tpd_notifier_call_chain);

