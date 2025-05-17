# ESP32 Smart Home with MQTT

This project is a smart home system built using ESP32 and MQTT protocol, controlling and monitoring six different rooms in a miniature house. It supports real-time communication with Node-RED and a web dashboard.

## 🏠 Room Overview and Features

### 1. Teras (Porch)
- **RFID Scanner** to open the servo door if a valid card is detected.
- **I2C LCD** to display welcome messages.
- **PIR Motion Sensor** (active between 1-3 AM) to detect movement and trigger an **alarm** (pin 27), MQTT-controlled.
- **Relay** to control lights via MQTT (Node-RED/Web Dashboard).

### 2. Ruang Tamu (Living Room)
- Control **lampu (lights)** and **fan** via MQTT (relays).

### 3. Kamar Mandi (Bathroom)
- **PIR Sensor** to turn on light automatically (with delay), using relay.

### 4. Kamar 1 (Bedroom 1)
- Control **light** via MQTT (relay).

### 5. Kamar 2 (Bedroom 2)
- Control **light** via MQTT (relay).
- Monitor **temperature & humidity** using **DHT22** sensor.
  - MQTT Topics:
    - `home/kamar2/suhu`
    - `home/kamar2/kelembapan`

### 6. Dapur (Kitchen)
- Control **light** via MQTT (relay).
- Detect **gas leakage** using MQ-15 sensor and trigger alarm (pin 27).

---

## 📡 MQTT Topics

| Feature             | Topic                          | Direction | Description                         |
|---------------------|--------------------------------|-----------|-------------------------------------|
| Lampu semua ruangan | `home/[ruangan]/lampu/set`     | SUB       | ON/OFF command                      |
|                     | `home/[ruangan]/lampu/status`  | PUB       | Current lampu status                |
| Kipas ruang tamu    | `home/ruangtamu/fan/set`       | SUB       | ON/OFF fan                          |
| Suhu/Kelembapan     | `home/kamar2/suhu`             | PUB       | Suhu dari DHT22                     |
|                     | `home/kamar2/kelembapan`       | PUB       | Kelembapan dari DHT22               |
| Alarm gerak teras   | `home/teras/alarm/set`         | SUB       | Aktif/nonaktif sensor gerak malam  |
| RFID                | `home/teras/rfid/status`       | PUB       | Status kartu RFID yang di-scan      |
| Gas alarm dapur     | `home/dapur/gas/status`        | PUB       | Status gas (NORMAL/BAHAYA)         |

---

## 🔧 Hardware Used

- ESP32
- RFID RC522
- Servo Motor (SG90)
- I2C LCD 16x2
- PIR Motion Sensors
- Relays (6+)
- DHT22 Sensor
- MQ-15 Gas Sensor
- Buzzer / Alarm
- Node-RED / Web Dashboard
- Power Supply & Jumper Wires

---

## 🚀 How to Use

1. Flash the Arduino code to ESP32.
2. Configure your WiFi and MQTT credentials in the sketch.
3. Use Node-RED or Web UI to control and monitor each room.
4. Test RFID, PIR, and gas sensor functionality.

---

## 📌 Note

- All relays send their state to MQTT for web sync.
- Alarm is shared between gas and motion detection at night.
- LCD uses I2C interface (SDA/SCL).
- System is modular and expandable.

---

## 🧑‍💻 License

MIT License
