extern "C" {
#include "loragw_com.h"
#include "loragw_aux.h"
#include "loragw_hal.h"
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

#define SX1302_RESET 12
#define SX1302_CS 13
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

int i, x;
uint32_t ft = (uint32_t)((915.0*1e6) + 0.5);
int8_t rf_power = 20;
uint8_t sf = 7;
uint16_t bw_khz = 125;
uint32_t nb_pkt = 10;
unsigned int nb_loop = 1, cnt_loop;
uint8_t size = 20;
char mod[64] = "LORA";
float br_kbps = 50;
uint8_t fdev_khz = 25;
int8_t freq_offset = 0;
float xf = 0.0;
uint8_t clocksource = 0;
uint8_t rf_chain = 0;
lgw_radio_type_t radio_type = LGW_RADIO_TYPE_SX1250; //LGW_RADIO_TYPE_SX1255, LGW_RADIO_TYPE_SX1257, LGW_RADIO_TYPE_SX1250
uint16_t preamble = 8;
bool invert_pol = false;
bool no_header = false;
bool single_input_mode = false;
bool full_duplex = false;

struct lgw_conf_board_s boardconf;
struct lgw_conf_rxrf_s rfconf;
struct lgw_pkt_tx_s pkt;
struct lgw_tx_gain_lut_s txlut; /* TX gain table */
uint8_t tx_status;
uint32_t count_us;
uint32_t trig_delay_us = 1000000;
bool trig_delay = false;

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

    Serial.print("===== sx1302 HAL TX test =====\n");

    txlut.size = 0;
    memset(txlut.lut, 0, sizeof txlut.lut);

    printf("Sending %i LoRa packets on %u Hz (BW %i kHz, SF %i, CR %i, %i bytes payload, %i symbols preamble, %s header, %s polarity) at %i dBm\n", nb_pkt, ft, bw_khz, sf, 1, size, preamble, (no_header == false) ? "explicit" : "implicit", (invert_pol == false) ? "non-inverted" : "inverted", rf_power);

    /* Configure the gateway */
    memset( &boardconf, 0, sizeof boardconf);
    boardconf.lorawan_public = true;
    boardconf.clksrc = clocksource;
    boardconf.full_duplex = full_duplex;
    boardconf.cs = SX1302_CS;
    if (lgw_board_setconf(&boardconf) != LGW_HAL_SUCCESS) {
        printf("ERROR: failed to configure board\n");
        while(1);
    }

    memset( &rfconf, 0, sizeof rfconf);
    rfconf.enable = true; /* rf chain 0 needs to be enabled for calibration to work on sx1257 */
    rfconf.freq_hz = ft;
    rfconf.type = radio_type;
    rfconf.tx_enable = true;
    rfconf.single_input_mode = single_input_mode;
    if (lgw_rxrf_setconf(0, &rfconf) != LGW_HAL_SUCCESS) {
        printf("ERROR: failed to configure rxrf 0\n");
        while(1);
    }

    memset( &rfconf, 0, sizeof rfconf);
    rfconf.enable = (((rf_chain == 1) || (clocksource == 1)) ? true : false);
    rfconf.freq_hz = ft;
    rfconf.type = radio_type;
    rfconf.tx_enable = false;
    rfconf.single_input_mode = single_input_mode;
    if (lgw_rxrf_setconf(1, &rfconf) != LGW_HAL_SUCCESS) {
        printf("ERROR: failed to configure rxrf 1\n");
        while(1);
    }

    if (txlut.size > 0) {
        if (lgw_txgain_setconf(rf_chain, &txlut) != LGW_HAL_SUCCESS) {
            printf("ERROR: failed to configure txgain lut\n");
            while(1);
        }
    }

    for (cnt_loop = 0; cnt_loop < nb_loop; cnt_loop++) {
        
        Serial.print("SX1302 Reset\n");
        /* Board reset */
        digitalWrite(SX1302_RESET,HIGH);
        delay(100);
        digitalWrite(SX1302_RESET,LOW);
        delay(100);

        /* connect, configure and start the LoRa concentrator */
        x = lgw_start();

        /* Send packets */
        memset(&pkt, 0, sizeof pkt);
        pkt.rf_chain = rf_chain;
        pkt.freq_hz = ft;
        pkt.rf_power = rf_power;
        if (trig_delay == false) {
            pkt.tx_mode = IMMEDIATE;
        } else {
            if (trig_delay_us == 0) {
                pkt.tx_mode = ON_GPS;
            } else {
                pkt.tx_mode = TIMESTAMPED;
            }
        }
        if ( strcmp( mod, "CW" ) == 0 ) {
            pkt.modulation = MOD_CW;
            pkt.freq_offset = freq_offset;
            pkt.f_dev = fdev_khz;
        }
        else if( strcmp( mod, "FSK" ) == 0 ) {
            pkt.modulation = MOD_FSK;
            pkt.no_crc = false;
            pkt.datarate = br_kbps * 1e3;
            pkt.f_dev = fdev_khz;
        } else {
            pkt.modulation = MOD_LORA;
            pkt.coderate = CR_LORA_4_5;
            pkt.no_crc = false;
        }
        pkt.invert_pol = invert_pol;
        pkt.preamble = preamble;
        pkt.no_header = no_header;
        pkt.payload[0] = 0x40; /* Confirmed Data Up */
        pkt.payload[1] = 0xAB;
        pkt.payload[2] = 0xAB;
        pkt.payload[3] = 0xAB;
        pkt.payload[4] = 0xAB;
        pkt.payload[5] = 0x00; /* FCTrl */
        pkt.payload[6] = 0; /* FCnt */
        pkt.payload[7] = 0; /* FCnt */
        pkt.payload[8] = 0x02; /* FPort */
        for (i = 9; i < 255; i++) {
            pkt.payload[i] = i;
        }

        for (i = 0; i < (int)nb_pkt; i++) {
            if (trig_delay == true) {
                if (trig_delay_us > 0) {
                    lgw_get_instcnt(&count_us);
                    printf("count_us:%u\n", count_us);
                    pkt.count_us = count_us + trig_delay_us;
                    printf("programming TX for %u\n", pkt.count_us);
                } else {
                    printf("programming TX for next PPS (GPS)\n");
                }
            }

            if( strcmp( mod, "LORA" ) == 0 ) {
                pkt.datarate = (sf == 0) ? (uint8_t)random(5, 12) : sf;
            }

            switch (bw_khz) {
                case 125:
                    pkt.bandwidth = BW_125KHZ;
                    break;
                case 250:
                    pkt.bandwidth = BW_250KHZ;
                    break;
                case 500:
                    pkt.bandwidth = BW_500KHZ;
                    break;
                default:
                    pkt.bandwidth = (uint8_t)random(BW_125KHZ, BW_500KHZ);
                    break;
            }

            pkt.size = (size == 0) ? (uint8_t)random(9, 255) : size;

            pkt.payload[6] = (uint8_t)(i >> 0); /* FCnt */
            pkt.payload[7] = (uint8_t)(i >> 8); /* FCnt */
            x = lgw_send(&pkt);
            if (x != 0) {
                printf("ERROR: failed to send packet\n");
                while(1);
            }
            /* wait for packet to finish sending */
            do {
                delay(5);
                lgw_status(pkt.rf_chain, TX_STATUS, &tx_status); /* get TX status */
            } while ((tx_status != TX_FREE));
 
            printf("TX done\n");
            delay(1000);
        }

    printf( "\nNb packets sent: %u (%u)\n", i, cnt_loop + 1 );

    /* Stop the gateway */
    x = lgw_stop();    
    }
    while(1);
}

void loop(){


}
