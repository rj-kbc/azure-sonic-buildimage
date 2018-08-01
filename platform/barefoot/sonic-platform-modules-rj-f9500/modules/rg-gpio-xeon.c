/*
 *  GPIO interface for XEON Super I/O chip
 *
 *  Author: cdd <cdd@ruijie.com.cn>
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
#include <linux/delay.h>
#include <asm/delay.h>
#include <linux/miscdevice.h>

#define GPIO_NAME           "xeon-gpio"
#define GPIO_IOSIZE         7
#define GPIO_BASE           0x500

#define GPIO_USE_SEL        GPIO_BASE
#define GP_IO_SEL           (GPIO_BASE + 0x4)
#define GP_LVL              (GPIO_BASE + 0xC)
#define GPIO_USE_SEL2       (GPIO_BASE + 0x30)
#define GP_IO_SEL2          (GPIO_BASE + 0x34)
#define GP_LVL2             (GPIO_BASE + 0x38)
#define GPIO_USE_SEL3       (GPIO_BASE + 0x40)
#define GP_IO_SEL3          (GPIO_BASE + 0x44)
#define GP_LVL3             (GPIO_BASE + 0x48)


#define GPIO_BASE_ID        0
#define BANKSIZE            32

static DEFINE_SPINLOCK(sio_lock);
 
/****************** i2c adapter with gpio ***********************/
#define GPIO_SDA        (GPIO_BASE_ID + 17)
#define GPIO_SCL        (GPIO_BASE_ID + 1)

static struct i2c_gpio_platform_data i2c_pdata = {
    .sda_pin = GPIO_SDA,
    .scl_pin = GPIO_SCL,
    .udelay = 10,
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
    .id             = -1,
    .dev = {
        .platform_data = &i2c_pdata,
        .release = i2c_gpio_release,
    }
};

static int xeon_gpio_get(struct gpio_chip *gc, unsigned gpio_num)
{
    unsigned int data;
    unsigned int bank, offset;

    bank = gpio_num / BANKSIZE;
    offset = gpio_num % BANKSIZE;

    data = 0;
    spin_lock(&sio_lock);
    if (bank == 0) {
        data = inl(GP_LVL) & (1 << offset);
        if (data) {
            data = 1;
        }
    } else if (bank == 1) {
        data = inl(GP_LVL2) & (1 << offset);
        if (data) {
            data = 1;
        }
    } else if (bank == 2) {
        data = inl(GP_LVL3) & (1 << offset);
        if (data) {
            data = 1;
        }
    }
    spin_unlock(&sio_lock);
    
    return data;
}

static int xeon_gpio_direction_in(struct gpio_chip *gc, unsigned gpio_num)
{
    unsigned int data;
    unsigned int bank, offset;

    bank = gpio_num / BANKSIZE;
    offset = gpio_num % BANKSIZE;

    spin_lock(&sio_lock);
    if (bank == 0) {
        data = inl(GP_IO_SEL);
        data = data | (1 << offset);
        outl(data, GP_IO_SEL);
    } else if (bank == 1) {
        data = inl(GP_IO_SEL2);
        data = data | (1 << offset);
        outl(data, GP_IO_SEL2);
    } else if (bank == 2) {
        data = inl(GP_IO_SEL3);
        data = data | (1 << offset);
        outl(data, GP_IO_SEL3);
    }
    spin_unlock(&sio_lock);

    return 0;
}

static void xeon_gpio_set(struct gpio_chip *gc,
                unsigned gpio_num, int val)
{
    unsigned int data;
    unsigned int bank, offset;

    bank = gpio_num / BANKSIZE;
    offset = gpio_num % BANKSIZE;
    
    spin_lock(&sio_lock);
    if (bank == 0) {
        data = inl(GP_LVL);
        if (val) {
            data = data | (1 << offset);
        } else {
            data = data & ~(1 << offset);
        }
        outl(data, GP_LVL);
    } else if (bank == 1) {
        data = inl(GP_LVL2);
        if (val) {
            data = data | (1 << offset);
        } else {
            data = data & ~(1 << offset);
        }
        outl(data, GP_LVL2);
    } else if (bank == 2) {
        data = inl(GP_LVL3);
        if (val) {
            data = data | (1 << offset);
        } else {
            data = data & ~(1 << offset);
        }
        outl(data, GP_LVL3);
    }
    spin_unlock(&sio_lock);
}

static int xeon_gpio_direction_out(struct gpio_chip *gc,
                    unsigned gpio_num, int val)
{
    unsigned int data;
    unsigned int bank, offset;

    bank = gpio_num / BANKSIZE;
    offset = gpio_num % BANKSIZE;

    spin_lock(&sio_lock);
    if (bank == 0) {
        data = inl(GP_IO_SEL);
        data = data & ~(1 << offset);
        outl(data, GP_IO_SEL);
    
        data = inl(GP_LVL);
        if (val) {
            data = data | (1 << offset);
        } else {
            data = data & ~(1 << offset);
        }
        outl(data, GP_LVL);
    } else if (bank == 1) {
        data = inl(GP_IO_SEL2);
        data = data & ~(1 << offset);
        outl(data, GP_IO_SEL2);
    
        data = inl(GP_LVL2);
        if (val) {
            data = data | (1 << offset);
        } else {
            data = data & ~(1 << offset);
        }
        outl(data, GP_LVL2);
    } else if (bank == 2) {
        data = inl(GP_IO_SEL3);
        data = data & ~(1 << offset);
        outl(data, GP_IO_SEL3);
    
        data = inl(GP_LVL3);
        if (val) {
            data = data | (1 << offset);
        } else {
            data = data & ~(1 << offset);
        }
        outl(data, GP_LVL3);
    }
    spin_unlock(&sio_lock);

    return 0;
}

static struct gpio_chip xeon_gpio_chip = {
    .label          = GPIO_NAME,
    .owner          = THIS_MODULE,
    .get            = xeon_gpio_get,
    .direction_input    = xeon_gpio_direction_in,
    .set            = xeon_gpio_set,
    .direction_output   = xeon_gpio_direction_out,
};

static void xeon_gpio_use_sel(unsigned int gpio_num)
{
    unsigned int data;
    unsigned int bank, offset;

    bank = gpio_num / BANKSIZE;
    offset = gpio_num % BANKSIZE;
    
    spin_lock(&sio_lock);
    if (bank == 0) {
        data = inl(GPIO_USE_SEL);
        data = data | (1 << offset);
        outl(data, GPIO_USE_SEL);
    } else if (bank == 1) {
        data = inl(GPIO_USE_SEL2);
        data = data | (1 << offset);
        outl(data, GPIO_USE_SEL2);
    } else if (bank == 2) {
        data = inl(GPIO_USE_SEL3);
        data = data | (1 << offset);
        outl(data, GPIO_USE_SEL3);
    }
    spin_unlock(&sio_lock);
}

static int __init xeon_gpio_init(void)
{
    int err;

    /* Set GPIO1 and GPIO17 as GPIO pin */
    xeon_gpio_use_sel(1);
    xeon_gpio_use_sel(17);
    /* Set GPIO22 as GPIO pin ,for irq*/
    xeon_gpio_use_sel(22);

    /* Set GPIO for JTAG, cpld upgrade*/
    xeon_gpio_use_sel(6);
    xeon_gpio_use_sel(32);
    xeon_gpio_use_sel(50);
    xeon_gpio_use_sel(65);
    xeon_gpio_use_sel(67);

    
    if (!request_region(GPIO_BASE, GPIO_IOSIZE, GPIO_NAME))
        return -EBUSY;

    xeon_gpio_chip.base = GPIO_BASE_ID;
    xeon_gpio_chip.ngpio = 96;

    err = gpiochip_add(&xeon_gpio_chip);
    if (err < 0)
        goto gpiochip_add_err;

    err = platform_device_register(&i2c_gpio);
    if (err < 0) {
        //DBG_ERROR("register i2c gpio fail(%d). \n", err);
        goto i2c_get_adapter_err;
    }

   	return 0;

i2c_get_adapter_err:
    platform_device_unregister(&i2c_gpio);
    gpiochip_remove(&xeon_gpio_chip);

gpiochip_add_err:
    release_region(GPIO_BASE, GPIO_IOSIZE);
    return -1;
}

static int __exit xeon_gpio_exit(void)
{
    platform_device_unregister(&i2c_gpio);
    mdelay(100);
    gpiochip_remove(&xeon_gpio_chip);
    release_region(GPIO_BASE, GPIO_IOSIZE);
    return 0;
}

module_init(xeon_gpio_init);
module_exit(xeon_gpio_exit);

MODULE_AUTHOR("cdd <cdd@ruijie.com.cn>");
MODULE_DESCRIPTION("GPIO interface for XEON Super I/O chip");
MODULE_LICENSE("GPL");
