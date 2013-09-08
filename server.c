/*
*
    Fix: server write information about connected clients into database 
    Build:
        gcc -o server server.c `mysql_config --cflags --libs`

**/

#include <stdio.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <my_global.h>
#include <mysql.h>
#include <pthread.h>

#define BUFFER_SIZE 100
#define MAXIMUM_CLIENT_CONNECTION 100

// Client information will send to server
typedef struct client_data
{
    char HostName[50];
    char MacAddress[20];
    char TimeConnectServer[20];
    char LocalIPAddress[10];

} CLIENT_DATA;


char *database_servername = "localhost";
char *database_username   = "root";
char *database_password   = "cxphong";
char *database_name       = "boards";
char *table_name          = "panda_board_connecting";

/*
 * This will handle connection for each client
 * */

void *connection_handler (void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    char *onoff = NULL;
    char previous_value[5];
    char recbuf[BUFFER_SIZE], sendbuf[BUFFER_SIZE], mySQLcommand[BUFFER_SIZE];
    char *temp = NULL;
    char *pch = NULL;
    CLIENT_DATA clientData;

    memset(sendbuf,'\0',sizeof(sendbuf));
    memset(recbuf,'\0',sizeof(recbuf));
    memset(mySQLcommand,'\0',sizeof(mySQLcommand));
    memset(previous_value,'\0',sizeof(previous_value));
    memset(clientData.HostName,'\0',sizeof(clientData.HostName));
    memset(clientData.MacAddress,'\0',sizeof(clientData.MacAddress));
    memset(clientData.TimeConnectServer,'\0',sizeof(clientData.TimeConnectServer));
    memset(clientData.LocalIPAddress,'\0',sizeof(clientData.LocalIPAddress));
    
    // Receive client's message
    if (-1 == read(sock, recbuf , sizeof(recbuf)))
    {
        fprintf(stderr,"Receive failed!\n");
        return;
    }

    puts(recbuf);

    // Export recbuf
    temp = recbuf;

    pch = strchr (temp,'&');
    strncpy (clientData.HostName, temp, pch - temp);
   

    temp = pch + 1;
    pch = strchr (pch + 1,'&');
    strncpy (clientData.MacAddress, temp, pch - temp);
    

    temp = pch + 1;
    strcpy (clientData.TimeConnectServer, temp);
    
    printf("hostname = %s\n", clientData.HostName);
    printf("Mac = %s\n", clientData.MacAddress);
    printf("date = %s\n", clientData.TimeConnectServer);


    // Connect to the database
    MYSQL *con = mysql_init(NULL);

    if (con == NULL) 
    {
        fprintf(stderr, "%s\n", mysql_error(con));
        return;
    }

    if ( mysql_real_connect(con, database_servername, database_username, database_password,
         database_name, 0, NULL, 0) == NULL) 
    {
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        return;
    }

    // Write client information into database
    sprintf(mySQLcommand,"INSERT INTO %s (ID, HostName, TimeStartConnect) VALUES('%s', '%s', '%s')", 
           table_name, clientData.MacAddress, clientData.HostName, clientData.TimeConnectServer);

    if (mysql_query(con, mySQLcommand)) 
    {
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        return;
    }

    // Always check the database
    while (1)
    {
        // Read data from mysql database
        memset(mySQLcommand,'\0',sizeof(mySQLcommand));
        sprintf(mySQLcommand,"SELECT * FROM %s", table_name); 
       
        if (mysql_query(con, mySQLcommand)) 
        {
            fprintf(stderr, "%s\n", mysql_error(con));
            mysql_close(con);
            return;
        }

        MYSQL_RES *result = mysql_store_result(con);
        int num_fields = mysql_num_fields(result);
        MYSQL_ROW row;
        int i;
      
        while ((row = mysql_fetch_row(result))) 
        { 
            for(i = 0; i < num_fields; i++) 
            { 
                //printf("%s ", row[i] ? row[i] : "NULL"); 
                onoff = row[3];
                //printf("%s\n", onoff);
            } 
            //printf("\n"); 
        }
      
        mysql_free_result(result);
        
        temp = "";
        // Decide turn on/off led
        if (onoff != NULL)
        {
            if ( !strcmp(onoff,"Off") )
            {
                temp = "0"; // off
            }
            else
            {
                temp = "1"; // on
            }
        }

        // Only send to client when client choose the different value
        if (strcmp(previous_value,temp))
        {
            printf("temp = %s\n", temp);
            strncpy(sendbuf, temp, strlen(temp));
            if(-1 == send(sock , sendbuf, sizeof(sendbuf), 0))
            {
                fprintf(stderr,"Client is now disconnected!\n");

                // Delete client's information out of the database
                memset(mySQLcommand,'\0',sizeof(mySQLcommand));
                sprintf(mySQLcommand,"DELETE FROM %s WHERE ID='%s'", table_name, clientData.MacAddress); 
               
                if (mysql_query(con, mySQLcommand)) 
                {
                    fprintf(stderr, "%s\n", mysql_error(con));
                    mysql_close(con);
                    return;
                }

                return;
            }

            //printf("temp = %s\n", temp);
            //printf("previous_value = %s\n", previous_value);
            puts("Sent!");
            memcpy(previous_value,temp,strlen(temp));
        }

    }
    
    free(socket_desc);
     
    return;
}

// Main function
int main (int argc , char *argv[])
{
    int socket_desc , new_socket, *new_sock;
    struct sockaddr_in server , client;
    socklen_t sock_size;

    // Server always run
    while (1)
    {
        //Create socket
        socket_desc = socket (AF_INET , SOCK_STREAM , 0);
        if (-1 == socket_desc)
        {
            fprintf(stderr,"Could not create socket!\n");
            continue;
        }
         
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(8888);
         
        //Bind
        while (bind (socket_desc,(struct sockaddr *)&server , sizeof(server)))
        {
            printf("Binding ... !\n");
            sleep(1);
        }

        puts ("Bind done!");
         
        //Listen
        listen (socket_desc , MAXIMUM_CLIENT_CONNECTION);
         
        //Accept and incoming connection
        puts("Waiting for incoming connections...");
        sock_size = sizeof(struct sockaddr_in);

        // Multiple threads processing
        while (1)
        {
            new_socket = accept(socket_desc, (struct sockaddr *)&client, &sock_size);

            if (new_socket < 0)
            {
                fprintf(stderr,"Accept failed!\n");
                break;
            }

            // Create new thread
            pthread_t sniffer_thread;
            new_sock = malloc(1);
            *new_sock = new_socket;
             
            if ( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void* ) new_sock) < 0)
            {
                fprintf(stderr, "Could not create thread!");
                break;
            }
             
            puts("Handler assigned");
        }
    }

    close(socket_desc);
    close(new_socket);
     
    return 0;
}
