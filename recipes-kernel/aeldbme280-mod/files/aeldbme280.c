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
#include <linux/uaccess.h>

#define pr_fmt(fmt) "aeld_BME280: " fmt

#define I2C_BUS_AVAILABLE   (1)
#define BME280_DEVICE_ADDR  (0x76)
#define BME280_DEVICE_NAME  ("aeldbme280")
#define BME280_CLASS_NAME   ("aeldbme280_class")


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

struct aeld_bme280_comp_param {
    int32_t comp_temp;
    
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;

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
  struct cdev cdev;
  dev_t devt;
  struct aeld_bme280_comp_param comp_param;
};

static struct i2c_adapter *aeld_bme280_i2c_adapter = NULL;
static struct class *aeld_bme280_class = NULL;
static struct aeld_bme280_dev bme280_dev;

static int aeld_bme280_write_cmd(struct aeld_bme280_dev *bme280p, uint8_t reg_addr, uint8_t cmd)
{
  int status = 0;
  uint8_t msg[2] = {0};
  pr_info("write cmd\n");
  msg[0] = reg_addr;
  msg[1] = cmd;
  status = i2c_master_send(bme280p->client, msg, 2);
  if (status < 0)
  {
    pr_err("write cmd failed\n");
  }
  return status;
}

static int aeld_bme280_read_bytes(struct aeld_bme280_dev *bme280p, uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
  int status = 0;
  pr_info("reading %d bytes \n", len);
  status = i2c_master_send(bme280p->client, &reg_addr, 1);
  status |= i2c_master_recv(bme280p->client, buf, len);
  
  if (status < 0)
  {
    pr_err("read bytes failed\n");
  }
  return status;
}

static int aeld_bme280_com_param_init(struct aeld_bme280_dev *bme280p)
{
  uint8_t comp_param_block_1[26];
  uint8_t comp_param_block_2[7];
  
  aeld_bme280_read_bytes(bme280p, 0x88, &comp_param_block_1[0], 26);
  aeld_bme280_read_bytes(bme280p, 0xE1, &comp_param_block_2[0], 7);
  
  bme280p->comp_param.dig_T1 = (uint16_t) (comp_param_block_1[1] << 8) | comp_param_block_1[0];
  bme280p->comp_param.dig_T2 = (int16_t) ((comp_param_block_1[3] << 8) | comp_param_block_1[2]);
  bme280p->comp_param.dig_T3 = (int16_t) ((comp_param_block_1[5] << 8) | comp_param_block_1[4]);
  bme280p->comp_param.dig_P1 = (uint16_t) (comp_param_block_1[7] << 8) | comp_param_block_1[6];
  bme280p->comp_param.dig_P2 = (int16_t) ((comp_param_block_1[9] << 8) | comp_param_block_1[8]);
  bme280p->comp_param.dig_P3 = (int16_t) ((comp_param_block_1[11] << 8) | comp_param_block_1[10]);
  bme280p->comp_param.dig_P4 = (int16_t) ((comp_param_block_1[13] << 8) | comp_param_block_1[12]);
  bme280p->comp_param.dig_P5 = (int16_t) ((comp_param_block_1[15] << 8) | comp_param_block_1[14]);
  bme280p->comp_param.dig_P6 = (int16_t) ((comp_param_block_1[17] << 8) | comp_param_block_1[16]);
  bme280p->comp_param.dig_P7 = (int16_t) ((comp_param_block_1[19] << 8) | comp_param_block_1[18]);
  bme280p->comp_param.dig_P8 = (int16_t) ((comp_param_block_1[21] << 8) | comp_param_block_1[20]);
  bme280p->comp_param.dig_P9 = (int16_t) ((comp_param_block_1[23] << 8) | comp_param_block_1[22]);
  bme280p->comp_param.dig_H1 = comp_param_block_1[25];
  bme280p->comp_param.dig_H2 = (int16_t) ((comp_param_block_2[1] << 8) | comp_param_block_2[0]);
  bme280p->comp_param.dig_H3 = comp_param_block_2[2];
  bme280p->comp_param.dig_H4 = (int16_t) ((comp_param_block_2[3] << 4) | (comp_param_block_2[4] & 0x0F));
  bme280p->comp_param.dig_H5 = (int16_t) ((comp_param_block_2[5] << 4) | (comp_param_block_2[4] >> 4));
  bme280p->comp_param.dig_H6 = (int8_t) comp_param_block_2[6];
  
  return 0;
}

static int32_t aeld_bme280_comp_temp(struct aeld_bme280_dev *bme280p, int raw_temperature)
{
  int tmp1, tmp2, temperature;
  tmp1 = ((((raw_temperature >> 3) - ((int32_t)bme280p->comp_param.dig_T1<<1))) * ((int32_t) bme280p->comp_param.dig_T2)) >> 11;
  tmp2 = (((((raw_temperature >> 4) - ((int32_t)bme280p->comp_param.dig_T1)) * ((raw_temperature>>4) - ((int32_t) bme280p->comp_param.dig_T1)))
          >> 12) * ((int32_t)bme280p->comp_param.dig_T3)) >> 14;
  bme280p->comp_param.comp_temp = tmp1 + tmp2;
  temperature = (bme280p->comp_param.comp_temp * 5 + 128) >> 8;
  return temperature;
}

static int32_t aeld_bme280_comp_press(struct aeld_bme280_dev *bme280p, int raw_pressure)
{
  int64_t tmp1, tmp2, pressure;
  tmp1 = ((int64_t)bme280p->comp_param.comp_temp) - 128000;
  tmp2 = tmp1 * tmp1 * (int64_t)bme280p->comp_param.dig_P6;
  tmp2 = tmp2 + ((tmp1*(int64_t)bme280p->comp_param.dig_P5)<<17);
  tmp2 = tmp2 + (((int64_t)bme280p->comp_param.dig_P4)<<35);
  tmp1 = ((tmp1 * tmp1 * (int64_t)bme280p->comp_param.dig_P3)>>8) + ((tmp1 * (int64_t)bme280p->comp_param.dig_P2)<<12);
  tmp1 = (((((int64_t)1)<<47)+tmp1))*((int64_t)bme280p->comp_param.dig_P1)>>33;
  if (tmp1 == 0)
  {
  return 0; // avoid exception caused by division by zero
  }
  pressure = 1048576-raw_pressure;
  pressure = (((pressure<<31)-tmp2)*3125)/tmp1;
  tmp1 = (((int64_t)bme280p->comp_param.dig_P9) * (pressure>>13) * (pressure>>13)) >> 25;
  tmp2 = (((int64_t)bme280p->comp_param.dig_P8) * pressure) >> 19;
  pressure = ((pressure + tmp1 + tmp2) >> 8) + (((int64_t)bme280p->comp_param.dig_P7)<<4);
  return (int32_t) (pressure / 256);
}

static int32_t aeld_bme280_comp_hum(struct aeld_bme280_dev *bme280p, int raw_humidity)
{
  uint32_t humidity;
  humidity = (bme280p->comp_param.comp_temp - ((int32_t)76800));
  humidity = (((((raw_humidity << 14) - (((int32_t)bme280p->comp_param.dig_H4) << 20) - (((int32_t)bme280p->comp_param.dig_H5) * humidity)) + 
  ((int32_t)16384)) >> 15) * (((((((humidity *
  ((int32_t)bme280p->comp_param.dig_H6)) >> 10) * (((humidity * ((int32_t)bme280p->comp_param.dig_H3)) >> 11) +
  ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)bme280p->comp_param.dig_H2) + 8192) >> 14));
  humidity = (humidity - (((((humidity >> 15) * (humidity >> 15)) >> 7) * ((int32_t)bme280p->comp_param.dig_H1)) >> 4));
  humidity = (humidity < 0 ? 0 : humidity);
  humidity = (humidity > 419430400 ? 419430400 : humidity);
  return (int32_t)(humidity >> 12) / 1024;
}

static int aeld_bme280_do_measurement(struct aeld_bme280_dev *bme280p, int32_t result[])
{
  uint8_t is_measuring;
  uint8_t raw_data[8] = {0};
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
  while (is_measuring & IS_MEASURING_BIT);
  aeld_bme280_read_bytes(bme280p, MEAS_DATA_START_ADDR, &raw_data[0], 8);
  raw_pressure = (int) (raw_data[0] << 12) | (raw_data[1] << 4) | (raw_data[2] >> 4);
  raw_temperature = (int) (raw_data[3] << 12) | (raw_data[4] << 4) | (raw_data[5] >> 4);
  raw_humidity = (int) (raw_data[6] << 8) | (raw_data[7]);
  result[0] = aeld_bme280_comp_temp(bme280p, raw_temperature);
  result[1] = aeld_bme280_comp_press(bme280p, raw_pressure);
  result[2] = aeld_bme280_comp_hum(bme280p, raw_humidity);
  pr_info("measurement data: temp: %d, press: %d, hum: %d\n", result[0], result[1], result[2]);
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
  int32_t result[3] = {0};
  
  pr_info("Device read\n");
  aeld_bme280_do_measurement(&bme280_dev, &result[0]);
  
  return count - copy_to_user(buf, result, sizeof(result));
}

static loff_t aeld_bme280_llseek(struct file *filp, loff_t offset, int whence)
{
  return 0;
}

static struct file_operations aeld_bme280_fops = {
  .owner          = THIS_MODULE,
  .open           = aeld_bme280_open,
  .release        = aeld_bme280_release,
  .read           = aeld_bme280_read,
  .llseek         = aeld_bme280_llseek,
};

static int aeld_bme280_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
  uint8_t is_resetting;
  
  pr_info("device probed\n");
  
  mdelay(10);
  
  do
  {
    mdelay(1);
    aeld_bme280_read_bytes(&bme280_dev, STATUS_REG_ADDR, &is_resetting, 1);
  }
  while (is_resetting & IS_RESETING_BIT);
  
  aeld_bme280_com_param_init(&bme280_dev);
  aeld_bme280_write_cmd(&bme280_dev, CTRL_HUM_REG_ADDR, CMD_H_OVERSAMPLING);
  aeld_bme280_write_cmd(&bme280_dev, CTRL_MEAS_REG_ADDR, CTRL_MEAS_REG_VAL);
  
  return 0;
}

static void aeld_bme280_remove(struct i2c_client *client)
{
  pr_info("device removed\n");
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
  },
  .probe = aeld_bme280_probe,
  .remove = aeld_bme280_remove,
  .id_table = aeld_bme280_id,
};

static struct i2c_board_info aeld_bme280_i2c_board_info = {
  I2C_BOARD_INFO(BME280_DEVICE_NAME, BME280_DEVICE_ADDR)
};


static int __init aeld_bme280_driver_init(void)
{
  int status = 0;
  
  pr_info("Device init\n");
  
  status = alloc_chrdev_region(&bme280_dev.devt, 0, 1, BME280_DEVICE_NAME);
  if (status < 0)
  {
    pr_err("Cannot allocate major number\n");
    return status;
  }
  pr_info("Major = %d, Minor = %d \n", MAJOR(bme280_dev.devt), MINOR(bme280_dev.devt));
  
  cdev_init(&bme280_dev.cdev, &aeld_bme280_fops);
  
  status = cdev_add(&bme280_dev.cdev, bme280_dev.devt, 1);
  if (status < 0)
  {
    pr_err("Cannot add the device to the system\n");
    return status;
  }
  pr_info("device added to system\n");
  
  aeld_bme280_class = class_create(THIS_MODULE, BME280_CLASS_NAME);
  if (aeld_bme280_class == NULL)
  {
    pr_err("Class could not be created\n");
    return -1;
  }
  if ((device_create(aeld_bme280_class, NULL, bme280_dev.devt, NULL, BME280_DEVICE_NAME)) == NULL)
  {
    pr_err("Cannot create the device \n");
    return -1;
  }
  aeld_bme280_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
  
  if (NULL != aeld_bme280_i2c_adapter)
  {
    pr_info("i2c adapter added\n");
    bme280_dev.client = i2c_new_client_device(aeld_bme280_i2c_adapter, &aeld_bme280_i2c_board_info);
    if (NULL != bme280_dev.client)
    {
      pr_info("i2c client added\n");
      i2c_add_driver(&aeld_bme280_i2c_driver);
    }
    i2c_put_adapter(aeld_bme280_i2c_adapter);
  }
  
  pr_info("driver init complete\n");

  return 0;
}

static void __exit aeld_bme280_driver_exit(void)
{
  i2c_unregister_device(bme280_dev.client);
  i2c_del_driver(&aeld_bme280_i2c_driver);
  device_destroy(aeld_bme280_class, bme280_dev.devt);
  class_destroy(aeld_bme280_class);
  cdev_del(&bme280_dev.cdev);
  unregister_chrdev_region(bme280_dev.devt, 1);
  pr_info("Driver Removed!!!\n");
}

module_init(aeld_bme280_driver_init);
module_exit(aeld_bme280_driver_exit);

MODULE_DESCRIPTION("BME280 driver");
MODULE_AUTHOR("PlaceholderForAProperUsername");
MODULE_LICENSE("GPL");
