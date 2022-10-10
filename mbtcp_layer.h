#ifndef _MODBUSTCP_H
#define _MODBUSTCP_H

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

/****************
 *    HEADER    *
 ****************/
#define TIdent_MIN      0
#define TIdent_MAX      65535 // De onde vem este nr?
#define PROT_ID     0
#define MBAP_SIZE   7 // TransIdent + ProtIdent + Lenght + UnitIdent

/*********************
 * PDU DA CAMADA TCP *
 *********************/
#define PDU_TCP_MAX MBAP_SIZE+APDU_MAX //Modbus TCP frame max 

#define BUFFER_SIZE     300

/********************
 * GLOBAL VARIABLES *
 ********************/
uint8_t buffer[BUFFER_SIZE];
uint16_t ti; // transaction identificar
int socket_data_global; // sockets
struct sockaddr remote_global;
socklen_t addlen_global;


/*********************
 *      HEADERS      *
 *********************/


int Send_Modbus_request(char* server_add, int port, uint8_t * APDU, uint16_t APDUlen, uint8_t * APDU_R, uint16_t * APDU_Rlen);
/*******************************************************************************************
 * Send the client's request to the server.
 * Arguments description:
 *  - server_add - struct that contains the server's address;
 *  - port - port that will be used for the connection;
 *  - APDU - it's the stream that will be send to the server; 
 *  - APDUlen - length of the stream on the application layer; unit identifier + APDU
 *  - APDU_R - stream relative to the server's response (value returned by the argument).
 *******************************************************************************************/


#endif