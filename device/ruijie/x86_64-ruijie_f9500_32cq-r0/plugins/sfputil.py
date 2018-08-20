# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 35
    PORTS_IN_BLOCK = 32

    EEPROM_OFFSET = 11

    _port_to_eeprom_mapping = {}

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return range(0, self.PORTS_IN_BLOCK + 1)

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        eeprom_path = "/sys/bus/i2c/devices/{0}-0050/eeprom"

        for x in range(0, self.port_end - 1):
            self._port_to_eeprom_mapping[x] = eeprom_path.format(x + self.EEPROM_OFFSET)

        self._port_to_eeprom_mapping[self.port_end - 1] = eeprom_path.format(self.port_end - 2 + self.EEPROM_OFFSET)
        self._port_to_eeprom_mapping[self.port_end] = eeprom_path.format(self.port_end - 2 + self.EEPROM_OFFSET)
        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        if port_num <= 7:
            presence_path = "/sys/bus/i2c/devices/1-0036/sfp_presence1"
        elif port_num >= 8 and port_num <= 15:
            presence_path = "/sys/bus/i2c/devices/1-0036/sfp_presence2"
        elif port_num >= 16 and port_num <= 23:
            presence_path = "/sys/bus/i2c/devices/1-0034/sfp_presence3"
        elif port_num >= 24 and port_num <= 31:
            presence_path = "/sys/bus/i2c/devices/1-0034/sfp_presence4"
        elif port_num >= 32 and port_num <= 33:
            presence_path = "/sys/bus/i2c/devices/1-0036/sfp_presence5"
        elif port_num >= 34 and port_num <= 35:
            return True  
        else:
            return False

        try:
            data = open(presence_path, "rb")
        except IOError:
            return False

        result = int(data.read(2), 16)
        data.close()

        # ModPrsL is active low
        if result & (1 << (port_num % 8)) == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num

        return True

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        return True
