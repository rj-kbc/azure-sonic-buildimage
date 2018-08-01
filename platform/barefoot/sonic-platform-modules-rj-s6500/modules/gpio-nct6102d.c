/*
 *  GPIO interface for NCT6102D Super I/O chip
 *
 *  Author: chen lei <cl@ruijie.com.cn>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License 2 as published
 *  by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio.h>
#include <asm/delay.h>
#include <linux/miscdevice.h>

#define SIO_CHIP_ID         0xc452
#define CHIP_ID_HIGH_BYTE   0x20
#define CHIP_ID_LOW_BYTE    0x21
#define CONFIG_ENTER_DATA   0x87
#define CONFIG_EXIT_DATA    0xaa

#define GPIO_LOG_DEBUG                    3
#define GPIO_LOG_INFO                     2
#define GPIO_LOG_ERR                      1
#define GPIO_LOG_NONE                     0

static u8 ports[2] = { 0x2e, 0x4e };
static u8 port;
static int nct6102d_dbg = GPIO_LOG_NONE;

static DEFINE_SPINLOCK(sio_lock);

#define GPIO_NAME       "nct6102d-gpio"
#define GPIO_BA_HIGH_BYTE   0x60
#define GPIO_BA_LOW_BYTE    0x61
#define GPIO_IOSIZE         7
#define GPIO_BASE_ID        100

/****************** i2c adapter with gpio ***********************/
#define GPIO_SDA        (GPIO_BASE_ID + 37) // group 4/5
#define GPIO_SCL        (GPIO_BASE_ID + 38) // group 4/6

static struct i2c_gpio_platform_data i2c_pdata = {
    .sda_pin = GPIO_SDA,
    .scl_pin = GPIO_SCL,
    .udelay = 1,
    .scl_is_output_only = 0,
    .sda_is_open_drain  = 0,
    .scl_is_open_drain = 0,
};

static void i2c_gpio_release(struct device *dev)
{
    return;
}

static struct platform_device i2c_gpio = {
    .name           = "i2c-gpio",
    .num_resources  = 0,
    .id             = 6,
    
    .dev = {
        .platform_data = &i2c_pdata,
        .release = i2c_gpio_release,
    }
};


#define GPIO_LOG_DBG(dbg, fmt, arg...)                  \
do {                                                    \
    if (dbg <= nct6102d_dbg) {                          \
        printk(KERN_ERR"[Net <%s>:<%d>] " fmt,          \
            __FUNCTION__, __LINE__, ##arg);             \
    }                                                   \
} while (0)

static u16 gpio_ba;

static u8 read_reg(u8 addr, u8 port)
{
    outb(addr, port);
    return inb(port + 1);
}

static void write_reg(u8 data, u8 addr, u8 port)
{
    outb(addr, port);
    outb(data, port + 1);
}

static void enter_conf_mode(u8 port)
{
    outb(CONFIG_ENTER_DATA, port);
    outb(CONFIG_ENTER_DATA, port);
}

static void exit_conf_mode(u8 port)
{
    outb(CONFIG_EXIT_DATA, port);
}

static void enter_gpio_mode(u8 port)
{
    write_reg(0x8, 0x7, port);  /* device 8 */
}

static void enable_gpio_base(u8 port)
{
    u8 value;
    u8 i;

    value = read_reg(0x30, port);
    GPIO_LOG_DBG(GPIO_LOG_DEBUG, "gpio base:0x%x \n", value);
    write_reg(value | 0x2, 0x30, port);

    for (i = 0; i < 8; i++) {
        GPIO_LOG_DBG(GPIO_LOG_DEBUG, "0x%x:0x%x \n", 0xe0 + i, read_reg(0xe0 + i, port));
    }
}

static int nct6102d_gpio_get(struct gpio_chip *gc, unsigned gpio_num)
{
    u8 value, bit;
    u8 index;
        u8 val = 0;

    index = gpio_num / 8;
    bit = gpio_num % 8;
    spin_lock(&sio_lock);
    outb(index, gpio_ba);  /* select group */ 
    inb(gpio_ba);

    value = inb(gpio_ba + 2);
    spin_unlock(&sio_lock);
    GPIO_LOG_DBG(GPIO_LOG_DEBUG, "index:0x%x value:0x%x bit:%d \n",
     index, value, bit);
    if (value & (1 << bit)) {
        val = 1;
    }

    return val;
}

static int nct6102d_gpio_direction_in(struct gpio_chip *gc, unsigned gpio_num)
{
    u8 curr_dirs;
    u8 bit, index;

    bit = gpio_num % 8;
    index = gpio_num / 8;

    spin_lock(&sio_lock);
    outb(index, gpio_ba);  /* select group */ 
    inb(gpio_ba);

    curr_dirs = inb(gpio_ba + 1);
    GPIO_LOG_DBG(GPIO_LOG_DEBUG, "index:0x%x curr_dirs:0x%x bit:%d \n",
     index, curr_dirs, bit);
    if ((curr_dirs & (1 << bit)) == 0) {
        /* 0: out put */
        outb(curr_dirs | (1 << bit), gpio_ba + 1);
        inb(gpio_ba + 1);
        GPIO_LOG_DBG(GPIO_LOG_DEBUG, "set input mode.\n");
    }

    spin_unlock(&sio_lock);
    return 0;
}

static void nct6102d_gpio_set(struct gpio_chip *gc,
                unsigned gpio_num, int val)
{
    u8 curr_vals, bit, index, value;

    index = gpio_num / 8;
   bit = gpio_num % 8;

    spin_lock(&sio_lock);
    outb(index, gpio_ba);  /* select group */ 
    inb(gpio_ba);

    curr_vals = inb(gpio_ba + 2);
    if (val) {
        value = curr_vals | (1 << bit);
    } else {
        value = curr_vals & ~(1 << bit);
    }
    outb(value, gpio_ba + 2);
    inb(gpio_ba + 2);
    spin_unlock(&sio_lock);
}

static int nct6102d_gpio_direction_out(struct gpio_chip *gc,
                    unsigned gpio_num, int val)
{
    u8 curr_dirs, curr_vals, value;
    u8 bit, index;

    bit = gpio_num % 8;
    index = gpio_num / 8;

    spin_lock(&sio_lock);
    outb(index, gpio_ba);  /* select group */ 
    inb(gpio_ba);

    curr_dirs = inb(gpio_ba + 1);
    GPIO_LOG_DBG(GPIO_LOG_DEBUG, "index:0x%x curr_dirs:0x%x bit:%d \n",
     index, curr_dirs, bit);
    if (curr_dirs & (1 << bit)) {
        /* 1: int put */
        //GPIO_LOG_DBG(GPIO_LOG_DEBUG, "set direction out %d.\n", val);
        outb(curr_dirs & ~(1 << bit), gpio_ba + 1);
    }
    
    curr_vals = inb(gpio_ba + 2);
    if (val) {
        value = curr_vals | (1 << bit);
    } else {
        value = curr_vals & ~(1 << bit);
    }
    outb(value, gpio_ba + 2);
    inb(gpio_ba + 2);
    spin_unlock(&sio_lock);

    return 0;
}

static struct gpio_chip nct6102d_gpio_chip = {
    .label          = GPIO_NAME,
    .owner          = THIS_MODULE,
    .get            = nct6102d_gpio_get,
    .direction_input    = nct6102d_gpio_direction_in,
    .set            = nct6102d_gpio_set,
    .direction_output   = nct6102d_gpio_direction_out,
};

static struct i2c_board_info pca9548_board_info[] = 
{
    {
        .type = "rg_pca9548",
        .addr = 0x71,
    },
    {
        .type = "rg_pca9548",
        .addr = 0x72,
    },
    {
        .type = "rg_pca9548",
        .addr = 0x73,
    },
    {
        .type = "rg_pca9548",
        .addr = 0x74,
    },
    {
        .type = "rg_pca9548",
        .addr = 0x75,
    },
};

static int __init nct6102d_gpio_init(void)
{
    int i, id, err;
    int val;
    struct i2c_adapter *adapter;

    /* chip and port detection */
    for (i = 0; i < ARRAY_SIZE(ports); i++) {
        spin_lock(&sio_lock);
        enter_conf_mode(ports[i]);
        id = (read_reg(CHIP_ID_HIGH_BYTE, ports[i]) << 8) +
                read_reg(CHIP_ID_LOW_BYTE, ports[i]);

        exit_conf_mode(ports[i]);
        spin_unlock(&sio_lock);

        GPIO_LOG_DBG(GPIO_LOG_DEBUG, "get chip id:0x%x \n", id);
        if (id == SIO_CHIP_ID) {
            port = ports[i];
            break;
        }
    }

    if (!port)
        return -ENODEV;

    /* fetch GPIO base address */
    enter_conf_mode(port);
    enter_gpio_mode(port);
    gpio_ba = (read_reg(GPIO_BA_HIGH_BYTE, port) << 8) +
                read_reg(GPIO_BA_LOW_BYTE, port);
    enable_gpio_base(port);
   	write_reg(0x9, 0x7, port);  /* device 8 */
    val = read_reg(0xe0, port);
    write_reg(0x66, 0xe0, port);
    val = read_reg(0xe0, port);
    exit_conf_mode(port);
    GPIO_LOG_DBG(GPIO_LOG_DEBUG, "get gpio base addr:0x%x \n", gpio_ba);

    if (!request_region(gpio_ba, GPIO_IOSIZE, GPIO_NAME))
        return -EBUSY;

    nct6102d_gpio_chip.base = GPIO_BASE_ID;
    nct6102d_gpio_chip.ngpio = 57;

    err = gpiochip_add(&nct6102d_gpio_chip);
    if (err < 0)
        goto gpiochip_add_err;

    err = platform_device_register(&i2c_gpio);
    if (err < 0) {
        GPIO_LOG_DBG(GPIO_LOG_ERR, "register i2c gpio fail(%d). \n", err);
        gpiochip_remove(&nct6102d_gpio_chip);
        goto gpiochip_add_err;
    }
   
    adapter = i2c_get_adapter(6);
    if (!adapter) {
        goto i2c_get_adapter_err;
    }

    for (i = 0; i < ARRAY_SIZE(pca9548_board_info); i++) {
        i2c_new_device(adapter, &pca9548_board_info[i]);
    }

    i2c_put_adapter(adapter);
  
   	return 0;
i2c_get_adapter_err:
    platform_device_unregister(&i2c_gpio);
    gpiochip_remove(&nct6102d_gpio_chip);
gpiochip_add_err:
    release_region(gpio_ba, GPIO_IOSIZE);
    gpio_ba = 0;
    return -1;
}

static void __exit nct6102d_gpio_exit(void)
{
    if (gpio_ba) {
        gpiochip_remove(&nct6102d_gpio_chip);
        release_region(gpio_ba, GPIO_IOSIZE);
        gpio_ba = 0;
    }
   platform_device_unregister(&i2c_gpio);
}

module_init(nct6102d_gpio_init);
module_exit(nct6102d_gpio_exit);
module_param(nct6102d_dbg, int, 0644);

MODULE_AUTHOR("chen lei <cl@ruijie.com.cn>");
MODULE_DESCRIPTION("GPIO interface for NCT6102D Super I/O chip");
MODULE_LICENSE("GPL");
