/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2019 Semtech

Description:
    Functions used to handle LoRa concentrator SX1255/SX1257 radios SPI access.

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

#ifndef _SX125X_SPI_H
#define _SX125X_SPI_H

/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC int sx125x_spi_r(int cs, uint8_t spi_mux_target, uint8_t address, uint8_t *data);
EXTERNC int sx125x_spi_w(int cs, uint8_t spi_mux_target, uint8_t address, uint8_t data);

#undef EXTERNC


#endif
/* --- EOF ------------------------------------------------------------------ */


