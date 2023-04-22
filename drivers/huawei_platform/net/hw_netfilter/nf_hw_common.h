

#ifndef _NF_HW_COMMON
#define _NF_HW_COMMON
#define MAX_HASH 256
const char *strfind(const char *s1, const char *s2);
const char *strfindpos(const char *s1, const char *s2, int len);
const char *strfinds2(const char *s1, const char *s2, int s2l);
const char *substring(const char *src, const char *f, const char *s, int *l);
void byte_to_hex(const void *pin, int ilen, void *pout, int olen);
void hex_to_byte(const void *pin, int ilen, void *pout, int olen);
u64 get_cur_time(void);
void right_trim(char *p);
const char *left_trim(const char *p);
char *get_url_form_data(const char *data, int datalen);
char *get_url_path(const char *data, int datalen);
unsigned int get_hash_id(int uid);
#endif /*_NF_HW_COMMON*/
