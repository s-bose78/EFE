| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | -------- | -------- | -------- |

# Blink Example

(See the README.md file in the upper level 'examples' directory for more information about examples.)

This example demonstrates how to switch on\off a LED by using the touch sensor connected to Pin 14.

## How to Use Example

Before project configuration and build, be sure to set the correct chip target using `idf.py set-target <chip_name>`.

### Hardware Required
* A development board with normal LED or addressable LED on-board (e.g., ESP32-S3-DevKitC, ESP32-C6-DevKitC etc.)
* A USB cable for Power supply and programming

See [Development Boards](https://www.espressif.com/en/products/devkits) for more information about it.

### Configure the Project

Open the project configuration menu (`idf.py menuconfig`).

In the `Example Configuration` menu:

* Select the LED type in the `Blink LED type` option.
  * Use `GPIO` for regular LED
* Set the GPIO number used for the signal in the `Blink GPIO number` option.

### Build and Flash

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

As you run the example, you will see the LED blinking, according to the previously defined period. For the addressable LED, you can also change the LED color by setting the `led_strip_set_pixel(led_strip, 0, 16, 16, 16);` (LED Strip, Pixel Number, Red, Green, Blue) with values from 0 to 255 in the [source file](main/blink_example_main.c).

```text
I (265) touch: Example configured to blink GPIO LED!
I (265) gpio: GPIO[38]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (275) touch: Touch Configuration Complete
I (10275) touch: Initial Debounce Complete
I (10275) touch: Creating touch task
I (10275) main_task: Returned from app_main()
I (17275) touch: Turn LED ON
I (22275) touch: Turn LED OFF
I (26275) touch: Turn LED ON
I (30275) touch: Turn LED OFF
```
