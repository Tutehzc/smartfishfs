#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/pagemap.h> /* 为了使用 PAGE_SIZE 等宏 */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Smartfish Developer");
MODULE_DESCRIPTION("A simple in-memory file system: smartfishfs");
MODULE_VERSION("0.3");

#define SMARTFISH_MAGIC 0x1314520 /* 定义一个魔数，如同文件系统的指纹 */

// --- 超级块操作 ---
// 当用户使用 'df' 命令查看磁盘空间时，会回调这个函数
static const struct super_operations smartfishfs_ops = {
    .statfs = simple_statfs, // 使用内核通用的统计函数
    .drop_inode = generic_delete_inode,
};

// --- 初始化超级块 ---
// 这是 mount 过程的核心：创建根目录的 inode 和 dentry
static int smartfishfs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *inode;

    // 1. 设置超级块基本属性
    sb->s_blocksize = PAGE_SIZE;     // 规定：咱们仓库货架最小格子是 4KB
    sb->s_blocksize_bits = PAGE_SHIFT;  
    sb->s_magic = SMARTFISH_MAGIC;  // 挂牌：门口挂上“SmartFish”的招牌，别让人以为是 NTFS
    sb->s_op = &smartfishfs_ops;    // 客服手册：遇到大事（比如仓库要拆迁）该找谁处理
    sb->s_time_gran = 1;

    // 2. 创建根目录的 inode
    // new_inode 是内核提供的函数，从内存中分配一个 inode
    inode = new_inode(sb);
    if (!inode)
        return -ENOMEM;

    // 3. 设置根 inode 的属性
    inode->i_ino = 1; // 根目录 inode 号通常为 1 或 2
    inode->i_mode = S_IFDIR | 0755; // S_IFDIR 表示它是目录，0755 是权限
    // 设置时间戳为当前时间
    inode_set_mtime_to_ts(inode, inode_set_atime_to_ts(inode, inode_set_ctime_to_ts(inode, current_time(inode))));
    
    // 关键：为了让目录能工作，我们暂时借用内核自带的 simple_dir 操作
    // 这样我们就能 ls 这个目录，虽然它现在是空的
    inode->i_op = &simple_dir_inode_operations;
    inode->i_fop = &simple_dir_operations;
    
    // 目录的链接数通常是 2 (. 和 ..)
    set_nlink(inode, 2);

    // 4. 创建根 dentry 并与 inode 关联
    // d_make_root 会将 inode 封装成 dentry，如果失败会自动释放 inode
    sb->s_root = d_make_root(inode);
    if (!sb->s_root) {
        return -ENOMEM;
    }

    pr_info("smartfishfs: superblock initialized, root created!\n");
    return 0;
}

// --- 挂载入口 ---
static struct dentry *smartfishfs_mount(struct file_system_type *fs_type,
                                        int flags, const char *dev_name, void *data)
{
    // 调用 mount_nodev，传入我们写好的 fill_super
    return mount_nodev(fs_type, flags, data, smartfishfs_fill_super);
}

// --- 驱动结构体 ---
static struct file_system_type smartfishfs_type = {
    .owner = THIS_MODULE,
    .name = "smartfishfs",
    .mount = smartfishfs_mount,
    .kill_sb = kill_litter_super,
    .fs_flags = FS_USERNS_MOUNT,
};

// --- 模块生命周期 (不变) ---
static int __init smartfishfs_init(void)
{
    int ret = register_filesystem(&smartfishfs_type);
    if (ret) {
        pr_err("smartfishfs: register failed\n");
        return ret;
    }
    pr_info("smartfishfs: registered!\n");
    return 0;
}

static void __exit smartfishfs_exit(void)
{
    unregister_filesystem(&smartfishfs_type);
    pr_info("smartfishfs: unregistered!\n");
}

module_init(smartfishfs_init);
module_exit(smartfishfs_exit);