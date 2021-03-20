extern "C" {
#include <loragw_com.h>
#include <loragw_aux.h>
#include <loragw_hal.h>
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


#define BUFF_SIZE_SPI       1024

#define SX1302_AGC_MCU_MEM  0x0000
#define SX1302_REG_COMMON   0x5600
#define SX1302_REG_AGC_MCU  0x5780

#define SX1302_RESET 12
#define SX1302_CS 13

/* Buffers */
static uint8_t * test_buff = NULL;
static uint8_t * test_buff_backup = NULL;
static uint8_t * read_buff = NULL;

uint16_t max_buff_size;
uint8_t data = 0;
int i, x;
uint16_t size;
int cycle_number;

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

    Serial.print("Beginning of test for loragw_com\n");
    x = lgw_com_open(SX1302_CS);

    /* normal R/W test */
    /* TODO */

    /* burst R/W test, small bursts << LGW_BURST_CHUNK */
    /* TODO */

    /* burst R/W test, large bursts >> LGW_BURST_CHUNK */
    /* TODO */

    x = lgw_com_r(LGW_SPI_MUX_TARGET_SX1302, SX1302_REG_COMMON + 6, &data);
    if (x != 0) {
        Serial.print("ERROR: failed to read register ");
        Serial.println(__LINE__);
        while(1);
    }
    Serial.print("SX1302 version: ");
    Serial.println(data,HEX);

    x = lgw_com_r(LGW_SPI_MUX_TARGET_SX1302, SX1302_REG_AGC_MCU + 0, &data);
    if (x != 0) {
        Serial.print("ERROR: failed to read register ");
        Serial.println(__LINE__);
        while(1);
    }
    x = lgw_com_w(LGW_SPI_MUX_TARGET_SX1302, SX1302_REG_AGC_MCU + 0, 0x06); /* mcu_clear, host_prog */
    if (x != 0) {
        Serial.print("ERROR: failed to read register ");
        Serial.println(__LINE__);
        while(1);
    }


    /* Allocate buffers according to com type capabilities */
    max_buff_size = BUFF_SIZE_SPI;
    test_buff = (uint8_t*)malloc(max_buff_size * sizeof(uint8_t));
    if (test_buff == NULL) {
        Serial.print("ERROR: failed to allocate memory for test_buff\n");
        while(1);
    }
    test_buff_backup = (uint8_t*)malloc(max_buff_size * sizeof(uint8_t));
    if (test_buff_backup == NULL) {
        Serial.print("ERROR: failed to allocate memory for test_buff\n");
        while(1);
    }    read_buff = (uint8_t*)malloc(max_buff_size * sizeof(uint8_t));
    if (read_buff == NULL) {
        Serial.print("ERROR: failed to allocate memory for read_buff \n");
        while(1);
    }

}

void loop(){
    /*************************************************
     *
     *      WRITE BURST TEST
     *
     * ***********************************************/

    size = random(max_buff_size);
    for (i = 0; i < size; ++i) {
        test_buff[i] = random(255);
        test_buff_backup[i] = test_buff[i];
    }
    Serial.print("Cycle> ");
    Serial.println(cycle_number);

    /* Write burst with random data */
    x = lgw_com_wb(LGW_SPI_MUX_TARGET_SX1302, SX1302_AGC_MCU_MEM, test_buff, size);
    if (x != 0) {
        Serial.print("ERROR: failed to read register ");
        Serial.println(__LINE__);
        while(1);
    }

    /* Read back */
    x = lgw_com_rb(LGW_SPI_MUX_TARGET_SX1302, SX1302_AGC_MCU_MEM, read_buff, size);
    if (x != 0) {
        Serial.print("ERROR: failed to read register ");
        Serial.println(__LINE__);
        while(1);
    }

    /* Compare read / write buffers */
    for (i=0; ((i<size) && (test_buff_backup[i] == read_buff[i])); ++i);
    if (i != size) {
        Serial.print("error during the buffer comparison\n");

        /* Print what has been written */
        Serial.print("Written values:\n");
        for (i=0; i<size; ++i) {
            Serial.print(test_buff_backup[i],HEX);
            if (i%16 == 15) printf("\n");
        }
        Serial.println("");

        /* Print what has been read back */
        Serial.print("Read values:\n");
        for (i=0; i<size; ++i) {
            Serial.print(read_buff[i],HEX);
            if (i%16 == 15) printf("\n");
        }
        Serial.println("");

        /* exit */
        while(1);
    } else {
        Serial.print("did a R/W on a data buffer with no error\n");
        Serial.println(size);
        ++cycle_number;
    }

    /*************************************************
     *
     *      WRITE SINGLE BYTE TEST
     *
     * ***********************************************/

    /* Single byte r/w test */
    Serial.print("Cycle> ");
    Serial.println(cycle_number);

    test_buff[0] = random(255);

    /* Write single byte */
    x = lgw_com_w(LGW_SPI_MUX_TARGET_SX1302, SX1302_AGC_MCU_MEM, test_buff[0]);
    if (x != 0) {
        Serial.print("ERROR: failed to read register ");
        Serial.println(__LINE__);
        while(1);
    }

    /* Read back */
    x = lgw_com_r(LGW_SPI_MUX_TARGET_SX1302, SX1302_AGC_MCU_MEM, &read_buff[0]);
    if (x != 0) {
        Serial.print("ERROR: failed to read register ");
        Serial.println(__LINE__);
        while(1);
    }

    /* Compare read / write bytes */
    if (test_buff[0] != read_buff[0]) {
        Serial.print("error during the byte comparison\n");

        /* Print what has been written */
        Serial.print("Written value: ");
        Serial.println(test_buff[0],HEX);

        /* Print what has been read back */
        Serial.print("Read values: ");
        Serial.println(read_buff[0],HEX);

        /* exit */
        while(1);
    } else {
        Serial.print("did a 1-byte R/W on a data buffer with no error\n");
        ++cycle_number;
    }

}
