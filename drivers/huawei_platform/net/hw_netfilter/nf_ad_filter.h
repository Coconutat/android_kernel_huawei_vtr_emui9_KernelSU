

#ifndef _NF_AD_FILTER
#define _NF_AD_FILTER
#define  AD        	0
#define  DELTA     1
struct list_info{
	struct list_head head[MAX_HASH];
	spinlock_t lock[MAX_HASH];
	int type;
	u64 lastrepot;
};

extern struct list_info g_adlist_info ;
extern struct list_info g_deltalist_info;

void uninit_ad(void);
void init_ad(void);

void add_ad_rule(struct list_info *listinfo,const char *rule, bool reset);
void output_ad_rule(struct list_info *listinfo);
void clear_ad_rule(struct list_info *listinfo,int opt, const char *data);
bool match_ad_uid(struct list_info *listinfo,unsigned int uid);
bool match_ad_url(struct list_info *listinfo,unsigned int uid, const char *data, int datalen);
bool match_url_string(const char *url, const char *filter, int len);
bool is_droplist(char *url, int len);
void add_to_droplist(char *url, int len);
#endif /*_NF_AD_FILTER*/
