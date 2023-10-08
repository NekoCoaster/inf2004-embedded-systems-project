# Raspberry Pi Pico PWM Fan Control

Here in this folder, contains all the nessasary documentation on how we plan to wire a Raspberry Pi Pico to control a 5V, 4-pin PWM fan. 

## Hardware Requirements

- Raspberry Pi Pico
- Noctua NF-A4x10 5V, 4-pin PWM Fan (40mm x 10mm)
- Jumper Wires
- Breadboard (optional)
- Power Supply (5V) for both the Fan and Raspberry Pi Pico

## 4-pin PWM Fan Header Pinout and wiring proposal.

Based on the [**PWM specification paper**](/drivers/5v-pwm-fan/Noctua_PWM_specifications_white_paper.pdf)
 published by Noctua, the 4-pin PWM fan header pinout is as follows: 
<br/>
![Alt text](/drivers/5v-pwm-fan/img/4-pin-header.png)

## Factors to consider when driving the PWM signal:

 - 🟢 The PWM signal should be a 25kHz square wave with a duty cycle between 20% and 100%.
 - 🟡 For the PWM signal to be considered as a logic low, the voltage should be below 0.8V. Which means that the Raspberry Pi Pico's 3.3V output should be sufficient enough to drive the PWM signal. A user forum post here also confirms that they are able to drive a 5V PWM fan directly, using a Raspberry Pi Pico's GPIO pin: https://forums.raspberrypi.com/viewtopic.php?t=310374
 - 🔴 Since the Raspberry Pi Pico's GPIO pins are capable of supplying up to 16mA of current, we will need to use at least a 660 ohm resistor to bring down the GPIO's current from 16mA to 5mA as specified. Even safer would be to use a 1k ohm resistor to bring down the current to 3.3mA. So as to ensure that the fan's PWM input is not damaged.

![Alt text](/drivers/5v-pwm-fan/img/pdf-pwm-specs.png)

## Wiring Diagram
For our use case, we will be assuming that the whole system will be running off of a 5V, USB-A breakout. This will allow us to easily power both the Raspberry Pi Pico and the fan from a single power source. </br>
⚠️**Important Note:** The Raspberry Pi Pico and the Fan's GND pins needs to be connected together. This is to ensure that the PWM signal is referenced to the same ground. </br>
</br>
![Wiring Diagram](/drivers/5v-pwm-fan/img/conn-diagram.jpg)
### Connecting PWM signal pin
**GPIO Pin:** GP2 </br>
**GPIO Pin Mode:** PWM @ 25KHz, Duty Cycle Range: 20% ~ 100% </br>
**PWM Signal Pin:** Blue wire </br>
For our PWM signal pin, we will be using GP2, which is the 4th pin from the top left of the Raspberry Pi Pico. </br>
While the GPIO pins on the Raspberry Pi Pico are capable of supplying up to 16mA of current, the Fan's PWM input signal pin is only rated to handle up to 5mA. And as such, we will be using a 1k ohm resistor to bring down the current to 3.3mA which is well within the fan's PWM input pin's current rating.</br>
In addition to the 1k ohm resistor, we will also be using a 1N4148 diode to protect the GPIO pin from any voltage spikes that may occur when the fan is turned on. </br>

### Connecting the Tachometer signal pin
**GPIO Pin:** GP15 </br>
**GPIO Pin Mode:** INPUT_PULLUP </br>
**Tachometer Signal Pin:** Green wire </br>
For our Tachometer signal pin, we will be using GP15, which is the last pin at the edge of the bottom right. </br>
The Tachometer signal pin is an open-collector output, which means that it will be pulled to ground twice per revolution of the fan. As such, we will need to enable the internal pull-up resistor on the Raspberry Pi Pico's GPIO pin. </br>
However, a resistor is not required as the Tachometer signal pin is rated to handle up to 5mA of current, whereas the internal pull-up resistor has an impedance of 50k ohms, this would mean that the current flowing through the Tachometer signal pin would be 3.3V / 50k ohms = 0.066mA, which is well within the Tachometer signal pin's current rating. </br>
⚠️ **HOWEVER**, Noctua recommends that the voltage on the Tachometer signal should be around 5V, there may be some issues with using 3.3V as the signal voltage on the Tachometer signal pin. There should be no damages done to both the Raspberry Pi Pico and the fan, but the Tachometer signal may not be as accurate as it should be at best, or not work at all at worst. </br>
More investigation and testing will be required to determine if the Tachometer signal pin will work with 3.3V as the signal voltage. </br>
As for the diode, while it is not required as the Tachometer signal pin is an open-collector output, for insurance, we will be using a 1N4148 diode to protect the GPIO pin from feedback voltage from the fan.</br>
