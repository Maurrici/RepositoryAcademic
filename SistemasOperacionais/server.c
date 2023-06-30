#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <winsock.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Threads
pthread_t threads[5] = {-1};
int aux_thread = 0;

int main(){
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 0), &wsa);

    // SOCKET DO SERVIDOR
    // Configuração
    struct sockaddr_in saddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(PORT)
    };

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Ligação com a porta
    if(bind(serverSocket, (struct sockaddr *) &saddr, sizeof saddr) < 0){
        perror("Erro na ligação do socket");
        exit(EXIT_FAILURE);
    }

    // Permição de conexões
    listen(serverSocket, 5);

    struct sockaddr_in caddr;
    int clientSocket;
    int clientSocketSize = sizeof caddr;

    while (1)
    {
        clientSocket = accept(serverSocket, (struct sockaddr *) &caddr, &clientSocketSize);
        
        while(threads[aux_thread] != -1){
            aux_thread = aux_thread < 4 ? aux_thread : 0; 
        }

        int result = pthread_create(&threads[aux_thread], NULL, handle_request, &clientSocket);
        if (result != 0) {
            printf("Erro ao criar a thread %d. Código de erro: %d\n", aux_thread, result);
            exit(EXIT_FAILURE);
        }  
    }
    
    closesocket(serverSocket);
    WSACleanup();
    
    return 0;
}

int *handle_request(void *args){
    int sizeData;
    int clientSocket = *(int *) args;
    char buffer[BUFFER_SIZE] = {0};

    sizeData = recv(clientSocket, buffer, sizeof buffer, 0);

    printf("Tamanho da requisição %d\n", &sizeData);
    puts(buffer);

    char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!";

    send(clientSocket, response, strlen(response), 0);

    closesocket(clientSocket);
}