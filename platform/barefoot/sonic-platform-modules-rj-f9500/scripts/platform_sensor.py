#!/usr/bin/python

# On F9500, the Platform Management Controller runs the
# thermal algorithm. It provides a mailbox for the Host
# to query relevant thermals. The dell_mailbox module
# provides the sysfs support for the following objects:
#   * onboard temperature sensors
#   * FAN trays
#   * PSU
#
import os
import sys
import logging
import re
from sonic_platform import get_machine_info
from sonic_platform import get_platform_info

x = get_platform_info(get_machine_info())
if x != None:
 filepath = "/usr/share/sonic/device/" + x 
import sys
sys.path.append(filepath)

from monitor import status
try:
    sys.modules['monitor']
except Exception,e:
    print 'not module monitor  %s'%e
    
    
def printerr(str):
    print("\033[0;31m%s\033[0m" % str)

print  get_platform_info(get_machine_info())
print 'Adapter: Ruijie Platform Management Controller'
#cputemp
print '\nOnboard coretemp Sensors:'

try:
    cputemp = []
    status.getcputemp(cputemp)
    for ctemp in cputemp:
        print '%10s : %10s %s (high = %s C , hyst = %s C)' % (ctemp["name"], ctemp["temp"], "C", ctemp["max"], ctemp["crit"] )
except Exception as e:
    printerr("%10s" % "get coretemp error")
##temp
print '\nOnboard Temperature Sensors:'

try:
    b = []
    status.getTemp(b)
    for temp in b:
        print '%10s : %10s %s (high = %s C , hyst = %s C)' % (temp["id"], temp["temp1_input"], "C", temp["temp1_max"], temp["temp1_max_hyst"] )
except Exception as e:
    printerr("%10s" % "get Temperature error")
    
##fan
print '\nOnboard fan Sensors:'
try:
    a = []
    status.checkFan(a)
    for inta in a:
        if ( inta["errcode"] != 0 ):         
            print '%10s : %s' % (inta["id"], inta["errmsg"])
            continue
        else:
            print '%10s :' % (inta["id"])
        print '%20s : %s'% ("fan_type",inta["fan_type"])
        print '%20s : %s'% ("sn",inta["sn"])
        print '%20s : %s'% ("hw_version",inta["hw_version"])
        print '%20s : %s %s'% ("Speed",inta["Speed"], "RPM")
        print '%20s : %s'% ("status",inta["errmsg"])     
except Exception as e:
    printerr("%10s" % "get fan error")
    
##psu
print '\nOnboard Power Supply Unit Sensors:'
try:
    c = []
    status.getPsu(c)
    for intc in c:    
        if ( intc["errcode"] != 0 ):         
            print '%10s : %s' % (intc["id"], intc["errmsg"])
            continue
        else:
            print '%10s :' % (intc["id"])
            print '%20s : %s'% ("type",intc["type1"])
            print '%20s : %s'% ("sn",intc["sn"])
            print '%20s : %s %s'% ("in_current",intc["in_current"], 'A')
            print '%20s : %s %s'% ("in_voltage",intc["in_voltage"], 'V')
            print '%20s : %s %s'% ("out_current",intc["out_current"], 'A')
            print '%20s : %s %s'% ("out_voltage",intc["out_voltage"], 'V')
            print '%20s : %s %s'% ("temp",intc["temp"], "C")
except Exception as e:
    printerr("%10s" % "get fan error")