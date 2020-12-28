# Arduino High Voltage Programmer / Eraser for ATtiny

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Hit Counter](https://hitcounter.pythonanywhere.com/count/tag.svg?url=https%3A%2F%2Fgithub.com%2FArminJo%2FATtiny-HighVoltageProgrammer_FuseEraser)](https://github.com/brentvollebregt/hit-counter)

Based on [Recover Bricked ATtiny Using Arduino as high voltage programmer](http://www.electronics-lab.com/recover-bricked-attiny-using-arduino-as-high-voltage-programmer/).<br/>
This program restores the default fuse settings of the ATtiny and can perform a chip erase & lock bits reset.<br/>
After resetting, the fuses can then easily be changed with the programmer you use for uploading your program i.e. avrdude.<br/>
To burn micronucleus bootloader on an empty chip use the `Burn_avrdude-*` scripts available [here](https://github.com/ArminJo/micronucleus-firmware/tree/master/utils).

### Modified for easy use with Nano board on a breadboard
- Added option to press button instead of sending character to start programming
- Improved serial output information
- After programming the internal LED blinks
- Added timeout for reading data
- Allow Serial command input
- Added readout of lockbits
- Added Chip Erase 

# Installation
Just copy the **[content](https://raw.githubusercontent.com/ArminJo/ATtiny-HighVoltageProgrammer_FuseEraser/master/src/HVProgrammer.cpp)** of the HVProgrammer.ino file into a **new Arduino sketch** and run it.

In the **Arduino serial monitor** with baudrate **115200** you will see something like:
```
START C:\...\ArduinoSketches\sketch_jun15a\sketch_jun15a.ino
Version 3.2 from Jun 15 2019

Enter 'r' to only read fuses and lock bits...
Enter 'e' to erase flash and lock bits...
Enter 'w' or any other character or press button at pin 6 to to write fuses to default...
```

and **after button press or sending a character** you see for an **ATtiny85**:
```
Reading signature from connected ATtiny...
Signature is: 930B
Reading signature complete..

The ATtiny is detected as ATtiny85.

Reading fuse settings from ATtiny...
  LFuse: 62, HFuse: DF, EFuse: FF
Reading fuse values complete.

Reading lock bits...
  Lock: FF
    LB1 Not Programmed
    LB2 Not Programmed
No memory lock features enabled.
Reading Lock Bits complete.

Write LFUSE: 0x62
Writing fuse value 62 to ATtiny...
Writing complete.

Write HFUSE: 0xDF
Writing fuse value DF to ATtiny...
Writing complete.

Write EFUSE: 0xFF
Writing fuse value FF to ATtiny...
Writing complete.

Fuses will be read again to check values...
Reading fuse settings from ATtiny...
  LFuse: 62, HFuse: DF, EFuse: FF
Reading fuse values complete.

Reading / programming finished, allow a new run.

```
 or if **no ATtiny** is attached:
```
Reading signature from connected ATtiny...
Signature is: 0
Reading signature complete..

No valid ATtiny signature detected!
Try again.
```

# Breadboard
This circuit uses an Arduino nano and an [ebay DC-DC Step-Up Modul](https://www.ebay.de/itm/2PCS-2A-Booster-Board-DC-DC-Step-Up-Modul-2-24V-5-9-12-28V-Replace-XL6009-BAF-/263413727169?hash=item3d54ae7fc1)

Breadboard top view
![Breadboard_Top](https://github.com/ArminJo/ATtiny-HighVoltageProgrammer_FuseEraser/blob/HEAD/pictures/HVProgrammer_Top.jpg)
Breadboard back view
![Breadboard_Back](https://github.com/ArminJo/ATtiny-HighVoltageProgrammer_FuseEraser/blob/HEAD/pictures/HVProgrammer_Back.jpg)

# Revision History
### Version 3.2
- Added lock bit readout
### Version 3.0
- Added erase and read only commands
### Version 2.2
- First public version
