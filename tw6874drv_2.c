/*
 * tw6874drv_1.c
 *
 *  Created on: 2016-12-8
 *      Author: eternal
 */
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>

#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/io.h>

#include "tw6874.h"
#include "tw6874_ioctl_cmd.h"

unsigned char TW6874_SLAVE_ID0=0xd2;

unsigned char bAutoEQlock=0;
U8 bAutoCh[4];

static unsigned char handle_tw6874_status_once(unsigned char chn){
	unsigned char souce_format=RES_NONE;
	//printk("handle chn:%d\n",chn);
	souce_format=check_tw6874_input_source(chn);
	//printk("format code:%d\n",souce_format);
	return souce_format;
}
static void tw6874_initialization(void){
	//assert_reset_TW6874();
	TW6874_Init();
}

static long tw6874_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	unsigned char* temp;
	switch(cmd)
		{
			case CMD_CHECK_ADN_LOCK_SDI:
				temp=(unsigned char*)arg;
				*temp=handle_tw6874_status_once(*temp);
				break;
			default:
				printk("Unrecongnised command.\n");
				return -1;
		}
	return 0;
}

static int tw6874_open(struct inode * inode, struct file * file) {
	return 0;
}
static int tw6874_close(struct inode * inode, struct file * file) {
	return 0;
}

static struct file_operations tw6874_fops = {
		.owner = THIS_MODULE,
		.unlocked_ioctl = tw6874_ioctl,
		.open = tw6874_open,
		.release = tw6874_close
};

static struct miscdevice tw6874_dev = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "tw6874_driver_2",
		.fops = &tw6874_fops,
};


static int __init tw6874_init(void) {
	int ret;
	printk("tw6874 initialization...\n");
	tw6874_initialization();

	ret = misc_register(&tw6874_dev);
	if (0 != ret)
		return -1;
	printk("tw6874 initialization success!\n");
	return 0;
}

static void __exit tw6874_exit(void) {
	misc_deregister(&tw6874_dev);
}

module_init(tw6874_init);
module_exit(tw6874_exit);
MODULE_LICENSE("GPL");

