// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

static int __init hello_init(void)
{
	pr_info("hello_mod: loaded, hello from the kernel module\n");
	return 0;
}

static void __exit hello_exit(void)
{
	pr_info("hello_mod: unloaded, goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple HW1 test module");
