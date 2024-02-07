#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/file.h>
#include <linux/uaccess.h>

int fih_tray_state = 0;  /* 0=REMOVE , 1=INSERT */

/* called by drivers/mmc/host/mtk-sd.c | msdc_get_cd() */
void fih_tray_msdc(int card_inserted)
{
	fih_tray_state = card_inserted;
}

static int fih_tray_read(struct seq_file *m, void *v)
{
	if (fih_tray_state) {
		seq_printf(m, "1\n");  /* INSERT */
	} else {
		seq_printf(m, "0\n");  /* REMOVE */
	}

	return 0;
}

static int fih_tray_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_tray_read, NULL);
}

static const struct file_operations fih_tray_fops = {
	.open    = fih_tray_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int __init fih_tray_init(void)
{
	proc_create("AllHWList/tray_status", 0, NULL, &fih_tray_fops);
	return 0;
}
module_init(fih_tray_init);

static void __exit fih_tray_exit(void)
{
	remove_proc_entry ("AllHWList/tray_status", NULL);
}
module_exit(fih_tray_exit);
