# Build custom firmware for the TI CC2650 SensorTag

Unfortunately the information to build an OAD image for the cc2650 SensorTag
is fragmented. These are useful links:

https://e2e.ti.com/support/wireless_connectivity/bluetooth_low_energy/f/538/t/479739

https://e2e.ti.com/support/wireless_connectivity/bluetooth_low_energy/f/538/t/448400

http://processors.wiki.ti.com/index.php/CC2650_SensorTag_User%27s_Guide#Building_SensorTag_SuperHex_File

As mentioned in the above links, for the SensorTag it's enough to merge the BIM,
 application and stack hex files and convert the super hex to bin.

## Setup

* IAR Emebeeded Workbench for ARM 7.60 (30 days temporary license) in a Windows 7 VM
* BLE SDK 2.1.1

With IAR open the SensorTag project:

```
C:\ti\simplelink\ble_cc26xx_2_01_01_44627\Projects\ble\SensorTag\CC26xx\IAR\SensorTag.eww
```

You need to build the three subprojects:

1. BIM_extflash. Make sure you select the FlashOnly_SensorTag configuration
2. CC2650App. Make sure you select the FlashOnlyOAD configuration
3. CC2650Stack. The only configuration here is FlashROM

Locate the three hex files. They can be found in the Output folder of each subproject.

Then I've copied the three hex files in Linux and created a super hex and a bin
file as (partially) described in the third link above.

Install **pip** and **intelhex**:

```
$ sudo apt-get install python-pip
$ sudo pip install intelhex
```

locate the intelhex scripts:

```
$ which hexmerge.py
```

cd to the directory where you copied the hex files.

Run the following command to create the super hex and the bin files:

```
$ python /usr/local/bin/hexmerge.py -o SensorTagSuper.hex -r 0000:1FFFF SensorTagAppFlashOnlyOAD.hex:0000:1EFFF SensorTagStackFlashROM.hex BIM_ext.hex:1F000:1FFFF --overlap=error
$ python /usr/local/bin/hex2bin.py SensorTagSuper.hex SensorTagSuper.bin
```

Update the sensor tag firmware using the Android app.
The above bin file must be copied to the Andorid Dowloads directory.

## Buzzer update

* Download buzzer.c and buzzer.h provided by TI http://e2e.ti.com/support/wireless_connectivity/bluetooth_low_energy/f/538/p/475610/1713829
* Add buzzer.c and buzzer.h to the CC2650App IAR subproject
* Modify SensorTag_IO.c as in this repository
