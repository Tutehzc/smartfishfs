#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

// 模块许可证，GPL是必须的，否则无法使用很多内核API
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Smartfish Developer");
MODULE_DESCRIPTION("A simple in-memory file system: smartfishfs");
MODULE_VERSION("0.1");

// 模块加载函数
static int __init smartfishfs_init(void)
{
    pr_info("smartfishfs: module loaded successfully!\n");
    return 0;
}

// 模块卸载函数
static void __exit smartfishfs_exit(void)
{
    pr_info("smartfishfs: module unloaded, see you again!\n");
}

// 注册加载和卸载的入口
module_init(smartfishfs_init);
module_exit(smartfishfs_exit);