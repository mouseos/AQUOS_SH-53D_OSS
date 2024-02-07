#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/file.h>
#include <linux/uaccess.h>

#define FIH_PROC_PATH  "SIMSlot"

static char simslot[128];

static int fih_sim_proc_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", simslot);
	return 0;
}

static int fih_sim_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_sim_proc_read, NULL);
}

static const struct file_operations fih_sim_fops = {
	.open    = fih_sim_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int fih_sim_property(struct platform_device *pdev)
{
	int rc = 0;
	static const char *p_chr;

	p_chr = of_get_property(pdev->dev.of_node, "fih_sim,simslot", NULL);
	if (!p_chr) {
		pr_info("%s:%d, simslot not specified\n", __func__, __LINE__);
	} else {
		memset(simslot, 0, sizeof(simslot));
		snprintf(simslot, sizeof(simslot), "%s\n", p_chr);
		pr_info("%s: (%s)\n", __func__, simslot);
	}

	return rc;
}

static int fih_sim_probe(struct platform_device *pdev)
{
	int rc = 0;

	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s: Unable to load device node\n", __func__);
		return -ENOTSUPP;
	}

	rc = fih_sim_property(pdev);
	if (rc) {
		pr_err("%s Unable to set property\n", __func__);
		return rc;
	}

	if (proc_create(FIH_PROC_PATH, 0, NULL, &fih_sim_fops) == NULL) {
		pr_err("fail to create proc/%s\n", FIH_PROC_PATH);
		return 1;
	}

	return rc;
}

static int fih_sim_remove(struct platform_device *pdev)
{
	remove_proc_entry(FIH_PROC_PATH, NULL);

	return 0;
}

static const struct of_device_id fih_sim_dt_match[] = {
	{.compatible = "fih_sim"},
	{}
};
MODULE_DEVICE_TABLE(of, fih_sim_dt_match);

static struct platform_driver fih_sim_driver = {
	.probe = fih_sim_probe,
	.remove = fih_sim_remove,
	.shutdown = NULL,
	.driver = {
		.name = "fih_sim",
		.of_match_table = fih_sim_dt_match,
	},
};

static int __init fih_sim_init(void)
{
	int ret;

	ret = platform_driver_register(&fih_sim_driver);
	if (ret) {
		pr_err("%s: failed!\n", __func__);
		return ret;
	}

	return ret;
}
module_init(fih_sim_init);

static void __exit fih_sim_exit(void)
{
	platform_driver_unregister(&fih_sim_driver);
}
module_exit(fih_sim_exit);
