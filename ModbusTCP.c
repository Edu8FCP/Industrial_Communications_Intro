#include "ModbusAP.h"
#include "ModbusTCP.h"

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


int Send_Modbus_request(char* server_add, int port, uint8_t * APDU, uint16_t APDUlen, uint8_t * APDU_R, uint16_t * APDU_Rlen){

    // check params
    if(NULL == server_add){ // vazio, sem endereço
        printf("[ERRO 1] Send_Modbus_request - Invalid server address\n");
        return -1;
    }
    else if((0 > port) || (65535 < port)){ // nr de porto válido
        printf("[ERRO 2] Send_Modbus_request - Invalid port connection \n");
        return -2;
    }
    else if(NULL == APDU){ // se a camada de cima não enviou algo vazio (ou seja não há pedido na vdd)
        printf("[ERRO 3] Send_Modbus_request - ModbusAP layer not sent a APDU (empty)\n");
        return -3;
    }
    else if(APDU_MAX < APDUlen){ // tamanho
        printf("[ERRO 4] Send_Modbus_request - APDUlen from ModbusAP layer excedeed \n");
        return -4;
    }

    // VARIABLES
    int         count,         // variavel para iterações
                len_pdu_sent,        // tamanho total do envio
                len_pdu_recv,     // tamanho total da resposta
                sock;          // socket's file descriptor relative to the client side
    uint16_t    trans_ID_sent,        
                prot_ID_sent,           
                len_pdu_sent,       // Header params sent
                trans_ID_recv,     
                prot_ID_recv,     
                len_pdu_recv;   // Header params recv
    uint8_t     unit_ID_sent,                // unit identifier - vai no header
                unit_ID_recv,            // unit identifier - vem no header
                pdu_r_mbap[MBAP_SIZE];  // cabeçalho da resposta que serve para ver se veio tudo em ordem
    uint8_t     *PDU;                   // dados 

    // identificador da transição que serve para ver a que pedido a resposta diz respeito
    trans_ID_sent = ti;    // definição
    ti++;                   // atualização
    
    //DEBUG
    printf("[Send_Modbus_request - TI] %u\n", trans_ID_sent);

    // assembles PDU = APDU(SDU) + MBAP
    PDU = (uint8_t *) malloc(sizeof(uint8_t) * (MBAP_SIZE + APDUlen));
    if(NULL == PDU){
        printf("[ERRO 5] Send_Modbus_request  - couldn't allocate memory for APDU\n");
        return -5;
    }
    
    // preenche campos do cabeçalho

    prot_ID_sent = PROT_ID;
    len_pdu_sent = APDUlen + 1;
    unit_ID_sent = 1;

    // Preenche PDU da camada de TCP com os campos dos cabeçalho
    // Formato Big Endian para campos de 2 bytes

    PDU[0] = (uint8_t)(trans_ID_sent >> 8);    // Rigth shift        
    PDU[1] = (uint8_t)(trans_ID_sent & 0x00ff); // and para ficar com só com os primeiros 8 bits
    PDU[2] = (uint8_t)(prot_ID_sent >> 8);
    PDU[3] = (uint8_t)(prot_ID_sent & 0x00ff);
    PDU[4] = (uint8_t)(len_pdu_sent >> 8);
    PDU[5] = (uint8_t)(len_pdu_sent & 0x00ff); // mesmo para as outras, big endian, primeiro o MSB
    PDU[6] = unit_ID_sent; // Seria um campo relevante para servidores que lidassem com múltiplos clients ao mesmo tempo

    // colocar APDU da APLayer na PDU da TCPLayer

    for(count = 0; count < MBAP_SIZE + APDUlen; count++)            // copies the apdu to the pdu
        PDU[count+7] = APDU[count];

    // SOCKET PARA O TCP DO CLIENT
    sock = socket(PF_INET, SOCK_STREAM, 0);    // opens a new socket to connect / interface with the server
    if(-1 == sock){
        printf("[ERRO 6] Send_Modbus_request - couldn't create a socket to execute the operation \n");
        free(PDU);
        PDU = NULL;
        return -6;
    }
    printf("Socked created with success...\n \tAF_INET, 127.0.0.1, port 5502 File Descriptor: %d\n", sock);

    serv.sin_family = AF_INET;
    serv.sin_port = htons(port); // host to network short
    serv.sin_addr.s_addr = inet_addr(server_add); // ver este na documentação

    // CONECTAR ao servidor remoto
    if(-1 == connect(sock, (struct sockaddr *) &serv, addlen)){ 
        printf("[ERRO 7] Send_Modbus_request - Socket couldn't establish a connection\n");
        close(sock);
        free(PDU);
        PDU = NULL;
        return -7;
    } else printf("Connection established with success...\n");

    /******************************
    *  BIDIRECTIONAL CONNECTION   *
    ******************************/

    // write (fd, PDU, PDUlen) // sends Modbus TCP PDU
    // guarda o valor retornado que corresponde ao tamanho do pacote enviado
    len_pdu_sent = send(sock, PDU, MBAP_SIZE + APDUlen, 0);
    if(-1 == len_pdu_sent){                     
        printf("[ERRO 8] Send_Modbus_request - Socket couldn't sent his APDU \n");
        close(sock);
        free(PDU);
        PDU = NULL;
        return -8;
    }

    // read (fd, PDU_R, PDU_Rlen) // response o timeout
    // 1st step - read mbap
    len_pdu_recv = recv(sock, pdu_r_mbap, MBAP_SIZE, 0);
    if(-1 == len_pdu_recv){                       // error happened in the receiving operation
        printf("[ERROR 9] Send_Modbus_request - Socket didn't received a response (Response length is 0, empty)\n");
        close(sock);
        free(PDU);
        PDU = NULL;
        return -9;
    } else print("Recebi resposta... \n");

    /********************************
    * PROCESSO INVERSO APÓS RECEBER *
    ********************************/

    /******************************************************************
    *                                                                 *
    *  TEMOS DE VERIFICAR SE A MENSAGEM CHEGOU TODA.                  * 
    *               2 CASOS                                           *
    *   CHEGOU MENOS QUE O HEADER E TEMOS DE ESPERAR PELO HEADER TODO *
    *   CHEGOU O CAMPO LEN E TEMOS DE ESPERAR QUE CHEGUEM OS BYTES    *
    * CORRESPONDENTES AO CAMPO HEADER                                 *
    *                                                                 *
    * ****************************************************************/
    // Vamos descontruir o pacote recebido e verificar os seus campos

    // Remover MBAP da resposta
    trans_ID_recv = (pdu_r_mbap[0] << 8) + pdu_r_mbap[1];
    prot_ID_recv = (pdu_r_mbap[2] << 8) + pdu_r_mbap[3];
    len_pdu_recv =  (pdu_r_mbap[4] << 8) + pdu_r_mbap[5];
    unit_ID_recv = pdu_r_mbap[6];

    // CABEÇALHO EXTRAÍDO QUE TEMOS DE COMPARAR PARA VER SE ESTÁ TUDO EM ORDEM
    printf("TI=%u\tPROT_ID=%u\tLENGTH=%u\tUnit_ID=%u\n", trans_ID_recv, prot_ID_recv, len_pdu_recv, unit_ID_recv);
    /*********************************/

    if(trans_ID_sent != trans_ID_recv){           // Verificar se o header mantém a correspondência dos params
            printf("[ERRO 10] Send_Modbus_request - Transaction ID don't match \n");
            close(sock);
            free(PDU);
            PDU = NULL;
            return -10;
    }

    if(prot_ID_sent != prot_ID_recv){          
            printf("[ERRO 11] Send_Modbus_request - Protocol ID don't match\n");
            close(sock);
            free(PDU);
            PDU = NULL;
            return -11;
    }

    if(unit_ID_sent != unit_ID_recv){           // Disp. de troca de dados
            printf("[ERRO 12] Send_Modbus_request - Unit ID don't match \n");
            close(sock);
            free(PDU);
            PDU = NULL;
            return -12;
    }

    /*************************************
    * COPIAR PDU RECEBIDO PARA CAMADA AP *
    **************************************/

    for(count = 0; count < APDUlen - MBAP_SIZE; count++)
        APDU[count] = PDU[count+7];

    // closes TCP client socket with server (*)
    close(sock);
    free(PDU);
    PDU = NULL;

    // returns: APDU_R and 0 – ok, <0 – error (timeout)

    /***************************
    *         RETORNO          *
    ***************************/
    return 0;
}