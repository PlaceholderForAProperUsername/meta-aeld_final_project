#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/err.h>

#define RS  (5)
#define E   (6)
#define D4  (21)
#define D5  (20)
#define D6  (26)
#define D7  (16)

#define pr_fmt(fmt) "aeld_HD44780: " fmt

dev_t dev = 0;
static struct cdev aeld_HD44780_cdev;

static int __init aeld_HD44780_init(void);
static void __exit aeld_HD44780_exit(void);

static int aeld_HD44780_open(struct inode *inode, struct file *file);
static int aeld_HD44780_release(struct inode *inode, struct file *file);
static ssize_t aeld_HD44780_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);
static loff_t aeld_HD44780_llseek(struct file *filp, loff_t offset, int whence);

static int aeld_HD44780_open(struct inode *inode, struct file *file)
{
  pr_info("Open device\n");
  return 0;
}

static int aeld_HD44780_release(struct inode *inode, struct file *file)
{
  pr_info("Closing device\n");
  return 0;
}

static ssize_t aeld_HD44780_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
  return 0;
}
static loff_t aeld_HD44780_llseek(struct file *filp, loff_t offset, int whence)
{
  return 0;
}

static struct file_operations aeld_HD44780_fops =
{
  .owner    = THIS_MODULE,
  .write    = aeld_HD44780_write,
  .llseek   = aeld_HD44780_llseek,
  .open     = aeld_HD44780_open,
  .release  = aeld_HD44780_release, 
};

static int __init aeld_HD44780_init(void)
{
  pr_info("Loading Module\n");
  
  if (alloc_chrdev_region(&dev, 0, 1, "aeld_HD44780") < 0)
  {
    pr_err("Can't get major number\n");
    goto unregister;
  }
  pr_info("Device major number = %d\n", MAJOR(dev));
  
  cdev_init(&aeld_HD44780_cdev, &aeld_HD44780_fops);
  if (cdev_add(&aeld_HD44780_cdev, dev, 1) < 0)
  {
    pr_err("Device could not be added to the system\n");
    goto delete;
  }
  
  if (gpio_request(RS, "GPIO5") < 0)
  {
    pr_err("Request of GPIO %d failed\n", RS);
    goto gpio_rs;
  }
  
  if (gpio_request(E, "GPIO6") < 0)
  {
    pr_err("Request of GPIO %d failed\n", E);
    goto gpio_e;
  }
  
  if (gpio_request(D4, "GPIO21") < 0)
  {
    pr_err("Request of GPIO %d failed\n", D4);
    goto gpio_d4;
  }
  
  if (gpio_request(D5, "GPIO20") < 0)
  {
    pr_err("Request of GPIO %d failed\n", D5);
    goto gpio_d5;
  }
  
  if (gpio_request(D6, "GPIO26") < 0)
  {
    pr_err("Request of GPIO %d failed\n", D6);
    goto gpio_d6;
  }
  
  if (gpio_request(D7, "GPIO16") < 0)
  {
    pr_err("Request of GPIO %d failed\n", D7);
    goto gpio_d7;
  }
  
  pr_info("Module loading complete\n");
  
  return 0;

gpio_d7:
  gpio_free(D7);
gpio_d6:
  gpio_free(D6);
gpio_d5:
  gpio_free(D5);
gpio_d4:
  gpio_free(D4);
gpio_e:
  gpio_free(E);
gpio_rs:
  gpio_free(RS);
delete:
  cdev_del(&aeld_HD44780_cdev);
unregister:
  unregister_chrdev_region(dev,1);
  pr_err("Module loading failed\n");
  return -1;
}

static void __exit aeld_HD44780_exit(void)
{
  gpio_free(D7);
  gpio_free(D6);
  gpio_free(D5);
  gpio_free(D4);
  gpio_free(E);
  gpio_free(RS);
  cdev_del(&aeld_HD44780_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info("Module unloaded\n");
}

module_init(aeld_HD44780_init);
module_exit(aeld_HD44780_exit);

MODULE_DESCRIPTION("Minimal implementation to use HD44780 LCD device for aeld final project");
MODULE_AUTHOR("PlaceholderForAProperUsername");
MODULE_LICENSE("GPL");
