#include <stdio.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <my_global.h>
#include <mysql.h>

#define BUFFER_SIZE 100

int main (int argc , char *argv[])
{
    char recbuf[BUFFER_SIZE], sendbuf[BUFFER_SIZE];
    char *temp = NULL;
    int socket_desc , new_socket;
    struct sockaddr_in server , client;
    socklen_t sock_size;
    char *onoff = NULL;
    char previous_value[5];
    
    
    memset (recbuf,'\0',sizeof(recbuf));
    memset (sendbuf,'\0',sizeof(sendbuf));
    memset (previous_value,'\0',sizeof(previous_value));

    //Create socket
    socket_desc = socket (AF_INET , SOCK_STREAM , 0);
    if (-1 == socket_desc)
    {
        fprintf(stderr,"Could not create socket!\n");
        return -1;
    }
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);
     
    //Bind
    if (bind (socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        fprintf (stderr,"Bind failed!\n");
        return -1;
    }

    puts ("Bind done!");
     
    //Listen
    listen (socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    sock_size = sizeof(struct sockaddr_in);
    new_socket = accept(socket_desc, (struct sockaddr *)&client, &sock_size);
    if (new_socket < 0)
    {
        fprintf(stderr,"Accept failed!\n");
        return -1;
    }
    
    //Receive client's message
    if(-1 == read(new_socket, recbuf , sizeof(recbuf)))
    {
	   fprintf(stderr,"Receive failed!\n");
	   return -1;
    }
    
    puts(recbuf);
     
    //Connect to mysql database
    MYSQL *con = mysql_init(NULL);

    if (con == NULL) 
    {
        fprintf(stderr, "%s\n", mysql_error(con));
        exit(1);
    }

    if ( mysql_real_connect(con, "localhost", "root", "cxphong", "boards", 0, NULL, 0) == NULL) 
    {
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        exit(1);
    }

    //Reply to the client. Turn on/off led.
    while (1)
    {
        /*
    	printf("\n");
    	printf("Enter your value [0/1]: \n");
    	scanf("%d", &onoff);
        */

        // Read data from mysql database
        if (mysql_query(con, "SELECT * FROM panda_board_connecting")) 
        {
            fprintf(stderr, "%s\n", mysql_error(con));
            mysql_close(con);
            exit(1);
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
                onoff = row[1];
                //printf("%s\n", onoff);
            } 
            //printf("\n"); 
        }
      
        mysql_free_result(result);

        // Decide turn on/off led
    	if ( !strcmp(onoff,"Off") )
    	{
    		temp = "0"; // off
    	}
    	else
    	{
    		temp = "1"; // on
    	}

        // Omly send to client when client choose the different value
        if (strcmp(previous_value,temp))
        {
            strncpy(sendbuf, temp, strlen(temp));
            if(-1 == send(new_socket , sendbuf, sizeof(sendbuf), 0))
            {
                fprintf(stderr,"Send failed!\n");
                return -1;
            }

            printf("temp = %s\n", temp);
            printf("previous_value = %s\n", previous_value);
            puts("Sended!");
            memcpy(previous_value,temp,strlen(temp));
        }
    }

    close(socket_desc);
    close(new_socket);
     
    return 0;
}
