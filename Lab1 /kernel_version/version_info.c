#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>

#define PROC_NAME "version_info"

static int version_info_show(struct seq_file *m, void *v)
{
    struct new_utsname *uts = utsname();
    
    seq_printf(m, "%s version %s (%s) %s\n",
        uts->sysname,        // 操作系统名称
        uts->release,        // 内核版本
        uts->version,        // 内核编译信息
        uts->machine        // 机器硬件名
    );
    
    return 0;
}

static int version_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, version_info_show, NULL);
}

static const struct proc_ops version_info_fops = {
    .proc_open = version_info_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init version_info_init(void)
{
    proc_create(PROC_NAME, 0, NULL, &version_info_fops);
    printk(KERN_INFO "version_info module loaded\n");
    return 0;
}

static void __exit version_info_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "version_info module unloaded\n");
}

module_init(version_info_init);
module_exit(version_info_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ec3o");
MODULE_DESCRIPTION("A simple module to display system version information");