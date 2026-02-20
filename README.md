# 🌱 ESP32 Hydroponic Control System

An automated hydroponic nutrient monitoring and control system built using **ESP32**, **Arduino IDE**, and environmental sensors.  
The system continuously monitors water quality parameters and automatically adjusts pH and TDS levels to maintain optimal plant growth conditions.

---

## 🚀 Features

- 🌡 Real-time Temperature & Humidity Monitoring (DHT11)
- 💧 Automatic pH Regulation (Acid / Alkali dosing pumps)
- ⚡ Automatic TDS Control (Nutrient / Water control)
- ⏱ 5-Second Safety Delay before correction
- 🔁 Continuous Monitoring & Correction Logic
- 📊 Live Serial Monitor Status Output
- 🛑 Automatic motor stop when values reach safe range

---

## 🛠 Hardware Components

- ESP32 Microcontroller
- DHT11 Temperature & Humidity Sensor
- Analog pH Sensor
- 2-Channel Relay Module
- Acid Dosing Pump
- Alkali Dosing Pump
- Power Supply Unit

---

## 📌 Pin Configuration
```
| Component        | ESP32 GPIO|
|------------------|-----------|
| pH Sensor        | GPIO 34   |
| DHT11            | GPIO 21   |
| Acid Relay       | GPIO 12   |
| Alkali Relay     | GPIO 13   |
```
---

## 🎯 Target Operating Range
```
| Parameter | Minimum | Maximum |
|-----------|---------|---------|
| pH        | 5.5     | 6.5     |
| TDS       | 800 ppm | 1200 ppm|
```
---

## 🧠 System Working Logic

1. System initializes sensors and relay modules.
2. Temperature and humidity are read from DHT11 (real values).
3. pH and TDS values are monitored and compared against predefined thresholds.
4. If values are out of range:
   - System waits 5 seconds (safety delay).
   - Activates appropriate dosing pump.
5. Motors run continuously until parameters return to safe range.
6. System returns to stable monitoring state.

---

## 📂 Project Structure

```
ESP32-Hydroponic-Control-System/
│
├── esp32_hydroponic_control_system.ino
├── README.md
│
└── images/
    └── circuit_diagram.png
```

---

## 🔮 Future Improvements

- 🌐 WiFi-based IoT Dashboard
- ☁ Cloud Monitoring Integration
- 📱 Mobile App Integration
- 📊 Real-time Web Data Visualization
- 🔬 Integration of Actual TDS Sensor (Replace simulated logic)

---
---

## 👨‍💻 Contributors

<table>
<tr>

<td align="center">
<img src="https://github.com/akhileshwar-p-s.png" width="120px;" alt="Akhileshwar Pratap Singh"/><br>
<b>Akhileshwar Pratap Singh</b><br>
🏆 Project Lead<br>
<a href="https://github.com/akhileshwar-p-s">GitHub</a> |
<a href="https://linkedin.com/in/akhileshwar-p-s">LinkedIn</a>
</td>

<td align="center">
<img src="https://github.com/vignesh27-s.png" width="120px;" alt="Vigneshwaran S"/><br>
<b>Vigneshwaran S</b><br>
<a href="https://github.com/vignesh27-s">GitHub</a> |
<a href="https://linkedin.com/in/vignesh27-s">LinkedIn</a>
</td>

<td align="center">
<img src="https://github.com/prs-24.png" width="120px;" alt="Prateek Raj"/><br>
<b>Prateek Raj</b><br>
<a href="https://github.com/prs-24">GitHub</a> |
<a href="https://linkedin.com/in/prajsinha">LinkedIn</a>
</td>

</tr>
</table>

---


## 📜 License

This project is licensed under the MIT License.

---

## ⭐ Acknowledgment

This project was developed as part of a smart agriculture / embedded systems initiative focusing on automation in hydroponic farming.
