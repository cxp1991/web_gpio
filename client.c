#include <stdio.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <arpa/inet.h> //inet_addr
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 100

// Client information will send to server
typedef struct client_data
{
	char HostName[50];
	char MacAddress[20];
	char TimeConnectServer[20];
	char LocalIPAddress[10];

} CLIENT_DATA;

// Get hostname
char* getyourhostname()
{
  char *ret = malloc(20);
  gethostname(ret,20);
  return ret;
}

// Get Mac address
char *getmac(char *iface)
{
	#define MAC_STRING_LENGTH 13
  	char *ret = malloc(MAC_STRING_LENGTH);
  	struct ifreq s;
  	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

	strcpy(s.ifr_name, iface);
	if (fd >= 0 && ret && 0 == ioctl(fd, SIOCGIFHWADDR, &s))
	{
	int i;
	for (i = 0; i < 6; ++i)
	  snprintf(ret+i*3,MAC_STRING_LENGTH-i*2+1,"%02x:",(unsigned char) s.ifr_addr.sa_data[i]);
	}
	else
	{
	perror("malloc/socket/ioctl failed");
	exit(1);
	}

	ret[strlen(ret) -1] = '\0';
	return(ret);
}

// Get current time
char* getCurrentTime(void)
{
    char *ret = malloc(20);
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    sprintf(ret, "%d:%d:%d %d/%d/%d", tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
    return ret; 
}

// Get local IP address
char* getIPAddress(void)
{
    int fd;
    struct ifreq ifr;
    char* ret = malloc (20);

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want IP address attached to "eth0" */
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    /* display result */
    sprintf(ret, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    return ret;
}

int main(int argc , char *argv[])
{
	char message[BUFFER_SIZE] , server_reply[BUFFER_SIZE], client_information[BUFFER_SIZE];
	int socket_desc;
	struct sockaddr_in server;
	CLIENT_DATA clientData;
	char *serverAddress = "192.168.7.1";
	int  serverPort = 8888;

	memset(message,'\0',sizeof(message));
	memset(server_reply,'\0',sizeof(server_reply));
	memset(client_information,'\0',sizeof(client_information));
	memset(clientData.HostName,'\0',sizeof(clientData.HostName));
	memset(clientData.MacAddress,'\0',sizeof(clientData.MacAddress));
	memset(clientData.TimeConnectServer,'\0',sizeof(clientData.TimeConnectServer));
	memset(clientData.LocalIPAddress,'\0',sizeof(clientData.LocalIPAddress));

	while (1)
	{
		// Get client information
		strcpy(clientData.HostName, getyourhostname());
		strcpy(clientData.MacAddress, getmac("eth0"));
		strcpy(clientData.TimeConnectServer, getCurrentTime());
		//strcpy(clientData.LocalIPAddress, getIPAddress());

		strcat(client_information, clientData.HostName);
		strcat(client_information,"&");
		strcat(client_information, clientData.MacAddress);
		strcat(client_information,"&");
		strcat(client_information, clientData.TimeConnectServer);
		//strcat(client_information,"&");
		//strcat(client_information, clientData.LocalIPAddress);
		//strcat(client_information,"&");
		puts(client_information);

		//Create socket
		socket_desc = socket (AF_INET , SOCK_STREAM , 0);
		if (-1 == socket_desc) 
		{
			fprintf (stderr,"Could not create socket!\n");
			return -1;
		}
		 
		server.sin_addr.s_addr = inet_addr (serverAddress);
		server.sin_family = AF_INET;
		server.sin_port = htons (serverPort);

		//Connect to remote server
		while (1)
		{
			printf("Connecting to server ...\n");
			if (0 == connect(socket_desc , (struct sockaddr *)&server , sizeof(server)))
			{
				//fprintf(stderr,"Connect failed\n");
				//return -1;
				break;
			}

			sleep(1);
			//puts("Connected");
		}

		//Send some data
		strncpy(message, client_information, strlen(client_information));
		if(-1 == write(socket_desc , message , sizeof(message)))
		{
			fprintf(stderr,"Send failed!");
			return -1;
		}

		// Receive from server then decide turn on/off led    
		while (1)
		{
			// Receive a reply from the server
			if(recv (socket_desc, server_reply , sizeof (server_reply) , 0) <= 0)
			{	
				fprintf (stderr,"Receive failed!");
				//return -1;
				break;
			}

			puts (server_reply);

			// Control gpios
			if (!strcmp (server_reply,"0"))
				system ("echo 0 > /sys/devices/virtual/gpio/gpio67/value");
			else if (!strcmp(server_reply,"1"))
				system ("echo 1 > /sys/devices/virtual/gpio/gpio67/value");
			else
				printf ("Wrong value! Not [0/1] value!\n");
		}
		
	   	close (socket_desc);
    }
    
    return 0;
}
