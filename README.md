avr_fdvlib
==========

C++ Library for Atmel Atmega168/328/1280/2560. 
Supports general purpose algorithms, compression/decompression algorithms, datetime, ADC, interrupts, 
time scheduler, memory handlers, strings, vectors, Circular Buffers, Array, EEprom array, random numbers, 
onewire, FTP Server, W5100 controller, HTTP Server, HTTP Server with scripting, UDP Client, NTP Client, 
MAC/ARP/ICMP/IP/UDP stack, UART, SPI, TwoWire, AXE033, DS1307, DS18B20, DS2406, MQ7, MRF24J40, 
ENC28J60, SD Card, ESP8266, SIM900, softserial.


To add avr_fdvlib as link to Atmel Studio project run:

mklink /J avr_fdvlib c:\avr_fdvlib_installation_path


Now from Solution Explorer:
- click the button "Show All Files"
- right-click "Include in Project" on folders or files you want to include in your project


Finally, in project settings -> toolchain:
- AVR/GNU C Compiler -> Symbols, add "F_CPU=16000000" (or the frequency of your chip)
- AVR/GNU C++ Compiler -> Symbols, add "F_CPU=16000000" (or the frequency of your chip)
- AVR/GNU C++ Compiler -> General, check "Use subroutines for function prologues and epilogues"
- AVR/GNU C++ Compiler -> Optimizations, remove check "Pack Structure members together"
