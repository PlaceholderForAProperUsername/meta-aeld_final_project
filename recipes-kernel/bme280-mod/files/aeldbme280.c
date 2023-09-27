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

#define BME280_DEVICE_NAME  ("aeldbme280")

static struct aeld_bme280_comp_param {
    u16 dig_T1;
    s16 dig_T2;
    s16 dig_T3;
    
    s32 comp_temp;

    u16 dig_P1;
    s16 dig_P2;
    s16 dig_P3;
    s16 dig_P4;
    s16 dig_P5;
    s16 dig_P6;
    s16 dig_P7;
    s16 dig_P8;
    s16 dig_P9;

    u8 dig_H1;
    s16 dig_H2;
    u8 dig_H3;
    s16 dig_H4;
    s16 dig_H5;
    s8 dig_H6;
};

struct aeld_bme280_dev {
  struct i2c_client *client;
  dev_t devt;
  float temperatue;
  float pressure;
  float humidity;
  struct aeld_bme280_comp_param comp_param;
};

static struct class *aeld_bme280_class = NULL;

static int aeld_bme280_write_cmd(aeld_bme280_dev *bme280p, u8 reg_addr, u8 cmd)
{
  struct i2c_msg msg[2];
  int status = 0;
  msg[0].addr = bme280p->client->adapter;
  msg[0].flags = 0;
  msg[0].len = 1;
  msg[0].buf = &reg_addr;
  
  msg[1].addr = bme280p->client->adapter;
  msg[1].flags = 0;
  msg[1].len = 1;
  msg[1].buf = &cmd;
  
  status = i2c_transfer(bme280p->client->adapter, msg, 2);
  if (status < 0)
  {
    pr_err("write cmd failed\n");
  }
  return status;
}

static int aeld_bme280_read_bytes(aeld_bme280_dev *bme280p, u8 reg_addr, u8 *buf, u8 len)
{
  struct i2c_msg msg[2];
  int status = 0;
  msg[0].addr = bme280p->client->adapter;
  msg[0].flags = 0;
  msg[0].len = 1;
  msg[0].buf = &reg_addr;
  
  msg[1].addr = bme280p->client->adapter;
  msg[1].flags = I2C_M_RD;
  msg[1].len = len;
  msg[1].buf = buf;
  
  status = i2c_transfer(bme280p->client->adapter, msg, 2);
  if (status < 0)
  {
    pr_err("write cmd failed\n");
  }
  return status;
}

static int aeld_bme280_com_param_init(aeld_bme280_dev *bme280p)
{
  u8 comp_param_block_1[26];
  u8 comp_param_block_2[7];
  
  aeld_bme280_read_bytes(bme280p, 0x88, &comp_param_block_1[0], 26);
  aeld_bme280_read_bytes(bme280p, 0xe1, &comp_param_block_2[0], 7);
  
  bme280p->comp_param.dig_T1 = (u16) (comp_param_block_1[1] << 8) | comp_param_block_1[0];
  bme280p->comp_param.dig_T2 = (s16) ((comp_param_block_1[3] << 8) | comp_param_block_1[2]);
  bme280p->comp_param.dig_T3 = (s16) ((comp_param_block_1[5] << 8) | comp_param_block_1[4]);
  bme280p->comp_param.dig_P1 = (u16) (comp_param_block_1[7] << 8) | comp_param_block_1[6];
  bme280p->comp_param.dig_P2 = (s16) ((comp_param_block_1[9] << 8) | comp_param_block_1[8]);
  bme280p->comp_param.dig_P3 = (s16) ((comp_param_block_1[11] << 8) | comp_param_block_1[10]);
  bme280p->comp_param.dig_P4 = (s16) ((comp_param_block_1[13] << 8) | comp_param_block_1[12]);
  bme280p->comp_param.dig_P5 = (s16) ((comp_param_block_1[15] << 8) | comp_param_block_1[14]);
  bme280p->comp_param.dig_P6 = (s16) ((comp_param_block_1[17] << 8) | comp_param_block_1[16]);
  bme280p->comp_param.dig_P7 = (s16) ((comp_param_block_1[19] << 8) | comp_param_block_1[18]);
  bme280p->comp_param.dig_P8 = (s16) ((comp_param_block_1[21] << 8) | comp_param_block_1[20]);
  bme280p->comp_param.dig_P9 = (s16) ((comp_param_block_1[23] << 8) | comp_param_block_1[22]);
  bme280p->comp_param.dig_H1 = comp_param_block_1[25];
  bme280p->comp_param.dig_H2 = (s16) ((comp_param_block_2[1] << 8) | comp_param_block_2[0]);
  bme280p->comp_param.dig_H3 = comp_param_block_2[2];
  bme280p->comp_param.dig_H4 = (s16) ((comp_param_block_2[3] << 4) | (comp_param_block_2[4] & 0x0F));
  bme280p->comp_param.dig_H5 = (s16) ((comp_param_block_2[5] << 4) | (comp_param_block_2[4] >> 4));
  bme280p->comp_param.dig_H6 = (s8) comp_param_block_2[6];
  
  return 0;
}

// TODO

static int aeld_bme280_comp_temp(aeld_bme280_dev *bme280p, int raw_temperature)
{
  return 0;
}

static int aeld_bme280_comp_press(aeld_bme280_dev *bme280p, int raw_pressure)
{
  return 0;
}

static int aeld_bme280_comp_temp(aeld_bme280_dev *bme280p, int raw_humidity)
{
  return 0;
}

static int aeld_bme280_do_measurement(aeld_bme280_dev *bme280p)
{
  return 0;
}

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
