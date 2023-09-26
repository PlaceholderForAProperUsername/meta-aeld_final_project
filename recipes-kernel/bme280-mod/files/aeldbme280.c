#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#define pr_fmt(fmt) "aeld_BME280: " fmt

#define I2C_BUS             (1)
#define SLAVE_DEVICE_NAME   ("aeldbme280")
#define BME280_SLAVE_ADDR   (0x76)

static struct i2c_adapter *I2C_ADAPTER  = NULL;
static struct i2c_client  *I2C_CLIENT   = NULL; 

static int i2c_write(unsigned char *buf, unsigned int len)
{
  int ret = i2c_master_send(I2C_CLIENT, buf, len);
  
  return ret;
}

static int i2c_read(unsigned char *out_buf, unsigned int len)
{
  int ret = i2c_master_recv(I2C_CLIENT, out_buf, len);
  
  return ret;
}

static int aeldbme280_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
  // place init function here
  
  pr_info("aeldbme280 probed\n");
  
  return 0;
}

static int aeldbme280_remove(struct i2c_client *client)
{
  pr_info("aeldbme280 removed\n");
  
  return 0;
}

static const struct i2c_device_id aeldbme280_id[] = {
  {SLAVE_DEVICE_NAME, 0},
  {}
};
MODULE_DEVICE_TABLE(i2c, aeldbme280_id);

static struct i2c_driver aeldbme280_driver = {
  .driver = {
    .name   = SLAVE_DEVICE_NAME,
    .owner  = THIS_MODULE,  
  },
  .probe    = aeldbme280_probe,
  .remove   = aeldbme280_remove,
  .id_table = aeldbme280_id,
}

static struct i2c_board_info aeldbme280_board_info = {
  I2C_BOARD_INFO(SLAVE_DEVICE_NAME, BME280_SLAVE_ADDR)
};

static int __init aeldbme280_driver_init(void)
{
  int ret = -1;
  I2C_ADAPTER = i2c_get_adapter(I2C_BUS);
  
  if (NULL != I2C_ADAPTER)
  {
    I2C_CLIENT  = i2c_new_device(I2C_ADAPTER, &aeldbme280_board_info);
    
    if (NULL != I2C_CLIENT)
    {
      i2c_add_driver(&aeldbme280_driver);
      ret = 0;
    }
    i2c_put_adapter(I2C_ADAPTER);
  }
  
  pr_info("Driver added\n");
  
  return ret;
}

static void __exit aeldbme280_driver_exit(void)
{
  i2c_unregister_device(I2C_CLIENT);
  i2c_del_driver(&aeldbme280_driver);
  pr_info("Driver removed\n");
}

module_init(aeldbme280_driver_init);
module_exit(aeldbme280_driver_exit);


MODULE_DESCRIPTION("BME280 driver");
MODULE_AUTHOR("PlaceholderForAProperUsername");
MODULE_LICENSE("GPL");
