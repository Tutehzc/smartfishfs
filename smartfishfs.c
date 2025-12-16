#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h> // copy_to_user 需要
#include <linux/pagemap.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Smartfish Developer");
MODULE_DESCRIPTION("A simple in-memory file system: smartfishfs");
MODULE_VERSION("0.4");

#define SMARTFISH_MAGIC 0x1314520
#define TMPSIZE 1024

// --- 文件内容 ---
static char *file_content = "Congratulations! You are reading data from SmartfishFS RAM!\n";

// --- 文件操作方法 ---

// 读文件：cat /mnt/readme.txt 时调用
static ssize_t smartfishfs_read(struct file *filp, char __user *buf,
                                size_t len, loff_t *ppos)
{
    // simple_read_from_buffer 是内核提供的神器
    // 它自动处理偏移量(*ppos)、缓冲区长度检查和 copy_to_user
    return simple_read_from_buffer(buf, len, ppos, file_content, strlen(file_content));
}

// 定义文件的操作结构体
static const struct file_operations smartfishfs_fops = {
    .owner = THIS_MODULE,
    .read = smartfishfs_read, // 我们目前只支持读
};

// --- 核心辅助函数：创建文件 ---
// sb: 超级块
// root: 根目录的 dentry
// name: 文件名
static void smartfishfs_create_file(struct super_block *sb, struct dentry *root, const char *name)
{
    struct dentry *dentry;
    struct inode *inode;

    // 1. 创建 Dentry (文件名)
    // d_alloc_name 在父目录(root)下分配一个名为 name 的 dentry
    dentry = d_alloc_name(root, name);
    if (!dentry) {
        pr_err("smartfishfs: failed to alloc dentry for %s\n", name);
        return;
    }

    // 2. 创建 Inode (文件元数据)
    inode = new_inode(sb);
    if (!inode) {
        // 如果 inode 创建失败，必须释放刚才创建的 dentry
        dput(dentry);
        return;
    }

    // 3. 设置 Inode 属性
    inode->i_ino = 2; // 给它个编号，根目录是1，它是2
    inode->i_mode = S_IFREG | 0644; // S_IFREG=普通文件, 0644=rw-r--r--
    inode_set_mtime_to_ts(inode, inode_set_atime_to_ts(inode, inode_set_ctime_to_ts(inode, current_time(inode))));
    
    // 关键：绑定我们定义的操作函数
    inode->i_fop = &smartfishfs_fops;

    // 4. 将 Dentry 与 Inode 绑定 (实例化)
    // 这一步之后，文件就在 VFS 树上可见了
    d_add(dentry, inode);
}

// --- 超级块操作 ---
static const struct super_operations smartfishfs_ops = {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
};

// --- 初始化超级块 ---
static int smartfishfs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *inode;

    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = SMARTFISH_MAGIC;
    sb->s_op = &smartfishfs_ops;
    sb->s_time_gran = 1;

    // 1. 创建根目录 Inode
    inode = new_inode(sb);
    if (!inode) return -ENOMEM;

    inode->i_ino = 1;
    inode->i_mode = S_IFDIR | 0755;
    inode_set_mtime_to_ts(inode, inode_set_atime_to_ts(inode, inode_set_ctime_to_ts(inode, current_time(inode))));
    
    // 根目录继续使用简单的通用操作
    inode->i_op = &simple_dir_inode_operations;
    inode->i_fop = &simple_dir_operations;
    set_nlink(inode, 2);

    // 2. 创建根 Dentry
    sb->s_root = d_make_root(inode);
    if (!sb->s_root) return -ENOMEM;

    // 3. 【新增步骤】在根目录下创建一个静态文件
    smartfishfs_create_file(sb, sb->s_root, "readme.txt");

    pr_info("smartfishfs: superblock initialized, root and file created!\n");
    return 0;
}

static struct dentry *smartfishfs_mount(struct file_system_type *fs_type,
                                        int flags, const char *dev_name, void *data)
{
    return mount_nodev(fs_type, flags, data, smartfishfs_fill_super);
}

static struct file_system_type smartfishfs_type = {
    .owner = THIS_MODULE,
    .name = "smartfishfs",
    .mount = smartfishfs_mount,
    .kill_sb = kill_litter_super,
    .fs_flags = FS_USERNS_MOUNT,
};

static int __init smartfishfs_init(void)
{
    return register_filesystem(&smartfishfs_type);
}

static void __exit smartfishfs_exit(void)
{
    unregister_filesystem(&smartfishfs_type);
}

module_init(smartfishfs_init);
module_exit(smartfishfs_exit);