//SPDX-License-Identifer: GPL-3.0
// *Copyright (c) 2021 Ryuichi Ueda and Ryota Hama. All rights reserved.

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/device.h>

MODULE_AUTHOR("Ryuichi Ueda & Ryota Hama");
MODULE_DESCRIPTION("driver for LED control");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.2");

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;
static volatile u32 *gpio_base = NULL;

void long_flash(void){
	gpio_base[7] = 1 << 25;
	ssleep(1);
	gpio_base[10] = 1 << 25;
	msleep(200);
}

void short_flash(void){
	gpio_base[7] = 1 << 25;
	msleep(200);
	gpio_base[10] = 1 << 25;
	msleep(200);
}
 


static ssize_t led_write(struct file*filp, const char*buf, size_t count, loff_t* pos)
{
	char c;
	int i;

	if(copy_from_user(&c, buf, sizeof(char)))
		return -EFAULT;

	if(c == '0'){
		for(i=100; i > 0; i--){
			gpio_base[7] = 1 << 25;
			msleep(i);
			gpio_base[10] = 1 << 25;
			msleep(i);
		}
	}
	else if(c == '1'){
		for(i=0; i <= 10; i++){
			short_flash();
			short_flash();
			msleep(300);
			gpio_base[7] = 1 << 25;
			msleep(500);
			gpio_base[10] = 1 << 25;
                }
        }
        else if(c == '2'){
		for(i=0; i<=3; i++){
			long_flash();
		}
		ssleep(2);

		for(i=0; i<=1; i++){
			short_flash();
			long_flash();
		}
		short_flash();
		ssleep(2);

 		for(i=0; i<=1; i++){
			long_flash();
			short_flash();
		}
		ssleep(2);
		
		for(i=0; i<=1; i++){
			short_flash();
		}
		long_flash();
		short_flash();
                ssleep(2);


		long_flash();
		for(i=0; i<=2; i++){
			short_flash();
		}
	}
        return 1;
}

static ssize_t sushi_read(struct file* flip, char* buf, size_t count, loff_t* pos)
{
	int size = 0;
	char sushi[] = {'s','u','s','h','i',0x0A};
	if(copy_to_user(buf+size, (const char *)sushi, sizeof(sushi))){
		printk( KERN_INFO "sushi : copy_to_user failed\n");
		return -EFAULT;
	}
	size += sizeof(sushi);
	return size;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.write = led_write,
	.read = sushi_read
};

static int __init init_mod(void)
{
	int retval;
	retval = alloc_chrdev_region(&dev, 0, 1, "myled");
	if(retval < 0){
		printk(KERN_ERR "alloc_chrdev_region failed.\n");
		return retval;
	}


		printk(KERN_INFO "%s is loaded. major: %d\n",__FILE__,MAJOR(dev));

		cdev_init(&cdv, &led_fops);
		retval = cdev_add(&cdv, dev, 1);
		if(retval < 0){
			printk(KERN_ERR "chev_add failed. major:%d, minor:%d\n",MAJOR(dev),MINOR(dev));
			return retval;
		}

		cls = class_create(THIS_MODULE,"myled");
                if(IS_ERR(cls)){
			printk(KERN_ERR "class_create failed.");
			return PTR_ERR(cls);
		}

		device_create(cls, NULL, dev, NULL, "myled%d",MINOR(dev));

		gpio_base = ioremap_nocache(0xfe200000, 0x0A);

		const u32 led = 25;
		const u32 index = led/10;
		const u32 shift = (led%10)*3;
		const u32 mask = ~(0x7 << shift);
		gpio_base[index] = (gpio_base[index] & mask ) | (0x1 << shift);

		return 0;
}

static void __exit cleanup_mod(void)
{
		cdev_del(&cdv);
		device_destroy(cls, dev);
		class_destroy(cls);
		unregister_chrdev_region(dev, 1);
		printk(KERN_INFO "%s is unloaded. major:%d\n",__FILE__,MAJOR(dev));
}
module_init(init_mod);
module_exit(cleanup_mod);
