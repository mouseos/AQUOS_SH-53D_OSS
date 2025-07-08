#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/trace_events.h>
#include <linux/preempt.h>
#include "fih_printk.h"
/*
 * We hook three kernel functions:
 * 1. do_syslog(int type, char __user *buf, int len, int source)
 * 2. _printk(const char *fmt, ...)
 *
 * On ARM64 platforms, function arguments are passed in registers:
 *   - 1st argument: regs[0] (x0)
 *   - 2nd argument: regs[1] (x1)
 *   - 3rd argument: regs[2] (x2)
 *   - 4th argument: regs[3] (x3)
 */

#define TAG "FIH_PRINTK"
#define BATT_LOG_KERNEL_TAG "FIHBATTLOG::"
#define BATT_LOG_KERNEL_TAG_LEN (sizeof(BATT_LOG_KERNEL_TAG) - 1)
#define __LOG_BLS_BUF_LEN 1024
static DEFINE_SPINLOCK(bls_logbuf_lock);
static unsigned log_bls_start = 0;    /* Index into log_bls_start: next char to be read by syslog() */
static unsigned log_bls_end=0;    /* Index into log_bls_end: most-recently-written-char + 1 */
static char __log_bls_buf[__LOG_BLS_BUF_LEN];
static char *log_bls_buf = __log_bls_buf;
static int log_bls_buf_len = __LOG_BLS_BUF_LEN;
#define LOG_BLS_BUF_MASK (log_bls_buf_len-1)
#define LOG_BLS_BUF(idx) (log_bls_buf[(idx) & LOG_BLS_BUF_MASK])
unsigned long flags;

static bool is_numeric_string(const char *str) {
    long val;

    if (kstrtol(str, 10, &val) == 0) {
        return true;
    }
    return false;
}

static int fih_printk_get_kernel_buffer(char __user *buf, int len)
{
    int error = -EINVAL;
    //unsigned i;
    //char c;
    int ret;
    unsigned count;

    if (!buf || len < 0)
        return error;
    
    if (!len)
        return 0;
    
    if (!access_ok(buf, len))
        return -EFAULT;

    //Checking the amount of data that can be read under spinlock protection
    spin_lock_irqsave(&bls_logbuf_lock, flags);
    if (log_bls_start == log_bls_end) {
        spin_unlock_irqrestore(&bls_logbuf_lock, flags);
        return -ENODATA;
    }
    count = log_bls_end - log_bls_start;
    if (count > (unsigned)len)
        count = len;
    spin_unlock_irqrestore(&bls_logbuf_lock, flags);

    //Copy data directly to user space (user space pages must have been locked)
    if (copy_to_user(buf, log_bls_buf + log_bls_start, count))
        return -EFAULT;

    //Update read position
    spin_lock_irqsave(&bls_logbuf_lock, flags);
    log_bls_start += count;
    spin_unlock_irqrestore(&bls_logbuf_lock, flags);

    ret = count;
    return ret;
}

/* ------------------------------
 * kprobe：hook do_syslog(int type, char __user *buf, int len, int source)
 * ------------------------------
 */
 
 //define a struct to save do_syslog parameters
 struct do_syslog_args {
    int type;
    char __user *buf;
    int len;
};

//kretprobe entry_handler：save parameters brfore into do_syslog
static int do_syslog_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct do_syslog_args *args = (struct do_syslog_args *)ri->data;
    args->type = regs->regs[0];
    args->buf  = (char __user *)regs->regs[1];
    args->len  = regs->regs[2];
    return 0;
}
 
//Process if type is SYSLOG_ACTION_GET_KERNEL_BUFFER
static int do_syslog_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct do_syslog_args *args = (struct do_syslog_args *)ri->data;
    if (args->type == SYSLOG_ACTION_GET_KERNEL_BUFFER) {
        int ret;
        //atomic context
        ret = fih_printk_get_kernel_buffer(args->buf, args->len);
        regs->regs[0] = ret;  // modify return value
        //trace_printk("[%s] kretprobe: captured SYSLOG_ACTION_GET_KERNEL_BUFFER, ret=%d\n", TAG, ret);
    }
    return 0;
}

//define kretprobe struct
static struct kretprobe kretprobe_do_syslog = {
    .handler        = do_syslog_ret_handler,
    .entry_handler  = do_syslog_entry_handler,
    .data_size      = sizeof(struct do_syslog_args),
    .maxactive      = 20, 
    .kp = {
        .symbol_name = "do_syslog",
    },
};

/* ------------------------------
 * kprobe：hook _printk(const char *fmt, ...)
 * ------------------------------
 */
static struct kprobe kp__printk = {
    .symbol_name = "_printk",
};

static int pre__printk(struct kprobe *p, struct pt_regs *regs)
{
    char fmt_buf[256] = {0};
    char printk_blsbuf[512];
    int r, i;
    char event_buf[128] = {0};

    if (copy_from_kernel_nofault(fmt_buf, (void *)regs->regs[0], sizeof(fmt_buf) - 1)) {
        //printk(KERN_ERR "[%s] pre__printk: Read fmt fail", TAG);
        trace_printk("[%s] pre__printk: Read fmt fail", TAG);
        return 0; // read fail
    }

    
    // check keyword "FIHBATTLOG::"
    char *keyword_pos = strstr(fmt_buf, BATT_LOG_KERNEL_TAG);
    if (keyword_pos == NULL) {
        return 0; //Does not include the keyword "FIHBATTLOG::"
    }
        // parsing parameter if message has format
    char *format_param = keyword_pos + BATT_LOG_KERNEL_TAG_LEN;
    if (strcmp(format_param, "%d") == 0 || strcmp(format_param, "%d\n") == 0) {
        // event is int format
        scnprintf(event_buf, sizeof(event_buf), "%d", (int)regs->regs[1]);
    } else if (strcmp(format_param, "%s") == 0 || strcmp(format_param, "%s\n") == 0) {
        // event is string format
        if (copy_from_kernel_nofault(event_buf, (char *)regs->regs[1], sizeof(event_buf) - 1)) {
            //printk(KERN_ERR "[%s] pre__printk: Read string format event fail", TAG);
            trace_printk("[%s] pre__printk: Read string format event fail", TAG);
            return 0; // read fail
        }
    //} else if (isdigit(format_param[0])) {
    } else if (is_numeric_string(format_param)) {
        // event is string
        scnprintf(event_buf, sizeof(event_buf), "%s", format_param);
    } else {
        //printk(KERN_ERR "[%s] pre__printk: other, %s", TAG, format_param);
        trace_printk("[%s] pre__printk: other, %s", TAG, format_param);
        return 0; //not process other message
    }
    r = scnprintf(printk_blsbuf, sizeof(printk_blsbuf), "%s%s", BATT_LOG_KERNEL_TAG, event_buf);
    //printk(KERN_INFO "[%s] pre__printk: captured log: %s\n", TAG, (printk_blsbuf + BATT_LOG_KERNEL_TAG_LEN));
    trace_printk("[%s] pre__printk: captured log: %s\n", TAG, (printk_blsbuf + BATT_LOG_KERNEL_TAG_LEN));

    spin_lock_irqsave(&bls_logbuf_lock, flags);
    for (i = 0; i < r; i++) {
        LOG_BLS_BUF(log_bls_end) = printk_blsbuf[i];
        log_bls_end++;
        if (log_bls_end - log_bls_start > log_bls_buf_len)
            log_bls_start = log_bls_end - log_bls_buf_len;
    }
    spin_unlock_irqrestore(&bls_logbuf_lock, flags);
    return 0;
}

static int __init fih_printk_hook_init(void)
{
    int ret;

    /* Register kretprobe for do_syslog */
    ret = register_kretprobe(&kretprobe_do_syslog);
    if (ret < 0) {
        printk(KERN_ERR "[%s] register_kretprobe for do_syslog failed, ret=%d\n", TAG, ret);
        return ret;
    }

    /* Register kprobe for _printk */
    kp__printk.pre_handler = pre__printk;
    ret = register_kprobe(&kp__printk);
    if (ret < 0) {
        printk(KERN_ERR "[%s] fih_printk_hook_init: register_kprobe for _printk failed, ret=%d\n", TAG, ret);
        unregister_kretprobe(&kretprobe_do_syslog);
        return ret;
    }

    printk(KERN_INFO "[%s] fih_printk_hook_init: All probes  registered successfully\n", TAG);
    return 0;
}

static void __exit fih_printk_hook_exit(void)
{
    unregister_kprobe(&kp__printk);
    unregister_kretprobe(&kretprobe_do_syslog);
    printk(KERN_INFO "[%s] fih_printk_hook_exit: All probes unregistered\n", TAG);
}

module_init(fih_printk_hook_init);
module_exit(fih_printk_hook_exit);

MODULE_AUTHOR("FIH ToddKuo <ToddCTKuo@fih-foxconn.com>");
MODULE_DESCRIPTION("FIH printk extensions");
MODULE_LICENSE("GPL");

