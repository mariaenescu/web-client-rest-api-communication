#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "buffer.h"
#include "requests.h"
#include "parson.c"

#define HOST "34.246.184.49"
#define PORT  8080
#define PAYLOAD_TYPE "application/json"

#define REGISTER_ROUTE "/api/v1/tema/auth/register"
#define LOGIN_ROUTE "/api/v1/tema/auth/login"
#define ACCESS_ROUTE "/api/v1/tema/library/access"
#define BOOKS_ROUTE "/api/v1/tema/library/books"
#define ID_BOOK_ROUTE "/api/v1/tema/library/books/"
#define LOGOUT_ROUTE "/api/v1/tema/auth/logout"

/* pentru functionalitatea lui DELETE prin GET, pentru a nu avea un cod duplicat*/
#define  compute_delete_request(host, url, token) compute_get_request(1, host, url, NULL, NULL, 0, token)

/* Functie pentru eliminarea caracterului newline
din stringul dat ca parametru. */
void remove_newline(char *str)
{
    char *newline = strchr(str, '\n');
    if (newline) {
        *newline = '\0';
    }
}

/* Functie pentru printarea unui mesaj si citirea din consola. */
void print_and_read_from_stdin(char *print, char *read)
{
    printf("%s=", print);
    fgets(read, LINELEN, stdin);
    remove_newline(read);
}

/* Functie pentru citirea usernameului si a parolei pentru inregistrarea
sau logarea unui user. */
void username_password(char *username, char *password, int *error_detected)
{
    print_and_read_from_stdin("username", username);
    print_and_read_from_stdin("password", password);

    if (strlen(username) == 0 || strlen(password) == 0 ||
        strstr(username, " ") != NULL || strstr(password, " ") != NULL) {
        printf("EROARE: Introduceti un/o username/parola valid(a)!\n");
        *error_detected = 1;
    }
}

/* Functie pentru extragerea unui string dintre 2 stringuri date ca
parametri. */
char *str_between_ends(char *response, char *start, char *end)
{
    char *str = strstr(response, start);
    str = strtok(str + strlen(start) - 1, end);
    char *duplicate = strdup(str);
    return duplicate;
}

void get_message_response(int sockfd, char *url, char **cookies, char *token,
                          char **response, char **message)
{
    *message = compute_get_request(0, HOST, url, NULL, cookies, 1, token);
    send_to_server(sockfd, *message);
    *response = receive_from_server(sockfd);
}

void post_message_response(int sockfd, char *url, char *content_type,
                           char **body_data, char *token, char **message,
                           char **response)
{
    *message = compute_post_request(HOST, url, content_type, body_data, 1,
                                    NULL, 1, token);
    send_to_server(sockfd, *message);
    *response = receive_from_server(sockfd);
}

void delete_message_response(int sockfd, char *url, char *token,
                             char **message, char **response)
{
    *message = compute_delete_request(HOST, url, token);
    send_to_server(sockfd, *message);
    *response = receive_from_server(sockfd);
}

void free_memory_message_response(char *message, char *response)
{
    free(message);
    free(response);
}

/* Functie ce afiseaza tipul de mesaj in functie de raspunsul serverului la
comanda register. */
void register_response(char *response)
{
    if (strstr(response, "400 Bad Request") != NULL) {
        printf("EROARE: Username-ul acesta este deja utilizat!\n");
    } else if (strstr(response, "201 Created") != NULL) {
        printf("SUCCES: User creat!\n");
    }
}

/* Functie ce afiseaza tipul de mesaj in functie de raspunsul serverului la
comanda login, iar in caz de succes clientu va retine un cookie pentru
autentificare. */
void login_response(char *response, char **cookies)
{
    if (strstr(response, "400 Bad Request") != NULL) {
        printf("EROARE: Nu exista acest user!\n");
    } else if (strstr(response, "200 OK") != NULL) {
        printf("SUCCES: User logat!\n");
        *cookies = str_between_ends(response, "Set-Cookie: ", ";");
    } 
}

/* Functie pentru inregistrarea sau logarea unui user. */
void register_login(char **cookies, int sockfd, char *command)
{
    char username[LINELEN];
    char password[LINELEN];

    int error_detected = 0;
    username_password(username, password, &error_detected);

    if (error_detected) {
        return;
    }

    char *compose_string_pretty = NULL;
    JSON_Value *new_value = json_value_init_object();
    JSON_Object *new_object = json_value_get_object(new_value);
    
    json_object_set_string(new_object, "username", username);
    json_object_set_string(new_object, "password", password);
    compose_string_pretty = json_serialize_to_string_pretty(new_value);

    char *message, *response;
    post_message_response(sockfd, strcmp(command, "register") == 0 ?
                          REGISTER_ROUTE : LOGIN_ROUTE, PAYLOAD_TYPE,
                          &compose_string_pretty, NULL, &message, &response);
    
    if (strcmp(command, "register") == 0) {
        register_response(response);
    } else {
        login_response(response, cookies);
    }

    json_free_serialized_string(compose_string_pretty);
    json_value_free(new_value);
    free_memory_message_response(message, response);
}

/* Functie ce permite userului sa acceseze biblioteca, generand si un token
retinut de client pentru permisiunea de acces. */
void enter_library(char *cookies, char **token, int sockfd)
{
    char *message, *response;
    get_message_response(sockfd, ACCESS_ROUTE, &cookies, NULL, &response,
                         &message);
    *token = str_between_ends(response,"\"token\":\"", "\"}");

    printf("SUCCES: Utilizatorul are acces la biblioteca!\n");
    free_memory_message_response(message, response);
}

/* Functie pentru parsarea si afisarea cartilor in format JSON. */
void parse_books_to_json(char *books)
{
    JSON_Value *json_value = json_parse_string(books);
    char *compose_string_pretty = json_serialize_to_string_pretty(json_value);

    json_value_free(json_value);
    printf("%s\n", compose_string_pretty);

    free(compose_string_pretty);
}

/* Functie pentru afisarea tuturor cartilor din biblioteca in format JSON. */
void get_books(char *cookies, char *token, int sockfd)
{
    char *message, *response;
    get_message_response(sockfd, BOOKS_ROUTE, NULL, token, &response, &message);

    char *books = str_between_ends(response, "[", "\n");
    parse_books_to_json(books);

    free(books);
    free_memory_message_response(message, response);
}

/* Functie pentru adaugarea unei noi carti in biblioteca. */
void add_book(char *cookies, char *token, int sockfd)
{
    char title[LINELEN];
    char author[LINELEN];
    char genre[LINELEN];
    char publisher[LINELEN];
    char page_count[LINELEN];

    print_and_read_from_stdin("title", title);
    print_and_read_from_stdin("author", author);
    print_and_read_from_stdin("genre", genre);
    print_and_read_from_stdin("publisher", publisher);
    print_and_read_from_stdin("page_count", page_count);

    if (strlen(title) == 0 || strlen(author) == 0 || strlen(genre) == 0 ||
        strlen(publisher) == 0 || strlen(page_count) == 0 ||
        atoi(page_count) == 0 || strchr(page_count, ',') != NULL ||
        strchr(page_count, '.') != NULL) {
        printf("EROARE: Datele nu sunt valide!\n");
        return;
    }

    JSON_Value *new_value = json_value_init_object();
    JSON_Object *new_object = json_value_get_object(new_value);
    
    json_object_set_string(new_object, "title", title);
    json_object_set_string(new_object, "author", author);
    json_object_set_string(new_object, "genre", genre);
    json_object_set_string(new_object, "publisher", publisher);
    json_object_set_string(new_object, "page_count", page_count);
    char *compose_string_pretty = json_serialize_to_string_pretty(new_value);

    char *message, *response;
    post_message_response(sockfd, BOOKS_ROUTE, PAYLOAD_TYPE,
                          &compose_string_pretty, token, &message, &response);

    printf("SUCCES: Carte adaugata!\n");
    json_free_serialized_string(compose_string_pretty);
    json_value_free(new_value);

    free_memory_message_response(message, response);
}

/* Functie pentru afisarea detaliilor unei anumite carti in format JSON. */
void get_book(char *cookies, char *token, int sockfd)
{
    char id[LINELEN];
    print_and_read_from_stdin("id", id);

    if (strlen(id) == 0 || atoi(id) == 0 || strchr(id, ',') != NULL ||
        strchr(id, '.') != NULL) {
        printf("EROARE: Introduceti un id valid!\n");
        return;
    }

    char *route = malloc(strlen(ID_BOOK_ROUTE) + strlen(id) + 1);
    char *message, *response;
    get_message_response(sockfd, strcat(memcpy(route, ID_BOOK_ROUTE,
                         strlen(ID_BOOK_ROUTE) + 1), id), NULL, token,
                         &response, &message);
    
    if (strstr(response, "200 OK") != NULL) {
        char *books = str_between_ends(response, "{", "\n");
        parse_books_to_json(books);

        free(books);
    } else if (strstr(response, "404 Not Found") != NULL) {
        printf("EROARE: Cartea cu acest id=%s nu exista!\n", id);
    }
    free(route);
    free_memory_message_response(message, response);
}

/* Functie pentru stergerea unei carti din biblioteca. */
void delete_book(char *cookies, char *token, int sockfd)
{
    char id[LINELEN];
    print_and_read_from_stdin("id", id);

    if (strlen(id) == 0 || atoi(id) == 0 || strchr(id, ',') != NULL ||
        strchr(id, '.') != NULL) {
        printf("EROARE: Introduceti un id valid!\n");
        return;
    }
    char *route = malloc(strlen(ID_BOOK_ROUTE) + strlen(id) + 1);

    char *message, *response;
    delete_message_response(sockfd, strcat(memcpy(route, ID_BOOK_ROUTE,
                            strlen(ID_BOOK_ROUTE) + 1), id), token, &message,
                            &response);

    if (strstr(response, "200 OK") != NULL) {
        printf("SUCCES: Carte stearsa!\n");
    } else if (strstr(response, "404 Not Found") != NULL) {
        printf("EROARE: Cartea cu acest id=%s nu exista!\n", id);
    } 

    free(route);
    free_memory_message_response(message, response);
}

/* Functie pentru delogarea unui user. */
void logout(char **cookies, char **token, int sockfd)
{
    char *message, *response;
    get_message_response(sockfd, LOGOUT_ROUTE, cookies, *token, &response,
                         &message);
    if (strstr(response, "200 OK") != NULL) {
        printf("SUCCES: Utilizatorul s-a delogat!\n");
        *cookies = NULL;
        *token = NULL;
        free(*cookies);
        free(*token);
    }
    free_memory_message_response(message, response);
}

int main()
{
    char *login_cookie = NULL;
    char *token = NULL;
    char command[BUFLEN];
    int sockfd;
    int is_login = 0; /* starea logarii in care se afla userul */
    int permission_library = 0; /* permisiunea de acces la biblioteca */

    while (1) {
        fgets(command, BUFLEN - 1, stdin);
        remove_newline(command);

        sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

        is_login = login_cookie ? 1 : 0;
        permission_library = token ? 1 : 0;
        
        if (strcmp(command, "register") == 0) {
            if (is_login) {
                printf("%s\n", "EROARE: Nu puteti crea un cont nou daca sunteti deja conectat!");
            } else {
                register_login(&login_cookie, sockfd, command);
            }
        } else if (strcmp(command, "login") == 0) {
            if (is_login) {
                printf("%s\n", "EROARE: Esti deja logat!");
            } else {
                register_login(&login_cookie, sockfd, command);
            }
        } else if (strcmp(command, "enter_library") == 0) {
            if (is_login) {
                enter_library(login_cookie, &token, sockfd);
            } else {
                printf("EROARE: Utilizatorul trebuie sa fie autentificat pentru a accesa biblioteca!\n");
            }
        } else if (strcmp(command, "get_books") == 0) {
            if (is_login && permission_library) {
                get_books(login_cookie, token, sockfd);
            } else if (!is_login) {
                printf("EROARE: Trebuie sa va autentificati pentru a vizualiza biblioteca!\n");
            } else {
                printf("EROARE: Nu aveti acces la biblioteca!\n");
            }
        } else if (strcmp(command, "add_book") == 0) {
            if (is_login && permission_library) {
               add_book(login_cookie, token, sockfd);
            } else if (!is_login) {
                printf("EROARE: Trebuie sa va autentificati pentru a adauga o carte!\n");
            } else {
                printf("EROARE: Nu aveti acces la biblioteca!\n");
            }
        } else if (strcmp(command, "get_book") == 0) {
            if (is_login && permission_library) {
                get_book(login_cookie, token, sockfd);
            } else if (!is_login) {
                printf("EROARE: Trebuie sa va autentificati pentru a vizualiza detaliile cartii!\n");
            } else {
                printf("EROARE: Nu aveti acces la vizualizarea detaliilor despre carte!\n");
            }
        } else if (strcmp(command, "delete_book") == 0) {
            if (is_login && permission_library) {
                delete_book(login_cookie, token, sockfd);
            } else if (!is_login) {
                printf("EROARE: Trebuie sa fii autentificat pentru a sterge o carte!\n");
            } else if (is_login && !permission_library) {
                printf("EROARE: Nu aveti acces la biblioteca, deci nici la stergerea cartii!\n");
            }
        } else if (strcmp(command, "logout") == 0) {
            if (is_login) {
                logout(&login_cookie, &token, sockfd);
            } else {
                printf("EROARE: Trebuie sa fiti autentificat pentru a iesi dintr-un cont!\n");
            }
        } else if (strcmp(command, "exit") == 0) {
            close_connection(sockfd);
            break;
        } else {
            printf("EROARE: Comanda invalida!\n");
        }
    }
    free(login_cookie);
    free(token);

    return 0;
}
