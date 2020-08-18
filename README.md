# OpenXPBMS
An Open Source BMS(Battery Management System) for Valence XP Batteries that communicates with the built-in modules over the factory communication cables. The goal of this project is not to reproduce all of the functionality available through the vendor diagnostic app. The BMS only has to read a very small subset of the total data available from the built in modules to determine if it should continue to allow the battery or batteries to maintain connected to the system. The BMS does NOT make any write changes to the module; it is only listening.
# Status : Early Development
## This repo has three main sections
### BMS 
Arduino sketch to run the actual BMS. At this point, it is in early stages on development. Full functionality for monitoring (1) battery has been implemented! CRC HAS NOW BEEN IMPLEMENTED! Please post feedback on use with multiple batteries!
Hop over to [https://diysolarforum.com/threads/interfacing-with-valence-built-in-monitoring.2183/] for more information.
### PCB
This directory now has a basic KiCad schematic showing how the modules should be connected together. In the future there will also be KiCad PCB design files in order to solder everything to one board.
### Utilities
Scripts and other tools to run from a PC for diagnostic or research purposes.
There is one script here currently. It allows you to obtain voltage and temperature readings from a battery assigned to ID 1 using python.
## The general gist of how to communicate with the built-in modules
Each battery communicates using a protocol similar to MODBUS over RS485.
First a wake up command is broadcast over RS485 to wake up any battery modules on the line.
Immediately after the broadcast command is sent, a request has to be made. In this projects's case, we make requests to a specific battery to read holding registers that contain the voltage and temperature information for that battery.
The built in modules listen to the request and the module that is being requested responds with it's information.