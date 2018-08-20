#ifndef __RUIJIE_H__
#define __RUIJIE_H__

#include<linux/kernel.h>
#include <linux/module.h>

/* 调试开关等级 */
typedef enum {
    DBG_START,
    DBG_VERBOSE,
    DBG_KEY,
    DBG_WARN,
    DBG_ERROR,
    DBG_END,
} dbg_level_t;

typedef enum dfd_cpld_id {
    BCM_CPLD0 = 0,
    BCM_CPLD1,
    CPLD0_MAC0,
    CPLD0_MAC1,
    CPLD1_MAC0,
    CPLD2_MAC1,
} dfd_cpld_id_t;

 typedef enum dfd_cpld_bus {
    SMBUS_BUS = 0 ,
    PCA9641_BUS = 1,
    GPIO_BUS = 2,
} dfd_cpld_bus_t;

 typedef struct dfd_i2c_dev_s {
     int bus;        /* bus号 */
     int addr;       /* 总线地址 */
 } dfd_i2c_dev_t;

 typedef enum dfd_cpld_addr {
    CPLD_ADDR_MIN   = 0x31,
    BCM_CPLD0_ADDR  = 0x32,   /* pca9641上 */
    CPLD0_MAC0_ADDR = 0x33,   /* SMBUS下 */
    CPLD0_MAC1_ADDR = 0x34,
    CPLD1_MAC0_ADDR = 0x35,
    CPLD2_MAC1_ADDR = 0x36,
    BCM_CPLD1_ADDR  = 0x37,
    CPLD_ADDR_MAX,
} dfd_cpld_addr_t;

typedef struct  dfd_dev_head_info_s {
    uint8_t   ver;                       /* E2PROM文件定义的版本号，初始版本为0x01  */
    uint8_t   flag;                      /* 新版本E2PROM标识为0x7E */
    uint8_t   hw_ver;                    /* 由主版本号与修正版本两部份构成 */
    uint8_t   type;                      /* 硬件类型定义信息 */
    int16_t   tlv_len;                   /* 有效数据长度（16 bits） */
} dfd_dev_head_info_t;

typedef enum dfd_intf_e{    
    DFD_INTF_GET_FAN_HW_VERSION,
    DFD_INTF_GET_FAN_STATUS,
    DFD_INTF_GET_FAN_SPEED_LEVEL,
    DFD_INTF_GET_FAN_SPEED,
    DFD_INTF_GET_FAN_ATTRIBUTE,
    DFD_INTF_GET_FAN_SN,
    DFD_INTF_GET_FAN_TYPE,
    DFD_INTF_SET_FAN_SPEED_LEVEL,
    DFD_INTF_GET_FAN_SUB_NUM,
    DFD_INTF_GET_FAN_FAIL_BITMAP,
}dfd_intf_t;
    
typedef struct dfd_dev_tlv_info_s {
    uint8_t  type;                       /* 数据类型 */
    uint8_t  len;                        /* 数据长度 */
    uint8_t  data[0];                    /* 数据 */
} dfd_dev_tlv_info_t;

typedef enum dfd_dev_info_type_e {
    DFD_DEV_INFO_TYPE_MAC        = 1,
    DFD_DEV_INFO_TYPE_NAME       = 2,
    DFD_DEV_INFO_TYPE_SN         = 3,
    DFD_DEV_INFO_TYPE_PWR_CONS   = 4,
    DFD_DEV_INFO_TYPE_HW_INFO    = 5,
    DFD_DEV_INFO_TYPE_DEV_TYPE   = 6,
} dfd_dev_tlv_type_t;

extern int debuglevel;
extern int dfd_cpld_read_chipid(int cpldid , uint32_t addr, int32_t size, unsigned char *buf);
extern int dfd_cpld_read(int32_t addr, uint8_t *val);
extern int dfd_cpld_write(int32_t addr, uint8_t val);
extern int ruijie_setdebug(int val);

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

#endif
