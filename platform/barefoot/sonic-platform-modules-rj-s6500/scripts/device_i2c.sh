#!/bin/bash

/sbin/modprobe i2c_dev
/sbin/modprobe i2c_gpio
/sbin/modprobe i2c_mux
/sbin/modprobe gpio-nct6102d
/sbin/modprobe rg_i2c-mux-pca954x
sleep 1
/sbin/modprobe lm75
/sbin/modprobe tmp401
/sbin/modprobe eeprom
/sbin/modprobe rg_cpld

# cpld
echo rg_cpld 0x63 > /sys/bus/i2c/devices/i2c-6/new_device

# qsfp
for i in $(seq 7 38)
do
    echo eeprom 0x50 > /sys/bus/i2c/devices/i2c-$i/new_device
done
