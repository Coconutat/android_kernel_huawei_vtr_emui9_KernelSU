
#include "dfs.h"

#include <linux/seq_file.h>
//#include <linux/fs.h>

#include "fusb3601_global.h"
#include "strings.h"

#include "../core/port.h"

/*
 * Display is a basic text-formated status screen that shows connection
 * and state information.
 * This interface is Read Only.
 */
static int display_show(struct seq_file *m, void *v) {
    struct Port *p = (struct Port *)m->private;

    if (!p) {
      return -1;
    }

    seq_printf(m,
      "==================== FUSB3601 @ 0x%02X %sIdle) ====================\n\n",
        p->i2c_addr_,
        p->idle_ ? "    (" : "(Not ");

    FUSB3601_ReadStatusRegisters(p);

    seq_printf(m," AlertL %02X AlertH %02X EventN %02X %02X %02X MUS %02X\n",
        p->registers_.AlertL.byte, p->registers_.AlertH.byte,
        p->registers_.Event1.byte, p->registers_.Event2.byte,
        p->registers_.Event3.byte, p->registers_.MUSInterrupt.byte);

    if (!p->tc_enabled_) {
        seq_printf(m, " Type-C State Machines Disabled.\n");
        return 0;
    }

    seq_printf(m, " TC State: %s\n", str_TypeCStateNames[p->tc_state_]);

    seq_printf(m, "                  Port Type:  %s   DFP Adv:   %s\n",
        str_PortType[p->port_type_],
        str_CurrentAdvert[p->src_current_]);

    if (p->source_or_sink_ == Source) {
        seq_printf(m, "                  CC1:      %5s   CC2:      %5s\n",
          p->cc_pin_ == CC1 ? str_Term[p->cc_term_] : str_Term[p->vconn_term_],
          p->cc_pin_ == CC1 ? str_Term[p->vconn_term_] : str_Term[p->cc_term_]);
    }
    else {
        seq_printf(m, "                  CC1:      %5s   CC2:      %5s\n",
          p->cc_pin_ == CC1 ? str_Term[p->cc_term_] : str_Term[p->vconn_term_],
          p->cc_pin_ == CC1 ? str_Term[p->vconn_term_] : str_Term[p->cc_term_]);
    }

    if (!p->pd_enabled_) {
        seq_printf(m, " PD Disabled.\n");
        return 0;
    }

    if (!p->pd_active_) {
        seq_printf(m, " PD Inactive.\n");
        return 0;
    }

    seq_printf(m, " PD State: %s\n", str_PDStateNames[p->policy_state_]);

    seq_printf(m, "                  Power Role: %3s   Data Role:  %s\n",
        p->policy_is_source_ ? "Src" : "Snk",
        p->policy_is_dfp_ ? "DFP" : "UFP");

    return 0;
}

static int display_open(struct inode *inode, struct file *file) {
    return single_open(file, display_show, inode->i_private);
}

static const struct file_operations display_fops = {
    .open     = display_open,
    .read     = seq_read,
    .llseek   = seq_lseek,
    .release  = single_release
};

/*
 * Display Type-C Log values
 */
static int tclog_show(struct seq_file *m, void *v) {
    struct Port *p = (struct Port *)m->private;

    FSC_U32 time_Sec;
    FSC_U32 time_MS;
    FSC_U8 state;

    if (!p) {
      return -1;
    }

   /* Do we have messages to read? */
    while (p->log_.tc_readindex_ != p->log_.tc_writeindex_) {

      state = p->log_.tc_data_[p->log_.tc_readindex_];

      time_MS = ((FSC_U32)p->log_.tc_data_[p->log_.tc_readindex_+1] << 8) |
        p->log_.tc_data_[p->log_.tc_readindex_+2];
      time_Sec = ((FSC_U32)p->log_.tc_data_[p->log_.tc_readindex_+3] << 8) |
        p->log_.tc_data_[p->log_.tc_readindex_+4];

      p->log_.tc_readindex_ = (p->log_.tc_readindex_ + 5) % FSC_LOG_SIZE;

      seq_printf(m, "[%4u.%04u]\t%s\n", time_Sec, time_MS,
        str_TypeCStateNames[state]);
    }

    p->log_.tc_overrun_ = FALSE;

    return 0;
}

static int tclog_open(struct inode *inode, struct file *file) {
    return single_open(file, tclog_show, inode->i_private);
}

static const struct file_operations tclog_fops = {
    .open     = tclog_open,
    .read     = seq_read,
    .llseek   = seq_lseek,
    .release  = single_release
};

/*
 * Display PD Log values
 */
static int pdlog_show(struct seq_file *m, void *v) {
    struct Port *p = (struct Port *)m->private;

    FSC_U32 time_Sec;
    FSC_U32 time_MS;
    FSC_U8 state;

    if (!p) {
      return -1;
    }

   /* Do we have messages to read? */
    while (p->log_.pe_readindex_ != p->log_.pe_writeindex_) {

      state = p->log_.pe_data_[p->log_.pe_readindex_];

      time_MS = ((FSC_U32)p->log_.pe_data_[p->log_.pe_readindex_+1] << 8) |
        p->log_.pe_data_[p->log_.pe_readindex_+2];
      time_Sec = ((FSC_U32)p->log_.pe_data_[p->log_.pe_readindex_+3] << 8) |
        p->log_.pe_data_[p->log_.pe_readindex_+4];

      p->log_.pe_readindex_ = (p->log_.pe_readindex_ + 5) % FSC_LOG_SIZE;

      seq_printf(m, "[%4u.%04u]\t%s\n", time_Sec, time_MS,
        str_PDStateNames[state]);
    }

    p->log_.pe_overrun_ = FALSE;

    return 0;
}

static int pdlog_open(struct inode *inode, struct file *file) {
    return single_open(file, pdlog_show, inode->i_private);
}

static const struct file_operations pdlog_fops = {
    .open     = pdlog_open,
    .read     = seq_read,
    .llseek   = seq_lseek,
    .release  = single_release
};

/*
 * The hostcom interface is a R/W, byte-formated channel for viewing/
 * controlling status and mode settings.
 */
static int hostcom_show(struct seq_file *m, void *v) {
    struct Port *p = (struct Port *)m->private;

    if (!p) {
      return -EINVAL;
    }

    /* TODO - Display result of last hostcom command */
    seq_printf(m, "FUSB XXXX Hostcom Show! %2x\n\n", p->i2c_addr_);

    return 0; /* 0 indicates EOF */
}

static int hostcom_open(struct inode *inode, struct file *file) {
    return single_open(file, hostcom_show, inode->i_private);
}

static ssize_t hostcom_write (struct file *f, const char *data,
                              size_t size, loff_t *off)
{
    unsigned int cmd, value;

    /* TODO - Process Hostcom commands */
    if (sscanf(data, "%x %x", &cmd, &value) != 2) {
        return -EINVAL;
    }

    pr_err("FUSB XXXX HOSTCOM WRITE: %d %d Size %zd\n", cmd, value, size);
    return size;
}

static const struct file_operations hostcom_fops = {
    .open     = hostcom_open,
    .read     = seq_read,
    .llseek   = seq_lseek,
    .release  = single_release,
    .write    = hostcom_write
};

/*
 * Initialize DebugFS file objects
 */
FSC_S32 fusb_InitializeDFS(void)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return -ENOMEM;
    }

    /* Try to create our top level dir */
    chip->debugfs_parent = debugfs_create_dir("fusb3601", NULL);

    if (!chip->debugfs_parent)
    {
        pr_err("FUSB  %s - Couldn't create DebugFS dir!\n", __func__);
        return -ENOMEM;
    }

    debugfs_create_file("display", 0444, chip->debugfs_parent,
                        &(chip->port), &display_fops);

    debugfs_create_file("tclog", 0444, chip->debugfs_parent,
                        &(chip->port), &tclog_fops);

    debugfs_create_file("pdlog", 0444, chip->debugfs_parent,
                        &(chip->port), &pdlog_fops);

/*
    debugfs_create_file("hostcom", 0666, chip->debugfs_parent,
                        &(chip->port), &hostcom_fops);
*/

    /* Direct accessors */
    debugfs_create_bool("tcenabled", 0666, chip->debugfs_parent,
                        &(chip->port.tc_enabled_));
    debugfs_create_bool("pdenabled", 0666, chip->debugfs_parent,
                        &(chip->port.pd_enabled_));

    debugfs_create_u8("porttype", 0666, chip->debugfs_parent,
                      (u8 *)&(chip->port.port_type_));

    debugfs_create_u8("src_current", 0666, chip->debugfs_parent,
                      (u8 *)&(chip->port.src_current_));

	debugfs_create_u8("snk_current", 0666, chip->debugfs_parent,
                      (u8 *)&(chip->port.snk_current_));

    return 0;
}

/*
 * Cleanup/remove unneeded DebugFS file objects
 */
FSC_S32 fusb_DFS_Cleanup(void)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return -ENOMEM;
    }

    if (chip->debugfs_parent != NULL) {
        debugfs_remove_recursive(chip->debugfs_parent);
    }

    return 0;
}
