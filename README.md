# Early Warning System for Banking Infrastructure

An ESP32-based IoT monitoring and early warning system designed to support environmental monitoring and access control in banking infrastructure.

## System Overview

The system consists of three ESP32 devices connected through MQTT communication:

- **Data Center:** Collects environmental data from multiple sensors and publishes monitoring information through MQTT.
- **Vault:** Handles access identification, door status monitoring, vibration detection, and related alert functions.
- **Monitoring:** Receives MQTT messages and controls visual and audible indicators based on system events.

## Hardware Architecture

- 3 ESP32 devices
- MQTT communication
- Wi-Fi network
- Environmental and security monitoring components
- Visual and audible alert indicators

## Repository Structure

```text
early-warning-system-iot/
├── DATA_CENTER_MQTT.ino
├── VAULT_MQTT.ino
└── MONITORING_MQTT.ino
```

## Technologies

- ESP32
- Arduino IDE
- MQTT
- Wi-Fi
- Mosquitto MQTT Broker

## My Contribution

- Hardware integration and wiring across the three ESP32 devices.
- Firmware implementation for sensor data acquisition, access identification, and automated alert responses.
- MQTT communication integration and hardware troubleshooting.

## Notes

Wi-Fi credentials and other environment-specific configurations are excluded from this repository. Update these settings before uploading the firmware to your devices.
