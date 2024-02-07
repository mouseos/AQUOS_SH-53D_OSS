#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/of_fdt.h>

#define FIH_PROC_DIR   "AllHWList"
#define FIH_PROC_PATH  "AllHWList/cpuinfo"

/* Reference:
 * arch/arm64/kernel/setup.c
 *   setup_machine_fdt()
 *     name = of_flat_dt_get_machine_name(); | /proc/device-tree/model
 *     machine_desc_set(name);
 * arch/arm64/kernel/cpuinfo.c
 *   machine_desc_set(const char *str)
 *     machine_desc_str = str;
 *   c_show() | /proc/cpuinfo
 *     seq_printf(m, "Hardware\t: %s\n", machine_desc_str);
 */

static char fih_proc_data[128];

static int fih_proc_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", fih_proc_data);

	return 0;
}

static int fih_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_proc_read, NULL);
}

static const struct file_operations fih_proc_fops = {
	.open    = fih_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int __init fih_proc_init(void)
{
	const char *p;

	memset(fih_proc_data, 0, sizeof(fih_proc_data));

	p = of_flat_dt_get_machine_name();
	if (p) {
		snprintf(fih_proc_data, sizeof(fih_proc_data), "%s", p);
	} else {
		pr_err("%s: fail to get machine name\n", __func__);
		snprintf(fih_proc_data, sizeof(fih_proc_data), "MT6853V/ZA");
	}

	proc_mkdir(FIH_PROC_DIR, NULL);
	if (proc_create(FIH_PROC_PATH, 0, NULL, &fih_proc_fops) == NULL) {
		proc_mkdir(FIH_PROC_DIR, NULL);
		if (proc_create(FIH_PROC_PATH, 0, NULL, &fih_proc_fops) == NULL) {
			pr_err("%s: fail to create proc/%s\n", __func__, FIH_PROC_PATH);
			return 1;
		}
	}

	pr_info("%s: cpuinfo = %s\n", __func__, fih_proc_data);
	return 0;
}

static void __exit fih_proc_exit(void)
{
	remove_proc_entry(FIH_PROC_PATH, NULL);
}

module_init(fih_proc_init);
module_exit(fih_proc_exit);
