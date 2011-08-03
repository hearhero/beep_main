#include <linux/module.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/io.h>

#include "beep.h"

int beep_major = 248;
int beep_minor = 0;
int cnt_def = 6;

struct class *beep_class;
struct cdev cdev;

unsigned long *GPBCON = NULL;
unsigned long *TCFG0 = NULL;
unsigned long *TCFG1 = NULL;
unsigned long *TCNTB0 = NULL;
unsigned long *TCMPB0 = NULL;
unsigned long *TCON = NULL;

static DEFINE_SPINLOCK(beep_lock);

int beep_count = 0;

static int beep_open(struct inode *inode, struct file *filp)
{
	spin_lock(&beep_lock);

	if (0 < beep_count)
	{
		spin_unlock(&beep_lock);
		return -EBUSY;
	}

	beep_count++;
	spin_unlock(&beep_lock);

	*TCFG0 &= ~0xff;

	*TCFG1 &= ~0xf;
	*TCFG1 |= 0x1;

	*TCNTB0 = cnt_def;
	*TCMPB0 = cnt_def / 2;

	*TCON &= ~0xf;
	*TCON |= 0xa;			

	return 0;
}

static int beep_release(struct inode *node, struct file *filp)
{
	spin_lock(&beep_lock);
	beep_count--;
	spin_unlock(&beep_lock);

	*TCON &= ~0xf;

	return 0;
}

static int beep_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) 
	{
	case BEEP_ON:
		*GPBCON &= ~0x3;
		*GPBCON |= 0x2;

		*TCON &= ~0xf;
		*TCON |= 0x9;
		break;
	case BEEP_OFF:
		*TCON &= ~0xf;
		break;
	case BEEP_CNT:
		*TCON &= ~0xf;

		*TCNTB0 = arg;
		*TCMPB0 = arg / 2;
		*TCON &= ~ 0xf;
		*TCON |= 0xa;

		*GPBCON &= ~0x3;
		*GPBCON |= 0x2;

		*TCON &= ~0xf;
		*TCON |= 0x9;
		break;
	case BEEP_PRE:
		*TCON &= ~0xf;

		*TCFG0 &= ~0xff;
		*TCFG0 |= arg & 0xff;

		*GPBCON &= ~0x3;
		*GPBCON |= 0x2;

		*TCON &= ~0xf;
		*TCON |= 0x9;
		break;
	case BEEP_DEF:
		*TCON &= ~0xf;

		*TCFG0 &= ~0xff;
		*TCNTB0 = cnt_def;
		*TCMPB0 = cnt_def / 2;
		*TCON &= ~ 0xf;
		*TCON |= 0xa;	

		*GPBCON &= ~0x3;
		*GPBCON |= 0x2;

		*TCON &= ~0xf;
		*TCON |= 0x9;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

struct file_operations beep_fops = {
	.owner   = THIS_MODULE,
	.open    = beep_open,
	.release = beep_release,
	.ioctl   = beep_ioctl,	
};

static void char_reg_setup_cdev(void)
{
	int error;
	dev_t dev;

	dev = MKDEV(beep_major, beep_minor);

	cdev_init(&cdev, &beep_fops);
	cdev.owner = THIS_MODULE;
	cdev.ops = &beep_fops;
	error = cdev_add(&cdev, dev, 1);

	if (0 > error)
	{
		printk(KERN_NOTICE "Error %d adding char_reg_setup_cdev\n", error);
		return;
	}

	beep_class = class_create(THIS_MODULE, "beep_class");

	if (IS_ERR(beep_class))
	{
		printk("Failed to create beep_class\n");
		return;
	}

	device_create(beep_class, NULL, dev, NULL, "BEEP");
}

static int __init beep_init(void)
{
	int ret = 0;
	dev_t dev;

	dev = MKDEV(beep_major, beep_minor);

	ret = register_chrdev_region(dev, 1, "BEEP");

	if(0 > ret)
	{
		printk(KERN_WARNING "beep: can't get the major number %d\n", beep_major);
		return ret;
	}

	char_reg_setup_cdev();

	TCFG0 = (unsigned long *)ioremap(0x51000000, 4);
	TCFG1 = (unsigned long *)ioremap(0x51000004, 4);
	TCON = (unsigned long *)ioremap(0x51000008, 4);
	TCNTB0 = (unsigned long *)ioremap(0x5100000c, 4);
	TCMPB0 = (unsigned long *)ioremap(0x51000010, 4);
	GPBCON = (unsigned long *)ioremap(0x56000010, 4);

	return ret;
}

static void __exit beep_exit(void)
{
	dev_t dev;

	dev = MKDEV(beep_major, beep_minor);

	device_destroy(beep_class, dev);
	class_destroy(beep_class);

	cdev_del(&cdev);
	unregister_chrdev_region(dev, 1);

	iounmap(TCFG0);
	iounmap(TCFG1);
	iounmap(TCON);
	iounmap(TCNTB0);
	iounmap(TCMPB0);
	iounmap(GPBCON);
}

module_init(beep_init);
module_exit(beep_exit);

MODULE_LICENSE("GPL");
