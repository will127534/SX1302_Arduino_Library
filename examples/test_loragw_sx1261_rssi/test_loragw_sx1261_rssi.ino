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

#define SX1302_RESET 12
#define SX1302_CS 13
#define SX1261_CS 13

#define BUFF_SIZE 16

using namespace std;

extern "C" {
int _write(int fd, char *ptr, int len) {
  (void) fd;
  return Serial.write(ptr, len);
}
}

int i, x;
unsigned int arg_u;

uint8_t buff[BUFF_SIZE];
uint32_t freq_hz = (uint32_t)((915.5*1e6) + 0.5); //sx1261 freq
float rssi_inst;
uint32_t fa = (uint32_t)((915.0*1e6) + 0.5);
uint32_t fb = (uint32_t)((915.5*1e6) + 0.5);
uint8_t clocksource = 0;
lgw_radio_type_t radio_type = LGW_RADIO_TYPE_SX1250;

struct lgw_conf_board_s boardconf;
struct lgw_conf_rxrf_s rfconf;

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

    Serial.print("Beginning of test for loragw_sx1261_rssi\n");

    /* Configure the gateway */
    memset( &boardconf, 0, sizeof boardconf);
    boardconf.lorawan_public = true;
    boardconf.clksrc = clocksource;
    boardconf.full_duplex = false;
    boardconf.cs = SX1302_CS;
    boardconf.com_path[sizeof boardconf.com_path - 1] = '\0'; /* ensure string termination */
    if (lgw_board_setconf(&boardconf) != LGW_HAL_SUCCESS) {
        Serial.print("ERROR: failed to configure board\n");
        while(1);
    }

    /* set configuration for RF chains */
    memset( &rfconf, 0, sizeof rfconf);
    rfconf.enable = true; /* must be enabled to proper RF matching */
    rfconf.freq_hz = fa;
    rfconf.type = radio_type;
    rfconf.rssi_offset = 0.0;
    rfconf.tx_enable = false;
    rfconf.single_input_mode = false;
    if (lgw_rxrf_setconf(0, &rfconf) != LGW_HAL_SUCCESS) {
        Serial.print("ERROR: failed to configure rxrf 0\n");
        while(1);
    }

    memset( &rfconf, 0, sizeof rfconf);
    rfconf.enable = true; /* must be enabled to proper RF matching */
    rfconf.freq_hz = fb;
    rfconf.type = radio_type;
    rfconf.rssi_offset = 0.0;
    rfconf.tx_enable = false;
    rfconf.single_input_mode = false;
    if (lgw_rxrf_setconf(1, &rfconf) != LGW_HAL_SUCCESS) {
        Serial.print("ERROR: failed to configure rxrf 1\n");
        while(1);
    }

    /* Connect to the concentrator board */
    x = lgw_start();
    if (x != LGW_REG_SUCCESS) {
        Serial.print("ERROR: Failed to connect to the concentrator\n");
        while(1);
    }

    /* Connect to the sx1261 radio */
    x = sx1261_connect(SX1261_CS);

    x = sx1261_calibrate(freq_hz);
    if (x != LGW_REG_SUCCESS) {
        Serial.print("ERROR: Failed to calibrate the sx1261\n");
        while(1);
    }

    x = sx1261_setup();
    if (x != LGW_REG_SUCCESS) {
        Serial.print("ERROR: Failed to setup the sx1261\n");
        while(1);
    }

    x = sx1261_set_rx_params(freq_hz, BW_125KHZ);
    if (x != LGW_REG_SUCCESS) {
        Serial.print("ERROR: Failed to set RX params\n");
        while(1);
    }


}
void loop(){
    /* databuffer R/W stress test */
    buff[0] = 0x00;
    buff[1] = 0x00;
    sx1261_reg_r(SX1261_GET_RSSI_INST, buff, 2);

    rssi_inst = -((float)buff[1] / 2);

    Serial.print("\rSX1261 RSSI at");
    Serial.print(freq_hz);
    Serial.print(" Hz: ");
    Serial.print(rssi_inst);
    Serial.println(" dBm");

    delay(100);
}
