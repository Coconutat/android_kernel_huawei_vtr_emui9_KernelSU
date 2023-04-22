#ifndef __FDLEAK_H_
#define __FDLEAK_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FDLEAK_PID_NUM         5
#define MAX_PROBE_COUNT            3
#define MAX_STACK_TRACE_COUNT    20
#define FDLEAK_MAX_STACK_TRACE_DEPTH    64
#define MAX_FDLEAK_WP_NAME_LEN     256

typedef enum {
    FDLEAK_WP_MIN            = 0,
    FDLEAK_WP_EVENTFD        = 0,
    FDLEAK_WP_EVENTPOLL        = 1,
    FDLEAK_WP_DMABUF        = 2,
    FDLEAK_WP_SYNCFENCE        = 3,
    FDLEAK_WP_SOCKET        = 4,
    FDLEAK_WP_PIPE            = 5,
    FDLEAK_WP_ASHMEM        = 6,
    FDLEAK_WP_NUM_MAX,
    FDLEAK_WP_UNKNOWN        
}fdleak_wp_id;

#define MAX_EVENTPOLL_PROBE_CNT    0
#define MAX_EVENTFD_PROBE_CNT    0
#define MAX_DMABUF_PROBE_CNT    3
#define MAX_SYNCFENCE_PROBE_CNT    2
#define MAX_SOCKET_PROBE_CNT    2
#define MAX_PIPE_PROBE_CNT        2
#define MAX_ASHMEM_PROBE_CNT    2

#define MASK_FDLEAK_WP_EVENTFD        (1 << FDLEAK_WP_EVENTFD)
#define MASK_FDLEAK_WP_EVENTPOLL    (1 << FDLEAK_WP_EVENTPOLL)
#define MASK_FDLEAK_WP_DMABUF        (1 << FDLEAK_WP_DMABUF)
#define MASK_FDLEAK_WP_SYNCFENCE    (1 << FDLEAK_WP_SYNCFENCE)
#define MASK_FDLEAK_WP_SOCKET        (1 << FDLEAK_WP_SOCKET)
#define MASK_FDLEAK_WP_PIPE            (1 << FDLEAK_WP_PIPE)
#define MASK_FDLEAK_WP_ASHMEM        (1 << FDLEAK_WP_ASHMEM)

typedef struct {
    int magic;
    int pid;
    int wp_mask;
}fdleak_op;

typedef struct{
    unsigned long long stack[FDLEAK_MAX_STACK_TRACE_DEPTH];
}stack_item;

typedef struct {
    int magic;                                //IN: magic number for this ioctl
    int pid;                                //IN: from which pid want to get stack info
    fdleak_wp_id wpid;                        //IN: from which fdleak watchpoint want to get stack info
    int is_32bit;                            //OUT: is current task 32bit
    int probe_cnt;                            //OUT: how many probe count this watchpoint contain
    char wp_name[MAX_FDLEAK_WP_NAME_LEN];    //OUT: name of fdleak watchpoint
    int hit_cnt[MAX_PROBE_COUNT][MAX_STACK_TRACE_COUNT];    //OUT: how many time it occur for each stack
    int diff_cnt[MAX_PROBE_COUNT];                            //OUT: how many different stack
    stack_item list[MAX_PROBE_COUNT][MAX_STACK_TRACE_COUNT];    //OUT: detail stack info, include so name and offset
}fdleak_stackinfo;

/* the following are used for IOCTL */
#define FDLEAK_MAGIC 0x5366FEFA
#define FDLEAK_CMD_INVALID 0xFF

#define __FDLEAKIO  0xAC
#define FDLEAK_ENABLE_WATCH        _IO(__FDLEAKIO, 1)
#define FDLEAK_DISABLE_WATCH    _IO(__FDLEAKIO, 2)
#define FDLEAK_GET_STACK        _IO(__FDLEAKIO, 3)

#define FDLEAK_CMD_MAX            _IO(__FDLEAKIO, 4)

long fdleak_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
int fdleak_report(fdleak_wp_id index, int probe_id);

#ifdef __cplusplus
}
#endif
#endif /* __FDLEAK_H_ */
