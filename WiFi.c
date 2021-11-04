/* (c) Rajesh Panicker, ECE, NUS */

#include "main.h"
#include "wifi.h"
#include <stdlib.h>   // for rand(). Can be removed if valid sensor data is sent instead
#include <stdio.h>

#define MAX_LENGTH 400  // adjust it depending on the max size of the packet you expect to send or receive
#define WIFI_READ_TIMEOUT 10000
#define WIFI_WRITE_TIMEOUT 10000
#define USING_IOT_SERVER // This line should be commented out if using Packet Sender (not IoT server connection)

const char* WiFi_SSID = "HT";               // Replacce mySSID with WiFi SSID for your router / Hotspot
const char* WiFi_password = "AforEE2028";   // Replace myPassword with WiFi password for your router / Hotspot
const WIFI_Ecn_t WiFi_security = WIFI_ECN_WPA2_PSK; // WiFissecurity your router / Hotspot. No need to change it unless you use something other than WPA2 PSK
const uint16_t SOURCE_PORT = 0;  // source port. Type "netstat -a" in cmd. Find "IP address: source port" under local address column

uint8_t ipaddr[4] = {192, 168, 0, 0}; // IP address of your laptop wireless lan adapter, which is the one you successfully used to test Packet Sender above.
                                    // If using IoT platform, this will be overwritten by DNS lookup, so the values of x and y doesn't matter
                                            //(it should still be filled in with numbers 0-255 to avoid compilation errors)

#ifdef USING_IOT_SERVER
    const char* SERVER_NAME = "demo.thingsboard.io";    // domain name of the IoT server used
    const uint16_t DEST_PORT = 80;          // 'server' port number. Change according to application layer protocol. 80 is the destination port for HTTP protocol.
#else
    const uint16_t DEST_PORT = 2028;        // 'server' port number - this is the port Packet Sender listens to (as you set in Packer Sender)
                                                // and should be allowed by the OS firewall
#endif
SPI_HandleTypeDef hspi3;

const char*API_KEY = "99dkSTMFbMnV4WjgWNB8";

int main(void)
{
  HAL_Init();

  uint8_t req[MAX_LENGTH];  // request packet
  uint8_t resp[MAX_LENGTH]; // response packet
  uint16_t Datalen;
  WIFI_Status_t WiFi_Stat; // WiFi status. Should remain WIFI_STATUS_OK if everything goes well
  WiFi_Stat = WIFI_Init();                      // if it gets stuck here, you likely did not include EXTI1_IRQHandler() in stm32l4xx_it.c as mentioned above
  WiFi_Stat &= WIFI_Connect(WiFi_SSID, WiFi_password, WiFi_security); // joining a WiFi network takes several seconds. Don't be too quick to judge that your program has 'hung' :)
  if(WiFi_Stat!=WIFI_STATUS_OK) while(1);                   // halt computations if a WiFi connection could not be established.

#ifdef USING_IOT_SERVER
  WiFi_Stat = WIFI_GetHostAddress(SERVER_NAME, ipaddr); // DNS lookup to find the ip address, if using a connection to an IoT server
#endif
  // WiFi_Stat = WIFI_Ping(ipaddr, 3, 200);                 // Optional ping 3 times in 200 ms intervals
  WiFi_Stat = WIFI_OpenClientConnection(1, WIFI_TCP_PROTOCOL, "conn", ipaddr, DEST_PORT, SOURCE_PORT); // Make a TCP connection.
                                                                  // "conn" is just a name and serves no functional purpose

  if(WiFi_Stat!=WIFI_STATUS_OK) while(1);                   // halt computations if a connection could not be established with the server

  while (1)
  {
      int temper = rand()%40; // Just a random value for demo. Use the reading from sensors as appropriate
#ifdef USING_IOT_SERVER
      char json[100];   // change the size to the max size of the JSON data
      sprintf((char*)json,"{\"temperature\":%3d}",temper);
      sprintf((char*)req, "POST /api/v1/%s/telemetry HTTP/1.1\r\nHost: demo.thingsboard.io\r\nContent-Length: %d\r\n\r\n%s", API_KEY, strlen(json), json);
      //Important : Replace myAPIkey with your API key.
#else
      sprintf((char*)req, "temperature : %d\r", temper);
#endif
      WiFi_Stat = WIFI_SendData(1, req, (uint16_t)strlen((char*)req), &Datalen, WIFI_WRITE_TIMEOUT);

#ifdef USING_IOT_SERVER
      WiFi_Stat = WIFI_ReceiveData(1, resp, MAX_LENGTH, &Datalen, WIFI_READ_TIMEOUT); // Get response from the server.
      // This can also be used with PacketSender if you wish to receive a response. However, make sure that you send the response fast enough that it won't time out.
      // This function will block until a response is received, or timeout occurs, whichever is earlier.
      // You can print the server response to UART (TeraTerm) if necessary for debugging purposes.
      // Thingsboard will send a response similiar to the one below if the telemetry it received is good.
      /* HTTP/1.1 200
         Content-Length: 0
         Date: Sun, 01 Nov 2020 10:25:18 GMT */
      resp[Datalen] = '\0'; // to null terminate the response string
      if( (Datalen == 0) || !strstr((char *)resp, "200") ) while(1);
      // halt computations if the server does not respond (see below) or if the server sends a bad response (likely a wrong API key)
      // Thingsboard server closes connection a few seconds after a previous telemetery. Keep this in mind if you pause within this main while() loop.
#endif
      HAL_Delay(1000);

  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch(GPIO_Pin){
  case GPIO_PIN_1:
      SPI_WIFI_ISR();
      break;
  } // use more cases in the case statement for other EXTI interrupts
}

void SPI3_IRQHandler(void)
{
    HAL_SPI_IRQHandler(&hspi3);
}
