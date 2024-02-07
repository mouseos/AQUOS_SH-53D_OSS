#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <asm/memory.h>
#include <linux/of_fdt.h>
#include <linux/kmsg_dump.h>

/* reference /proc/pl_lk
 * kernel-4.19/drivers/misc/mediatek/log_store/log_store.c
 * log_store_late_init()
 */

static char *fih_proc_name = "pl_lk_exp";
static unsigned int fih_proc_addr = 0x9AE00000;
static unsigned int fih_proc_size = 0x00200000;
static char *fih_proc_pbuf = NULL;

static void *remap_lowmem(phys_addr_t start, phys_addr_t size)
{
	struct page **pages;
	phys_addr_t page_start;
	unsigned int page_count;
	pgprot_t prot;
	unsigned int i;
	void *vaddr;

	page_start = start - offset_in_page(start);
	page_count = DIV_ROUND_UP(size + offset_in_page(start), PAGE_SIZE);

	prot = pgprot_noncached(PAGE_KERNEL);

	pages = kmalloc_array(page_count, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return NULL;

	for (i = 0; i < page_count; i++) {
		phys_addr_t addr = page_start + i * PAGE_SIZE;

		pages[i] = pfn_to_page(addr >> PAGE_SHIFT);
	}
	vaddr = vmap(pages, page_count, VM_MAP, prot);
	kfree(pages);
	if (!vaddr) {
		pr_notice("%s: Failed to map %u pages\n", __func__, page_count);
		return NULL;
	}

	return vaddr + offset_in_page(start);
}

static int fih_seq_show(struct seq_file *m, void *v)
{
	if (fih_proc_pbuf == NULL) {
		seq_puts(m, "log buff is null.\n");
		return 0;
	}

	seq_write(m, fih_proc_pbuf, fih_proc_size);

	return 0;
}

static int fih_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_seq_show, inode->i_private);
};

static struct file_operations fih_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fih_seq_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

static int __init fih_module_init(void)
{
	struct proc_dir_entry *entry;

	fih_proc_pbuf = remap_lowmem(fih_proc_addr, fih_proc_size);
	if (fih_proc_pbuf == NULL) {
		pr_err("%s: ioremap fail\n", fih_proc_name);
		return 0;
	}

	entry = proc_create(fih_proc_name, 0, NULL, &fih_file_ops);
	if (!entry) {
		pr_err("%s: fail create proc\n", fih_proc_name);
		return 0;
	}

	return 0;
}

static void __exit fih_module_exit(void)
{
	remove_proc_entry(fih_proc_name, NULL);
}

module_init(fih_module_init);
module_exit(fih_module_exit);
