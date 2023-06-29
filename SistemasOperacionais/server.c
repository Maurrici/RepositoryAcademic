#include <stdio.h>
#include <winsock.h>

#define PORT 8080
#define BUFFER_SIZE 1024

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
    int clientSocket, sizeData;
    int clientSocketSize = sizeof caddr;
    char buffer[BUFFER_SIZE] = {0};
    while (1)
    {
        clientSocket = accept(serverSocket, (struct sockaddr *) &caddr, &clientSocketSize);
        sizeData = recv(clientSocket, buffer, sizeof buffer, 0);

        printf("Tamanho da requisição %d\n", &sizeData);
        puts(buffer);

        char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!";

        send(clientSocket, response, strlen(response), 0);

        closesocket(clientSocket);
    }
    
    
    return 0;
}