# STM32 Firmware Documentation

## 1. Scope and Role
This firmware runs on the STM32F103C8T6 and is responsible for all time-critical control tasks of the AGV:
- High-frequency line-following PID control and motor actuation.
- Low-latency sensor acquisition (line sensors, ultrasonic, RFID trigger).
- UART command execution from the ESP32 coordinator.
- Safety handling for obstacle detection without blocking the control loop.

## 2. Hardware Interfaces
### 2.1 MCU and Clocks
- MCU: STM32F103C8T6 (Blue Pill).
- System clock: HSI -> PLL, configured in main.c.

### 2.2 Timers
- TIM3: PWM generation for motor control (speed and direction control in motor_control.c).
- TIM2: 1 MHz free-running counter used to measure ultrasonic echo pulse width.

### 2.3 GPIO and EXTI
- TRIG (HC-SR04): PB1, output push-pull.
- ECHO (HC-SR04): PB11, EXTI on rising and falling edges.
- RFID trigger: EXTI line configured in CubeMX (GPIO_PIN_5 in main.c).
- Error LED: Error_1_GPIO_Port / Error_1_Pin.

### 2.4 UART
- UART1: interrupt-driven RX, one byte at a time.
- Packet parsing and command queue handled in ESP32_comm.c.

## 3. Software Architecture
### 3.1 Main Loop (Non-Blocking)
The firmware uses a single main loop with periodic scheduling, avoiding long delays:
- Trigger HC-SR04 at >= 60 ms intervals.
- Update ultrasonic timeout state each loop.
- Evaluate obstacle condition and manipulate the AGV state machine.
- Always call AGV_Update() to maintain PID timing and UART command handling.

### 3.2 State Machine
The AGV state machine is implemented in states_handling.c and drives motor behavior:
- AGV_STATE_FOLLOW_LINE: PID control active.
- AGV_STATE_LOST_LINE: line reacquire logic with timeout.
- AGV_STATE_ROTATE: rotation to find the line.
- AGV_STATE_REACQUIRE_LINE: fine alignment back to the line.
- AGV_STATE_STOP / AGV_STATE_IDLE: motors stopped.

### 3.3 Command Handling
Commands are received from ESP32 over UART and processed in AGV_Update():
- CMD_FORWARD / CMD_LEFT / CMD_RIGHT -> follow line.
- CMD_ROTATE -> rotate state.
- CMD_STOP -> stop state.

## 4. Timing and Real-Time Behavior
- SysTick (HAL_GetTick) provides ms resolution for scheduling and timeouts.
- The PID loop is called every iteration of AGV_Update().
- The only blocking operation is a 10 us trigger pulse for HC-SR04.
- All other sensor logic is interrupt or state-machine driven.

## 5. Sensors
### 5.1 Line Sensors
- Sampled inside AGV_Update().
- PID computes correction based on weighted line position.

### 5.2 HC-SR04 Ultrasonic (Non-Blocking)
- Trigger: 10 us pulse (blocking for 10 us only).
- Echo measured with TIM2 and EXTI both edges:
  - Rising edge: capture timer and timestamp.
  - Falling edge: compute pulse width -> distance.
- Timeout after 30 ms sets distance = 999.0 cm (no obstacle).

### 5.3 RFID Trigger
- RFID reader pulses an EXTI line.
- HAL_GPIO_EXTI_Callback forwards to AGV_OnRFIDEvent().

## 6. Obstacle Handling (Current Logic)
- Object detection is based on the latest ultrasonic distance.
- When obstacle is detected:
  - Error LED is set.
  - The firmware forces AGV_STATE_STOP and calls Motor_Stop().
- When obstacle clears:
  - Error LED is cleared.
  - If the last command is CMD_FORWARD, the AGV resumes AGV_STATE_FOLLOW_LINE.
- PID loop remains active because AGV_Update() is always called.

## 7. Communication
### 7.1 Protocol
- Fixed 3-byte packets: HEADER, CMD, CRC (header XOR cmd).
- See ESP32_comm.h for protocol definition.

### 7.2 UART Flow
- UART interrupt receives one byte at a time.
- ESP32_ReceiveByte() pushes valid commands into a queue.
- AGV_Update() consumes queued commands and updates state.

## 8. Relevant Modules
- Core/Src/main.c: main loop, scheduling, and HAL callbacks.
- Core/Src/Hcsr04.c: non-blocking ultrasonic driver (EXTI + TIM2).
- Core/Src/motor_control.c: PWM motor control.
- Core/Src/states_handling.c: AGV state machine and PID logic.
- Core/Src/ESP32_comm.c: UART protocol parsing and command queue.

## 9. Configuration Notes
- Ensure ECHO EXTI is enabled on both edges in CubeMX and NVIC.
- TIM2 must run at 1 MHz for accurate distance conversion.
- Trigger HC-SR04 no faster than 60 ms to avoid crosstalk.
- Keep main loop non-blocking to preserve control stability.

## 10. Known Limitations and Extensions
- Obstacle handling currently forces STOP locally; ESP32 is not notified yet.
- For tighter safety, consider rejecting UART motion commands while obstacle is present.
- Consider sending an error byte to ESP32 for UI display and logging.
