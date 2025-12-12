#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>       // 核心：文件系统相关的头文件

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Smartfish Developer");
MODULE_DESCRIPTION("A simple in-memory file system: smartfishfs");
MODULE_VERSION("0.2");

// --- 前向声明 ---
// 这是 mount_nodev 需要的回调函数。
// 目前我们先让它返回 0 (成功)，但不做任何实际初始化。
// 到了 Phase 3 我们会在这里填充根目录。
static int smartfishfs_fill_super(struct super_block *sb, void *data, int silent)
{
    pr_info("smartfishfs: mounting... (fill_super called)\n");
    return 0;
}

// --- 接口实现 ---

// 1. 挂载入口
// 当用户执行 mount -t smartfishfs 时，内核调用此函数
static struct dentry *smartfishfs_mount(struct file_system_type *fs_type,
                                        int flags, const char *dev_name, void *data)
{
    // mount_nodev 是内核提供的辅助函数，用于创建无物理设备的超级块
    // 它会创建一个超级块，然后调用 smartfishfs_fill_super 来初始化它
    return mount_nodev(fs_type, flags, data, smartfishfs_fill_super);
}

// 2. 定义文件系统类型结构体
// 这是文件系统的"身份证"
static struct file_system_type smartfishfs_type = {
    .owner = THIS_MODULE,
    .name = "smartfishfs",        // 关键：mount命令用的名字
    .mount = smartfishfs_mount,   // 挂载时执行的方法
    .kill_sb = kill_litter_super, // 卸载时清理内存的标准方法 (用于无设备FS)
    .fs_flags = FS_USERNS_MOUNT,  // 允许在用户命名空间挂载 (可选，增加兼容性)
};

// --- 模块生命周期 ---

static int __init smartfishfs_init(void)
{
    int ret;

    // 向内核注册我们的文件系统
    ret = register_filesystem(&smartfishfs_type);
    if (ret) {
        pr_err("smartfishfs: failed to register filesystem\n");
        return ret;
    }

    pr_info("smartfishfs: registered successfully!\n");
    return 0;
}

static void __exit smartfishfs_exit(void)
{
    int ret;

    // 从内核注销
    ret = unregister_filesystem(&smartfishfs_type);
    if (ret) {
        pr_err("smartfishfs: failed to unregister filesystem\n");
    }

    pr_info("smartfishfs: unregistered successfully!\n");
}

module_init(smartfishfs_init);
module_exit(smartfishfs_exit);