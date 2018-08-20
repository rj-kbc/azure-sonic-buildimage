#!/usr/bin/env python
# -*- coding: UTF-8 -*-
import sys
import click
import os
import commands
import logging
import time
import syslog
from sonic_platform import get_machine_info
from sonic_platform import get_platform_info

x = get_platform_info(get_machine_info())
if x != None:
 filepath = "/usr/share/sonic/device/" + x 
import sys
sys.path.append(filepath)

from monitor import status
import traceback

CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

class AliasedGroup(click.Group):
    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx)
                   if x.startswith(cmd_name)]
        if not matches:
            return None
        elif len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))
    
def healthwarninglog(s):
    syslog.openlog("HEALTHMONITOR",syslog.LOG_PID)
    syslog.syslog(syslog.LOG_WARNING,s)
    
def healtherror(s):
    syslog.openlog("HEALTHMONITOR",syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR,s)
    
def healthwarningdebuglog(s):
    if FANCTROLDEBUG == 1:
        syslog.openlog("HEALTHMONITOR",syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG,s)
 

class HealthMonitor():
    critnum = 0
    def __init__(self):
        self._fanOKNum = 0
        self._intemp = -100.0
        self._mac_aver = -100.0
        self._mac_max = -100.0
        self._preIntemp = -1000 #上一时刻温度
        self._outtemp = -100
        self._boardtemp = -100
        self._cputemp = -1000
        pass

    @property
    def fanOKNum(self):
        return self._fanOKNum;
        
    @property
    def cputemp(self):
        return self._cputemp;
 
    @property
    def intemp(self):
        return self._intemp;
        
    @property
    def outtemp(self):
        return self._outtemp;
        
    @property
    def boardtemp(self):
        return self._boardtemp;

    @property
    def mac_aver(self):
        return self._mac_aver;

    @property
    def preIntemp(self):
        return self._preIntemp;

    @property
    def mac_max(self):
        return self._mac_max;
    
    def sortCallback(self, element):
        return element['id']

    def getFanPresentNum(self,curFanStatus):
        fanoknum = 0;
        for i,item in enumerate(curFanStatus):
            if item["errcode"] == 0:
                fanoknum += 1
                if int(item["Speed"]) < 1000:
                    healthwarninglog("%%FAN-ERROR : %s Speed too slow %s %s" % (item["id"], item["Speed"], "RPM"))
            else:
                healthwarninglog("%%FAN-ERROR : %s : %s" % (item["id"], item["errmsg"]))

        self._fanOKNum = fanoknum
        if self.fanOKNum < 4:
            healthwarninglog("%%FAN-ERROR_FAN_NUM : %d" % (self.fanOKNum))

    def getMonitorInTemp(self, temp):
        for i,item in enumerate(temp):
            if item["id"] == "lm75in":
                self._intemp = float(item["temp1_input"])
            if item["id"] == "lm75out":
                self.outtemp = float(item["temp1_input"])
            if item["id"] == "lm75hot":
                self.boardtemp = float(item["temp1_input"])

    def checkPsu(self, psu):
        for i,item in enumerate(psu):
            if item["errcode"] != 0:
                healthwarninglog("%%PSU-ERROR : %10s : %s" % (item["id"], item["errmsg"]))
                
    def getCpuTemp(self, cputemp):
        for i,item in enumerate(cputemp):
            if item["name"] == "Physical id 0":
                self._cputemp = float(item["temp"])
                
    def getCpuStatus(self):
        try:
           cputemp = []
           status.getcputemp(cputemp)
           self.getCpuTemp(cputemp)
        except AttributeError as e:
            healtherror(str(e))
    
    def getFanStatus(self):
        try:
            curFanStatus = []
            status.checkFan(curFanStatus)
            curFanStatus.sort(key = self.sortCallback)
            self.getFanPresentNum(curFanStatus)
        except AttributeError as e:
            healtherror(str(e))

    def getTempStatus(self):
        try:
            monitortemp = []
            status.getTemp(monitortemp)
            monitortemp.sort(key = self.sortCallback)
            self.getMonitorInTemp(monitortemp)
        except AttributeError as e:
            healtherror(str(e))

    def getPsuStatus(self):
        try:
            psuStates = []
            status.getPsu(psuStates)
            psuStates.sort(key = self.sortCallback)
            self.checkPsu(psuStates)
        except AttributeError as e:
            healtherror(str(e))
    
    def fanStatusCheck(self): # 风扇状态检测
        if self.fanOKNum < 4:
            healthwarninglog("%%FAN-ERROR_FAN_NUM : %d" % (self.fanOKNum))
            return False
        return True

    # 获取温度信息
    def checkBoardMonitorMsg(self):
        try:
            self.getFanStatus()
            # self.getTempStatus()
            # self.getCpuStatus()
            self.getPsuStatus()

            # self.fanStatusCheck()
            return True
        except Exception as e:
            healtherror(str(e))
            return False;    
        
def callback():
    pass

def run(interval):
    health = HealthMonitor()
    while True:
        try:
            health.checkBoardMonitorMsg()
            time.sleep(interval)
        except Exception as e:
            traceback.print_exc()
            print(e)

@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    pass
    
@main.command()
def start():
    '''start fan control'''
    healthwarninglog("HEALTHMONITOR start")
    interval = 30
    run(interval)

@main.command()
def stop():
    '''stop fan control '''
    healthwarninglog("stop")

##device_i2c operation
if __name__ == '__main__':
    main()