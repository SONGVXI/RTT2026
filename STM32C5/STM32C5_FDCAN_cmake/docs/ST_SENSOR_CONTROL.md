# MC to ST environment control

Thresholds are RAM-only. SAVE and boot-time Flash loading are disabled.

MC hardware: DHT11 DATA=P3_1; light AO=P1_6 (ADC0 A22); DO unused.
MC keeps sampling automatically. Only complete valid AO samples enter the
cache. CAN sends the most recent sample while its age is <=5 seconds; sensor
failure before the first sample produces `CAN_PEER,WAIT,no-fresh-environment-sample`.
Stop sampling with `env_stop`; samples stop being transmitted once expired.

## CAN packet

CAN FD standard frames, BRS, arbitration 500 kbit/s, data 2 Mbit/s.
MC request ID=0x101; ST status ID=0x102, both DLC=8.

| Byte | Value |
| --- | --- |
| 0 | 0xA1 magic |
| 1 | Request sequence |
| 2..3 | Temperature *10, uint16 little-endian |
| 4..5 | Humidity *10, uint16 little-endian |
| 6..7 | ADC raw, uint16 little-endian |

Example: 26.0 C, 55.0%RH, raw 31000, sequence 7:
`A1 07 04 01 26 02 18 79`.
ST rejects wrong frame type, identifier, length, magic or out-of-range
temperature/humidity. Do not use arbitrary bytes in `can_peer_send` for normal
operation; its six payload bytes must obey the sensor encoding above.

## ST status response and MC LED

ST replies with `[A2, sequence, ST_LED, ST_exceeded_count, request_CRC32 LE32]`.
ST_LED is 1 when at least two values exceed ST's thresholds, otherwise 0.
MC verifies frame type, ID, length, FD/BRS, sequence, request CRC32 and the
consistency of ST_LED/count before accepting the status. An old echo is invalid.

MC independently compares the same transmitted sample against 25 C, 60%RH,
raw 30000. MC P3_18 red LED is active LOW and turns ON only when ST_LED=1
AND MC also has at least two exceeded values. Both OFF keeps MC LED OFF.
Two exceeded values and three exceeded values both mean alarm; counts need
not be identical. ST UART SET changes ST thresholds only, not MC thresholds.
Missing fresh samples, transmit failure and status timeout turn MC LED OFF.

Both boards must be rebuilt and flashed for this protocol change.
This edit has not been compiled or tested on hardware.

## LED rule

Defaults: temperature 25 C, humidity 60%RH, raw 30000.
Each comparison is strictly greater than its threshold. Two or three true
comparisons turn the PA5 status LED on; zero or one turns it off. Equality does
not count. Example: (26,61,20000) -> ON; (26,50,30000) -> OFF.
No initial valid sample -> OFF. After five seconds without a valid CAN sample,
ST turns LED off and prints a warning. Updating thresholds immediately
re-evaluates the latest still-valid sample. The main loop no longer forces LED
on after app_process; initialization failures retain the original error blink.

## USART2 console

PA2 TX / PA3 RX, 115200 baud, 8 data bits, no parity, 1 stop bit, no flow control.
USE_TRACE=1 enables printf output. UART uses an interrupt-driven RX ring; command
parsing and printf run in the main loop. Commands are ASCII, case sensitive,
and terminated with CR, LF or CRLF:

```text
GET
SET 25 50 30000
SET 28 60 35000
HELP
```

`SET` expects three whole numbers: temperature 0..50 C, humidity 0..100%RH,
raw 0..65535. Invalid commands leave all thresholds unchanged. Thresholds are
RAM-only and reset to defaults on reboot. Overlong lines, buffer overflow and
UART receive errors reject affected input rather than applying truncated commands.

Example received log:
`ENV,seq=7,T=26.0C,H=55.0%,raw=31000,over=2,LED=ON`.

For NUCLEO-C542RC / MB2213, USART2 PA2/PA3 reaches STLINK-V3EC VCP with
SB12/SB26 ON and SB46 OFF. Use the ST-LINK USB connector and its COM port in a
serial terminal. Check actual board solder bridges if the VCP is silent.
Source: [ST UM3615, section 7.12](https://www.st.com/resource/en/user_manual/um3615-stm32c5-nucleo64-board-mb2213-stmicroelectronics.pdf).

## Build and verification

MC target: Keil rtthread-frdm-mcxa346, output build/rtthread.axf.
ST sources: STM32C5/STM32C5_FDCAN_cmake. CMake debug_GCC_NUCLEO-C542RC
build outputs for this change are in the ST project build/debug_GCC_NUCLEO-C542RC.
ST toolchain installed here is Arm GNU 15.2.1 despite the existing CMake
toolchain filename GCC.14.3.1.cmake.

Protocol host tests: the MC project tests/test_sensor_protocol.c tests all eight threshold
combinations, equality, little-endian fields, sensor ranges and invalid SET
commands with atomic updates. Both firmwares require flashing for the new
sensor format; no board flashing or serial loopback test was performed here.
