#==========================================================================
# (c) 2021 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Serial-Multichannel IO Library
# File    : smcio.py
#--------------------------------------------------------------------------
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#--------------------------------------------------------------------------
#
# Environment Requirements: None
#
#==========================================================================

#==========================================================================
# IMPORTS
#==========================================================================
import abc
import queue
import threading

#==========================================================================
# VERSION
#==========================================================================

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================

#==========================================================================
# CLASSES
#==========================================================================
class packet:
    def __init__(self):
        self.type = 0
        self.count = 0
        self.length = 0
        self.checksum = 0
        self.payload = []

        return

    @classmethod
    def fromTypeString(cls, packet_type, packet_count, packet_str):
        a = cls()
        a.type = packet_type
        a.count = packet_count
        for c in packet_str:
            a.payload.append(ord(c))
        a.length = len(a.payload)
        # calculate checksum
        a.checksum = 0xAB
        return a

    def validate(self):

        if (self.length != len(self.payload)):
            return False

        temp_checksum = 0

        for p in self.payload:
            temp_checksump += p

        temp_checksum = ~temp_checksum

        if (self.checksum != temp_checksum):
            return False

        return True

    def payload_to_string(self):
        temp_str = ''

        for i in self.payload:
            temp_str += chr(i)

        return temp_str

    def encode(self):
        temp_bytes = b'\x01'
        temp_bytes += bytes([self.type])
        temp_bytes += bytes([self.count & 0xFF])    # packet parameter for 'count' is only 8-bits
        temp_bytes += bytes([(self.length & 0xFF00) >> 8])
        temp_bytes += bytes([(self.length & 0xFF)])
        temp_bytes += b'\x02'
        temp_bytes += bytes(self.payload)
        temp_bytes += b'\x03'
        temp_bytes += bytes([self.checksum])
        temp_bytes += b'\x04'

        return temp_bytes

    def __str__(self):
        temp_str = ''

        temp_str += 'Type: ' + str(self.type) + '\n'
        temp_str += 'Count: ' + str(self.count) + '\n'
        temp_str += 'Length: ' + str(self.length) + '\n'
        temp_str += 'Checksum: ' + str(self.checksum) + '\n'
        temp_str += 'Payload: '
        for i in self.payload:
            temp_str += chr(i)
        temp_str += '\n'

        return temp_str

class packet_parser:
    def __init__(self):
        self.reset()

        return

    def add_bytes(self, byte_list):
        self.byte_list += byte_list

        return

    def parse(self, b):
        self.byte_list += b

        #print(self.state + str(b))
        temp_int = int.from_bytes(b, byteorder='little', signed=False)
        if ((self.state == 'idle') and (temp_int == 0x01)):
            self.state = 'soh'
        elif (self.state == 'soh'):
            self.temp_packet = packet()
            self.temp_packet.type = chr(temp_int)
            self.state = 'type'
        elif (self.state == 'type'):
            self.temp_packet.count = temp_int
            self.byte_count = 0
            self.state = 'count'
        elif (self.state == 'count'):
            if (self.byte_count == 0):
                self.temp_packet.length = temp_int
                self.byte_count += 1
            else:
                self.temp_packet.length <<= 8
                self.temp_packet.length |= temp_int
                self.state = 'length'
        elif (self.state == 'length'):
            if (temp_int == 0x02):
                self.byte_count = 1
                self.state = 'sot'
            else:
                self.state = 'idle'
        elif (self.state == 'sot'):
            self.temp_packet.payload.append(temp_int)
            if (self.byte_count < self.temp_packet.length):
                self.byte_count += 1
            else:
                self.state = 'payload'
        elif (self.state == 'payload'):
            if (temp_int == 0x03):
                self.state = 'eo_text'
            else:
                self.state = 'idle'
        elif (self.state == 'eo_text'):
            self.temp_packet.checksum = temp_int
            self.state = 'checksum'
        elif (self.state == 'checksum'):
            if (temp_int == 0x04):
                #print(self.temp_packet)
                self.latest_packet = self.temp_packet
                self.temp_packet = None
            self.state = 'idle'

        return

    def reset(self):
        self.byte_list = []
        self.state = 'idle'
        self.byte_count = 0
        self.temp_packet = None
        self.latest_packet = None

        return

    def get_new_packet(self):
        temp = self.latest_packet
        self.latest_packet = None
        return temp

class serial_io_interface(metaclass=abc.ABCMeta):
    @classmethod
    def __subclasshook__(cls, subclass):
        return (hasattr(subclass, 'write') and
                callable(subclass.write) and
                hasattr(subclass, 'read') and
                callable(subclass.read) or
                NotImplemented)

    @abc.abstractmethod
    def write(self, data: bytes):
        """Write data"""
        raise NotImplementedError

    @abc.abstractmethod
    def read(self, length: int):
        """Read some data"""
        raise NotImplementedError

class channel:

    def __init__(self, id, rx_callback, rx_callback_arg, send_packet):
        self.id = id

        self.rx_callback = rx_callback
        self.rx_callback_arg = rx_callback_arg
        self.rx_q = queue.Queue()
        self.rx_packet_count = None

        self.send_packet = send_packet
        self.tx_packet_count = 0
        return

    def write(self, packet_string):
        self.send_packet(packet.fromTypeString(ord(self.id[0]), self.tx_packet_count, packet_string))
        self.tx_packet_count += 1
        return

    def read(self):
        temp_str = ''

        while (not self.rx_q.empty()):
            temp_str += self.rx_q.get()

        return temp_str

class processor:

    def __init__(self, serial_io: serial_io_interface, verbose = False):
        self.io = serial_io
        self.channels = dict()
        self.tx_q = queue.Queue()
        self.parser = packet_parser()
        self.stop_event = threading.Event()
        self.verbose = verbose

        self.rx_parse_thread = threading.Thread(target=self.rx_parser, daemon=True)
        self.tx_packets_thread = threading.Thread(target=self.tx_packets, daemon=True)
        return

    def rx_parser(self):
        while not self.stop_event.is_set():
            l = self.io.read(1)
            if (l is not None):
                self.parser.parse(l)

                p = self.parser.get_new_packet()
                if (p is not None):
                    if (self.verbose):
                        print("RX Packet:")
                        print(p)

                    key = p.type
                    if (key in self.channels):
                        c = self.channels[key]

                        if ((c.rx_packet_count is not None) and
                           ((c.rx_packet_count + 1) != p.count)):
                            print("rx_packet_count lost sync!")
                        c.rx_packet_count = p.count

                        s = p.payload_to_string()
                        c.rx_q.put(s)
                        c.rx_q.task_done()
                        if (c.rx_callback is not None):
                            c.rx_callback(c.rx_callback_arg, s)

        return

    def tx_packets(self):

        while not self.stop_event.is_set():
            if (not self.tx_q.empty()):
                tx_packet = self.tx_q.get()

                if (self.verbose):
                    print("TX Packet:")
                    print(tx_packet)
                self.io.write(tx_packet.encode())
                self.tx_q.task_done()

        return

    def add_channel(self, id, callback, callback_arg):
        self.channels[id] = channel(id, callback, callback_arg, self.tx_q.put)
        return

    def read_channel(self, id):
        return self.channels[id].read()

    def write_channel(self, id, packet_string):
        self.channels[id].write(packet_string)
        return

    def start(self):
        self.rx_parse_thread.start()
        self.tx_packets_thread.start()
        return

    def stop(self):
        self.stop_event.set()
        self.rx_parse_thread.join()
        self.tx_packets_thread.join()
        return

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================
