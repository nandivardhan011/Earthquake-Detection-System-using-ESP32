# Earthquake-Detection-System-using-ESP32
IoT-based Earthquake Monitoring and Alert System using ESP32 and MPU6050

# 🌍 Earthquake Detection System using ESP32

An IoT-based real-time earthquake monitoring and alert system using *ESP32* and *MPU6050* accelerometer.  
The system detects ground vibrations and sends visual, audio, and SMS alerts using the *Twilio API*.

---

## 🧩 Components Used
- ESP32 Dev Board  
- MPU6050 Accelerometer  
- OLED Display (0.96”)  
- Buzzer  
- 3 LEDs (Green, Yellow, Red)  
- Wi-Fi connectivity for SMS alert

---

## ⚙ Working Principle
1. *Calibration* – Removes bias from accelerometer readings  
2. *Vibration Detection* – Calculates magnitude sqrt(x² + y² + z²)  
3. *Alert Levels:*
   | Magnitude | Level | Response |
   |------------|--------|-----------|
   | 5–10 m/s² | Minor | Green LED |
   | 10–15 m/s² | Moderate | Yellow LED + Buzzer + SMS |
   | >15 m/s² | Extreme | Red LED + Continuous buzzer + SMS |

---

## 📲 Features
- Real-time monitoring  
- Multi-level alerts (Visual + Audio + SMS)  
- Wi-Fi + Twilio API integration  
- Cost-effective IoT implementation  

---

## 🧠 Future Enhancements
- Add GPS for epicenter location  
- Cloud logging (ThingSpeak / Blynk)  
- LoRa/ESP-NOW network for large coverage  
- Solar-powered version  

---

