


#ifndef _NF_APP_DL_MONITOR
#define _NF_APP_DL_MONITOR

#define ID_START		1000 /* the start  download id  */
#define ID_MAXID		20000000/* the max download id  */
#define MAX_THREAD		2
#define WAIT_DELAY		10
#define MAX_WAIT		12000

#define DLST_NOT	0
#define DLST_WAIT	1
#define DLST_ALLOW	2
#define DLST_REJECT	3

#include <linux/types.h>
#include <linux/ip.h>
#include <linux/tcp.h>

struct dl_info {
	struct list_head list;
	u64 start_time;
	struct timer_list timer;
	bool bwait;
	unsigned int uid;
	unsigned int dlid;
	struct sk_buff *skb;
	char *url;
	int len;
	int action;/* user select action ，0 allow ，other reject */
};
void init_appdl(void);
void uninit_appdl(void);
void add_appdl_rule(const char *rule, bool bReset);
void clear_appdl_rule(int opt, const char *rule);
void output_appdl_rule(void);
bool match_appdl_uid(unsigned int uid);
bool match_appdl_url(const char *data, int datalen);
struct dl_info *get_download_monitor(struct sk_buff *skb, unsigned int uid,
				     const char *url);
char *get_report_msg(unsigned int dlid, unsigned int uid, const char *data,
		     char *ip);
void download_notify(int dlid, const char *action);
void free_node(struct dl_info *node);
int get_select(struct sk_buff *skb);
unsigned int  wait_user_event(struct dl_info *node);
#endif /*_NF_APP_DL_MONITOR*/
