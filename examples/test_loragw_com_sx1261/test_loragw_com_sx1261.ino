extern "C" {
#include "loragw_aux.h"
#include "loragw_hal.h"
#include "loragw_reg.h"
#include "loragw_com.h"
#include "loragw_sx1261.h"
}

#include <SPI.h>
#include <Arduino.h>
#include <stdio.h>
#include <iostream>

using namespace std;

extern "C" {
int _write(int fd, char *ptr, int len) {
  (void) fd;
  return Serial.write(ptr, len);
}
}

#define BUFF_SIZE  16
#define SX1302_RESET 12
#define SX1302_CS 13
#define SX1261_CS 5

uint8_t test_buff[BUFF_SIZE];
uint8_t read_buff[BUFF_SIZE];
uint32_t test_val, read_val;
int cycle_number = 0;
int i, x;

void setup(){
    Serial.begin(115200);
    SPI.begin();

    pinMode(SX1302_RESET,OUTPUT);
    pinMode(SX1302_CS,OUTPUT);

    digitalWrite(SX1302_RESET,LOW);
    digitalWrite(SX1302_CS,HIGH);

    while(!Serial){
        delay(1000);
    }

    Serial.print("SX1302 Reset\n");
    /* Board reset */
    digitalWrite(SX1302_RESET,HIGH);
    delay(100);
    digitalWrite(SX1302_RESET,LOW);
    delay(100);

    Serial.print("Beginning of test for loragw_com_sx1261\n");

    /* Connect to the concentrator board */
    x = lgw_connect(SX1302_CS);
    
    /* Connect to the sx1261 radio */
    x = sx1261_connect(SX1261_CS);

    /* Set Radio in Standby mode */
    test_buff[0] = (uint8_t)SX1261_STDBY_RC;
    x = sx1261_reg_w(SX1261_SET_STANDBY, test_buff, 1);
    if (x != LGW_REG_SUCCESS) {
        printf("ERROR(%d): Failed to configure sx1261\n", __LINE__);
        while(1);
    }
    delay(10);

    test_buff[0] = 0x00;
    x = sx1261_reg_r(SX1261_GET_STATUS, test_buff, 1);
    if (x != LGW_REG_SUCCESS) {
        printf("ERROR(%d): Failed to get sx1261 status\n", __LINE__);
        while(1);
    }
    printf("SX1261: get_status: 0x%02X\n", test_buff[0]);
}

void loop(){
    /* databuffer R/W stress test */
    test_buff[0] = rand() & 0x7F;
    test_buff[1] = rand() & 0xFF;
    test_buff[2] = rand() & 0xFF;
    test_buff[3] = rand() & 0xFF;
    test_val = (test_buff[0] << 24) | (test_buff[1] << 16) | (test_buff[2] << 8) | (test_buff[3] << 0);
    sx1261_reg_w(SX1261_SET_RF_FREQUENCY, test_buff, 4);

    read_buff[0] = 0x08;
    read_buff[1] = 0x8B;
    read_buff[2] = 0x00;
    read_buff[3] = 0x00;
    read_buff[4] = 0x00;
    read_buff[5] = 0x00;
    read_buff[6] = 0x00;
    sx1261_reg_r(SX1261_READ_REGISTER, read_buff, 7);
    read_val = (read_buff[3] << 24) | (read_buff[4] << 16) | (read_buff[5] << 8) | (read_buff[6] << 0);

    printf("Cycle %i > ", cycle_number);
    if (read_val != test_val) {
        printf("error during the buffer comparison\n");
        printf("Written value: %08X\n", test_val);
        printf("Read value:    %08X\n", read_val);
        while(1);
    } else {
        printf("did a %i-byte R/W on a register with no error\n", 4);
        ++cycle_number;
    }

}
