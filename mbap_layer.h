#ifndef _MODBUSAP_H
#define _MODBUSAP_H

#include "mbtcp_layer.h"
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define APDU_MAX 253 // Application Layer max
#define PORTO_MIN 0
#define PORTO_MAX 65535 // Limites dos portos em que a conexão pode ser estabelecida

// endereços máximos e mínimos dos registos
#define ADDR_MIN 0
#define ADDR_MAX 65535 // endereços máximos e mínimos
#define REGS_SIZE 16

// Funções
#define WRITE_MULTIPLE_REGS_FUNC 0x10
#define READ_HOLDING_REGS_FUNC 0x03

#define APDU_LEN_READ_H_REGS 5

#define READ_H_REGS_DEFAULT             -100
#define READ_H_REGS_ILLG_FUNCTION       -101
#define READ_H_REGS_DATA_ADDRESS        -102
#define READ_H_REGS_DATA_VALUE          -103
#define WRITE_MULT_REGS_DEFAULT         -100
#define WRITE_MULT_REGS_ILLG_FUNCTION   -101
#define WRITE_MULT_REGS_DATA_ADDRESS    -102
#define WRITE_MULT_REGS_DATA_VALUE      -103

/*********************
 * FUNCTIONS HEADERS *
 *********************/
/*******************************************************************************************
 * READ_H_REGS()
 * Execute the function of reading holding registers in the server.
 * Arguments description:
 *  - server_add - struct that contains the server's address;
 *  - port - port that will be used for the connection;
 *  - st_r = start address of the holding registers - 1, because of uint16_t that can only
 *      store values from 0 to 65535;
 *  - n_r - number of holding registers to be read by the server;
 *  - val - vector that will be returned with the pretended values. For reference, the
 *      length of the vector is n_r.
 *******************************************************************************************/
int Read_h_regs(char * server_add, int port, uint32_t st_r, uint16_t n_r, uint16_t * val);

/*******************************************************************************************
 * WRITE_MULTIPLE_REGS()
 * Execute the function of write holding registers in the server.
 * Arguments description:
 *  - server_add - struct that contains the server's address;
 *  - port - port that will be used for the connection;
 *  - st_r = start address of the holding registers - 1, because of uint16_t that can only
 *      store values from 0 to 65535;
 *  - n_r - number of holding registers to be written by the server;
 *  - val - vector that is passed with the pretended values. For reference, the
 *      length of the vector is n_r.
 *******************************************************************************************/
int Write_multiple_regs(char * server_addr, int port, uint32_t st_r, uint16_t n_r, uint16_t * val);

#endif



