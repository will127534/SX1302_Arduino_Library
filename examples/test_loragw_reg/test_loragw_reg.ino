extern "C" {
#include "loragw_com.h"
#include "loragw_reg.h"
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

extern const struct lgw_reg_s loregs[LGW_TOTALREGS+1];

int x, i;
int32_t val;
bool error_found = false;
uint8_t rand_values[LGW_TOTALREGS];
bool reg_ignored[LGW_TOTALREGS]; /* store register to be ignored */
uint8_t reg_val;
uint8_t reg_max;

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

    printf("SX1302 Reset\n");
    /* Board reset */
    digitalWrite(SX1302_RESET,HIGH);
    delay(100);
    digitalWrite(SX1302_RESET,LOW);
    delay(100);

    Serial.print("Beginning of test for loragw_reg\n");

    x = lgw_connect(SX1302_CS);
    if (x != LGW_REG_SUCCESS) {
        printf("ERROR: failed to connect\n");
        while(1);
    }

    /* The following registers cannot be tested this way */
    memset(reg_ignored, 0, sizeof reg_ignored);
    reg_ignored[SX1302_REG_COMMON_CTRL0_CLK32_RIF_CTRL] = true; /* all test fails if we set this one to 1 */

    /* Test 1: read all registers and check default value for non-read-only registers */
    printf("## TEST#1: read all registers and check default value for non-read-only registers\n");
    error_found = false;
    for (i = 0; i < LGW_TOTALREGS; i++) {
        if (loregs[i].rdon == 0) {
            x = lgw_reg_r(i, &val);
            if (x != LGW_REG_SUCCESS) {
                printf("ERROR: failed to read register at index %d\n", i);
                while(1);
            }
            if (val != loregs[i].dflt) {
                printf("ERROR: default value for register at index %d is %d, should be %d\n", i, val, loregs[i].dflt);
                error_found = true;
            }
        }
    }
    printf("------------------\n");
    printf(" TEST#1 %s\n", (error_found == false) ? "PASSED" : "FAILED");
    printf("------------------\n\n");

    /* Test 2: read/write test on all non-read-only, non-pulse, non-w0clr, non-w1clr registers */
    printf("## TEST#2: read/write test on all non-read-only, non-pulse, non-w0clr, non-w1clr registers\n");
    /* Write all registers with a random value */
    error_found = false;
    for (i = 0; i < LGW_TOTALREGS; i++) {
        if ((loregs[i].rdon == 0) && (reg_ignored[i] == false)) {
            /* Peek a random value different form the default reg value */
            reg_max = pow(2, loregs[i].leng) - 1;
            if (loregs[i].leng == 1) {
                reg_val = !loregs[i].dflt;
            } else {
                /* ensure random value is not the default one */
                do {
                    if (loregs[i].sign == 1) {
                        reg_val = rand() % (reg_max / 2);
                    } else {
                        reg_val = rand() % reg_max;
                    }
                } while (reg_val == loregs[i].dflt);
            }
            /* Write selected value */
            x = lgw_reg_w(i, reg_val);
            if (x != LGW_REG_SUCCESS) {
                printf("ERROR: failed to read register at index %d\n", i);
                while(1);
            }
            /* store value for later check */
            rand_values[i] = reg_val;
        }
    }
    /* Read all registers and check if we got proper random value back */
    for (i = 0; i < LGW_TOTALREGS; i++) {
        if ((loregs[i].rdon == 0) && (loregs[i].chck == 1) && (reg_ignored[i] == false)) {
            x = lgw_reg_r(i, &val);
            if (x != LGW_REG_SUCCESS) {
                printf("ERROR: failed to read register at index %d\n", i);
                while(1);
            }
            /* check value */
            if (val != rand_values[i]) {
                printf("ERROR: value read from register at index %d differs from the written value (w:%u r:%d)\n", i, rand_values[i], val);
                error_found = true;
            } else {
                //printf("INFO: MATCH reg %d (%u, %u)\n", i, rand_values[i], (uint8_t)val);
            }
        }
    }
    printf("------------------\n");
    printf(" TEST#2 %s\n", (error_found == false) ? "PASSED" : "FAILED");
    printf("------------------\n\n");
}

void loop(){


}
