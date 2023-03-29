#ifndef _OASES_SIGNING_H
#define _OASES_SIGNING_H

enum {
	SIG_TYPE_OASES,
	SIG_TYPE_VENDOR
};

int __init oases_init_signing_keys(void);
void oases_destroy_signing_keys(void);

int oases_verify_sig(char *data, unsigned long *_patchlen, int sig_type);

#endif /* _OASES_SIGNING_H */
