/*
 * rg_cpld.c - A driver for control rg_cpld base on rg_cpld.c
 *
 * Copyright (c) 1998, 1999  Frodo Looijaard <frodol@dds.nl>
 * Copyright (c) 2018 wk <zhengwenkai@ruijie.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/uaccess.h>


/* 调试开关等级 */
typedef enum {
    DBG_START,
    DBG_VERBOSE,
    DBG_KEY,
    DBG_WARN,
    DBG_ERROR,
    DBG_END,
} dbg_level_t;

static int debuglevel = 0;
module_param(debuglevel, int, S_IRUGO);

#define DBG_DEBUG(fmt, arg...)  do { \
    if ( debuglevel > DBG_START && debuglevel < DBG_ERROR) { \
          printk(KERN_INFO "[DEBUG]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
    } else if ( debuglevel >= DBG_ERROR ) {   \
        printk(KERN_ERR "[DEBUG]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
    } else {    } \
} while (0)

#define DBG_ERROR(fmt, arg...)  do { \
     if ( debuglevel > DBG_START) {  \
        printk(KERN_ERR "[ERROR]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
       } \
 } while (0)

static const unsigned short rg_i2c_cpld[] = { 0x34, 0x36, 0x37, I2C_CLIENT_END };

#define CPLD_SIZE 256

struct cpld_data {
    struct i2c_client   *client;
    struct device       *hwmon_dev;
    struct mutex        update_lock;
    char                valid;       /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    u8          data[CPLD_SIZE]; /* Register value */
};

static ssize_t show_fan_rpm_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int index = to_sensor_dev_attr_2(da)->index;
    uint8_t size;

    size = 0;
    mutex_lock(&data->update_lock);
    data->data[0] = i2c_smbus_read_byte_data(client, index);
    data->data[1] = i2c_smbus_read_byte_data(client, index + 1);
    DBG_DEBUG("cpld reg pos:0x%x value:0x%x\n",  index, data->data[0]);
    DBG_DEBUG("cpld reg pos:0x%x value:0x%x\n",  index + 1, data->data[1]);
    mutex_unlock(&data->update_lock);
    if (data->data[1] == 0xff && data->data[0] == 0xff) {
        size = snprintf(buf, CPLD_SIZE, "%d\n", 0);
    } else if (data->data[1] == 0 && data->data[0] == 0) {
        size = snprintf(buf, CPLD_SIZE, "%d\n", 0);
    } else {
        size = snprintf(buf, CPLD_SIZE, "%d\n", 15000000 / ((data->data[1] << 8) + data->data[0]));
    }
    return size;
}

static ssize_t set_cpld_sysfs_value(struct device *dev, struct device_attribute *da, char *buf, size_t
                                    count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    unsigned long val;
    int err;

    err = kstrtoul(buf, 16, &val);
    if (err)
        return err;
    if ((val < 0) || (val > 0xff)) {
        DBG_ERROR("please enter 0x00 ~ 0xff\n");
        return -1;
    }
    mutex_lock(&data->update_lock);
    data->data[0] = (u8*)val;
    DBG_DEBUG("pos: 0x%02x count = %d, data = 0x%02x\n", attr->index, count, data->data[0]);
    i2c_smbus_write_byte_data(client, attr->index, data->data[0]);
    mutex_unlock(&data->update_lock);

    return count;
}

static ssize_t show_cpld_version(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);
    i2c_smbus_read_i2c_block_data(client, 0, 4, data->data);
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%02x %02x %02x %02x \n", data->data[0], data->data[1], data->data[2],
                   data->data[3]);
}

static ssize_t show_cpld_sysfs_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);
    data->data[0] = i2c_smbus_read_byte_data(client, attr->index);
    DBG_DEBUG("cpld reg pos:0x%x value:0x%02x\n",  attr->index, data->data[0]);
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%02x\n", data->data[0]);
}

/* common */
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO | S_IWUSR, show_cpld_version, NULL, NULL);
/*0x37 hwmon*/
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO | S_IWUSR , show_fan_rpm_value, NULL, 0x1B);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO | S_IWUSR , show_fan_rpm_value, NULL, 0x1D);
static SENSOR_DEVICE_ATTR(fan3_input, S_IRUGO | S_IWUSR , show_fan_rpm_value, NULL, 0x1F);
static SENSOR_DEVICE_ATTR(fan4_input, S_IRUGO | S_IWUSR , show_fan_rpm_value, NULL, 0x21);
/* 0x37 */
static SENSOR_DEVICE_ATTR(fan_present, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, NULL, 0x30);
static SENSOR_DEVICE_ATTR(fan_status, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, NULL, 0x31);
static SENSOR_DEVICE_ATTR(psu_status, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, NULL, 0x51);
/* 0x34 */
static SENSOR_DEVICE_ATTR(sfp_presence3, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, NULL, 0x41);
static SENSOR_DEVICE_ATTR(sfp_presence4, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, NULL, 0x42);
/* 0x36 */
static SENSOR_DEVICE_ATTR(sfp_presence1, S_IRUGO | S_IWUSR , show_cpld_sysfs_value, NULL, 0x41);
static SENSOR_DEVICE_ATTR(sfp_presence2, S_IRUGO | S_IWUSR , show_cpld_sysfs_value, NULL, 0x42);
static SENSOR_DEVICE_ATTR(sfp_presence5, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, NULL, 0x43);

static struct attribute *cpld37_sysfs_attrs[] = {
    &sensor_dev_attr_fan_present.dev_attr.attr,
    &sensor_dev_attr_fan_status.dev_attr.attr,
    &sensor_dev_attr_psu_status.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    NULL
};

static struct attribute *cpld34_sysfs_attrs[] = {
    &sensor_dev_attr_sfp_presence3.dev_attr.attr,
    &sensor_dev_attr_sfp_presence4.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    NULL
};

static struct attribute *cpld36_sysfs_attrs[] = {
    &sensor_dev_attr_sfp_presence1.dev_attr.attr,
    &sensor_dev_attr_sfp_presence2.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    NULL
};

static const struct attribute_group cpld37_sysfs_group = {
    .attrs = cpld37_sysfs_attrs,
};

static const struct attribute_group cpld34_sysfs_group = {
    .attrs = cpld34_sysfs_attrs,
};

static const struct attribute_group  cpld36_sysfs_group = {
    .attrs = cpld36_sysfs_attrs,
};

static struct attribute *cpld_hwmon_attrs[] = {
    &sensor_dev_attr_fan1_input.dev_attr.attr,
    &sensor_dev_attr_fan2_input.dev_attr.attr,
    &sensor_dev_attr_fan3_input.dev_attr.attr,
    &sensor_dev_attr_fan4_input.dev_attr.attr,
    NULL
};
ATTRIBUTE_GROUPS(cpld_hwmon);

static int cpld_detect(struct i2c_client *new_client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = new_client->adapter;
    int conf;
    DBG_DEBUG("=========cpld_detect(0x%x)===========\n", new_client->addr);
    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA |
                                 I2C_FUNC_SMBUS_WORD_DATA))
        return -ENODEV;
    conf = i2c_smbus_read_byte_data(new_client, 0);
    if (!conf)
        return -ENODEV;
    strlcpy(info->type, "rg_cpld", I2C_NAME_SIZE);
    return 0;
}

static int cpld_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct cpld_data *data;
    int status;

    status = -1;
    DBG_DEBUG("=========cpld_probe(0x%x)===========\n", client->addr);
    data = devm_kzalloc(&client->dev, sizeof(struct cpld_data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    switch (client->addr) {
    case 0x37:
        status = sysfs_create_group(&client->dev.kobj, &cpld37_sysfs_group);
        if (status != 0) {
            break;
        }
        data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, data, cpld_hwmon_groups);
        if (IS_ERR(data->hwmon_dev)) {
            sysfs_remove_group(&client->dev.kobj, &cpld37_sysfs_group);
            return PTR_ERR(data->hwmon_dev);
        }
        break;
    case 0x34:
        status = sysfs_create_group(&client->dev.kobj, &cpld34_sysfs_group);
        break;
    case 0x36:
        status = sysfs_create_group(&client->dev.kobj, &cpld36_sysfs_group);
        break;
    default:
        break;
    }

    if (status != 0) {
        DBG_ERROR("%s %d sysfs_create_group err\n", __func__, __LINE__);
    }
    return status;
}

static int cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    DBG_DEBUG("=========cpld_probe(0x%x)===========\n", client->addr);
    switch (client->addr) {
    case 0x37:
        hwmon_device_unregister(data->hwmon_dev);
        sysfs_remove_group(&client->dev.kobj, &cpld37_sysfs_group);
        break;
    case 0x34:
        sysfs_remove_group(&client->dev.kobj, &cpld34_sysfs_group);
        break;
    case 0x36:
        sysfs_remove_group(&client->dev.kobj, &cpld36_sysfs_group);
        break;
    default:
        break;
    }

    return 0;
}

static const struct i2c_device_id cpld_id[] = {
    { "rg_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver rg_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name   = "rg_cpld",
    },
    .probe      = cpld_probe,
    .remove     = cpld_remove,
    .id_table   = cpld_id,
    .detect     = cpld_detect,
    .address_list   = rg_i2c_cpld,
};

module_i2c_driver(rg_cpld_driver);
MODULE_AUTHOR("wk <zhengwenkai@ruijie.com.cn>");
MODULE_DESCRIPTION("ruijie CPLD driver");
MODULE_LICENSE("GPL");
