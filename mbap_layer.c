#include "mbap_layer.h"
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

/******************
 * FUNCTIONS CODE *
 ******************/
/*******************************************************************************************
 * READ_H_REGS()
 * Execute the function of reading holding registers in the server.
 * Params description:
 *  - server_addr - endereço do servidor;
 *  - port - porto de conexão;
 *  - st_r = endereço base a partir do qual se quer ler
 *          O protocolo Modbus implementa os endereços do 0 ao 65535.
 *          Se o campo for 1, vamos ler o endereço 0, que é o primeiro no servidor
 *  - n_r - number de registos que vamos querer ler. Quantos a partir do endereço-base.
 *  - val - Vetor onde vamos retornar o valor dos registos que foram pedidos para ler.
 *          O seu tamanho vai ser n_r. 
 *******************************************************************************************/

// Endereços de 1  a 65536
int Read_h_regs(char * server_addr, int port, uint32_t st_r, uint16_t n_r, uint16_t * val){
    
    // Verificação de Erros

    if(NULL == server_addr){
        printf("[ERRO 1] Read_h_regs - No server_addr\n");
        val = NULL;
        return -1;
    }
    else if((PORTO_MIN > port) || (PORTO_MAX < port)){
        printf("[ERRO 2] Read_h_regs - Connection Port invalid\n");
        val = NULL;
        return -2;
    }
    else if((ADDR_MIN > st_r) || (ADDR_MAX < st_r)){
        printf("[ERRO 3] Read_h_regs - Base Addr limit exceeded \n");
        val = NULL;
        return -3;
    }
    else if((ADDR_MAX-ADDR_MIN+1)-st_r < (uint32_t)n_r){
        printf("[ERRO 4] Read_h_regs - Nº of registers to read exceeded \n");
        val = NULL;
        return -4;
    }

    // Variáveis
    int         count,      // 
                count1,     // 
                count2;     // para iterações
    uint16_t    APDUlen,    // tamanho da APDU enviada
                APDU_Rlen;  // tamanho da APDU recebida
    uint8_t     APDU_func;  // função
    uint8_t     *APDU,      // apontador para o vetor da frame que vai ser enviada
                *APDU_R;    // apontador para o vetor da frame que vai ser recebida

    // Construção da APDU 
    APDUlen = APDU_LEN_READ_H_REGS;
    APDU_func = READ_HOLDING_REGS_FUNC;

    APDU = (uint8_t *) malloc(sizeof(uint8_t) * APDUlen);
    if(NULL == APDU){
        printf("[ERROR 6] Read_h_regs - Couldn't allocate memory \n");
        val = NULL;
        return -6;
    }

    // correção dos endereços
    st_r = st_r - 1;

    APDU[0] = (uint8_t) APDU_func;          // func é só 1 byte
    APDU[1] = (uint8_t) (st_r >> 8);        // start address -> passagem para Big Endian format
    APDU[2] = (uint8_t) (st_r & 0x00ff);    // 
    APDU[3] = (uint8_t) (n_r >> 8);         // nr of regs -> passagem para Big Endian format
    APDU[4] = (uint8_t) (n_r & 0xff);       // 

    // DEBUG
    printf("[Read_h_regs - APDU] ");
    for(count = 0; count < APDUlen; count++)
        printf("%u | ", APDU[count]);
    putchar('\n');

    // passagem à camada abaixo - Modbus TCP
    // Send_Modbus_request (server_add,port,APDU,APDUlen,APDU_R)
    if(0 > Send_Modbus_request(server_addr, port, APDU, APDUlen, APDU_R, &APDU_Rlen)){
        printf("[ERROR 7] Read_h_regs - Error in Modbus TCP layer\n");
        val = NULL;
        free(APDU);
        APDU = NULL;
        return -7;
    }

    // DEBUG - TAMANHO DA RESPOSTA
    printf("[Read_h_regs - APDU_R] ");
    for(count = 0; APDU_Rlen > count; count++)
        printf("%u | ", APDU_R[count]);
    putchar('\n');
    /*********************************/

    // checks the reponse (APDU_R or error_code)
    if(APDU_func + 0x08 == APDU_R[0]){  // hadling the error existence in the previous apdu
        switch(APDU_R[1]){
            case 0x01:                  // illegal function
                printf("[ERROR READ_H_REGS_ILLG_FUNCTION] Read_h_regs\n");
                val = NULL;
                free(APDU);
                APDU = NULL;
                free(APDU_R);
                APDU_R = NULL;
                return READ_H_REGS_ILLG_FUNCTION;
                break;

            case 0x02:                  // illegal data address
                printf("[ERROR READ_H_REGS_DATA_ADDRESS] Read_h_regs\n");
                val = NULL;
                free(APDU);
                APDU = NULL;
                free(APDU_R);
                APDU_R = NULL;
                return READ_H_REGS_DATA_ADDRESS;
                break;

            case 0x03:                  // illegal data value
                printf("[ERROR READ_H_REGS_DATA_VALUE] Read_h_regs\n");
                val = NULL;
                free(APDU);
                APDU = NULL;
                free(APDU_R);
                APDU_R = NULL;
                return READ_H_REGS_DATA_VALUE;
                break;

            default:
                printf("[ERROR READ_H_REGS_DEFAULT] Read_h_regs\n");
                val = NULL;
                free(APDU);
                APDU = NULL;
                free(APDU_R);
                APDU_R = NULL;
                return READ_H_REGS_DEFAULT;
        }
    }

    // returns: number of read regs/ coils – ok, <0 – error
    val = (uint16_t *) malloc(sizeof(uint16_t) * n_r);
    if(NULL == val){
        printf("[ERROR 8] Read_h_regs\n");
        val = NULL;
        free(APDU);
        APDU = NULL;
        free(APDU_R);
        APDU_R = NULL;   
        return -8;
    }

    count1 = 0;             // pointer in the "val" vector
    count2 = 2;             // pointer in the APDU_R
    while(count1 < n_r){
        val[count1] = (APDU_R[count2]<<8) + APDU_R[count2+1];
        count1++;
        count2+=2;
    }
    
    free(APDU);
    APDU = NULL;
    free(APDU_R);
    APDU_R = NULL;
    return n_r;
}


    /**********************************
    *          OUTRA FUNÇÃO           *
    **********************************/
   /*******************************************************************************************
 * WRITE_MULTIPLE_REGS()
 * Execute the function of write holding registers in the server.
 * Arguments description:
 *  - server_add - endereço do servidor ao qual nos vamos querer conectar;
 *  - port - porto que vai ser usado para conexão;
 *  - st_r - endereço base a partir do qual vamos querer escrever. Mesma lógica da função anterior
 *           uma vez que o servidor é o mesmo, a sua lógica de endereçamento será sempre a mesma
 *  - n_r - nr de registos que vamos querer escrever no servidor
 *  - val - vetor com os valores que queremos escrever. O seu tamanho será n_r
 *******************************************************************************************/
int Write_multiple_regs(char * server_addr, int port, uint32_t st_r, uint16_t n_r, uint16_t * val){

    // Verificação de erros

    if(NULL == server_addr){
        printf("[ERRO 1] Write_multiple registers - No server_addr\n");
        return -1;
    }
    else if((PORTO_MIN > port) || (PORTO_MAX < port)){
        printf("[ERRO 2] Write_multiple_regs - Connection Port invalid\n");
        return -2;
    }
    else if((ADDR_MIN > st_r) || (ADDR_MAX < st_r)){
        printf("[ERRO 3] Write_multiple_regs - Base Addr limit exceeded\n");
        return -3;
    }
    else if((ADDR_MAX-ADDR_MIN+1)-st_r < (uint32_t)n_r){
        printf("[ERRO 4] Write_multiple_regs - Nº of registers to read exceeded\n");
        return -4;
    }
    else if(NULL == val){
        printf("[ERRO 5] Write_multiple_regs - No structure with values to write\n");
        return -5;
    }

    // variables declaration
    int     count,      // 
            count1,     // iterativas para copiar os registos (percorre-los e copiá-los)
            count2,     // 
            reg_write;  // number of written registers
    int16_t    APDUlen,    // tamanho da APDU enviada
                APDU_Rlen;  // tamanho da APDU recebida
    uint8_t     APDU_func;  // função
    uint8_t     *APDU,      // apontador para o vetor da frame que vai ser enviada
                *APDU_R;    // apontador para o vetor da frame que vai ser recebida

    // construção da APDU (MODBUS PDU)
    APDUlen = n_r*2 + 6;  // tamanho = numero de registos *2 (cada registo são 2 bytes) + 6 bytes de cabeçalho
    // 2 para o endereço inicial, 2 para o nr de registos, 1 para o código da função e um para o campo len
    APDU_func = WRITE_MULTIPLE_REGS_FUNC;

    // Aloca espaço
    APDU = (uint8_t *) malloc(sizeof(uint8_t) * APDUlen);
    if(NULL == APDU){
        printf("[ERRO 6] Write_multiple_regs - Couldn't allocate memory\n");
        return -6;
    }

    st_r = st_r - 1;                        // conversão do endereço

    APDU[0] = (uint8_t) APDU_func;          // código da função
    APDU[1] = (uint8_t) (st_r >> 8);        //
    APDU[2] = (uint8_t) (st_r & 0x00ff);    // Big Endian para o endereço base
    APDU[3] = (uint8_t) (n_r >> 8);         // 
    APDU[4] = (uint8_t) (n_r & 0xff);       // Big Endian para o numero de registos a ler
    APDU[5] = (uint8_t) (n_r * 2);          // number of remaining bytes in frame

    count1 = 0;      // para contar (iterar) os registos
    count2 = 6;      // para iterar dentro da frame e escrever os valores em Big Endian
    while(count1 < n_r){
        APDU[count2]   = (uint8_t) (val[count1] >> 8);      
        APDU[count2+1] = (uint8_t) (val[count1] & 0x00ff);  
        count1++;
        count2+=2;
    }

    // DEBUG
    printf("[Write_multiple_regs - APDU] ");
    for(count = 0; count < APDUlen; count++)
        printf("%u | ", APDU[count]);
    putchar('\n');

    // Passa a PDU da camada AP para a camada abaixo, a Modbus TCP
    if(0 > Send_Modbus_request(server_addr, port, APDU, APDUlen, APDU_R, &APDU_Rlen)){
        printf("[ERRO 7] Write_multiple_regs - Error in the layer below (Modbus TCP)\n");
        free(APDU);
        APDU = NULL;
        return -7;
    }

    // DEBUG
    printf("[Write_multiple_regs - APDU_R] ");
    for(count = 0; APDU_Rlen > count; count++)
        printf("%u | ", APDU_R[count]);
    putchar('\n');

    // checks the reponse (APDU_R or error_code)
    if(APDU_func + 0x08 == APDU_R[0]){  // hadling the error existence in the previous apdu
        switch(APDU_R[1]){
            case 0x01:                  // illegal function
                printf("[ERROR WRITE_MULT_REGS_ILLG_FUNCTION] Write_multiple_regs\n");
                free(APDU);
                APDU = NULL;
                free(APDU_R);
                APDU_R = NULL;
                return WRITE_MULT_REGS_ILLG_FUNCTION;
                break;

            case 0x02:                  // illegal data address
                printf("[ERROR WRITE_MULT_REGS_DATA_ADDRESS] Write_multiple_regs\n");
                free(APDU);
                APDU = NULL;
                free(APDU_R);
                APDU_R = NULL;
                return WRITE_MULT_REGS_DATA_ADDRESS;
                break;

            case 0x03:                  // illegal data value
                printf("[ERROR WRITE_MULT_REGS_DATA_VALUE] Write_multiple_regs\n");
                free(APDU);
                APDU = NULL;
                free(APDU_R);
                APDU_R = NULL;
                return WRITE_MULT_REGS_DATA_VALUE;
                break;

            default:
                printf("[ERROR WRITE_MULT_REGS_DEFAULT] Write_multiple_regs\n");
                val = NULL;
                free(APDU);
                APDU = NULL;
                free(APDU_R);
                APDU_R = NULL;
                return WRITE_MULT_REGS_DEFAULT;
        }
    }

    // returns: number of written regs – ok, <0 – error
    // os dados escritos vêm na APDU de resposta nos campos ... 
    reg_write = (int)((APDU_R[3] << 8) + APDU_R[4]); 
    free(APDU);  // liberta os espaços das APDU (de envio e resposta) pois os dados já foram escritos
    APDU = NULL;
    free(APDU_R);
    APDU_R = NULL;
    return reg_write;
}
