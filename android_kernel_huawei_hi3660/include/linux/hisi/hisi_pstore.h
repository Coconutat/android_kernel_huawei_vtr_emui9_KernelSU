#ifndef __BB_HISI_PERSIST_STORE__
#define __BB_HISI_PERSIST_STORE__

#ifdef CONFIG_HISI_BB
void hisi_save_pstore_log(char *name, void *data, size_t size);
void hisi_free_persist_store(void);
void hisi_create_pstore_entry(void);
void hisi_remove_pstore_entry(void);
#else
static inline void hisi_save_pstore_log(char *name, void *data, size_t size) {};
static inline void hisi_free_persist_store(void) {};
static inline void hisi_create_pstore_entry(void) {};
static inline void hisi_remove_pstore_entry(void) {};
#endif

#endif //__BB_HISI_PERSIST_STORE__
