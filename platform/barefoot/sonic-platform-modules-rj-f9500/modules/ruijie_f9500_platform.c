#include <linux/module.h>   
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/i2c-mux-gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/i2c-smbus.h>
#include <linux/string.h>
#include "ruijie.h"

#define DFD_CPLD_I2C_RETRY_TIMES    3
#define DFD_CPLD_I2C_RETRY_DELAY    100  /* ms */

int debuglevel = 0;
EXPORT_SYMBOL(debuglevel);

int ruijie_setdebug(int val)
{
    debuglevel = val;
    return 0;
}
EXPORT_SYMBOL(ruijie_setdebug);

/**
 * dfd_cpld_i2c_read - 读i2c
 * @dev: i2c设备名称
 * @addr: 设备地址
 * @offset_addr: 偏移地址
 * @buf: 读缓冲区
 * @size:  读buf的大小
 *
 * return: 成功返回0或正数，失败负值
 */
static int32_t dfd_cpld_i2c_read(char *dev, uint32_t addr, uint32_t offset_addr, unsigned char *
buf, int32_t size)
{
    struct file *fp;
    mm_segment_t fs;
    struct i2c_client client;
    int i ,j ;
    int rv;

    rv = 0;
    fp = filp_open(dev, O_RDWR, S_IRUSR | S_IWUSR);
    if (IS_ERR(fp)) {
        DBG_ERROR("i2c open fail.\n");
        return -1;
    }
    memcpy(&client, fp->private_data, sizeof(struct i2c_client));
    client.addr = addr;
    fs = get_fs();
    set_fs(KERNEL_DS);
    for (j = 0 ;j < size ;j++){
        for (i = 0; i < DFD_CPLD_I2C_RETRY_TIMES; i++) {
          if ((* (buf + j) = i2c_smbus_read_byte_data(&client, (offset_addr + j)))< 0) {
              DBG_DEBUG("read try(%d)time  offset_addr:%x write_buf:%x\n", i, offset_addr);
              continue;
          }
          break;
        }
    }
    filp_close(fp, NULL);
    set_fs(fs);
    return rv;
}

/**
 * dfd_write_port_i2c - 写i2c
 * @i2c_name: i2c设备名称
 * @dev_addr: 设备地址
 * @offset_addr: 偏移地址
 * @write_buf: 写缓冲区co
 * @size:  读buf的大小
 *
 * return: 成功返回0或正数，失败负值
 */
int32_t dfd_cpld_i2c_write(char *dev, uint16_t dev_addr, uint16_t offset_addr, 
                           uint8_t write_buf)
{
    struct file *fp;
    mm_segment_t fs;
    struct i2c_client client;
    int i;
    int rv;
    
    rv = 0;
    fp = filp_open(dev, O_RDWR, S_IRUSR | S_IWUSR);
    if (IS_ERR(fp)) {
        DBG_ERROR("i2c open fail.\n");
        return -1;
    }
    memcpy(&client, fp->private_data, sizeof(struct i2c_client));
    client.addr = dev_addr;
    fs = get_fs();
    set_fs(KERNEL_DS);
    for (i = 0; i < DFD_CPLD_I2C_RETRY_TIMES; i++) {
      if (( rv = i2c_smbus_write_byte_data(&client, offset_addr , write_buf))< 0) {
            DBG_DEBUG("write try(%d)time  offset_addr:%x write_buf:%x\n", i, offset_addr, write_buf);
          continue;
      }
        break;
    }
    filp_close(fp, NULL);
    set_fs(fs);
    return rv;
}

/**
* dfd_get_i2c_dev - 读取设备对应的I2C总线和设备地址
* @cpldid: cpldid
* @dev_t: 返回的dfd_i2c_dev_t
*
* return: 成功返回0或正数，失败负值
*/
int dfd_get_i2c_dev(int cpldid, dfd_i2c_dev_t *dev_t)
{
    int i2c_bus ,rv;
    uint32_t daddr;

    rv = 0;
    switch (cpldid) {
        case BCM_CPLD0:
             daddr = BCM_CPLD0_ADDR;
             i2c_bus = SMBUS_BUS;
             break;
        case BCM_CPLD1:
             daddr = BCM_CPLD1_ADDR;
             i2c_bus = PCA9641_BUS;
             break;
        case CPLD0_MAC0:
             daddr = CPLD0_MAC0_ADDR;
             i2c_bus = PCA9641_BUS;
             break;
        case CPLD0_MAC1:
             daddr = CPLD0_MAC1_ADDR;  
             i2c_bus = GPIO_BUS;
             break;
        case CPLD1_MAC0:
             daddr = CPLD1_MAC0_ADDR;
             i2c_bus = PCA9641_BUS;
             break;
        case CPLD2_MAC1:
             daddr = CPLD2_MAC1_ADDR;
             i2c_bus = GPIO_BUS;
             break;
        default:
            break;
    }
    if (i2c_bus < 0) {
        DBG_ERROR("cpldid error\n");
        rv = -1;
    }

    if( daddr >= CPLD_ADDR_MAX || daddr <= CPLD_ADDR_MIN) {
        DBG_ERROR("cpldid error\n");
        rv = -2;
    }

    dev_t->bus = i2c_bus;
    dev_t->addr = daddr;
    
    return rv;
}

/**
* dfd_cpld_read_chipid - 读取多字节
* @cpldid: dfd_cpld_read_chipid
* @dev_t: 返回的dfd_i2c_dev_t
*
* return: 成功返回0或正数，失败负值
*/
int dfd_cpld_read_chipid(int cpldid , uint32_t addr, int32_t size, unsigned char *buf) 
{
    char i2c_path[32];
    int  rv;
    dfd_i2c_dev_t dev_t;

    rv = 0;
    memset(i2c_path, 0, 32);
    memset(&dev_t, 0, sizeof(dfd_i2c_dev_t));
    rv = dfd_get_i2c_dev(cpldid, &dev_t); 
    if (rv < 0) {
        DBG_ERROR("%s  %d\n","get i2c error", rv);
        goto end;
    }
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", dev_t.bus);
    DBG_DEBUG("i2c_path:%s bus:%d addr:%d\n", i2c_path, dev_t.bus, dev_t.addr);
    rv = dfd_cpld_i2c_read(i2c_path, dev_t.addr, addr, buf, size);
    if (rv != 0) {
        DBG_ERROR("[DFD_CPLD]get_my_type, %s read fail\r\n", i2c_path);
        goto end;
    }
end:
    return rv;
}

int dfd_cpld_write_chipid(int cpldid , uint32_t addr, uint8_t buf) 
{
    int rv;
    char i2c_path[32];
    dfd_i2c_dev_t dev_t;

    memset(&dev_t, 0, sizeof(dfd_i2c_dev_t));
    memset(i2c_path, 0, 32);

    rv = dfd_get_i2c_dev(cpldid, &dev_t);
    if (rv < 0) {
        DBG_ERROR("%s %d %s  %d\n", __func__, __LINE__,"get i2c error", rv);
        return rv;
    }
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", dev_t.bus);
    rv = dfd_cpld_i2c_write(i2c_path, dev_t.addr, addr, buf);
    if (rv != 0) {
        DBG_ERROR("[DFD_CPLD]dfd_cpld_i2c_write, %s read fail\r\n", i2c_path);
        return rv;
    }
    return rv;
}

/**
 * dfd_cpld_read - 读cpld寄存器
 * @addr: cpld寄存器地址
 * @value: 读缓冲区
 *
 * return: 成功返回0或正数，失败返回负值
 */
int dfd_cpld_read_i2c(int32_t addr, uint8_t *value)
{
    int rv;
    int cpld_id, cpld_addr;

    cpld_id = (addr >> 16) & 0x00ff;
    cpld_addr = addr & 0xffff;
    DBG_DEBUG(" cpld_id:%x cpld_addr %x\n", cpld_id, cpld_addr);
    rv = dfd_cpld_read_chipid(cpld_id, cpld_addr, 1, value); /* 读一个字节 */    
    DBG_DEBUG("value %x\n", value);
    if (rv < 0) {
        DBG_ERROR("chipid:%d\n", cpld_id);
    }
    return rv;
}

/**
 * dfd_cpld_read_i2c - 写cpld寄存器
 * @addr: cpld寄存器地址
 * @value: 读缓冲区
 *
 * return: 成功返回0或正数，失败返回负值
 */
int dfd_cpld_write_i2c(int32_t addr, uint8_t value)
{
    int rv;
    int cpld_id, cpld_addr;
    
    cpld_id = (addr >> 16) & 0x00ff;
    cpld_addr = addr & 0xffff;
    rv = dfd_cpld_write_chipid(cpld_id, cpld_addr, value);
    if (rv < 0) {
        DBG_ERROR("write chipid error\n");
    }
    return rv;
}

int dfd_cpld_read(int32_t addr, uint8_t *val)
{
    return dfd_cpld_read_i2c(addr, val);
}
EXPORT_SYMBOL(dfd_cpld_read);

int dfd_cpld_write(int32_t addr, uint8_t val)
{
    return dfd_cpld_write_i2c(addr, val);
}
EXPORT_SYMBOL(dfd_cpld_write);




static int __init ruijie_b6510_platform_init(void)
{
     /* do nothing just for cpld kernel interface */
    return 0;
}

static void __exit ruijie_b6510_platform_exit(void)
{

}

module_init(ruijie_b6510_platform_init);
module_exit(ruijie_b6510_platform_exit);

MODULE_DESCRIPTION("ruijie_b6510 Platform Support");
MODULE_AUTHOR("tangjiamiao <tangjiamiao@ruijie.com.cn>");
MODULE_LICENSE("GPL");
