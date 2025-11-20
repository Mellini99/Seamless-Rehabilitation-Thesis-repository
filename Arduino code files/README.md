This folder contains three Arduino code files that are used for the soap dispenser prototype. 
- setup.ino for setting the time on the clock module and setting up the memory
- main.ino for defining how to count and store repetitions & sessions and how much soap to dispense
- read_out.ino for reading out the repetitions & sessions with their repsective time and date

To execute any of the files:
1. Download the Arduino IDE on your computer
2. Connect the usb cable that sticks out of the back of the prototype into a free usb port on your computer
3. Download the .ino file from the repository
4. Open the .ino file in Arduino IDE
5. Change Tool settings in menu to:

Board: Choose "Arduino Nano"
Port: Choose the port number where you plugged in the usb cable
Processor: Choose "ATmega328P (Old Bootloader)"

6.  If applicable, change values in the .ino file
7.  If nothing needs to be changed, upload code (it takes a while to compile and upload)

Most common issues:
- If code does not compile: Check Tool settings, especially that you connected the right usb port
- If code compiles, but doesnt upload: Try change Processor to "ATmega 168"



First, setup the prototype memory and time 
1. Download the setup.ino file from the repository
2. Change code line 14 "#define FORCE_OVERWRITE" value from false to true -> this is a protection line so we dont accidentily overwrite the memory, but in this case we want to set the memory and time new
3. Change the date and time of the clock module by changing the values in the code lines 8-12
4. Upload the code
5. Open the Serial Monitor view on the bottom of the screen and check if the time is set correctly
6. Set back code line 14 value to false to protect from accidentally over writing your data



Second, upload the instructions on how to count sessions and dispense soap on^to teh microcontroller 
1. Download the main.ino file from the repository
2. Upload the code
3. Use the prototype to check whether the sensors are calibrated correctly and the soap dispenses
4. If yes, unplug the usb cable from the computer and connect the usb to the powerbank, then it is ready to use

- If you want to change the order of the sensors change code lines 8-9, then upload the code
- If you want to adjust the sensor threshold or calibration, change code lines 12-14 + 17, then upload the code
- If you want to adjust the session duration change code line 11, then upload the code
- If you want to adjust the amount of soap dispensed, adjust the duration of the motor pumping per repetition in code lines 15-16, then upload the code
- If you want to test the sensor values measured during the interaction and whether a repetition was detected change code line 4 from PROD to DEBUG, upload the code keep the prototype connected to the computer, use the prototype and look at the output in the Serial Monitor screen




After using the prototype, read out the use data
1. Download the read_out.ino file from the repository
2. Upload the code
3. Switch from the Output window to the Serial Motor window -> Here you should see the use data printed out
4. Copy paste output to note file e.g. Word to save data on your computer



Note, after reading out the code you need to renew the instructions:
- If you want to continue counting repetitions, upload the main.ino file after using the read_out file
- If you want to start a new count, reset the memory and clock with the setup.ino file and then run the main.ino file again (make sure to have saves the previous repetitions as they will be completely deleted from the memory)

