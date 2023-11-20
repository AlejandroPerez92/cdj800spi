# Pioneer cdj 800 mk2 SPI protocol playground

This project is only for investigation purposes, it's based on this [project](https://github.com/DjFix/spi-spy).

The project is not compatible with the CDJ800 MK1.

## Introduction
The Pioneer CDJ800 mk2 has a microcontroller to drive the inputs and outputs in the surface controller (buttons, display, leds).
This microcontroller uses a standard SPI protocol to communicate with the main controller. In this communication the display microcontroller
takes the role of **MASTER** and the main controller works as a **SLAVE**

## Project
This project is focused to work on an Arduino Uno board, but can be easily ported to another boards, only be mindful of the voltages.
If your board uses 3.3v you will need a logic converter to work with 5v.

For build it you need PlatformIO and VSCode.

### Current Status
Currently, with this project you will be able to see the output messages from the microcontroller in the Serial output.

I'm having problems with the stability of the message because the message not have any CRC or mechanism to prevent malformed messages.

## Wiring
Make sure the main controller is disconnected before connecting the Arduino board, this is easy to do by unplugging the connector CN301 in the main board. 

Once the main board is disconnected you can solder 5 wires to the CN501 connector on the DFLB board.
![](https://github.com/AlejandroPerez92/cdj800spi/blob/main/images/wiring.png)

| CN501 | Arduino UNO | Name |
|-------|-------------|------|
| 1     | 13          | SCK  |
| 3     | 12          | MISO |
| 5     | 10          | SS   |
| 7     | 11          | MOSI |
| 14    | GND         | GND  |

## Outgoing message structure

The message is a 19bytes long similar to the CDJ1000mk3.

| Byte  | 0 | 1  | 2 | 3     | 4     | 5     | 6   | 7   | 8   | 9   | 10  | 11  | 12 | 13  | 14  | 15  | 16  | 17  | 18  |
|-------|---|----|---|-------|-------|-------|-----|-----|-----|-----|-----|-----|----|-----|-----|-----|-----|-----|-----|
| Value | 1 | 16 | 0 | TOUCH | PITCH | PITCH | 128 | 192 | JOG | JOG | JOG | JOG | 0  | BTN | BTN | BTN | BTN | BTN | BTN |