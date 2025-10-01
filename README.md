# Seamless-Rehabilitation-Thesis-repository
Contains Arduino Code and assembly files for the prototype used in the thesis


To read out the use data from the prototype:

1) Connect the usb cable that sticks out of the back of the prototype into a free usb port on your computer
2) Download the Read-out file from this repository 
4) Open Read-out file in Arduino IDE
5) Change Tool settings in menu
     - Board: Choose "Arduino Nano"
     - Port: Choose the port number where you plugged in the usb cable
     - Processor: Choose "ATmega328P (Old Bootloader)"
  
6) Upload Code (it takes a while to compile and upload)
7) Switch from the Output window to the Serial Motor window 
8) Here you should see the use data printed out
9) Copy paste output to note file e.g. Word to save data on your computer

Possible issues:
- If code does not compile: Check Tool settings, especially that you connected the right usb port
- If code compiles, but doesnt upload: Try change Processor to "ATmega 168"

