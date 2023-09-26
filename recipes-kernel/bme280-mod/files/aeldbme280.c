#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/types.h>

#define pr_fmt(fmt) "aeld_BME280: " fmt

#define I2C_BUS             (1)
#define BME280_DEVICE_NAME  ("aeldbme280")
#define BME280_SLAVE_ADDR   (0x76)

static struct aeld_bme280_comp_param {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    
    int32_t comp_temp;

    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;

    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
};

struct aeld_bme280_dev {
  struct i2c_client *client;
  dev_t devt;
  struct aeld_bme280_comp_param comp_param;
};

static struct class *aeld_bme280_class = NULL;

static int aeld_bme280_open(struct inode *inode, struct file *filp)
{
  pr_info("Device opened\n");
  return 0;
}

static int aeld_bme280_release(struct inode *inode, struct file *filp)
{
  pr_info("Device closed\n");
  return 0;
}

static int aeld_bme280_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
  return 0;
}

static struct file_operations aeld_bme280_fops = {
  .owner          = THIS_MODULE,
  .open           = aeld_bme280_open,
  .release        = aeld_bme280_release,
  .unlocked_ioctl = aeld_bme280_ioctl,
};

static const struct of_device_id aeld_bme280_dt_ids[] = {
  { .compatible = "custom,aeldbme280"},
  {},
};
MODULE_DEVICE_TABLE(of, aeld_bme280_dt_ids);

static int aeld_bme280_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
  int major;
  int err = 0;
  struct aeld_bme280_dev *aeld_bme280 = NULL;
  struct device *device = NULL;
  
  aeld_bme280 = devm_kzalloc(&client->dev, sizeof(struct aeld_bme280_dev), GFP_KERNEL);
  
  major = register_chrdev(0, BME280_DEVICE_NAME, &aeld_bme280_fops);
  if (major < 0)
  {
    pr_err("Failed to register device\n");
    return major;
  }
  aeld_bme280_dev->client = client;
  aeld_bme280_dev->devt = MKDEV(major, 0);
  device = device_create(aeld_bme280_class, NULL, aeld_bme280->devt, NULL, BME280_DEVICE_NAME);
  
  if (IS_ERR(device))
  {
    pr_err("Failed to create device\n");
    goto fail;
  }
  return 0;
fail:
  kfree(aeld_bme280);
  return PTR_ERR(device);
}

static int aeld_bme280_remove(struct i2c_client *client)
{
  struct aeld_bme280_dev *aeld_bme280 = i2c_get_clientdata(client);
  device_destroy(aeld_bme280_class, aeld_bme280->devt);
  return 0;
}

static const struct i2c_device_id aeld_bme280_id[] = {
  {BME280_DEVICE_NAME, 0},
  {},
};
MODULE_DEVICE_TABLE(i2c, aeld_bme280_id);

static struct i2c_driver aeld_bme280_i2c_driver = {
  .driver = {
    .owner = THIS_MODULE,
    .name = BME280_DEVICE_NAME,
    .of_match_table = of_match_ptr(aeld_bme280_dt_ids),
  },
  .probe = aeld_bme280_probe,
  .remove = aeld_bme280_remove,
  .id_table = aeld_bme280_id,
};

static int __init aeld_bme280_driver_init(void)
{
  int status;
  aeld_bme280_class = class_create(THIS_MODULE, BME280_DEVICE_NAME);
  if (IS_ERR(aeld_bme280_class))
  {
    pr_err("Cannot create class\n");
    return PTR_ERR(aeld_bme280_class);
  }
  status = i2c_register_driver(THIS_MODULE, &aeld_bme280_i2c_driver);
  if (status < 0)
  {
    class_destroy(aeld_bme280_class);
  }
  return status;
}

static void __exit aeld_bme280_exit(void)
{
  i2c_del_driver(&aeld_bme280_i2c_driver);
  class_destroy(aeld_bme280_class);
}

module_init(aeld_bme280_driver_init);
module_exit(aeld_bme280_driver_exit);


MODULE_DESCRIPTION("BME280 driver");
MODULE_AUTHOR("PlaceholderForAProperUsername");
MODULE_LICENSE("GPL");
