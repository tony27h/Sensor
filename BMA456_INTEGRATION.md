# BMA456 Accelerometer Impact Detection Integration

## Overview

This project now includes movement/impact detection using a Bosch BMA456 accelerometer connected over I2C. When the sensor detects acceleration forces greater than approximately 2g, it triggers an interrupt that turns on an LED for 5 seconds.

## Hardware Configuration

### BMA456 Connections
- **I2C Interface**: Connected to STM32WB05 I2C1 peripheral
- **I2C Address**: 0x18 (7-bit address)
- **INT1 Pin**: Connected to MCU pin **PA9** with pull-down resistor
- **Power**: VDD and GND

### MCU Configuration
- **PA9**: Configured as EXTI (External Interrupt) with rising edge trigger and pull-down
- **PB1**: LED output (LED_YELLO_Pin)
- **TIM16**: Used for 5-second LED timeout

## Feature Behavior

1. **Detection**: The BMA456 high-g feature continuously monitors acceleration on all axes (X, Y, Z)
2. **Threshold**: Configured to trigger at approximately 2g acceleration
3. **Interrupt**: When threshold is exceeded, BMA456 INT1 pin goes high
4. **LED Response**: 
   - PA9 interrupt immediately turns on PB1 LED
   - TIM16 timer starts for 5-second countdown
   - If another impact occurs while LED is on, the timer restarts (retriggerable)
5. **UART Output**: 
   - Accelerometer data is read immediately on interrupt
   - Force magnitude is calculated from X, Y, Z components
   - Data is transmitted via UART1 at 9600 baud
   - Format: `Impact detected! Force: X.XXg (X:X.XXg Y:X.XXg Z:X.XXg)`
6. **LED Off**: After 5 seconds with no new detections, LED turns off automatically

## Software Architecture

### Files Added
- `Core/Inc/bma456_app.h` - BMA456 application header
- `Core/Src/bma456_app.c` - BMA456 application implementation

### Files Modified
- `Core/Src/main.c` - Added BMA456 initialization call
- `Core/Src/stm32wb0x_it.c` - Added GPIO and timer interrupt handlers

### Key Components

#### BMA456 Configuration
```c
- High-g threshold: 256 (in 5.11g format) ≈ 2g
- Duration: 10 samples at 100Hz = 100ms
- Hysteresis: 2770 (in 0.74g format) ≈ 0.5g
- Axes enabled: X, Y, Z (all axes)
- Interrupt: Mapped to INT1, edge-triggered, active high
- Accelerometer ODR: 100Hz, Range: 2g
```

#### Bosch API Integration
The implementation provides three callback functions required by the Bosch BMA4 API:
1. **I2C Read**: `bma456_i2c_read()` - Uses HAL_I2C_Mem_Read
2. **I2C Write**: `bma456_i2c_write()` - Uses HAL_I2C_Mem_Write
3. **Delay**: `bma456_delay_us()` - Uses HAL_Delay (converts μs to ms)

## Building the Project

This is an STM32CubeIDE project. To build:

1. Open the project in **STM32CubeIDE**
2. Ensure all BMA456 source files are included in the build:
   - `Core/Src/bma4.c`
   - `Core/Src/bma456mm.c`
   - `Core/Src/bma456_app.c`
3. Build the project (Project → Build All)
4. Flash to the STM32WB05 target

## Testing the Feature

### Basic Functional Test

1. **Power on** the device with BMA456 connected
2. **Wait** for initialization (LED may blink briefly during startup)
3. **Shake** or **tap** the device to create ~2g acceleration
4. **Observe**: 
   - PB1 LED should turn on immediately
   - LED should remain on for exactly 5 seconds
   - LED should turn off automatically

### Retriggerable Behavior Test

1. Trigger an impact to turn on the LED
2. While LED is still on (within 5 seconds), trigger another impact
3. **Observe**: The 5-second timer should restart from the second impact

### Sensitivity Verification

- **Light taps**: Should NOT trigger (below 2g threshold)
- **Moderate shakes**: Should trigger reliably
- **Hard impacts**: Should always trigger

## Threshold Adjustment

If the sensitivity needs adjustment, modify these values in `bma456_app.h`:

```c
/* For higher threshold (less sensitive): increase value
 * For lower threshold (more sensitive): decrease value
 * Formula: threshold_value = (desired_g * 2048) / 16
 */
#define BMA456_HIGH_G_THRESHOLD   256  // Current: ~2g

/* For longer detection window: increase duration
 * Each unit = 10ms at 100Hz sampling
 */
#define BMA456_HIGH_G_DURATION    10   // Current: 100ms

/* For different LED on time: change duration
 * Value in milliseconds
 */
#define BMA456_LED_ON_DURATION_MS 5000  // Current: 5 seconds
```

After changing these values, rebuild and reflash the firmware.

## Troubleshooting

### LED Never Turns On

1. **Check I2C connection**: Verify BMA456 is at address 0x18
   ```c
   // Add debug code in bma456_app_init() to check chip ID
   if (bma456_dev.chip_id != BMA456MM_CHIP_ID) {
       // I2C communication issue or wrong sensor
   }
   ```

2. **Check INT1 connection**: Verify PA9 is connected to BMA456 INT1
3. **Check pull-down resistor**: PA9 should have pull-down to prevent floating

### LED Turns On Randomly

1. **Electrical noise**: Add capacitor (0.1μF) near BMA456 VDD pin
2. **Threshold too low**: Increase `BMA456_HIGH_G_THRESHOLD` value
3. **Check grounding**: Ensure BMA456 and MCU share common ground

### LED Doesn't Turn Off After 5 Seconds

1. **Check TIM16 configuration**: Verify prescaler and period values
2. **Check interrupt priority**: Ensure TIM16_IRQHandler is enabled
3. **Debug**: Add breakpoint in `bma456_app_timer_callback()`

## API Reference

### Public Functions

#### `HAL_StatusTypeDef bma456_app_init(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart)`
Initializes the BMA456 sensor and configures high-g detection.

**Parameters:**
- `hi2c`: Pointer to initialized I2C handle (must be I2C1)
- `huart`: Pointer to initialized UART handle (must be UART1) for force reporting

**Returns:**
- `HAL_OK`: Initialization successful
- `HAL_ERROR`: Initialization failed

**Called from:** `main.c` during system initialization

---

#### `void bma456_app_handle_interrupt(void)`
Handles BMA456 interrupt event. Reads interrupt status, accelerometer data, calculates force magnitude, sends data via UART, turns on LED, and starts timer.

**Called from:** `HAL_GPIO_EXTI_Callback()` in `stm32wb0x_it.c`

---

#### `void bma456_app_timer_callback(void)`
Timer timeout callback. Turns off LED and stops timer.

**Called from:** `HAL_TIM_PeriodElapsedCallback()` in `stm32wb0x_it.c`

## Notes

- **Thread Safety**: The current implementation assumes single-threaded operation. If using an RTOS, add mutex protection around LED state changes.
  
- **Power Consumption**: BMA456 is configured in continuous mode. For battery-powered applications, consider:
  - Enabling BMA456 auto-low-power mode
  - Using no-motion interrupt to enter sleep
  - Adjusting accelerometer ODR (currently 100Hz)

- **Interrupt Latency**: The latched interrupt mode ensures events are not missed. INT1 stays high until status is read.

- **Axis Configuration**: Currently all axes (X, Y, Z) are monitored. To monitor specific axes only, modify `axes_en` in `bma456_app.c`:
  ```c
  high_g_config.axes_en = BMA456MM_HIGH_G_X_EN | BMA456MM_HIGH_G_Y_EN;  // X and Y only
  ```

## References

- [BMA456 Datasheet](https://www.bosch-sensortec.com/products/motion-sensors/accelerometers/bma456/)
- [BMA4 API Documentation](https://github.com/BoschSensortec/BMA456-Sensor-API)
- STM32WB05 Reference Manual
- STM32 HAL Driver Documentation
