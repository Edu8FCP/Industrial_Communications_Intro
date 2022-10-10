// Para testar abrimos duas janelas (terminais) para corrermos o cliente e o servidor

// client
// gcc client.c -o client
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <stdlib.h> // para usar o fgets
#include <arpa/inet.h> // vi no google para corrigir o erro: warning: implicit declaration of function ‘inet_addr’
#include <unistd.h> // vi no google para corrigir o erro: warning: implicit declaration of function ‘close’; did you mean ‘pclose’?
#include "mbap_layer.h"
#include "mbtcp_layer.h"

#define SERVER_ADDR "127.0.0.1" // LOCAL LOOPBACK - PERMITE ÀS COMUNICAÇÕES INTERNAS DA MÁQUINA
// COMUNICAREM ENTRE SI SEM TEREM DE PASSAR POR TODA A PILHA PROTOCOLAR 
#define SERVER_PORT 5502  // Qualquer porta acima de 1024? Acho eu
#define IN_BUF_LEN 100

int main(){

    printf("Client waked up! Will try to create socket and establish a connection!\n");
 
    /****************************************
    * DECLARAÇÃO das estruturas necessárias *
    ****************************************/

    int sock, len, in; // estrutura para guardar o inteiro (file descriptor) que o socket gera
    struct sockaddr_in serv; //para lidar com o IPV4 mas depois fazemos cast - TEM 3 CAMPOS
    // address family, port in network byte order, internet address
    socklen_t addlen = sizeof(serv); // mede o tamanho da estrutura sockaddr_in 
    char buf[IN_BUF_LEN]; // buffer para os dados
    char in_buf[IN_BUF_LEN];

    /**************************
    *      CRIAR SOCKET       *
    ***************************/
    
    // PF_INET para Internet, SOCK_STREAM para o TCP, 0 = IPPROTO_TCP
    sock = socket(PF_INET, SOCK_STREAM, 0); // protocolo de utilização, e tipo (stream)
    if(sock){
        printf("Socked created with success...\n \tAF_INET, 127.0.0.1, port 5502\n");
    } else{
        printf("Socket not created...\n");
    }


    // Preparar a estrutura - 3 params
    serv.sin_family = AF_INET;
    serv.sin_port = htons(SERVER_PORT); // host to network short
    // inet_aton(SERVER_ADDR, &sad_loc.sin_addr) ; // ACHO QUE FAZ O MESMO QUE A LINHA SEGUINTE
    // o que faz o INET_ATON? Converte de network para dotted decimal <<<<----
    serv.sin_addr.s_addr = inet_addr(SERVER_ADDR); // ver este na documentação
    


    // CONECTAR ao servidor remoto
    // É sempre necessário enviar o tamanho, pois os sockets servem vários protocolos e os tamanhos podem diferir
    // socket - fd associado ao socket; address e address_len
    if(connect(sock, (struct sockaddr *) &serv, addlen)){
        printf("Connection established with success... \n");
    } else printf("Connection not established... \n");



    // Podíamos fazer bind do client se quiséssemos ligar o cliente a um porto específico

    /********************************
    *  BIDIRECTIONAL COMMUNICATION  *
    ********************************/

    // Enviar dados para o servidor
    printf("Waiting for data ... ");
    scanf("%s", &buf); // com scanf só estava a ler até ao primeiro espaço -> faltava o endereço no buf
    // fgets(buf, IN_BUF_LEN, stdin);
    buf[strlen(buf)] = '\0';
    len = send(sock, buf, strlen(buf)+1, 0); // Retorna o nr de bytes enviados
    if(len == strlen(buf)+1){
        printf("All data was sent...\n");
    } else if (len == -1){
        printf("ERRO - Data can't be sent...\n");
    }
    
    // Receber a resposta do servidor
    bzero(in_buf, IN_BUF_LEN); // colocamos o buffer a zeros
    
    in = recv(sock, in_buf, IN_BUF_LEN, 0); // O 4 param diz-nos como queremos usar a func receive
    if( in < 0)
    {
        printf("Recv failer...\n");
        return -1;
    }
    else    
        printf("Received data (%d bytes): %s\n", in, in_buf);

    // Fechar cliente
    close(sock);

    return 0;
}