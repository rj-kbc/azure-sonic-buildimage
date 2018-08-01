/*
 * wnc_cpld.c - A driver for control rg_cpld base on lm75.c
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

/* Addresses scanned */
static const unsigned short normal_i2c[] = { 0x63, I2C_CLIENT_END };

/* Size of EEPROM in bytes */
#define CPLD_SIZE 3

/* Each client has this additional data */
struct cpld_data {
    struct i2c_client   *client;
    struct mutex        update_lock;
    char            valid;       /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    u8          data[CPLD_SIZE]; /* Register value */
};

static ssize_t show_fan_rpm_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int index = to_sensor_dev_attr_2(da)->index;

    mutex_lock(&data->update_lock);
    data->data[0] = i2c_smbus_read_byte_data(client, index);
    data->data[1] = i2c_smbus_read_byte_data(client, index + 1);
    mutex_unlock(&data->update_lock);

    return sprintf(buf, "%d\n", (data->data[1] << 8) + data->data[0]);
}

static ssize_t show_sysfs_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);
    data->data[0] = i2c_smbus_read_byte_data(client, attr->index);
    mutex_unlock(&data->update_lock);

    return sprintf(buf, "%02x\n", data->data[0]);
}

static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, show_fan_rpm_value, NULL, 0x25);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO, show_fan_rpm_value, NULL, 0x27);
static SENSOR_DEVICE_ATTR(fan3_input, S_IRUGO, show_fan_rpm_value, NULL, 0x29);

static struct attribute *cpld_hwmon_attrs[] = {
    &sensor_dev_attr_fan1_input.dev_attr.attr,
    &sensor_dev_attr_fan2_input.dev_attr.attr,
    &sensor_dev_attr_fan3_input.dev_attr.attr,
    NULL
};

ATTRIBUTE_GROUPS(cpld_hwmon);

static SENSOR_DEVICE_ATTR(psu_status, S_IRUGO, show_sysfs_value, NULL, 6);
static SENSOR_DEVICE_ATTR(sfp_presence1, S_IRUGO, show_sysfs_value, NULL, 0x41);
static SENSOR_DEVICE_ATTR(sfp_presence2, S_IRUGO, show_sysfs_value, NULL, 0x42);
static SENSOR_DEVICE_ATTR(sfp_presence3, S_IRUGO, show_sysfs_value, NULL, 0x43);
static SENSOR_DEVICE_ATTR(sfp_presence4, S_IRUGO, show_sysfs_value, NULL, 0x44);

static struct attribute *cpld3_sysfs_attrs[] = {
    &sensor_dev_attr_psu_status.dev_attr.attr,
    &sensor_dev_attr_sfp_presence1.dev_attr.attr,
    &sensor_dev_attr_sfp_presence2.dev_attr.attr,
    &sensor_dev_attr_sfp_presence3.dev_attr.attr,
    &sensor_dev_attr_sfp_presence4.dev_attr.attr,
    NULL
};

static const struct attribute_group cpld3_sysfs_group = {
    .attrs = cpld3_sysfs_attrs,
};

/* Return 0 if detection is successful, -ENODEV otherwise */
static int cpld_detect(struct i2c_client *new_client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = new_client->adapter;
    int conf;

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA |
                     I2C_FUNC_SMBUS_WORD_DATA))
        return -ENODEV;

    /* Unused bits */
    conf = i2c_smbus_read_byte_data(new_client, 0);
    if (!conf)
        return -ENODEV;

    return 0;
}


static int cpld_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct device *hwmon_dev;
    struct cpld_data *data;
    int status;

    data = devm_kzalloc(&client->dev, sizeof(struct cpld_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &cpld3_sysfs_group);
    hwmon_dev = devm_hwmon_device_register_with_groups(&client->dev, client->name, data, cpld_hwmon_groups);

    return PTR_ERR_OR_ZERO(hwmon_dev);
}

static int cpld_remove(struct i2c_client *client)
{
    devm_hwmon_device_unregister(&client->dev);
    sysfs_remove_group(&client->dev.kobj, &cpld3_sysfs_group);
    return 0;
}

static const struct i2c_device_id cpld_id[] = {
    { "rg_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name   = "rg_cpld",
    },
    .probe      = cpld_probe,
    .remove     = cpld_remove,
    .id_table   = cpld_id,
    .detect     = cpld_detect,
    .address_list   = normal_i2c,
};

module_i2c_driver(cpld_driver);

MODULE_AUTHOR("wk <zhengwenkai@ruijie.com.cn>");
MODULE_DESCRIPTION("ruijie CPLD driver");
MODULE_LICENSE("GPL");
