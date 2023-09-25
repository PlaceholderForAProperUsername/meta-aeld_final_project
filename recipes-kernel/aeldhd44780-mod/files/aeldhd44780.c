#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define GPIO_RS  (5)
#define GPIO_E   (6)
#define GPIO_D4  (21)
#define GPIO_D5  (20)
#define GPIO_D6  (26)
#define GPIO_D7  (16)

#define CMD_ENTRY_MODE      (0x06)
#define CMD_DISP_ON_CUR_OFF (0x0C)
#define CMD_4_BIT_2_LINE    (0x28)
#define CMD_CLEAR_DISP      (0x01)
#define CMD_RETURN_HOME     (0x02)

#define pr_fmt(fmt) "aeld_HD44780: " fmt

dev_t dev = 0;
static struct cdev aeld_HD44780_cdev;
static struct class *aeld_HD44780_class;

static int __init aeld_HD44780_init(void);
static void __exit aeld_HD44780_exit(void);

static int aeld_HD44780_open(struct inode *inode, struct file *file);
static int aeld_HD44780_release(struct inode *inode, struct file *file);
static ssize_t aeld_HD44780_write(struct file *filp, const char __user *msg, size_t len, loff_t *off);
static loff_t aeld_HD44780_llseek(struct file *filp, loff_t offset, int whence);

static int write_cmd(int cmd);

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

static ssize_t aeld_HD44780_write(struct file *filp, const char __user *msg, size_t len, loff_t *off)
{
  char *buf;
  int recv_bytes = 0;
  buf = (char *) kmalloc(len, GFP_KERNEL);
  pr_info("Writing message\n");
  
  recv_bytes = len - copy_from_user(&buf[0], msg, len);
  pr_info("bytes to write %d\n", recv_bytes);
  for (int i = 0; i < recv_bytes; ++i)
  {
    gpio_set_value(GPIO_E, 0); 
    gpio_set_value(GPIO_RS, 1);
    
    gpio_set_value(GPIO_D7, buf[i] & 0x80);
    gpio_set_value(GPIO_D6, buf[i] & 0x40);
    gpio_set_value(GPIO_D5, buf[i] & 0x20);
    gpio_set_value(GPIO_D4, buf[i] & 0x10);
    
    gpio_set_value(GPIO_E, 1);
    gpio_set_value(GPIO_E, 0);
    
    udelay(40);
    
    gpio_set_value(GPIO_D7, buf[i] & 0x08);
    gpio_set_value(GPIO_D6, buf[i] & 0x04);
    gpio_set_value(GPIO_D5, buf[i] & 0x02);
    gpio_set_value(GPIO_D4, buf[i] & 0x01);
    
    gpio_set_value(GPIO_E, 1);
    gpio_set_value(GPIO_E, 0);
    
    udelay(40);
  }
  pr_info("Message complete. %d bytes where written to display\n", recv_bytes);
  return recv_bytes;
}
static loff_t aeld_HD44780_llseek(struct file *filp, loff_t offset, int whence)
{
  pr_info("Changing position of cursor to offset %lld\n", offset);
  gpio_set_value(GPIO_E, 0); 
  gpio_set_value(GPIO_RS, 0);
  
  gpio_set_value(GPIO_D7, 1);
  gpio_set_value(GPIO_D6, offset & 0x40);
  gpio_set_value(GPIO_D5, offset & 0x20);
  gpio_set_value(GPIO_D4, offset & 0x10);
  
  gpio_set_value(GPIO_E, 1);
  gpio_set_value(GPIO_E, 0);
  
  udelay(40);
  
  gpio_set_value(GPIO_D7, offset & 0x08);
  gpio_set_value(GPIO_D6, offset & 0x04);
  gpio_set_value(GPIO_D5, offset & 0x02);
  gpio_set_value(GPIO_D4, offset & 0x01);
  
  gpio_set_value(GPIO_E, 1);
  gpio_set_value(GPIO_E, 0);
  
  udelay(40);
  
  pr_info("Cursor position changed\n");
  return 0;
}

static int write_cmd(int cmd)
{
  mdelay(2);
  gpio_set_value(GPIO_E, 0); 
  gpio_set_value(GPIO_RS, 0);
  
  gpio_set_value(GPIO_D7, cmd & 0x80);
  gpio_set_value(GPIO_D6, cmd & 0x40);
  gpio_set_value(GPIO_D5, cmd & 0x20);
  gpio_set_value(GPIO_D4, cmd & 0x10);
  
  gpio_set_value(GPIO_E, 1);
  gpio_set_value(GPIO_E, 0);
  
  udelay(40);
  
  gpio_set_value(GPIO_D7, cmd & 0x08);
  gpio_set_value(GPIO_D6, cmd & 0x04);
  gpio_set_value(GPIO_D5, cmd & 0x02);
  gpio_set_value(GPIO_D4, cmd & 0x01);
  
  gpio_set_value(GPIO_E, 1);
  gpio_set_value(GPIO_E, 0);
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
  
  if (alloc_chrdev_region(&dev, 0, 1, "aeldhd44780") < 0)
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
  
  aeld_HD44780_class = class_create(THIS_MODULE, "aeldhd44780_class");
  if (IS_ERR(aeld_HD44780_class))
  {
    pr_err("Class could not be created\n");
    goto destroy_class;
  }
  
  if (IS_ERR(device_create(aeld_HD44780_class, NULL, dev, NULL, "aeldhd44780")))
  {
    pr_err("Device could not be created\n");
    goto destroy_device;
  }
  
  if (gpio_is_valid(GPIO_RS) == false)
  {
    pr_err("GPIO %d is not valid\n", GPIO_RS);
    goto destroy_device;
  }
  if (gpio_request(GPIO_RS, "GPIORS") < 0)
  {
    pr_err("Request of GPIO %d failed\n", GPIO_RS);
    goto gpio_rs;
  }
  
  if (gpio_is_valid(GPIO_E) == false)
  {
    pr_err("GPIO %d is not valid\n", GPIO_E);
    goto gpio_rs;
  }
  if (gpio_request(GPIO_E, "GPIOE") < 0)
  {
    pr_err("Request of GPIO %d failed\n", GPIO_E);
    goto gpio_e;
  }
  
  if (gpio_is_valid(GPIO_D4) == false)
  {
    pr_err("GPIO %d is not valid\n", GPIO_D4);
    goto gpio_e;
  }
  if (gpio_request(GPIO_D4, "GPIOD4") < 0)
  {
    pr_err("Request of GPIO %d failed\n", GPIO_D4);
    goto gpio_d4;
  }
  
  if (gpio_is_valid(GPIO_D5) == false)
  {
    pr_err("GPIO %d is not valid\n", GPIO_D5);
    goto gpio_d4;
  }
  if (gpio_request(GPIO_D5, "GPIOD5") < 0)
  {
    pr_err("Request of GPIO %d failed\n", GPIO_D5);
    goto gpio_d5;
  }
  
  if (gpio_is_valid(GPIO_D6) == false)
  {
    pr_err("GPIO %d is not valid\n", GPIO_D6);
    goto gpio_d5;
  }
  if (gpio_request(GPIO_D6, "GPIOD6") < 0)
  {
    pr_err("Request of GPIO %d failed\n", GPIO_D6);
    goto gpio_d6;
  }
  
  if (gpio_is_valid(GPIO_D7) == false)
  {
    pr_err("GPIO %d is not valid\n", GPIO_D7);
    goto gpio_d6;
  }
  if (gpio_request(GPIO_D7, "GPIOD7") < 0)
  {
    pr_err("Request of GPIO %d failed\n", GPIO_D7);
    goto gpio_d7;
  }
  
  // configure gpios as output
  gpio_direction_output(GPIO_RS, 0);
  gpio_direction_output(GPIO_E, 0);
  gpio_direction_output(GPIO_D4, 0);
  gpio_direction_output(GPIO_D5, 0);
  gpio_direction_output(GPIO_D6, 0);
  gpio_direction_output(GPIO_D7, 0);
  
  mdelay(2);
  
  // initialize LCD
  write_cmd(CMD_ENTRY_MODE);
  write_cmd(CMD_DISP_ON_CUR_OFF);
  write_cmd(CMD_4_BIT_2_LINE);
  write_cmd(CMD_CLEAR_DISP);
  write_cmd(CMD_RETURN_HOME);
  
  
  pr_info("Module loading complete\n");
  
  return 0;

gpio_d7:
  gpio_free(GPIO_D7);
gpio_d6:
  gpio_free(GPIO_D6);
gpio_d5:
  gpio_free(GPIO_D5);
gpio_d4:
  gpio_free(GPIO_D4);
gpio_e:
  gpio_free(GPIO_E);
gpio_rs:
  gpio_free(GPIO_RS);
destroy_device:
  device_destroy(aeld_HD44780_class, dev);
destroy_class:
  class_destroy(aeld_HD44780_class);
delete:
  cdev_del(&aeld_HD44780_cdev);
unregister:
  unregister_chrdev_region(dev,1);
  pr_err("Module loading failed\n");
  return -1;
}

static void __exit aeld_HD44780_exit(void)
{
  gpio_free(GPIO_D7);
  gpio_free(GPIO_D6);
  gpio_free(GPIO_D5);
  gpio_free(GPIO_D4);
  gpio_free(GPIO_E);
  gpio_free(GPIO_RS);
  device_destroy(aeld_HD44780_class, dev);
  class_destroy(aeld_HD44780_class);
  cdev_del(&aeld_HD44780_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info("Module unloaded\n");
}

module_init(aeld_HD44780_init);
module_exit(aeld_HD44780_exit);

MODULE_DESCRIPTION("Minimal implementation to use HD44780 LCD device for aeld final project");
MODULE_AUTHOR("PlaceholderForAProperUsername");
MODULE_LICENSE("GPL");
