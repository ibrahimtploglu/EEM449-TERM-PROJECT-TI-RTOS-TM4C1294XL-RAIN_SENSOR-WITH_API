
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// XDCtools Header files
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

/* TI-RTOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Idle.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/drivers/GPIO.h>
#include <ti/net/http/httpcli.h>

#include "Board.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#define HOSTNAME          "api.weatherbit.io"
#define REQUEST_URI       "/v2.0/current?lat=38.67742856350678&lon=26.75304690100679&units=M&lang=en&key=721684a7b7e64d45b4938b0fc2127efa"
#define USER_AGENT        "HTTPCli (ARM; TI-RTOS)"
#define SOCKETTEST_IP     "192.168.1.22"//PC_IP
#define TASKSTACKSIZE      4096
#define OUTGOING_PORT      5011//SERVER PORT
#define SERVER_TÝME_IP    "128.138.140.44"//NTP SERVER


extern Mailbox_Handle mailbox0;
extern Semaphore_Handle semaphore1;



char   precipstr[20];                     // precipitation string
char   timestr;
char   Time_transfer[20];
unsigned long int Time_keep;
char* buf ;
char Time_out[50];
/*
 *  ======== printError ========
 */
void printError(char *errString, int code)
{
    System_printf("Error! code = %d, desc = %s\n", code, errString);
    BIOS_exit(code);
}

bool sendData2Server(char *serverIP, int serverPort, char *data, int size)
{
    int sockfd, connStat, numSend;
    bool retval=false;
    struct sockaddr_in serverAddr;

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) {
        System_printf("Socket not created");
        close(sockfd);
        return false;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));  // clear serverAddr structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);     // convert port # to network order
    inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr));

    connStat = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if(connStat < 0) {
        System_printf("sendData2Server::Error while connecting to server\n");
    }
    else {
        numSend = send(sockfd, data, size, 0);       // send data to the server
        if(numSend < 0) {
            System_printf("sendData2Server::Error while sending data to server\n");
        }
        else {
            retval = true;      // we successfully sent the precipitation string
        }
    }
    System_flush();
    close(sockfd);
    return retval;
}






void  TimeFromNetworkTimeProtocol(char *serverIP, int serverPort, char *data, int size)
{
        System_printf("TimeFromNetworkTimeProtocol start\n");
        System_flush();

        int sockfd, connStat, tri;
       // char buf[80];
        struct sockaddr_in serverAddr;

        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd == -1) {
            System_printf("Socket not created");
            BIOS_exit(-1);
        }

        memset(&serverAddr, 0, sizeof(serverAddr));  // clear serverAddr structure
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(37);     // convert port # to network order
        inet_pton(AF_INET, SERVER_TÝME_IP , &(serverAddr.sin_addr));

        connStat = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        if(connStat < 0) {
            System_printf("sendData2Server::Error while connecting to server\n");
            if(sockfd>0) close(sockfd);
            BIOS_exit(-1);
        }

        tri = recv(sockfd, Time_transfer, sizeof(Time_transfer), 0);
        if(tri < 0) {
            System_printf("Error while receiving data from server\n");
            if (sockfd > 0) close(sockfd);
            BIOS_exit(-1);
        }

        if (sockfd > 0) close(sockfd);

        Time_keep = Time_transfer[0]*16777216 +  Time_transfer[1]*65536 + Time_transfer[2]*256 + Time_transfer[3];
        Time_keep += 10800;
        buf = ctime(&Time_keep);//the current time is calculated

        uint8_t i;
        for(i=0;i<27;i++){

          Time_out[i]=buf[i];//

         }

}



Void clientSocketTask(UArg arg0, UArg arg1)
{
    char ptr4;
    char data[] = "\f!!!BE CAREFUL!  THE WEATHER IS RAINY!!!    (SENSÖR VE HAVA DURUMU YAÐIÞLI)\r\n";
    char data2[] = "\f***THE WEATHER MAY BE RAINY***     (HAVA DURUMUNDA YAÐIÞ YOK(FAKAT SENSÖR YAÐIÞLI))\r\n";
    while(1) {

        // when precipitation string is retrieved from https://api.weatherbit.io site
        //TimeFromNetworkTimeProtocol updating

       TimeFromNetworkTimeProtocol(SERVER_TÝME_IP, 37,Time_out, strlen(Time_out));

        Mailbox_pend(mailbox0, &ptr4, BIOS_WAIT_FOREVER);//"precipstr" taken from the api is processed here.
        System_printf("mailbox-----> %s \n", &ptr4 );
        System_flush();

        GPIO_write(Board_LED0, 1); // turn on the LED

        // connect to SocketTest program on the system with given IP/port


        if (strcmp(&ptr4,"0")!=0){
           sendData2Server(SOCKETTEST_IP, OUTGOING_PORT, Time_out, strlen(Time_out));//time data is printed to the server

               if(sendData2Server(SOCKETTEST_IP, OUTGOING_PORT, data, strlen(data))) //information of precipitation is printed to the server
               {
                    System_printf("clientSocketTask:: precipitation is sent to the server\n");
                    System_flush();
               }

           System_printf("Hava Durumu--------->Yaðýþ var(APÝ------->RAINY\n");
           System_flush();

         }

         else
         {
           sendData2Server(SOCKETTEST_IP, OUTGOING_PORT, Time_out, strlen(Time_out));//time data is printed to the server

                if(sendData2Server(SOCKETTEST_IP, OUTGOING_PORT, data2, strlen(data2)))//information of precipitation is printed to the server
                {
                       System_printf("clientSocketTask:: precipitation is sent to the server\n");
                       System_flush();
                }


             System_printf("Hava Durumu-----> Yaðýþ yok(APÝ------>NO RAIN)\n");
             System_flush();
          }



       GPIO_write(Board_LED0, 0);  // turn off the LED
    }
}



/*
 *  ======== httpTask ========
 *  Makes a HTTP GET request
 */
Void httpTask(UArg arg0, UArg arg1)
{
    bool moreFlag = false;
    char data[64], *s1, *s2;
    int ret, precip_received=0, len;
    struct sockaddr_in addr;

    HTTPCli_Struct cli;
    HTTPCli_Field fields[3] = {
        { HTTPStd_FIELD_NAME_HOST, HOSTNAME },
        { HTTPStd_FIELD_NAME_USER_AGENT, USER_AGENT },
        { NULL, NULL }
    };

    while(1) {

        System_printf("Sending a HTTP GET request to '%s'\n", HOSTNAME);
        System_flush();

        HTTPCli_construct(&cli);

        HTTPCli_setRequestFields(&cli, fields);

        ret = HTTPCli_initSockAddr((struct sockaddr *)&addr, HOSTNAME, 0);
        if (ret < 0) {
            HTTPCli_destruct(&cli);
            System_printf("httpTask: address resolution failed\n");
            continue;
        }

        ret = HTTPCli_connect(&cli, (struct sockaddr *)&addr, 0, NULL);
        if (ret < 0) {
            HTTPCli_destruct(&cli);
            System_printf("httpTask: connect failed\n");
            continue;
        }

        ret = HTTPCli_sendRequest(&cli, HTTPStd_GET, REQUEST_URI, false);
        if (ret < 0) {
            HTTPCli_disconnect(&cli);
            HTTPCli_destruct(&cli);
            System_printf("httpTask: send failed");
            continue;
        }

        ret = HTTPCli_getResponseStatus(&cli);
        if (ret != HTTPStd_OK) {
            HTTPCli_disconnect(&cli);
            HTTPCli_destruct(&cli);
            System_printf("httpTask: cannot get status");
            continue;
        }

        System_printf("HTTP Response Status Code: %d\n", ret);

        ret = HTTPCli_getResponseField(&cli, data, sizeof(data), &moreFlag);
        if (ret != HTTPCli_FIELD_ID_END) {
            HTTPCli_disconnect(&cli);
            HTTPCli_destruct(&cli);
            System_printf("httpTask: response field processing failed\n");
            continue;
        }

        len = 0;
        do {
            ret = HTTPCli_readResponseBody(&cli, data, sizeof(data), &moreFlag);
            if (ret < 0) {
                HTTPCli_disconnect(&cli);
                HTTPCli_destruct(&cli);
                System_printf("httpTask: response body processing failed\n");
                moreFlag = false;
            }
            else
            {    // string is read correctly
                s1=strstr(data,"precip");// find "precip:" string

                if(s1)
                {
                   if(precip_received) continue;     // precip is retrieved before, continue
                    // is s1 is not null i.e. "precip" string is found
                    // search for comma
                    s2=strstr(s1, ",");

                    if(s2)
                    {
                        *s2=0;                      // put end of string
                        strcpy(precipstr, s1+8);     // copy the string
                        precip_received = 1;
                    }
                }
            }

            len += ret;     // update the total string length received so far
        } while (moreFlag);

        System_printf("Received %d bytes of payload\n", len);
        System_printf("precipitation %s\n", precipstr);
        System_flush();

        HTTPCli_disconnect(&cli);                               // disconnect from weatherbit
        HTTPCli_destruct(&cli);

        Semaphore_pend(semaphore1, BIOS_WAIT_FOREVER);// precipitation data is expected from the sensor
        Mailbox_post(mailbox0, precipstr, BIOS_NO_WAIT);//"precipstr" posted in clientsockettask();



        Task_sleep(5000);                                       // sleep 5 seconds
    }
}

Void sensorTask(UArg arg0, UArg arg1){
 char data3[]="\f---NO RAINFALL---     (SENSÖR YAÐIÞSIZ)\r\n";
    while(1){

         GPIO_write(Board_LED1, Board_LED_OFF);

         if(!(GPIO_read(Board_SENSOR))){//PB4 GPIO, rainfall data read from digital sensor pin

            GPIO_write(Board_LED_BREADBOARD, Board_LED_ON);//external led (PB5 GPIO pin) turns on

            System_printf("yaðmur var(SENSOR------>RAINY\n");
            System_flush();

            Semaphore_post(semaphore1);//If the digital reading data is rainy, the api is posted.

         }
         else
         {

             TimeFromNetworkTimeProtocol(SERVER_TÝME_IP, 37,Time_out, strlen(Time_out));//time data is updated

             GPIO_write(Board_LED_BREADBOARD, Board_LED_OFF);//external led (PB5 GPIO pin) turns off

             sendData2Server(SOCKETTEST_IP, OUTGOING_PORT, Time_out, strlen(Time_out));//time data is printed to the server

             if(sendData2Server(SOCKETTEST_IP, OUTGOING_PORT, data3, strlen(data3))) {//information of precipitation is printed to the server
                 System_printf("clientSocketTask:: information of precipitation is sent to the server\n");
                 System_flush();
              }


             System_printf("yaðmur yok(SENSOR------>NO RAIN)\n");
             System_flush();
         }
         Task_sleep(5000);
     }




}


bool createTasks(void)
{
    static Task_Handle taskHandle1, taskHandle2;
    Task_Params taskParams;
    Error_Block eb;

    Error_init(&eb);

    Task_Params_init(&taskParams);
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.priority = 1;
    taskHandle1 = Task_create((Task_FuncPtr)sensorTask, &taskParams, &eb);

    Task_Params_init(&taskParams);
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.priority = 1;
    taskHandle1 = Task_create((Task_FuncPtr)httpTask, &taskParams, &eb);

    Task_Params_init(&taskParams);
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.priority = 1;
    taskHandle2 = Task_create((Task_FuncPtr)clientSocketTask, &taskParams, &eb);



    if (taskHandle1 == NULL || taskHandle2 == NULL ) {
        printError("netIPAddrHook: Failed to create HTTP, Socket and Server Tasks\n", -1);
        return false;
    }

    return true;
}

//  This function is called when IP Addr is added or deleted
//
void netIPAddrHook(unsigned int IPAddr, unsigned int IfIdx, unsigned int fAdd)
{
    // Create a HTTP task when the IP address is added
    if (fAdd) {
        createTasks();
    }
}

int main(void)
{
    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();
    Board_initEMAC();

    /* Turn on user LED */
    GPIO_write(Board_LED0, Board_LED_ON);

    System_printf("Starting the HTTP GET example\nSystem provider is set to "
            "SysMin. Halt the target to view any SysMin contents in ROV.\n");
    /* SysMin will only print to the console when you call flush or exit */
    System_flush();


    /* Start BIOS */
    BIOS_start();

    return (0);
}
