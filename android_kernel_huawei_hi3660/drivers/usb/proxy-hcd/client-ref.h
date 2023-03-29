#ifndef _CLIENT_REF_H_
#define _CLIENT_REF_H_

#include <linux/spinlock.h>
#include <linux/completion.h>

#define CLIENT_REF_KILL_SYNC_TIMEOUT (5*HZ)

struct client_ref {
	unsigned long count;
	int live;
	spinlock_t lock;
	struct completion kill_completion;
	void (*release)(struct client_ref *);
};

static inline void client_ref_put(struct client_ref *ref)
{
	unsigned long flags;
	unsigned long count;

	spin_lock_irqsave(&ref->lock, flags);
	ref->count--;
	count = ref->count;
	spin_unlock_irqrestore(&ref->lock, flags);
	if ((count == 0) && (ref->release))
		ref->release(ref);
}

static inline bool client_ref_tryget_live(struct client_ref *ref)
{
	unsigned long flags;

	spin_lock_irqsave(&ref->lock, flags);
	if (ref->live >= 1) {
		ref->count++;
		spin_unlock_irqrestore(&ref->lock, flags);
		return true;
	}

	spin_unlock_irqrestore(&ref->lock, flags);
	return false;
}

static inline unsigned long client_ref_read(struct client_ref *ref)
{
	unsigned long flags;
	unsigned long count;

	spin_lock_irqsave(&ref->lock, flags);
	count = ref->count;
	spin_unlock_irqrestore(&ref->lock, flags);

	return count;
}

static inline bool client_ref_is_zero(struct client_ref *ref)
{
	return (client_ref_read(ref) == 0);
}

static inline void client_ref_kill(struct client_ref *ref)
{
	unsigned long flags;

	spin_lock_irqsave(&ref->lock, flags);
	ref->live--;
	ref->count--;
	if (ref->count == 0) {
		spin_unlock_irqrestore(&ref->lock, flags);
		if (ref->release)
			ref->release(ref);
		return;
	}
	spin_unlock_irqrestore(&ref->lock, flags);
}

/*
 * return 0 on sucess, others failed.
 */
static inline int client_ref_kill_sync(struct client_ref *client_ref)
{
	init_completion(&client_ref->kill_completion);
	client_ref_kill(client_ref);
	if (wait_for_completion_timeout(&client_ref->kill_completion, CLIENT_REF_KILL_SYNC_TIMEOUT) == 0) {
		pr_err("[%s]wait for client killed timeout\n", __func__);
		WARN_ON(1);
		return -ETIME;
	}

	WARN_ON(!client_ref_is_zero(client_ref));

	return 0;
}

static inline void client_ref_release(struct client_ref *ref)
{
	complete(&ref->kill_completion);
}

static inline void client_ref_start(struct client_ref *ref,
		void (*release)(struct client_ref *))
{
	unsigned long flags;

	spin_lock_irqsave(&ref->lock, flags);
	ref->count = 1UL;
	ref->live = 1;
	ref->release = release;
	spin_unlock_irqrestore(&ref->lock, flags);
}

static inline void client_ref_init(struct client_ref *ref)
{
	spin_lock_init(&ref->lock);
	ref->count = 0UL;
	ref->live = 0;
	ref->release = NULL;
}

#endif /* _CLIENT_REF_H_ */
