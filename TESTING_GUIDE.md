# BMA456 Impact Detection - Testing Guide

## Quick Start

This guide provides step-by-step instructions for testing the BMA456 accelerometer impact detection feature.

## Prerequisites

1. **Hardware Setup:**
   - STM32WB05 board with firmware flashed
   - BMA456 accelerometer connected:
     - I2C SDA/SCL to STM32 I2C1
     - INT1 pin to PA9
     - VDD and GND connected
     - Pull-down resistor on PA9 (if not internal)

2. **Build and Flash:**
   - Open project in STM32CubeIDE
   - Build project (Ctrl+B)
   - Flash to target (F11)

## Test Procedures

### Test 1: Basic Detection

**Objective:** Verify that impacts trigger LED

**Steps:**
1. Power on the device
2. Wait 2 seconds for initialization
3. Firmly tap the board or device with your finger
4. **Expected:** PB1 LED turns ON immediately
5. Wait 5 seconds
6. **Expected:** LED turns OFF automatically

**Pass Criteria:**
- LED turns on within 100ms of tap
- LED stays on for 5.0 ± 0.1 seconds
- LED turns off automatically

### Test 2: Threshold Sensitivity

**Objective:** Verify ~2g threshold

**Steps:**
1. Power on device
2. Perform these actions and observe LED:

   | Action | Expected Result |
   |--------|----------------|
   | Gentle touch | LED OFF (< 1g) |
   | Light tap | LED OFF (< 2g) |
   | Firm tap | LED ON (> 2g) |
   | Drop from 10cm | LED ON (> 2g) |
   | Shake vigorously | LED ON (> 2g) |

**Pass Criteria:**
- LED does NOT trigger on gentle movements (< 2g)
- LED DOES trigger on firm taps/impacts (> 2g)

### Test 3: Retriggerable Behavior

**Objective:** Verify timer restart on new impacts

**Steps:**
1. Trigger impact to turn LED ON
2. Wait 2 seconds (LED still on)
3. Trigger another impact
4. **Expected:** Timer restarts, LED stays on for another 5 seconds from the second impact
5. Total LED on time should be approximately 7 seconds (2s + 5s)

**Pass Criteria:**
- Second impact before timeout restarts the 5-second timer
- LED does NOT turn off between impacts
- Final timeout is 5 seconds from the LAST impact

### Test 4: Multi-Axis Detection

**Objective:** Verify all axes (X, Y, Z) trigger detection

**Steps:**
1. Place device flat on table (Z-axis vertical)
2. Tap from above (Z-axis impact)
3. **Expected:** LED ON

4. Hold device vertically
5. Tap from the side (X-axis impact)
6. **Expected:** LED ON

7. Hold device horizontally
8. Tap from the top (Y-axis impact)
9. **Expected:** LED ON

**Pass Criteria:**
- LED triggers for impacts on any axis
- No preferred direction

### Test 5: Continuous Operation

**Objective:** Verify stable operation over time

**Steps:**
1. Power on device
2. Trigger 10 impacts at 10-second intervals
3. Observe LED behavior for each

**Pass Criteria:**
- All 10 impacts successfully trigger LED
- No false triggers between intentional impacts
- No system hangs or crashes

## Troubleshooting

### Problem: LED Never Turns On

**Debug Steps:**
1. Check I2C connection:
   ```c
   // Add to bma456_app_init() after bma456mm_init():
   printf("BMA456 Chip ID: 0x%02X (expected 0x16)\n", bma456_dev.chip_id);
   ```

2. Verify PA9 interrupt:
   ```c
   // Add to HAL_GPIO_EXTI_Callback():
   printf("PA9 interrupt triggered!\n");
   ```

3. Check interrupt status:
   ```c
   // Add to bma456_app_handle_interrupt():
   printf("Int status: 0x%04X\n", int_status);
   ```

**Common Causes:**
- I2C not connected or wrong address
- INT1 not connected to PA9
- BMA456 not powered
- Pull-down missing on PA9

### Problem: LED Turns On Randomly

**Debug Steps:**
1. Check threshold sensitivity:
   ```c
   // In bma456_app.h, increase threshold:
   #define BMA456_HIGH_G_THRESHOLD   512  // 4g instead of 2g
   ```

2. Check electrical noise:
   - Add 0.1µF capacitor near BMA456 VDD
   - Verify ground connection
   - Check for EMI sources

**Common Causes:**
- Threshold too low
- Electrical noise on INT1 line
- Vibration from other components
- Poor grounding

### Problem: LED Doesn't Turn Off

**Debug Steps:**
1. Verify TIM16 running:
   ```c
   // Add to bma456_app_timer_callback():
   printf("Timer callback executed\n");
   ```

2. Check timer configuration:
   - Prescaler: 6399 (6400-1)
   - Period: 49999 (50000-1)
   - Should give 5 seconds at 64MHz

**Common Causes:**
- TIM16 interrupt not enabled
- Timer not started
- Callback not called

### Problem: Inconsistent Sensitivity

**Calibration Steps:**
1. Adjust threshold in `bma456_app.h`:
   ```c
   // More sensitive (lower threshold):
   #define BMA456_HIGH_G_THRESHOLD   128  // ~1g
   
   // Less sensitive (higher threshold):
   #define BMA456_HIGH_G_THRESHOLD   512  // ~4g
   ```

2. Adjust duration (debounce time):
   ```c
   // Shorter duration (more responsive):
   #define BMA456_HIGH_G_DURATION    5    // 50ms
   
   // Longer duration (less sensitive to brief spikes):
   #define BMA456_HIGH_G_DURATION    20   // 200ms
   ```

3. Rebuild and reflash after changes

## Performance Metrics

### Expected Values

| Metric | Target | Acceptable Range |
|--------|--------|------------------|
| Interrupt Latency | < 10ms | < 50ms |
| LED On Delay | < 100ms | < 200ms |
| LED Duration | 5000ms | 4950-5050ms |
| Detection Threshold | 2g | 1.8-2.2g |
| False Positive Rate | 0% | < 1% per hour |

### Measurement Procedures

**Interrupt Latency:**
```c
// Add timestamp in EXTI callback:
uint32_t timestamp = HAL_GetTick();
printf("Interrupt at: %lu ms\n", timestamp);
```

**LED Duration:**
```c
// Add timestamps:
static uint32_t led_on_time;

// In bma456_app_handle_interrupt():
led_on_time = HAL_GetTick();

// In bma456_app_timer_callback():
uint32_t duration = HAL_GetTick() - led_on_time;
printf("LED duration: %lu ms\n", duration);
```

## Test Report Template

```
Test Date: _____________
Tester: _____________
Firmware Version: _____________
Hardware Revision: _____________

Test 1: Basic Detection        [ PASS / FAIL ]
Comments: _______________________________

Test 2: Threshold Sensitivity  [ PASS / FAIL ]
Comments: _______________________________

Test 3: Retriggerable Behavior [ PASS / FAIL ]
Comments: _______________________________

Test 4: Multi-Axis Detection   [ PASS / FAIL ]
Comments: _______________________________

Test 5: Continuous Operation   [ PASS / FAIL ]
Comments: _______________________________

Overall Result: [ PASS / FAIL ]

Issues Found:
_______________________________________
_______________________________________
```

## Next Steps

After successful testing:
1. Document any threshold adjustments made
2. Update `BMA456_INTEGRATION.md` with production settings
3. Perform long-term stability testing
4. Consider power consumption optimization if needed

## Support

For issues not covered in this guide:
1. Check `BMA456_INTEGRATION.md` for detailed API documentation
2. Review BMA456 datasheet for sensor specifications
3. Consult STM32WB05 reference manual for GPIO/timer configuration
4. Contact developer for firmware-specific questions
