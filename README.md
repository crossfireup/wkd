# WKD
  this project include kernel-mode driver and user-mode control application
  - dirver 
    - run in kernel-mode can modify SSDT function 
    - log the process infomation that calls the modified syscall
    - log the process create and exit information
  - application 
    - create service to control the driver's load, unload, etc
    - Simple analyze of the process image file

# Prerequisties
  - install Visual Studio 2013 
  - Windows Driver Kits 7

# Installing
  - open the project file and build it, you get the driver file and application executable file
  - copy them to the same directory and open the exe file

# ChangLog
  - develop a kernel driver and can install use devcon
  - develop a user-mode application that controls the driver

# TODO
  - complete the IoControl within the driver
  - Add PE analysis to the application and perfect the window showup

# contact
  dobly kuniscplus@gmail.com 


  
