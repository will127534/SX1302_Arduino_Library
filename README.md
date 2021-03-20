# SX1302 HAL library for Arduino

This is a Arduino library to communicate with SX1302 LoRa gateway based on Semtech's SX1302 HAL: [https://github.com/Lora-net/sx1302_hal](https://github.com/Lora-net/sx1302_hal)

Currently this library has been tested with SX1303 + SX1250

### Tested:
1. SX1303 comunication test
2. SX1250 comunication test
3. RX test with SX1302+SX1250
4. TX test with SX1302+SX1250

### Untested:
1. SX1261 comunication test
2. SX1255/SX1257 comunication test
3. LBT and GPS related functions

### TODO:
1. LBT functions
2. GPS functions

### Changes from SX1302 HAL
1. Adding cs pin configuring for lgw_conf_board_s (=>boardconf.cs) to define the cs pin for SX1302
2. lgw_connect now takes CS pin number input instead of com_type and com_path
3. sx1261_connect also takes CS pin number input instead of com_type and com_path

