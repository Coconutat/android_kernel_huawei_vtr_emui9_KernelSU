#include <huawei_platform/power/power_genl.h>


#ifdef  HWLOG_TAG
#undef  HWLOG_TAG
#endif

#define HWLOG_TAG POWER_GENL
HWLOG_REGIST();

static LIST_HEAD(power_genl_easy_node_head);

static int probe_status = POWER_GENL_PROBE_UNREADY;

static struct genl_family power_genl = {
    .id = GENL_ID_GENERATE,
    .hdrsize = POWER_USER_HDR_LEN,
    .name = POWER_GENL_NAME,
    .maxattr = POWER_GENL_MAX_ATTR_INDEX,
    .parallel_ops = 1,
    .n_ops = 0,
    };

static ssize_t powerct_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    power_genl_target_t *target = container_of(attr, power_genl_target_t, dev_attr);
    return snprintf(buf, PAGE_SIZE, "%u", target->port_id);
}
static ssize_t powerct_store(struct device *dev, struct device_attribute *attr,
                                   const char *buf, size_t count)
{
    power_genl_target_t *target = container_of(attr, power_genl_target_t, dev_attr);
    power_genl_easy_node_t *temp;

    if(count != sizeof(unsigned) ) {
        hwlog_err("Illegal write length(%zu) found in %s.", count, __func__);
        return -EINVAL;
    }
    memcpy(&target->port_id, buf, count);
    list_for_each_entry(temp, &power_genl_easy_node_head, node) {
        if(temp->srv_on_cb) {
            if(temp->srv_on_cb()) {
                hwlog_err("%s srv_on_cb failed.\n", temp->name);
            }
        }
    }

    return count;
}

static power_genl_target_t nl_target[__TARGET_PORT_MAX] = {
    [POWERCT_PORT] = { 
                       .port_id = 0,
                       .probed = 0,
                       .dev_attr = __ATTR_RW(powerct), 
                     },
};

int power_genl_easy_send(power_mesg_node_t *genl_node, unsigned char cmd,
                         unsigned char version, void *data, unsigned int len)
{
    power_genl_easy_node_t *temp;
    power_genl_error_t ret_val;
    struct sk_buff *skb;
    void *msg_head;
    int i;

    list_for_each_entry(temp, &power_genl_easy_node_head, node) {
        if(temp == genl_node) {
            break;
        }
    }
    if(&temp->node == &power_genl_easy_node_head) {
        hwlog_err("this power_genl_node(%s) unregistered.\n", temp->name);
        return POWER_GENL_EUNREGED;
    }

    if( temp->target > TARGET_PORT_MAX || nl_target[temp->target].port_id == 0 ) {
        hwlog_err("target port id had not set.\n");
        return POWER_GENL_EPORTID;
    }
    if(len > NLMSG_GOODSIZE - POWER_GENL_MEM_MARGIN) {
        return POWER_GENL_EMESGLEN;
    }
    /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
    skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if(!skb) {
        hwlog_err("new general message failed in %s.\n", __func__);
        return POWER_GENL_EALLOC;
    }
    /* create the message headers */
    power_genl.version = version;
    msg_head = genlmsg_put(skb, POWER_GENL_PORTID, POWER_GENL_SEQ,
                           &power_genl, POWER_GENL_FLAG, cmd);
    if (!msg_head) {
        hwlog_err("Create message for genlmsg failed in %s.\n", __func__);
        ret_val = POWER_GENL_EPUTMESG;
        goto nosend;
    }
    /* add a BATT_RAW_MESSAGE attribute (actual value to be sent) */
    if (nla_put(skb, POWER_GENL_RAW_DATA_ATTR, len, data)) {
        hwlog_err("Add attribute to genlmsg failed in %s.\n", __func__);
        ret_val = POWER_GENL_EADDATTR;
        goto nosend;
    }

    /* finalize the message */
    genlmsg_end(skb, msg_head);

    /* send the message back */
    if( temp->target > TARGET_PORT_MAX ||
        genlmsg_unicast(&init_net, skb, nl_target[temp->target].port_id)) {
        hwlog_err("Unicast genlmsg failed in %s.\n", __func__);
        return POWER_GENL_EUNICAST;
    }
    hwlog_info("%s cmd %d data was sent in %s.\n", temp->name, cmd, __func__);

    return POWER_GENL_SUCCESS;

nosend:
    kfree_skb(skb);
    return ret_val;
}

int check_port_id(power_target_t type, unsigned int pid)
{
    if(type > TARGET_PORT_MAX) {
        hwlog_err("invalid power_target_t number.\n");
        return -1;
    }
    return !(nl_target[type].port_id == pid);
}

static int easy_node_mesg_cb(struct sk_buff *skb_in, struct genl_info *info)
{
    int i;
    int len;
    struct nlattr * raw_data_attr;
    void *data;
    power_genl_easy_node_t *temp;

    if(!info) {
        hwlog_err("info is null.\n");
        return -1;
    }

    if(!info->attrs) {
        hwlog_err("info attrs is null.\n");
        return -1;
    }

    raw_data_attr = info->attrs[POWER_GENL_RAW_DATA_ATTR];
    if(!raw_data_attr) {
        hwlog_err("raw_data_attr is null.\n");
        return -1;
    }

    len = nla_len(raw_data_attr);
    data = nla_data(raw_data_attr);

    list_for_each_entry(temp, &power_genl_easy_node_head, node) {
        for(i = 0; i < temp->n_ops; i++) {
            if(!temp->ops) {
                hwlog_err("ops is null.\n");
                break;
            }
            if(info->genlhdr->cmd == temp->ops[i].cmd) {
                if(check_port_id(POWERCT_PORT, info->snd_portid)) {
                    hwlog_err("%s cmd %d filted by checking port id failed.\n",
                              temp->name, info->genlhdr->cmd);
                    return POWER_GENL_EPORTID;
                }
                if (temp->ops[i].doit) {
                    hwlog_info("%s cmd %d called by %s.\n", temp->name, info->genlhdr->cmd, __func__);
                    temp->ops[i].doit(info->genlhdr->version, data, len);
                } else {
                    hwlog_info("%s cmd %d doit is null.\n", temp->name, info->genlhdr->cmd);
                }
                break;
            }
        }
    }

    return 0;
}

int power_genl_easy_node_register(power_genl_easy_node_t *genl_node)
{
    power_genl_easy_node_t *temp;
    int i,j;

    if(!genl_node->ops) {
        hwlog_err("power generic netlink operation to register is NULL.\n");
        return POWER_GENL_EUNCHG;
    }
    if(probe_status == POWER_GENL_PROBE_START) {
        return POWER_GENL_ELATE;
    }
    if(strnlen(genl_node->name,POWER_NODE_NAME_MAX_LEN) >= POWER_NODE_NAME_MAX_LEN) {
        hwlog_err("%s unsupports name which is not a C string.\n", __func__);
    }

    for(i = 0; i < genl_node->n_ops; i++) {
        for(j = i + 1; j < genl_node->n_ops; j++) {
            if(genl_node->ops[i].cmd == genl_node->ops[j].cmd) {
                hwlog_err("%s want to register same cmd %d more than one time.\n",
                          genl_node->name, genl_node->ops[i].cmd);
                return POWER_GENL_ECMD;
            }
        }
    }

    list_for_each_entry(temp, &power_genl_easy_node_head, node) {
        if(!strncmp(genl_node->name, temp->name, POWER_NODE_NAME_MAX_LEN)) {
            hwlog_err("name(%s) had been registered.\n", genl_node->name);
            return POWER_GENL_EREGED;
        }
        if(genl_node->target > TARGET_PORT_MAX) {
            hwlog_err("%s want illegal targer port(%d).\n", genl_node->name, genl_node->target);
            return POWER_GENL_ETARGET;
        }
        for(i = 0; i < temp->n_ops; i++) {
            for(j = 0; j < genl_node->n_ops; j++) {
                if(temp->ops[i].cmd == genl_node->ops[j].cmd) {
                    hwlog_err("%s want to register cmd %d (%s had register).\n",
                              genl_node->name, temp->ops[i].cmd, temp->name);
                    return POWER_GENL_ECMD;
                }
            }
        }
    }
    INIT_LIST_HEAD(&genl_node->node);
    list_add(&genl_node->node, &power_genl_easy_node_head);
    return POWER_GENL_SUCCESS;
}

int power_genl_init(void)
{
    struct genl_ops *ops;
    power_genl_easy_node_t *temp;
    int i;
    unsigned int total_ops;

    /* power_genl_register is not allowed from now on */
    probe_status = POWER_GENL_PROBE_START;

    total_ops = 0;
    list_for_each_entry(temp, &power_genl_easy_node_head, node) {
        total_ops += temp->n_ops;
    }
    if(total_ops > 255 || total_ops <= 0) {
        hwlog_err("illegal ops num(%d).\n", total_ops);
        goto probe_fail;
    }
    power_genl.n_ops = total_ops;
    ops = kzalloc(power_genl.n_ops * sizeof(struct genl_ops), GFP_KERNEL);
    if(!ops) {
        hwlog_err("malloc for genl_ops points failed.\n");
        goto probe_fail;
    }
    list_for_each_entry(temp, &power_genl_easy_node_head, node) {
        for(i = 0; i < temp->n_ops; i++) {
            ops->cmd = temp->ops[i].cmd;
            ops->doit = easy_node_mesg_cb;
            ops++;
        }
    }
    ops -= power_genl.n_ops;
    power_genl.ops = ops;
    if(genl_register_family(&power_genl)) {
        hwlog_err("power_genl register failed.\n");
        goto probe_fail;
    }
    
    list_for_each_entry(temp, &power_genl_easy_node_head, node) {
        if( temp->target > TARGET_PORT_MAX || nl_target[temp->target].probed ) {
            continue;
        }
        if( create_attr_for_power_mesg(&nl_target[temp->target].dev_attr) ) {
            hwlog_err("attribute %s created failed.\n", nl_target[temp->target].dev_attr.attr.name);
        }
        nl_target[temp->target].probed = 1;
    }

    hwlog_info("power_genl driver probe success.\n");
    return POWER_GENL_SUCCESS;

probe_fail:
    return POWER_GENL_EPROBE;
}
