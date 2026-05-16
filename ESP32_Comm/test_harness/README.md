ESP32 <-> STM32 Test Harness

Purpose
-------
Lightweight Python harness to exercise ESP32 firmware via MQTT and capture UART packets sent to the STM32.

Setup
-----
1. Create a Python virtualenv and install dependencies:

```bash
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
```

2. Configure your MQTT broker and serial connection. Defaults assume an MQTT broker on `localhost:1883` and no serial port (use `--serial-port COM3` to enable).

Run
---
Publish the sample order and wait for `state` message and a 3-byte UART packet (HEADER 0xB1):

```bash
python test_mqtt_uart.py --mqtt-host localhost --serial-port COM3 --order-file sample_order.json
```

Options
-------
- `--mqtt-host` MQTT broker host (default: localhost)
- `--mqtt-port` MQTT broker port (default: 1884)
- `--topic-order` Topic to publish orders to (default: order)
- `--topic-state` Topic to listen for state (default: state)
- `--serial-port` Serial port to capture UART (optional)
- `--serial-baud` Baud rate for serial (default: 115200)
- `--order-file` Path to JSON order file (default: sample_order.json)

Notes
-----
- Adjust `--topic-order` and `--topic-state` if your firmware uses different topic names in `config.h`.
- This harness only performs a simple smoke test. For automated CI, extend it to assert JSON contents and parse multiple messages.
