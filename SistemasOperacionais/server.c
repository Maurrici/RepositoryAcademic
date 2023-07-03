#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <winsock.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_REQUEST 5
#define METHOD_SIZE 8
#define PATH_SIZE 256

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

char* getNotFound(){
    char *headerResponse = {"HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s"};
    char *bodyResponse = {"<h1>404 - Not found</h1>"};

    char *response = (char *)malloc(strlen(headerResponse) + strlen(bodyResponse) + 1);
    if(response == NULL){
        perror("Erro ao alocar espaço 3");
        exit(EXIT_FAILURE);
    }
    
    sprintf(response, headerResponse, strlen(bodyResponse), bodyResponse);

    return response;
}

char* getBadRequest(){
    char *headerResponse = {"HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s"};
    char *bodyResponse = {"<h1>400 - Bad request</h1>"};

    char *response = (char *)malloc(strlen(headerResponse) + strlen(bodyResponse) + 1);
    if(response == NULL){
        perror("Erro ao alocar espaço 3");
        exit(EXIT_FAILURE);
    }
    
    sprintf(response, headerResponse, strlen(bodyResponse), bodyResponse);

    return response;
}

char* getFileData(char *path){
    // Header da resposta
    char *headerResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\nContent-Length: %ld\r\n\r\n%s";
    char headerSize = strlen(headerResponse);

    // Obtém o arquivo a ser devolvido
    FILE *file = fopen(path, "rb");
    if(file == NULL){
        return NULL;
    }
    
    // Calcula tamanho do arquivo
    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    rewind(file);
    
    // Prepara o body da resposta
    char *bodyResponse = malloc(fileSize);
    if(bodyResponse == NULL){
        perror("Erro ao alocar espaço 1");
        exit(EXIT_FAILURE);
    }
    
    if(fread(bodyResponse, 1, fileSize, file) != fileSize){
        perror("Erro durante a leitura do arquivo 2");
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
    response[headerSize + fileSize] = '\0';
    free(bodyResponse);
    
    return response;
}

int getRequestDetails(char *request, char *method, char *path) {
    const char *space = strchr(request, ' ');
    if (space == NULL) {
        printf("Erro request\n");
        return -1;
    }

    // Obtém o método
    size_t methodSize = space - request;
    strncpy(method, request, methodSize);
    method[methodSize] = '\0';

    const char *second_space = strchr(space + 1, ' ');
    if (second_space == NULL) {
        printf("Erro request\n");
        return -1;
    }

    // Obtém o path
    size_t pathSize = second_space - (space + 1);
    strncpy(path, space + 1, pathSize);
    path[pathSize] = '\0';

    if (path[0] == '/') {
        memmove(path, path + 1, pathSize);
        pathSize--;
        path[pathSize] = '\0';
    }

    return 0;
}

void *handle_request(void *args){
    int sizeData;
    ThreadArgs threadArgs = *(ThreadArgs *) args;

    char buffer[BUFFER_SIZE] = {0};
    sizeData = recv(threadArgs.socket, buffer, sizeof buffer, 0);
    //puts(buffer);
    printf("\n");

    char method[METHOD_SIZE];
    char path[PATH_SIZE];

    if(getRequestDetails(buffer, method, path) == -1){
        char *response400 = getBadRequest();
        send(threadArgs.socket, response400, strlen(response400), 0);
        free(response400);
        printf("Fechando thread %d\n", threadArgs.id);
        closesocket(threadArgs.socket);
        threads[threadArgs.id] = -1;
        return;
    }

    printf("Metodo: %s\n", method);
    printf("Path: %s\n", path);

    if(strncmp(method, "GET", 3) == 0){
        if(strncmp(path, "", 1) == 0){
            char *response = getIndexHTML();
            send(threadArgs.socket, response, strlen(response), 0);
            free(response);
        }else{
            char *response = getFileData(path);
            if(response == NULL){
                char *response404 = getNotFound();
                send(threadArgs.socket, response404, strlen(response404), 0);
                free(response404);
            }else{
                send(threadArgs.socket, response, strlen(response), 0);
            }
            free(response);
        }
    }
    
    printf("Fechando thread %d\n", threadArgs.id);
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

        // Repassa requisição para uma thread
        printf("Criando thread %d\n", aux_thread);
        int result = pthread_create(&threads[aux_thread], NULL, handle_request, &args);
        if (result != 0) {
            printf("Erro ao criar a thread %d. Código de erro: %d\n", aux_thread, result);
            exit(EXIT_FAILURE);
        }  
    }

    printf("Servidor fechado");
    closesocket(serverSocket);
    WSACleanup();
    
    return 0;
}