/*
 *
 *  Copyright (C) 2006 Antonino Daplas <adaplas@pol.net>
 *
 *	2001 - Documented with DocBook
 *	- Brad Douglas <brad@neruo.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */
#include "bms_notify.h"

static BLOCKING_NOTIFIER_HEAD(bms_notify_list);

/**
 *	bms_notify_register_client - register a client notifier
 *	@nb: notifier block to callback on events
 */
int bms_notify_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&bms_notify_list, nb);
}
EXPORT_SYMBOL(bms_notify_register_client);

/**
 *	bms_notify_unregister_client - unregister a client notifier
 *	@nb: notifier block to callback on events
 */
int bms_notify_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&bms_notify_list, nb);
}
EXPORT_SYMBOL(bms_notify_unregister_client);

/**
 * bms_notify_call_chain - notify clients of bms_notify_events
 *
 */
int bms_notify_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&bms_notify_list, val, v);
}
EXPORT_SYMBOL_GPL(bms_notify_call_chain);
