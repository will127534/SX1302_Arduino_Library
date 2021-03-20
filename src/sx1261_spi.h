/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2019 Semtech

Description:
    Functions used to handle LoRa concentrator SX1261 radio through SPI
    interface.

License: Revised BSD License, see LICENSE.TXT file include in the project
*/


#ifndef _SX1261_SPI_H
#define _SX1261_SPI_H

/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>     /* C99 types*/

#include "sx1261_defs.h"

#include "config.h"     /* library configuration options (dynamically generated) */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC MACROS -------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC TYPES --------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */
#ifdef __cplusplus
extern "C" {
#endif
int sx1261_spi_w(int cs, sx1261_op_code_t op_code, uint8_t *data, uint16_t size);
int sx1261_spi_r(int cs, sx1261_op_code_t op_code, uint8_t *data, uint16_t size);
#ifdef __cplusplus
}
#endif
#endif

/* --- EOF ------------------------------------------------------------------ */
