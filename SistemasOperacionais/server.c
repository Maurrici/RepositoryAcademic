#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <winsock.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_REQUEST 5

// ------------- THREADS -------------------- 
pthread_t threads[5] = {-1};
int aux_thread = 0;
typedef struct {
    int socket;
    int id;
} ThreadArgs;

char* getIndexHTML(){
    // Header da resposta
    char *headerResponse = {"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s"};
    char headerSize = strlen(headerResponse);

    // Obtém o arquivo a ser devolvido
    FILE *file = fopen("index.html", "r");
    if(file == NULL){
        perror("Erro ao abrir o arquivo");
        exit(EXIT_FAILURE);
    }

    // Calcula tamanho do arquivo
    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    rewind(file);
    printf("Tamanho do arquivo: %d", fileSize);
    // Prepara o body da resposta
    char *bodyResponse = malloc(fileSize);
    if(bodyResponse == NULL){
        perror("Erro ao alocar espaço 1");
        exit(EXIT_FAILURE);
    }

    if(fread(bodyResponse, 1, fileSize, file) != fileSize){
        perror("Erro durante a leitura do arquivo");
        exit(EXIT_FAILURE);
    }
    fclose(file);

    // Monta response
    char *response = (char *)malloc(headerSize + fileSize + 1);
    if(response == NULL){
        perror("Erro ao alocar espaço 2");
        exit(EXIT_FAILURE);
    }

    sprintf(response, headerResponse, fileSize, bodyResponse);
    free(bodyResponse);
    
    return response;
}

void *handle_request(void *args){
    int sizeData;
    ThreadArgs threadArgs = *(ThreadArgs *) args;

    char buffer[BUFFER_SIZE] = {0};

    sizeData = recv(threadArgs.socket, buffer, sizeof buffer, 0);
    puts(buffer);
    
    char *response = getIndexHTML();

    send(threadArgs.socket, response, strlen(response), 0);

    free(response);

    closesocket(threadArgs.socket);
    threads[threadArgs.id] = -1; // Libera espaço para o servidor receber uma nova request
}
// -------------------------------------------

int main(){
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 0), &wsa);

    // ------------- SOCKET DO SERVIDOR -------------
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
    listen(serverSocket, MAX_REQUEST);
    // ----------------------------------------------

    struct sockaddr_in caddr;
    int clientSocket;
    int clientSocketSize = sizeof caddr;

    while (1)
    {
        clientSocket = accept(serverSocket, (struct sockaddr *) &caddr, &clientSocketSize);
        
        // Verifica disponibilidade do servidor
        while(threads[aux_thread] != -1){
            aux_thread = aux_thread < MAX_REQUEST - 1 ? aux_thread : 0; 
        }

        ThreadArgs args = {
            .id = aux_thread,
            .socket = clientSocket
        };

        int result = pthread_create(&threads[aux_thread], NULL, handle_request, &args);
        if (result != 0) {
            printf("Erro ao criar a thread %d. Código de erro: %d\n", aux_thread, result);
            exit(EXIT_FAILURE);
        }  
    }
    printf("Saiu do while");
    closesocket(serverSocket);
    WSACleanup();
    
    return 0;
}