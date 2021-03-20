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

uint32_t fa = (uint32_t)((916.0*1e6) + 0.5); //Frequency A 916.0Mhz
uint32_t fb = (uint32_t)((916.5*1e6) + 0.5); //Frequency B 915.5Mhz
uint8_t clocksource = 0;
lgw_radio_type_t radio_type = LGW_RADIO_TYPE_SX1250; //LGW_RADIO_TYPE_SX1255, LGW_RADIO_TYPE_SX1257, LGW_RADIO_TYPE_SX1250
uint8_t max_rx_pkt = 16;
bool single_input_mode = false;
float rssi_offset = 0.0;
bool full_duplex = false;

struct lgw_conf_board_s boardconf;
struct lgw_conf_rxrf_s rfconf;
struct lgw_conf_rxif_s ifconf;

unsigned long nb_pkt_crc_ok = 0, nb_loop = 0, cnt_loop;
int nb_pkt;

uint8_t channel_mode = 0; /* LoRaWAN-like */

const int32_t channel_if_mode0[9] = {
    -400000,
    -200000,
    0,
    -400000,
    -200000,
    0,
    200000,
    400000,
    -200000 /* lora service */
};

const int32_t channel_if_mode1[9] = {
    -400000,
    -400000,
    -400000,
    -400000,
    -400000,
    -400000,
    -400000,
    -400000,
    -400000 /* lora service */
};

const uint8_t channel_rfchain_mode0[9] = { 1, 1, 1, 0, 0, 0, 0, 0, 1 };

const uint8_t channel_rfchain_mode1[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

struct lgw_pkt_rx_s rxpkt[16];

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

    Serial.print("===== sx1302 HAL RX test =====\n");

    int i, j, x;
    /* Configure the gateway */
    memset( &boardconf, 0, sizeof boardconf);
    boardconf.lorawan_public = true;
    boardconf.cs = SX1302_CS;
    boardconf.clksrc = clocksource;
    boardconf.full_duplex = full_duplex;
    if (lgw_board_setconf(&boardconf) != LGW_HAL_SUCCESS) {
        Serial.print("ERROR: failed to configure board\n");
        while(1);
    }

    /* set configuration for RF chains */
    memset( &rfconf, 0, sizeof rfconf);
    rfconf.enable = true;
    rfconf.freq_hz = fa;
    rfconf.type = radio_type;
    rfconf.rssi_offset = rssi_offset;
    rfconf.tx_enable = false;
    rfconf.single_input_mode = single_input_mode;
    if (lgw_rxrf_setconf(0, &rfconf) != LGW_HAL_SUCCESS) {
        Serial.print("ERROR: failed to configure rxrf 0\n");
        while(1);
    }

    memset( &rfconf, 0, sizeof rfconf);
    rfconf.enable = true;
    rfconf.freq_hz = fb;
    rfconf.type = radio_type;
    rfconf.rssi_offset = rssi_offset;
    rfconf.tx_enable = false;
    rfconf.single_input_mode = single_input_mode;
    if (lgw_rxrf_setconf(1, &rfconf) != LGW_HAL_SUCCESS) {
        Serial.print("ERROR: failed to configure rxrf 1\n");
        while(1);
    }

    /* set configuration for LoRa multi-SF channels (bandwidth cannot be set) */
    memset(&ifconf, 0, sizeof(ifconf));
    for (i = 0; i < 8; i++) {
        ifconf.enable = true;
        if (channel_mode == 0) {
            ifconf.rf_chain = channel_rfchain_mode0[i];
            ifconf.freq_hz = channel_if_mode0[i];
        } else if (channel_mode == 1) {
            ifconf.rf_chain = channel_rfchain_mode1[i];
            ifconf.freq_hz = channel_if_mode1[i];
        } else {
            Serial.print("ERROR: channel mode not supported\n");
            while(1);
        }
        ifconf.datarate = DR_LORA_SF7;
        if (lgw_rxif_setconf(i, &ifconf) != LGW_HAL_SUCCESS) {
            Serial.print("ERROR: failed to configure rxif");
            Serial.println(i);
            while(1);
        }
    }

    /* set configuration for LoRa Service channel */
    memset(&ifconf, 0, sizeof(ifconf));
    ifconf.rf_chain = channel_rfchain_mode0[i];
    ifconf.freq_hz = channel_if_mode0[i];
    ifconf.datarate = DR_LORA_SF7;
    ifconf.bandwidth = BW_250KHZ;
    if (lgw_rxif_setconf(8, &ifconf) != LGW_HAL_SUCCESS) {
        Serial.print("ERROR: failed to configure rxif for LoRa service channel\n");
        while(1);
    }

    /* set the buffer size to hold received packets */
    
    Serial.print("INFO: rxpkt buffer size is set to:");
    Serial.println(16);
    Serial.print("INFO: Select channel mode:");
    Serial.println(channel_mode);

}

void loop(){
    int i, j, x;
    cnt_loop += 1;

    /* connect, configure and start the LoRa concentrator */
    x = lgw_start();
    if (x != 0) {
        Serial.print("ERROR: failed to start the gateway\n");
        while(1);
    }

    /* Loop until we have enough packets with CRC OK */
    Serial.print("Waiting for packets...\n");
    nb_pkt_crc_ok = 0;
    while (1) {
        /* fetch N packets */
        nb_pkt = lgw_receive(ARRAY_SIZE(rxpkt), rxpkt);

        if (nb_pkt == 0) {
            delay(10);
        } 
        else {
            for (i = 0; i < nb_pkt; i++) {
                if (rxpkt[i].status == STAT_CRC_OK) {
                    nb_pkt_crc_ok += 1;
                }
                Serial.print("\n----- ");
                Serial.print((rxpkt[i].modulation == MOD_LORA) ? "LoRa" : "FSK");
                Serial.println(" packet -----");
                Serial.print("  count_us: ");
                Serial.println(rxpkt[i].count_us);
                Serial.print("  size:     ");
                Serial.println(rxpkt[i].size);
                Serial.print("  chan:     ");
                Serial.println(rxpkt[i].if_chain);
                Serial.print("  status:   ");
                Serial.println(rxpkt[i].status);
                Serial.print("  datr:     ");
                Serial.println(rxpkt[i].datarate);
                Serial.print("  codr:     ");
                Serial.println(rxpkt[i].coderate);
                Serial.print("  rf_chain  ");
                Serial.println(rxpkt[i].rf_chain);
                Serial.print("  freq_hz   ");
                Serial.println(rxpkt[i].freq_hz);
                Serial.print("  snr_avg:  ");
                Serial.println(rxpkt[i].snr);
                Serial.print("  rssi_chan:");
                Serial.println(rxpkt[i].rssic);
                Serial.print("  rssi_sig :");
                Serial.println(rxpkt[i].rssis);
                Serial.print("  crc:      ");
                Serial.println(rxpkt[i].crc);
                for (j = 0; j < rxpkt[i].size; j++) {
                    Serial.print(rxpkt[i].payload[j],HEX);
                    Serial.print(" ");
                }
                Serial.println("");
            }
        }
    }

}
