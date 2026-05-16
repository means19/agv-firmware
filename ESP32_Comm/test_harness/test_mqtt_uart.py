"""
Simple test harness: publish an `order` JSON to MQTT and capture UART bytes
from the ESP32 to validate STM32 command packets (HEADER CMD CRC).

Usage:
  python test_mqtt_uart.py --mqtt-host localhost --serial-port COM3

The script prints received `state` messages and any 3-byte packets
starting with 0xB1 observed on the serial port.
"""

import argparse
import json
import threading
import time
import sys

import paho.mqtt.client as mqtt
import serial

HEADER = 0xB1


def compute_crc(header, cmd):
    return header ^ cmd


class TestHarness:
    def __init__(self, mqtt_host, mqtt_port, topic_order, topic_state, serial_port, serial_baud, order_file, timeout):
        self.mqtt_host = mqtt_host
        self.mqtt_port = mqtt_port
        self.topic_order = topic_order
        self.topic_state = topic_state
        self.serial_port = serial_port
        self.serial_baud = serial_baud
        self.order_file = order_file
        self.timeout = timeout

        self.state_event = threading.Event()
        self.state_payload = None
        self.uart_event = threading.Event()
        self.uart_packets = []

        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message

        self.ser = None

    def on_connect(self, client, userdata, flags, rc):
        print(f"[MQTT] Connected rc={rc}")
        client.subscribe(self.topic_state)

    def on_message(self, client, userdata, msg):
        try:
            payload = msg.payload.decode('utf-8')
        except Exception:
            payload = str(msg.payload)
        print(f"[MQTT] Message on {msg.topic}: {payload}")
        if msg.topic == self.topic_state:
            self.state_payload = payload
            self.state_event.set()

    def start_mqtt(self):
        self.client.connect(self.mqtt_host, self.mqtt_port, 60)
        thread = threading.Thread(target=self.client.loop_forever, daemon=True)
        thread.start()

    def start_serial(self):
        if not self.serial_port:
            print("[UART] No serial port configured, skipping UART capture")
            return
        try:
            self.ser = serial.Serial(self.serial_port, self.serial_baud, timeout=0.1)
            print(f"[UART] Opened {self.serial_port} @ {self.serial_baud}")
        except Exception as e:
            print(f"[UART] Failed to open serial port: {e}")
            self.ser = None

        def read_loop():
            buf = bytearray()
            start_time = time.time()
            while time.time() - start_time < self.timeout:
                if not self.ser:
                    break
                data = self.ser.read(64)
                if data:
                    buf.extend(data)
                    # scan buffer for packets
                    while len(buf) >= 3:
                        # find header
                        idx = buf.find(bytes([HEADER]))
                        if idx == -1:
                            # drop old bytes
                            buf = bytearray()
                            break
                        if len(buf) - idx < 3:
                            # need more bytes
                            break
                        packet = buf[idx:idx+3]
                        hdr, cmd, crc = packet[0], packet[1], packet[2]
                        if compute_crc(hdr, cmd) == crc:
                            print(f"[UART] Valid packet: {packet.hex()} cmd=0x{cmd:02X}")
                            self.uart_packets.append(packet)
                            self.uart_event.set()
                        else:
                            print(f"[UART] Bad CRC packet: {packet.hex()}")
                        # consume packet
                        buf = buf[idx+3:]
                else:
                    time.sleep(0.01)

        t = threading.Thread(target=read_loop, daemon=True)
        t.start()

    def publish_order(self):
        with open(self.order_file, 'r') as f:
            payload = f.read()
        print(f"[MQTT] Publishing order to {self.topic_order}")
        self.client.publish(self.topic_order, payload)

    def run(self):
        self.start_mqtt()
        self.start_serial()
        # give connections a moment
        time.sleep(1.0)
        
        # 1. Gửi bản tin Order
        self.publish_order()
        print("[TEST] Waiting for order to be processed...")
        time.sleep(1.0)
        
        # 2. Giả lập việc AGV quét được thẻ RFID tại node_start
        print("[MQTT] Simulating RFID scan for 'node_start'")
        self.client.publish("test/rfid", "node_start")
        
        print("[TEST] Waiting for state message and UART packet (timeout {}s)".format(self.timeout))
        start = time.time()
        while time.time() - start < self.timeout:
            if self.state_event.is_set() and (self.uart_event.is_set() or self.ser is None):
                print("[TEST] Success: received state and UART packet (or UART skipped)")
                return 0
            time.sleep(0.1)
        print("[TEST] Timeout: did not receive expected events")
        return 2


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--mqtt-host', default='localhost')
    parser.add_argument('--mqtt-port', type=int, default=1884)
    parser.add_argument('--topic-order', default='order')
    parser.add_argument('--topic-state', default='state')
    parser.add_argument('--serial-port', default=None)
    parser.add_argument('--serial-baud', type=int, default=115200)
    parser.add_argument('--order-file', default='sample_order.json')
    parser.add_argument('--timeout', type=int, default=15)

    args = parser.parse_args()

    harness = TestHarness(
        mqtt_host=args.mqtt_host,
        mqtt_port=args.mqtt_port,
        topic_order=args.topic_order,
        topic_state=args.topic_state,
        serial_port=args.serial_port,
        serial_baud=args.serial_baud,
        order_file=args.order_file,
        timeout=args.timeout
    )

    rc = harness.run()
    sys.exit(rc)
