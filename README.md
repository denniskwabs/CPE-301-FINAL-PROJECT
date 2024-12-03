# Evaporation Cooling System (Swamp Cooler)

## Overview
The Evaporation Cooling System is an embedded system designed to provide an energy-efficient cooling solution for dry climates. By leveraging sensor inputs and state-based logic, the system dynamically controls cooling operations, detects errors, and provides real-time feedback to users. Developed as part of the CPE 301 Embedded Systems Design course at the University of Nevada, Reno, this project integrates hardware components like temperature and water level sensors, a fan motor, and a stepper motor for vent adjustment.

## Features
- Dynamic State Management:
  - Operates across four states (DISABLED, IDLE, RUNNING, ERROR) with automatic transitions based on sensor readings.
- Automated Cooling:
  - Activates a fan motor when the temperature exceeds a threshold.
- Error Detection:
  - Monitors water levels and transitions to an ERROR state when water is low.
- Manual Vent Control:
  - Allows users to adjust vent angles using a potentiometer and stepper motor.
- Real-Time Feedback:
  - Displays temperature, humidity, and state information on a 16x2 LCD.

## Hardware Components
- Arduino 2560 Microcontroller
- DHT11 Sensor (Temperature and Humidity)
- Water Level Sensor
- Stepper Motor (Vent Adjustment)
- LCD Display (16x2)
- Fan Motor
- LED Indicators (Yellow, Green, Blue, Red)
- Push Buttons (Reset and Stop)

## Software Setup
1. Install Required Tools:
   - Download and install the Arduino IDE from Arduino's official site.
   - Add the following libraries via the Library Manager:
     - DHT.h for the DHT11 sensor
     - LiquidCrystal.h for the LCD display
2. Upload the Code:
   - Open the provided main.ino file in the Arduino IDE.
   - Connect the Arduino 2560 to your computer via USB.
   - Verify and upload the code to the microcontroller.

## How It Works
1. Initial State: 
   - The system powers on in the DISABLED state (yellow LED ON).
2. Idle Monitoring:
   - In the IDLE state, the system monitors temperature and humidity, displaying readings on the LCD.
   - Transitions to RUNNING if the temperature exceeds a preset threshold.
3. Active Cooling:
   - In the RUNNING state, the fan motor activates to cool the environment, and users can adjust vent angles via a potentiometer.
4. Error Handling:
   - The system transitions to the ERROR state when the water level is low, stopping the fan and displaying an error message.
5. Manual Stop:
   - Pressing the stop button transitions the system to the DISABLED state, halting all operations.

## Challenges Faced
1. Fan Motor Stability:
   - Issue: The fan motor experienced delayed startup.
   - Solution: Optimized power connections and implemented a startup delay in the code.
2. Water Sensor Handling:
   - Issue: False readings occurred during testing.
   - Solution: Improved code filtering to ignore invalid inputs and ensured proper sensor handling.

## Future Improvements
1. Upgrade the DHT11 sensor to DHT22 for enhanced accuracy.
2. Use a more reliable fan motor for smoother operation.

## Credits
This project was developed by Dennis Affo and Cade Canon as part of the CPE 301 Embedded Systems Design course at the University of Nevada, Reno. We thank our professor and classmates for their guidance and support.

## License
This project is licensed under the MIT License.
