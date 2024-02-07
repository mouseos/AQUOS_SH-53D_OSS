#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/file.h>
#include <linux/uaccess.h>

static char pid[64];
static char bt_mac[64];
static char wifi_mac[64];
static char imei_1[64];
static char imei_2[64];

static int fih_mfd_proc_read_pid(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", pid);
	return 0;
}

static int fih_mfd_proc_open_pid(struct inode *inode, struct file *file)
{
	return single_open(file, fih_mfd_proc_read_pid, NULL);
}

static const struct file_operations fih_mfd_fops_pid = {
	.open    = fih_mfd_proc_open_pid,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int fih_mfd_proc_read_bt_mac(struct seq_file *m, void *v)
{
	char tmp[(sizeof(bt_mac) * 2)];
	unsigned int i, k;

	/* 0123456789AB -> 01:23:45:67:89:AB */
	memset(tmp, 0, sizeof(tmp));
	k = 0;
	for (i=0; i<strlen(bt_mac); i++) {
		if ((i > 0)&&((i % 2) == 0)) tmp[k++] = ':';
		tmp[k++] = bt_mac[i];
	}

	seq_printf(m, "%s\n", tmp);

	return 0;
}

static int fih_mfd_proc_open_bt_mac(struct inode *inode, struct file *file)
{
	return single_open(file, fih_mfd_proc_read_bt_mac, NULL);
}

static const struct file_operations fih_mfd_fops_bt_mac = {
	.open    = fih_mfd_proc_open_bt_mac,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int fih_mfd_proc_read_wifi_mac(struct seq_file *m, void *v)
{
	char tmp[(sizeof(wifi_mac) * 2)];
	unsigned int i, k;

	/* 0123456789AB -> 01:23:45:67:89:AB */
	memset(tmp, 0, sizeof(tmp));
	k = 0;
	for (i=0; i<strlen(wifi_mac); i++) {
		if ((i > 0)&&((i % 2) == 0)) tmp[k++] = ':';
		tmp[k++] = wifi_mac[i];
	}

	seq_printf(m, "%s\n", tmp);

	return 0;
}

static int fih_mfd_proc_open_wifi_mac(struct inode *inode, struct file *file)
{
	return single_open(file, fih_mfd_proc_read_wifi_mac, NULL);
}

static const struct file_operations fih_mfd_fops_wifi_mac = {
	.open    = fih_mfd_proc_open_wifi_mac,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int fih_mfd_proc_read_imei_1(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", imei_1);
	return 0;
}

static int fih_mfd_proc_open_imei_1(struct inode *inode, struct file *file)
{
	return single_open(file, fih_mfd_proc_read_imei_1, NULL);
}

static const struct file_operations fih_mfd_fops_imei_1 = {
	.open    = fih_mfd_proc_open_imei_1,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int fih_mfd_proc_read_imei_2(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", imei_2);
	return 0;
}

static int fih_mfd_proc_open_imei_2(struct inode *inode, struct file *file)
{
	return single_open(file, fih_mfd_proc_read_imei_2, NULL);
}

static const struct file_operations fih_mfd_fops_imei_2 = {
	.open    = fih_mfd_proc_open_imei_2,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int fih_mfd_property(struct platform_device *pdev)
{
	static const char *p_chr;

	memset(pid, 0, sizeof(pid));
	p_chr = of_get_property(pdev->dev.of_node, "fih_mfd,pid", NULL);
	if (!p_chr) {
		pr_info("%s:%d, pid not specified\n", __func__, __LINE__);
	} else {
		snprintf(pid, sizeof(pid), "%s\n", p_chr);
	}
	pr_info("%s: productid = (%s)\n", __func__, pid);

	memset(bt_mac, 0, sizeof(bt_mac));
	p_chr = of_get_property(pdev->dev.of_node, "fih_mfd,bt_mac", NULL);
	if (!p_chr) {
		pr_info("%s:%d, bt_mac not specified\n", __func__, __LINE__);
	} else {
		strlcpy(bt_mac, p_chr, sizeof(bt_mac));
	}
	pr_info("%s: bt_mac = (%s)\n", __func__, bt_mac);

	memset(wifi_mac, 0, sizeof(wifi_mac));
	p_chr = of_get_property(pdev->dev.of_node, "fih_mfd,wifi_mac", NULL);
	if (!p_chr) {
		pr_info("%s:%d, wifi_mac not specified\n", __func__, __LINE__);
	} else {
		strlcpy(wifi_mac, p_chr, sizeof(wifi_mac));
	}
	pr_info("%s: wifi_mac = (%s)\n", __func__, wifi_mac);

	memset(imei_1, 0, sizeof(imei_1));
	p_chr = of_get_property(pdev->dev.of_node, "fih_mfd,imei_1", NULL);
	if (!p_chr) {
		pr_info("%s:%d, wifi_mac not specified\n", __func__, __LINE__);
	} else {
		strlcpy(imei_1, p_chr, sizeof(imei_1));
	}
	pr_info("%s: imei_1 = (%s)\n", __func__, imei_1);

	memset(imei_2, 0, sizeof(imei_2));
	p_chr = of_get_property(pdev->dev.of_node, "fih_mfd,imei_2", NULL);
	if (!p_chr) {
		pr_info("%s:%d, wifi_mac not specified\n", __func__, __LINE__);
	} else {
		strlcpy(imei_2, p_chr, sizeof(imei_2));
	}
	pr_info("%s: imei_2 = (%s)\n", __func__, imei_2);

	return 0;
}

static int fih_mfd_probe(struct platform_device *pdev)
{
	int rc = 0;

	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s: Unable to load device node\n", __func__);
		return -ENOTSUPP;
	}

	rc = fih_mfd_property(pdev);
	if (rc) {
		pr_err("%s Unable to set property\n", __func__);
		return rc;
	}

	proc_create("productid", 0, NULL, &fih_mfd_fops_pid);
	proc_create("bt_mac", 0, NULL, &fih_mfd_fops_bt_mac);
	proc_create("wifi_mac", 0, NULL, &fih_mfd_fops_wifi_mac);
	proc_create("imei", 0, NULL, &fih_mfd_fops_imei_1);
	proc_create("imei2", 0, NULL, &fih_mfd_fops_imei_2);

	return rc;
}

static int fih_mfd_remove(struct platform_device *pdev)
{
	remove_proc_entry ("imei2", NULL);
	remove_proc_entry ("imei", NULL);
	remove_proc_entry ("wifi_mac", NULL);
	remove_proc_entry ("bt_mac", NULL);
	remove_proc_entry ("productid", NULL);

	return 0;
}

static const struct of_device_id fih_mfd_dt_match[] = {
	{.compatible = "fih_mfd"},
	{}
};
MODULE_DEVICE_TABLE(of, fih_mfd_dt_match);

static struct platform_driver fih_mfd_driver = {
	.probe = fih_mfd_probe,
	.remove = fih_mfd_remove,
	.shutdown = NULL,
	.driver = {
		.name = "fih_mfd",
		.of_match_table = fih_mfd_dt_match,
	},
};

static int __init fih_mfd_init(void)
{
	int ret;

	ret = platform_driver_register(&fih_mfd_driver);
	if (ret) {
		pr_err("%s: failed!\n", __func__);
		return ret;
	}

	return ret;
}
module_init(fih_mfd_init);

static void __exit fih_mfd_exit(void)
{
	platform_driver_unregister(&fih_mfd_driver);
}
module_exit(fih_mfd_exit);
