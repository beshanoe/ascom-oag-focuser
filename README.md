# ASCOM-Compatible OAG Focuser

<!-- toc -->

- [Introduction](#introduction)
  - [Is This For You?](#is-this-for-you)
  - [Use As A Standalone Focuser](#use-as-a-standalone-focuser)
- [Demo](#demo)
- [Finished Product](#finished-product)
- [Pre-Requisites](#pre-requisites)
- [Hardware](#hardware)
- [ASCOM Driver](#ascom-driver)
  - [Downloading And Installing The Driver](#downloading-and-installing-the-driver)
  - [Compiling The Driver (For Developers Only)](#compiling-the-driver-for-developers-only)
  - [Screenshots](#screenshots)
- [Standalone Focuser Application](#standalone-focuser-application)
- [Arduino Firmware](#arduino-firmware)
  - [Microcontroller Compatibility](#microcontroller-compatibility)
  - [Compiling And Uploading The Firmware](#compiling-and-uploading-the-firmware)
- [Electronic Circuit](#electronic-circuit)
- [Mechanical Components](#mechanical-components)
  - [Gear Drive vs Belt Drive](#gear-drive-vs-belt-drive)
  - [Backlash Measurement And Compensation](#backlash-measurement-and-compensation)
- [Calibration Procedure](#calibration-procedure)
- [Frequently Asked Questions](#frequently-asked-questions)
- [Ideas For Future Improvements](#ideas-for-future-improvements)
- [Credits](#credits)

<!-- tocstop -->

## Introduction

Shortly after I got started in astrophotography, I noticed that the stars in my images were slightly elongated due to some differential flexure somewhere in my imaging system (see [Cloudy Nights thread](https://www.cloudynights.com/topic/775260-good-guiding-but-elongated-stars-along-e-w-direction/) — I strongly suspect that it is sag in my focuser...) The permanent solution was to switch from using a guide scope to using an off-axis guider (OAG) and that immediately resolved my issue. Now, my stars are perfectly round, which is great! For reference, I purchased the [ZWO OAG](https://astronomy-imaging-camera.com/product/zwo-oag) and the accompanying [ZWO 1.25" helical focuser](https://astronomy-imaging-camera.com/product/zwo-1-25%E2%80%B3-helical-focuser).

As good as this sounds, the OAG brought a number of new challenges which I did not have to deal with before. First, when using an OAG, the field of view is very small, which sometimes makes it difficult for the guiding software to detect guide stars. Second, the OAG requires some amount of refocusing when switching filters. Indeed, filters are not always parfocal. Most importantly, a refractor (even an apochromatic triplet...) will focus various wavelengths at different distances, and this is the primary reason why it is required to change the focus of the main imaging camera (using filter offsets) when changing filters. Unfortunately, while the imaging sensor is located _behind_ the filter wheel, the OAG is always placed _before_ the filter wheel (Otherwise, especially when using narrowband filters, you would not be able to find a single guide star... Also, placing the filter wheel further away from the imaging sensor would cause vignetting...) and this is why, under specific conditions (see "Is this For You" paragraph below), the guide camera might show out-of-focus images while the imaging camera is in focus, and even though the guide camera was independently focused while another filter was selected in the filter wheel.

While guiding software like PHD2 can accommodate slightly out-of-focus guide stars by computing their centroid, out-of-focus guide images can contain a drastically lower number of detected guide stars, and those that are detected will have a much lower Signal to Noise Ratio (SNR), further reducing the accuracy of the centroid computation. In some situations, PHD2 may not even be able to detect a single guide star, and guiding will be impossible. This has happened to me in the past, and I was forced to _manually_ refocus the guide camera (independently of the main imaging camera of course, using the OAG helical focuser)

Manually refocusing the guide camera throughout the night is not exactly one of the most enjoyable parts of the hobby, so I looked for ways to automate that. Pegasus Astro sells a motorized OAG named [SCOPS OAG](https://pegasusastro.com/products/scops-oag/). However, it is a little expensive for me ($750 US — although it is a really great looking unit!) Furthermore, I discussed with the [N.I.N.A.](https://nighttime-imaging.eu/) developers on their [Discord server](https://discord.gg/fwpmHU4). Since N.I.N.A. can only connect to a single focuser at a time, there is no good solution yet to deal with SCOPS OAG. It is certainly possible to run a second instance of N.I.N.A. that connects to both SCOPS OAG and the guide camera, but there is no way to have those two N.I.N.A. instances communicate to do something upon filter change.

All of this has led me to design and build my own solution to this problem. In this repository, you will find everything you need to motorize and automatically control the ZWO OAG (list of parts, 3D model, electronic schematics, Arduino firmware, ASCOM driver, standalone ASCOM client application, instructions, etc.) I hope you consider building this project if you find yourself in a similar situation. Please note that the ASCOM driver can be used with SCOPS OAG, there is nothing in the Filter Wheel Proxy implementation that is specific to my device.

### Is This For You?

Not necessarily! It really depends on a lot of factors. For example, if you are using a mirrored telescope, i.e. not a refractor, and if all of your filters come from the same set, e.g., Antlia LRGB filter set, and the filter manufacturer claims they are parfocal, then you may not need this at all because the difference in focus between filters will be so minuscule, you probably won't need to worry about it. However, if you are using a refractor (even an APO triplet), or if you use filters that are not parfocal, you _may_ need this. The only way to find out is to focus your main imaging camera, and then, assuming that you've already defined your filter offsets, go through every single filter in your filter wheel. After every filter change, your imaging application will adjust the focus of your imaging train slightly depending on the filter offset you have defined for the selected filter, and you should look at the image coming out of your guide camera in PHD2. In my case, the stars in my guide camera can be pinpoint when I am using my Luminance filter, but they are large disks when I am on some other filter. That is how you know that this device can be useful for you. This definitely is not for everybody, but the people who have the problem I just described did not really have a good solution until now.

### Use As A Standalone Focuser

Please note that you can use this device, and the accompanying software, to motorize any helical focuser, regardless of how you use that focuser. For example, you could use it to control the focus of your entire imaging train! Just remember that if you have a helical focuser that is not from ZWO, you may need to make a few adjustments to the 3D model...

## Demo

The following video shows what the guide camera sees with and without the OAG focuser upon switching to a different filter:

[![YouTube video showing what the guide camera sees with and without an OAG focuser](images/Demo-Video-Thumbnail-1.jpg)](https://youtu.be/gUapuzT75Z0)

## Finished Product

Here is what the finished product looks like:

![Finished Product](images/Finished-Product-1.jpg)

![Finished Product](images/Finished-Product-2.jpg)

![Finished Product](images/Finished-Product-3.jpg)

And here is a demo of the system when attached to my telescope:

[![YouTube video showing the focuser in action](images/Demo-Video-Thumbnail-2.jpg)](https://youtu.be/FXXxpH1uZQA)

## Pre-Requisites

* A Windows computer (Windows 10 or newer)
* [Microsoft Visual Studio](https://visualstudio.microsoft.com/) (FYI, I used the 2022 edition...)
* [ASCOM Platform](https://ascom-standards.org/)
* [ASCOM Platform Developer Components](https://ascom-standards.org/COMDeveloper/Index.htm)
* [Arduino IDE](https://www.arduino.cc/en/software)
* [FreeCAD](https://www.freecadweb.org/), a free and open-source 3D parametric modeler
* A 3D printer able to print PETG, and a slicer (I use a heavily upgraded Creality Ender 3 v2, and Ultimaker Cura)
* A few basic tools that any tinkerer must own, such as a breadboard, a soldering iron, etc.

## Hardware

The following are just suggestions... Also, over time, some of the links may no longer work...

* [ZWO OAG](https://astronomy-imaging-camera.com/product/zwo-oag)
* [ZWO 1.25" helical focuser](https://astronomy-imaging-camera.com/product/zwo-1-25%E2%80%B3-helical-focuser)
* Arduino-compatible microcontroller board with built-in EEPROM support. I used an Arduino Nano clone with a USB-C connector (~$10 on Amazon)
* ULN2003 Darlington transistor array to control the stepper motor using the Arduino's digital I/O pins.
* 24BYJ-48 stepper motor — That is the 12V version of the popular 28BYJ48, but you can also use the standard 5V model instead, depending on how you power your imaging rig (12V is pretty standard in astrophotography, so I went with that)
* LEDs and resistors — These are not required, but they can be useful to debug the firmware while prototyping.
* Connectors — I used JST connectors, only because I already had a bunch of them, along with a crimping tool.

## ASCOM Driver

### Downloading And Installing The Driver

Starting with version `1.0.5`, you can install the ASCOM driver, as well as the standalone focuser control application, by running the executable setup file that you will find in the [releases page](https://github.com/jlecomte/ascom-oag-focuser/releases). By default, it places files under `C:\Program Files (x86)\Dark Sky Geek\OAG Focuser ASCOM Driver`.

### Compiling The Driver (For Developers Only)

Open Microsoft Visual Studio as an administrator (right-click on the Microsoft Visual Studio shortcut, and select "Run as administrator"). This is required because when building the code, by default, Microsoft Visual Studio will register the compiled COM components, and this operation requires special privileges (Note: This is something you can disable in the project settings...) Then, open the solution (`ASCOM_Driver\ASCOM.DarkSkyGeek.OAGFocuser.sln`), change the solution configuration to `Release` (in the toolbar), open the `Build` menu, and click on `Build Solution`. As long as you have properly installed all the required dependencies, the build should succeed and the ASCOM driver will be registered on your system. The binary file generated will be `ASCOM_Driver\bin\Release\ASCOM.DarkSkyGeek.OAGFocuser.dll`. You may also download this file from the [Releases page](https://github.com/jlecomte/ascom-oag-focuser/releases).

### Screenshots

The ASCOM driver registers two new components: `ASCOM.DarkSkyGeek.FilterWheelProxy`, which implements the `IFilterWheelV2` ASCOM interface, and `ASCOM.DarkSkyGeek.OAGFocuser`, which implements the `IFocuserV3` ASCOM interface. Both components have their own settings dialog. Here is what the filter wheel settings dialog looks like:

![Filter Wheel Settings Dialog](images/FilterWheelProxy-SetupDialog.png)

And here is what the focuser settings dialog looks like:

![Focuser Settings Dialog](images/Focuser-SetupDialog.png)

## Standalone Focuser Application

Open the `Focuser_App\ASCOM.DarkSkyGeek.FocuserApp.sln` solution in Microsoft Visual Studio, change the solution configuration to `Release` (in the toolbar), open the `Build` menu, and click on `Build Solution`. Very simple! The binary file generated will be `Focuser_App\bin\Release\ASCOM.DarkSkyGeek.FocuserApp.exe`. You may also download this file from the [Releases page](https://github.com/jlecomte/ascom-oag-focuser/releases). Here is what that little standalone application looks like:

![Standalone Focuser Control Application](images/Standalone-Focuser-App.png)

This application allows you to connect to and control DarkSkyGeek’s OAG focuser, and in particular, it enables you to test various backlash compensation values as well as set the zero position. If you use a SCOPS OAG, I can only assume that it came with its own standalone application with similar functionality...

## Arduino Firmware

### Microcontroller Compatibility

All Arduino-compatible microcontrollers that have a **built-in EEPROM** should work. Unfortunately, this excludes the popular [Seeeduino XIAO](https://www.seeedstudio.com/Seeeduino-XIAO-Arduino-Microcontroller-SAMD21-Cortex-M0+-p-4426.html) (one of my favorite microcontroller boards for hobby projects...) If you insist on using a unit that does not have a built-in EEPROM, you will have to customize the firmware to fit your needs. You could technically use a separate EEPROM chip, or you could use the [`FlashStorage` library](https://github.com/cmaglie/FlashStorage) or the [`FlashStorage_SAMD` library](https://github.com/khoih-prog/FlashStorage_SAMD), or you could simply disable the EEPROM code (in which case the device will not remember its last position after a power cycle...) There are quite a few options to choose from, but I recommend using something like an Arduino Nano for example, which is small, affordable, and has everything you need.

### Compiling And Uploading The Firmware

* If needed, add support for the board that you are using in your project.
* You may want to customize the name of the device when connected to your computer. To do that, you will have to update the appropriate `usb_product` key in the appropriate `boards.txt` file... I cannot give you specific instructions for that because they depend on the exact board you are using.
* Finally, connect your board to your computer using a USB cable, open the sketch file located at `Arduino_Firmware\Arduino_Firmware.ino`, and click on the `Upload` button in the toolbar.

## Electronic Circuit

The electronics circuit is fairly straightforward. I included a Fritzing file in the `Electronics/` folder. Here are the schematics:

![Breadboard Schematics](images/Breadboard-Schematics.png)

Here is what the prototype circuit looks like:

![Breadboard Prototype](images/Breadboard-Prototype.jpg)

Here are the top and bottom of the final circuit board:

![Final Circuit Board Top](images/Circuit-Board-Top.jpg)

![Final Circuit Board Bottom](images/Circuit-Board-Bottom.jpg)

And here is the circuit board inside the 3D printed enclosure:

![Circuit Board Inside Enclosure](images/Circuit-Board-Inside-Enclosure.jpg)

Please note that it is advisable to place a small piece of opaque black duct tape on top of the LEDs located on the Arduino board because they can be quite bright (the photograph above was taken before I implemented this small improvement).

## Mechanical Components

### Gear Drive vs Belt Drive

My first attempt to move the focuser with a stepper motor was done using a belt:

![Focuser Belt](images/Focuser-Belt.jpg)

Unfortunately, the belt occasionally slipped, so I gave helical gears a try:

![Focuser Gear](images/Focuser-Gear.jpg)

It turned out that helical gears, which are very easy to make on a 3D printer, worked absolutely flawlessly! Another thing that also helped get great results was to slightly loosen the focuser (there are 4 tiny set screws on the ZWO focuser you can loosen ever so slightly to make it easier to rotate the knurled knob) because these small stepper motors don't have that much torque...

I included both the belt and gear models in the `3D_Files/` directory so you can give them both a try and decide which one you want to use.

The "reverse rotation" checkbox in the focuser driver setup dialog window allows you to specify the direction of rotation when the number of steps increases. Check that option (default) if you chose a gear drive, and uncheck it if you chose a belt drive.

### Backlash Measurement And Compensation

There are many sources of backlash in this system. The stepper motor itself, due to its internal gearbox, already has some amount of backlash. The 3D printed gear and pinion also have some backlash. And finally, the helical focuser has some backlash as well. All of those sources combine. Thankfully, compensating for backlash is easy and supported by the software in this repository. The trick is to first measure the amount of backlash in your system. Here is how I do it using a dial indicator:

![Backlash Measurement](images/Backlash-Measurement.jpg)

Using the standalone focuser control application, setting a backlash compensation of 0, move in one direction by a large amount. Then, repetitively move in small increments in the opposite direction until the dial indicator starts moving. In my setup, I have a total of about 60 steps of backlash, so I set the backlash compensation amount to 100 (the software uses the so-called "overshoot" backlash compensation method) and it works absolutely flawlessly!

## Calibration Procedure

Before you can use this device, you have to calibrate it. Here is the procedure:

1. **Filter Offsets Measurement**

   Measuring filter offsets is something you have probably already done because it allows you to run an autofocus routine using your luminance (L) filter, which can be done with very short exposures, thereby saving a lot of time (it is also more accurate). Enter the filter offsets in your imaging application, e.g., N.I.N.A., as well as in the settings dialog of the `DarkSkyGeek’s Filter Wheel Proxy For OAG Focuser` ASCOM device (see screenshot above). Remember that it is critical that your imaging application and the driver have the same filter offset values!

   **Note:** If you measure your filter offsets with a reducer, they will be different. Don't worry about that. Do the entire calibration procedure with your standard astrophotography setup. Once properly set up, the OAG focuser configuration will not need to change whether or not you add a reducer to your imaging train.

2. **Backlash Measurement**

   See "Backlash Measurement" section. Enter the value of the backlash, in number of steps, in the settings dialog of the `DarkSkyGeek’s Filter Wheel Proxy For OAG Focuser` ASCOM device (see screenshot above)

3. **Zero Position**

   Manually rotate the OAG focuser until it hits its mechanical zero position. Connect the OAG focuser to a computer (assuming you had previously installed the OAG focuser ASCOM driver on that computer), open the standalone focuser app, connect to the device, and click on the "Set Zero position!" button.

4. **Steps Ratio Measurement**

   The "steps ratio" allows the Filter Wheel Proxy ASCOM driver to answer the following question: Upon filter change, the main focuser has to move n steps in this direction. How many steps does the OAG focuser need to move, and in which direction?

   The best way to measure the steps ratio is to use 2 instances of N.I.N.A. The first instance is connected to the telescope focuser and the main imaging camera. The second instance is connected to the OAG focuser and the guide camera. Then, follow these steps:

   * Run an autofocus routine in both N.I.N.A. instances (start with the one that controls the telescope focuser and the main imaging camera)
   * Take note of the position of the OAG focuser.
   * Move the telescope focuser by, for example, 300 steps outward.
   * Run another autofocus routine on the second N.I.N.A. instance (the one that controls the OAG focuser and the guide camera)
   * Subtract the new position of the OAG focuser with the initial position, and divide that number by the number of steps you moved the main telescope focuser (300 in our example). That will give you the steps ratio. Note that this will be a negative number. In my case, the steps ratio is -2.64.
   * You can run the autofocus routine several times, and average the positions obtained to increase accuracy.
   * Enter the steps ratio in the settings dialog of the `DarkSkyGeek’s Filter Wheel Proxy For OAG Focuser` ASCOM device (see screenshot above)

5. **Test**

   Once you have calibrated the OAG Focuser, you can close the second instance of N.I.N.A. and open PHD2. Use the first instance of N.I.N.A. as you normally would, although instead of connecting your filter wheel directly, you will connect to the `DarkSkyGeek’s Filter Wheel Proxy For OAG Focuser` device. Looking at the PHD2 live view, change the filter in the filter wheel and watch the PHD2 live view becoming blurry and then sharp again, automatically! Isn't technology beautiful?!

## Frequently Asked Questions

**I built this project and it does not work, can you help?**

_Maybe. As indicated in the `LICENSE` file, I do not provide any official guarantee or support. That being said, if you open a GitHub issue in this repository and ask nicely, I will likely respond. Just make sure that you provide all the necessary details so that I understand what the issue might be. While on that note, keep in mind that troubleshooting an issue on your own is by far the best way to learn new things._

**Gear drive or belt drive? Which one do you recommend?**

_Because of the possibility of belt slippage, I recommend the gear drive. The enclosure is designed for that. A belt driven system would require minor tweaks to the enclosure, which are not hard to do in Freecad if you know a little about that software._

**Why did you not use the `Stepper` or `AccelStepper` library in the Arduino firmware?**

_It might seem strange that I decided to "manually" control the stepper motor instead of using the standard [`Stepper` library](https://www.arduino.cc/reference/en/libraries/stepper/) or the popular [`AccelStepper` library](https://www.arduino.cc/reference/en/libraries/accelstepper/). There are two reasons for that:_

1. _I need to be able to handle incoming requests while the motor is moving, e.g., `COMMAND:FOCUSER:ISMOVING` or `COMMAND:FOCUSER:HALT`. This is not possible with any of the aforementioned libraries._
2. _To save power, to prevent heat buildup, and to eliminate vibrations, I de-energize the stepper motor by setting all the pins to LOW once it has reached the desired position. This is also not supported by any of the aforementioned libraries, and it makes a huge difference! If you don't believe me, try commenting out that part of the code, and play with the firmware for a little while (you don't even need to actively move the motor). Then, feel how hot the motor gets... Also, feel how much the motor vibrates while energized, and consider the impact that might have on your images..._

**Why is backlash compensation not implemented in the focuser driver?**

_The software included in this repository (specifically the `FilterWheelProxy` ASCOM component) was designed and implemented so that it may be used with other OAG focusers, including commercial units such as the SCOPS OAG, and I have no idea whether their driver handles backlash compensation. This way, no matter which focuser you use to adjust the focus of your guide camera, as long as its driver implements the standard `IFocuserV3` ASCOM interface, this will work and you will enjoy the benefits of backlash compensation!_

**What is the positional accuracy of this focuser**

Using a precision dial indicator (see "Backlash Measurement" section), I was able to measure the positional accuracy of this device (move 1,000 steps in one direction, move 1,000 steps in the opposite direction, and measure the difference between the starting and ending positions). Astoundingly, it is of the order of about 1μm!

## Ideas For Future Improvements

* Remove the need for a separate 12V power connector, i.e. use the USB cable for both data and power. This change would prevent us from using an Arduino-compatible board because the maximum current that can be delivered by an Arduino-compatible board is usually around 200mA @ 5V (although I have seen some that boast up to 500mA @ 5V), which is not quite enough, even for our small stepper motor, and you'd run the risk of tripping the internal fuse on the microcontroller board. In order to do this, you'd have to basically build your own Arduino board using a microcontroller chipd, an FTDI module, and a few other components. It's quite a project on its own... Another option is to use an onboard battery, and use a buck or boost converter to get the right voltage for your specific motor. This adds weight and complexity to the system, and if you forget to replace or recharge the battery, you find yourself with a dead unit... This is why I applied the [KISS principle](https://en.wikipedia.org/wiki/KISS_principle) to this project, even if the final product is not quite as optimal as it could possibly be, but hey, this is a hobbyist project!

## Credits

I would like to thank _Christophe de la Chapelle_ of the popular French-language YouTube channel [La Chaîne Astro](https://www.youtube.com/c/cdlc48) because he gave me the idea to build a focuser for my OAG after he demonstrated how he had built a simple focuser using 3D printed parts and a stepper motor for his telescope.

I would like to also thank _Stefan Berg_, creator of [N.I.N.A.](https://nighttime-imaging.eu/), and _Linwood Ferguson_ (see [his web site](https://www.captivephotons.com/Photography/Astrophotography/)) for giving me the idea to create a "proxy" filter wheel component while discussing the design of this system on the [N.I.N.A. Discord server](https://discord.gg/fwpmHU4).

## Frequently Asked Questions (FAQ)

**Question:** My antivirus identifies your setup executable file as a malware (some kind of Trojan)

**Answer:** This is a false detection, extremely common with installers created with [Inno Setup](https://jrsoftware.org/isinfo.php) because virus and malware authors also use Inno Setup to distribute their malicious payload... Anyway, there isn't much I can do about this, short of signing the executable. Unfortunately, that would require a code signing certificate, which costs money. So, even though the executable I uploaded to GitHub is perfectly safe, use at your own risk!
