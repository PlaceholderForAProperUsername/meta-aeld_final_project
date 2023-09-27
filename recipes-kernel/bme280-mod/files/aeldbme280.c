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


// ctrl_meas register
#define CTRL_MEAS_REG_ADDR      0xF4
#define CMD_T_OVERSAMPLING      0x20 // set to factor of 1
#define CMD_P_OVERSAMPLING      0x04 // set to factor of 1
#define CMD_FORCED_MODE         0x01
#define CTRL_MEAS_REG_VAL       (CMD_T_OVERSAMPLING | CMD_P_OVERSAMPLING)
#define ACTIVATE_FORCED_MODE    (CTRL_MEAS_REG_VAL | CMD_FORCED_MODE)

// ctrl_hum register
#define CTRL_HUM_REG_ADDR       0xF2
#define CMD_H_OVERSAMPLING      0x01

#define STATUS_REG_ADDR         0xF3

#define IS_MEASURING_BIT        (1<<3)
#define IS_RESETING_BIT         (1<<0)

#define MEAS_DATA_START_ADDR    0xF7

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
  struct aeld_bme280_comp_param comp_param;
};

static struct class *aeld_bme280_class = NULL;

static int aeld_bme280_write_cmd(struct aeld_bme280_dev *bme280p, u8 reg_addr, u8 cmd)
{
  struct i2c_msg msg[2];
  int status = 0;
  msg[0].addr = bme280p->client->addr;
  msg[0].flags = 0;
  msg[0].len = 1;
  msg[0].buf = &reg_addr;
  
  msg[1].addr = bme280p->client->addr;
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

static int aeld_bme280_read_bytes(struct aeld_bme280_dev *bme280p, u8 reg_addr, u8 *buf, u8 len)
{
  struct i2c_msg msg[2];
  int status = 0;
  msg[0].addr = bme280p->client->addr;
  msg[0].flags = 0;
  msg[0].len = 1;
  msg[0].buf = &reg_addr;
  
  msg[1].addr = bme280p->client->addr;
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

static int aeld_bme280_com_param_init(struct aeld_bme280_dev *bme280p)
{
  u8 comp_param_block_1[26];
  u8 comp_param_block_2[7];
  
  aeld_bme280_read_bytes(bme280p, 0x88, &comp_param_block_1[0], 26);
  aeld_bme280_read_bytes(bme280p, 0xE1, &comp_param_block_2[0], 7);
  
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

static double aeld_bme280_comp_temp(struct aeld_bme280_dev *bme280p, int raw_temperature)
{
  double tmp1, tmp2, temperature;
  tmp1 = (((double) raw_temperature)/16384.0 - ((double)bme280p->comp_param.dig_T1)/1024.0) * ((double) bme280p->comp_param.dig_T2);
  tmp2 = ((((double) raw_temperature)/131072.0 - ((double)bme280p->comp_param.dig_T1) / 8192.0) * 
    (((double)raw_temperature)/131072.0 - ((double)bme280p->comp_param.dig_T1)/8192.0)) * ((double)bme280p->comp_param.dig_T3);
  bme280p->comp_param.comp_temp = (s32) (tmp1 + tmp2);
  temperature = (tmp1 + tmp2) / 5120.0;
  return temperature;
}

static double aeld_bme280_comp_press(struct aeld_bme280_dev *bme280p, int raw_pressure)
{
  double tmp1, tmp2, pressure;
  tmp1 = ((double) bme280p->comp_param.comp_temp / 2.0) - 64000.0;
  tmp2 = tmp1 * tmp1 * ((double)bme280p->comp_param.dig_P6) / 32768.0;
  tmp2 = tmp2 + tmp1 * ((double) bme280p->comp_param.dig_P5) * 2.0;
  tmp2 = (tmp2 / 4.0) + (((double) bme280p->comp_param.dig_P4) * 65536.0);
  tmp1 = (((double) bme280p->comp_param.dig_P3) * tmp1 * tmp1 / 524288.0 + ((double) bme280p->comp_param.dig_P2) * tmp1) / 524288.0;
  tmp1 = (1.0 + tmp1 / 32768.0) * ((double) bme280p->comp_param.dig_P1);
  if (tmp1 == 0.0)
  {
    return 0;
  }
  pressure = 1048576.0 - (double) raw_pressure;
  pressure = (pressure - (tmp2 / 4096.0)) * 6250.0 / tmp1;
  tmp1 = ((double) bme280p->comp_param.dig_P9) * pressure * pressure / 2147483648.0;
  tmp2 = pressure * ((double) bme280p->comp_param.dig_P8) / 32768.0;
  pressure = pressure + (tmp1 + tmp2 + ((double) bme280p->comp_param.dig_P7)) / 16.0;
  return pressure;
}

static double aeld_bme280_comp_hum(struct aeld_bme280_dev *bme280p, int raw_humidity)
{
  double humidity;
  humidity = (((double) bme280p->comp_param.comp_temp) - 76800.0);
  humidity = (raw_humidity - (((double) bme280p->comp_param.dig_H4) * 64.0 +((double) bme280p->comp_param.dig_H5) / 16348.0 * 
    humidity)) * (((double) bme280p->comp_param.dig_H2) / 65536.0 * (1.0 + ((double) bme280p->comp_param.dig_H6) / 67108864.0 * 
    humidity * (1.0 + ((double) bme280p->comp_param.dig_H3) / 67108864.0 * humidity)));
  humidity = humidity * (1.0 - ((double) bme280p->comp_param.dig_H1) * humidity / 524288.0);
  if (humidity > 100.0)
  {
    humidity = 100.0;
  }
  else if (humidity < 0.0)
  {
    humidity = 0.0;
  }
  return humidity;
}

static int aeld_bme280_do_measurement(struct aeld_bme280_dev *bme280p, double result[])
{
  u8 is_measuring;
  u8 raw_data[8] = {0};
  int raw_temperature;
  int raw_pressure;
  int raw_humidity;
  aeld_bme280_write_cmd(bme280p, CTRL_MEAS_REG_ADDR, ACTIVATE_FORCED_MODE);
  mdelay(10);
  do
  {
    mdelay(1);
    aeld_bme280_read_bytes(bme280p, STATUS_REG_ADDR, &is_measuring, 1);
  }
  while (is_measuring | IS_MEASURING_BIT);
  aeld_bme280_read_bytes(bme280p, MEAS_DATA_START_ADDR, &raw_data[0], 8);
  raw_pressure = (int) (raw_data[0] << 12) | (raw_data[1] << 4) | (raw_data[2] >> 4);
  raw_temperature = (int) (raw_data[3] << 12) | (raw_data[4] << 4) | (raw_data[5] >> 4);
  raw_humidity = (int) (raw_data[6] << 8) | (raw_data[7]);
  result[0] = aeld_bme280_comp_temp(bme280p, raw_temperature);
  result[1] = aeld_bme280_comp_press(bme280p, raw_pressure);
  result[2] = aeld_bme280_comp_hum(bme280p, raw_humidity);
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

static ssize_t aeld_bme280_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
  double result[3] = {0};
  struct aeld_bme280_dev *bme280p = filp->private_data;
  aeld_bme280_do_measurement(bme280p, &result[0]);
  copy_to_user(buf, result, sizeof(result));
  return 0;
}

static struct file_operations aeld_bme280_fops = {
  .owner          = THIS_MODULE,
  .open           = aeld_bme280_open,
  .release        = aeld_bme280_release,
  .read           = aeld_bme280_read,
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
  u8 is_resetting;
  
  aeld_bme280 = devm_kzalloc(&client->dev, sizeof(struct aeld_bme280_dev), GFP_KERNEL);
  
  major = register_chrdev(0, BME280_DEVICE_NAME, &aeld_bme280_fops);
  if (major < 0)
  {
    pr_err("Failed to register device\n");
    return major;
  }
  aeld_bme280->client = client;
  aeld_bme280->devt = MKDEV(major, 0);
  device = device_create(aeld_bme280_class, NULL, aeld_bme280->devt, NULL, BME280_DEVICE_NAME);
  
  if (IS_ERR(device))
  {
    pr_err("Failed to create device\n");
    goto fail;
  }
  
  mdelay(10);
  
  do
  {
    mdelay(1);
    aeld_bme280_read_bytes(aeld_bme280, STATUS_REG_ADDR, &is_resetting, 1);
  }
  while (is_resetting | IS_RESETING_BIT);
  
  aeld_bme280_com_param_init(aeld_bme280);
  aeld_bme280_write_cmd(aeld_bme280, CTRL_HUM_REG_ADDR, CMD_H_OVERSAMPLING);
  aeld_bme280_write_cmd(aeld_bme280, CTRL_MEAS_REG_ADDR, CTRL_MEAS_REG_VAL);
  
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

static void __exit aeld_bme280_driver_exit(void)
{
  i2c_del_driver(&aeld_bme280_i2c_driver);
  class_destroy(aeld_bme280_class);
}

module_init(aeld_bme280_driver_init);
module_exit(aeld_bme280_driver_exit);

MODULE_DESCRIPTION("BME280 driver");
MODULE_AUTHOR("PlaceholderForAProperUsername");
MODULE_LICENSE("GPL");
