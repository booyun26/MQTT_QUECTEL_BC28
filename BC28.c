/**
  *********************************************************
  * @file	BC28.c
  * @brief  Quectel BC28 driver
  * @ver	0.02
  *********************************************************
  ***************** Application Notes *********************
  *********************************************************
  * 1. MUST implement wrapper functions.
  *	2. Call BC28_PushReceivedByte() in UART ISR or thread to handle received data.
  * 3. Call BC28_Init() before calling any public functions.
  * 4. BC28_Reboot() is a good solution for connection issues.
  * 5. Call BC28_WaitReady() before opening socket.
  * 6. Set socket listener to handle incoming data via internet.
  *********************************************************/

#include <string.h>
#include <stdio.h>
#include "BC28.h"

#define MAX_SOCKET_NUM			1		//keep it as 1, not support multi-socket yet
#define MAX_SOCKET_PACKET_SIZE	1024	//total memory usage will be 4x, so be careful of this value 
#define UART_RCV_BUF_SIZE		((MAX_SOCKET_PACKET_SIZE << 1) + 20)
#define SOCKET_RCV_BUF_SIZE		(MAX_SOCKET_PACKET_SIZE << 1)

static uint8_t	bufUartRcv[UART_RCV_BUF_SIZE];
static int		countUartRcvBuf = 0;

static uint8_t	bufSocketRcvQ[MAX_SOCKET_NUM][SOCKET_RCV_BUF_SIZE];
static int		headSocketRcvQ[MAX_SOCKET_NUM] = {0};
static int		tailSocketRcvQ[MAX_SOCKET_NUM] = {0};

static char		*pRcvBuf = NULL;
static int		ActOrNack = 0;	// -1: nack, 0: none, 1: ack

static char		IMSI[20] = "";
static char		IMEI[20] = "";

static BC28_TASK	taskSocketListener = NULL;

static volatile int mutexSendAT = 0;

static int BC28_SendATCmd(const char *cmd);

static int BC28_strlen(const char *src);
static char *BC28_strstr(const char *src, const char *tar);

static int InitSocketRcvQ(int socket);
static int PushSocketRcvQ(int socket, uint8_t *data, int size);
static int PopSocketRcvQ(int socket, uint8_t *data, int size);

static void ReadSocketTask(int socket, int size);
static char* FindField(const char *str, char separator, int index);


/**
  * @brief  This function initialize BC28.
  * @param  None
  * @retval 1: Done, others: Error
  */
int BC28_Init(void)
{
	int count = 10000/500;
	int ret = 0;
	char szRcv[64];

	countUartRcvBuf = 0;
	taskSocketListener = NULL;
	mutexSendAT = 0;

	// check response
	while(count--)
	{
		ret = BC28_SendATCmdWaitRcv("AT\r", szRcv, 60, 500);
		if(ret != 0)
			break;

		BC28_Wrap_Sleep(500);
	}

	// read IMSI
	if(ret)
	{
		if(BC28_SendATCmdWaitRcv("AT+CIMI\r", szRcv, 60, 500))
		{
			char *p = szRcv;
			int a = 0;
			while(*p == '\r' || *p == '\n') p++;
			while(*p != '\r' && a < 20)
			{
				IMSI[a++] = *p++;
			}
		}

		if(BC28_SendATCmdWaitRcv("AT+CGSN=1\r", szRcv, 60, 500))
		{
			char *p = strchr(szRcv, ':');

			if(p != NULL)
			{
				int a = 0;
				p++;
				while(*p != '\r' && a < 20)
				{
					IMEI[a++] = *p++;
				}
			}
		}
	}

	return ret;
}


/**
  * @brief  To get IMSI.
  * @param  None
  * @retval pointer to IMSI string
  */
const char* BC28_GetIMSI(void)
{
	return IMSI;
}


/**
  * @brief  To get IMEI.
  * @param  None
  * @retval pointer to IMEI string
  */
const char* BC28_GetIMEI(void)
{
	return IMEI;
}


/**
  * @brief  To reboot BC28. May wait for half minute.
  * @param  None
  * @retval None
  */
void BC28_Reboot(void)
{
	BC28_SendATCmd("AT+NRB\r");
	BC28_Wrap_Sleep(5000);
	BC28_Init();
}


/**
  * @brief  Call this function while receiving data via UART.
  * @param  received byte
  * @retval None
  */
void BC28_PushReceivedByte(uint8_t b)
{
	char *p;

	bufUartRcv[countUartRcvBuf++] = b;

	if(b == '\n' && countUartRcvBuf > 2)
	{
		bufUartRcv[countUartRcvBuf] = 0;

		// check NSONMI URC
		if((p = strstr((char*)bufUartRcv, "+NSONMI")) != NULL)
		{
			int socket, size = 0;

			p += 8;
			socket = *p - '0';

			p += 2;
			while(*p != '\r')
			{
				size = size*10 + (*p - '0');
				p++;
			}

			BC28_Wrap_PostTask(ReadSocketTask, socket, size);

			if(taskSocketListener != NULL)
			{
				BC28_Wrap_PostTask(taskSocketListener, socket, size);
			}
		}
		else if(pRcvBuf != NULL)
		{
			strcat(pRcvBuf, (char*)bufUartRcv);
			if(strstr((char*)bufUartRcv, "OK") != NULL)
			{
				ActOrNack = 1;
			}
			else if(strstr((char*)bufUartRcv, "ERROR") != NULL)
			{
				ActOrNack = -1;
			}
		}

		countUartRcvBuf = 0;
	}
}


/**
  * @brief  To send AT command and wait for response.
  * @param  cmd: pointer to AT command, 
  *			rcv: pointer to response buffer, rcv_size: max number of bytes,
  *			timeout: miliseconds
  * @retval 0: timeout, 1: OK, 2: ERROR
  */
int BC28_SendATCmdWaitRcv(const char* cmd, char *rcv, int rcv_size, int timeout)
{
	int count = 10000/100 + 1;

	while(mutexSendAT && count--)
		BC28_Wrap_Sleep(100);

	if(mutexSendAT == 0)
	{
		int ack;

		mutexSendAT = 1;

		count = timeout/100 + 1;

		pRcvBuf = rcv;
		*pRcvBuf = 0;
		ActOrNack = 0;
		countUartRcvBuf = 0;

		BC28_SendATCmd(cmd);
		if(strchr(cmd, '\r') == NULL)
			BC28_SendATCmd("\r");

		while(count-- && ActOrNack == 0)
		{
			BC28_Wrap_Sleep(100);
		}

		pRcvBuf = NULL;
		ack = ActOrNack;

		mutexSendAT = 0;

		return ack;
	}

	return 0;
}


/**
  * @brief  Use this function to wait for BC28 ready to connect network.
  * @param  timeout in miliseconds
  * @retval 1: Done, others: Failed
  */
int BC28_WaitReady(int timeout)
{
	int count = timeout/500 + 1;
	int ret = 0;

	// check response
	while(count--)
	{
		char szRcv[32];

		ret = BC28_SendATCmdWaitRcv("AT\r", szRcv, 30, 500);

		if(ret != 0)
			break;

		BC28_Wrap_Sleep(500);
	}

	// check registration
	if(ret == 1)
	{
		while(count--)
		{
			char szRcv[32];

			ret = BC28_SendATCmdWaitRcv("AT+CEREG?\r", szRcv, 30, 500);

			if(ret == 1)
			{
				char *p = strchr(szRcv, ',');
				if(p != NULL)
				{
					p++;
					if(*p == '1')
					{
						ret = 1;
						break;
					}
				}

				ret = 0;
			}

			BC28_Wrap_Sleep(500);
		}
	}

	return ret;
}


/**
  * @brief  Use this function to open TCP connection.
  * @param  ip: IPV4, port: port number
  * @retval -1: no connection, others: socket index
  */
int BC28_OpenTcpSocket(const char *ip, const char *port)
{
	int count = 3;
	int ret = 0;
	int socket = -1;
	char szRcv[32];

	while(count--)
	{
		ret = BC28_SendATCmdWaitRcv("AT+NSOCR=STREAM,6,4587,1\r", szRcv, 30, 1000);
		if(ret == 1)
		{
			char *p = szRcv;
			while(*p != 0 && *p < '+') p++;
			socket = (*p - '0');
			break;
		}

		BC28_Wrap_Sleep(1000);
	}

	if(socket >= 0)
	{
		char szCmd[64];

		sprintf(szCmd, "AT+NSOCO=%d,%s,%s\r", socket, ip, port);
		if(BC28_SendATCmdWaitRcv(szCmd, szRcv, 30, 5000) != 1)
		{
			sprintf(szCmd, "AT+NSOCL=%d\r", socket);
			BC28_SendATCmd(szCmd);
			socket = -1;
		}
		else
		{
			InitSocketRcvQ(socket);
		}
	}

	return socket;
}


/**
  * @brief  Use this function to send data via TCP connection.
  * @param  socket: socket index, data: pointer to data, size: number of bytes
  * @retval number of sent bytes
  */
int BC28_WriteTcpSocket(int socket, uint8_t *data, int size)
{
	char *szCmd = (char*)bufUartRcv;
	char szRcv[32];
	int i;

	size = (size < MAX_SOCKET_PACKET_SIZE ? size : MAX_SOCKET_PACKET_SIZE);
	sprintf(szCmd, "AT+NSOSD=%d,%d,", socket, size);

	for(i=0; i<size; i++)
	{
		char szHex[4];

		sprintf(szHex, "%02X", data[i]);
		strcat(szCmd, szHex);
	}

	strcat(szCmd, "\r");

	if(BC28_SendATCmdWaitRcv(szCmd, szRcv, 30, 5000) == 1)
	{
		char *p = strchr(szRcv, ',');
		
		size = 0;
		if(p != NULL)
		{
			char *p1;

			p++;
			p1 = strchr(p, '\r');
			if(p1 != NULL)
			{
				int digits;

				digits = p1 - p;
				for(i=0; i<digits; i++)
				{
					size = size*10 + (*p - '0');
					p++;
				}
			}
		}
	}
	else
	{
		size = 0;
	}

	return size;
}


/**
  * @brief  Use this function to read data from TCP connection.
  * @param  socket: socket index, data: pointer to data, size: number of bytes
  * @retval number of read bytes
  */
int BC28_ReadTcpSocket(int socket, uint8_t *data, int size)
{
	int count = 0;

	while(count < size)
	{
		if(PopSocketRcvQ(socket, &data[count], 1) == 0)
			break;

		count++; 
	}

	return count;
}


/**
  * @brief  Use this function to close TCP connection.
  * @param  socket: socket index
  * @retval 1: Done, 0: no response, -1: ERROR
  */
int BC28_CloseTcpSocket(int socket)
{
	char szCmd[32], szRcv[32];

	sprintf(szCmd, "AT+NSOCL=%d\r", socket);
	return BC28_SendATCmdWaitRcv(szCmd, szRcv, 30, 5000);
}


/**
  * @brief  Use this function to set listener to handle received data via socket.
  *			First parameter is socket index, and next parameter is number of bytes to read.
  * @param  function pointer to listener
  * @retval None
  */
void BC28_SetSocketListener(BC28_TASK listener)
{
	taskSocketListener = listener;
}


/**
  * Local functions
  */
static int InitSocketRcvQ(int socket)
{
	int idxQ = 0;	//TODO: get index of Q from socket#

	headSocketRcvQ[idxQ] = 0;
	tailSocketRcvQ[idxQ] = 0;

	return 0;
}

static int PushSocketRcvQ(int socket, uint8_t *data, int size)
{
	int i = 0;
	int idxQ = 0;	//TODO: get index of Q from socket#

	for(; i<size; i++)
	{
		bufSocketRcvQ[idxQ][tailSocketRcvQ[idxQ]++] = data[i];
		if(tailSocketRcvQ[idxQ] >= SOCKET_RCV_BUF_SIZE)
			tailSocketRcvQ[idxQ] = 0;

		//overwrite
		if(tailSocketRcvQ[idxQ] == headSocketRcvQ[idxQ])
			headSocketRcvQ[idxQ]++;
		if(headSocketRcvQ[idxQ] >= SOCKET_RCV_BUF_SIZE)
			headSocketRcvQ[idxQ] = 0;
	}

	return i;
}

static int PopSocketRcvQ(int socket, uint8_t *data, int size)
{
	int count = 0;
	int idxQ = 0;	//TODO: get index of Q from socket#

	if(tailSocketRcvQ[idxQ] == headSocketRcvQ[idxQ])
	{
		return 0;
	}

	while(count < size)
	{
		data[count++] = bufSocketRcvQ[idxQ][headSocketRcvQ[idxQ]++];
		if(headSocketRcvQ[idxQ] >= SOCKET_RCV_BUF_SIZE)
			headSocketRcvQ[idxQ] = 0;
		if(tailSocketRcvQ[idxQ] == headSocketRcvQ[idxQ])
		{
			//headSocketRcvQ[idxQ] = tailSocketRcvQ[idxQ] = 0;
			break;
		}
	}

	return count;
}

static int BC28_strlen(const char *src)
{
	char *p = (char*)src;

	if(src == NULL)
		return 0;

	while(*p != 0) p++;

	return (int)(p - src);
}

static char *BC28_strstr(const char *src, const char *tar)
{
	char *p = (char*)src;
	char *p1 = (char*)tar;

	if(src == NULL || tar == NULL)
		return NULL;

	while(*p != 0)
	{
		if(*p == *p1)
		{
			char *pr = p;

			while((*p != 0) && (*p1 != 0))
			{
				if(*p != *p1)
					break;

				p++;
				p1++;
			}

			if(*p1 == 0)
			{
				return pr;
			}
			else
			{
				p1 = (char*)tar;
				continue;
			}
		}

		p++;
	}

	return NULL;
}

static void ReadSocketTask(int socket, int size)
{
	char szCmd[32];
	int times = size/MAX_SOCKET_PACKET_SIZE + 1;
	int i, count = 0;
	char *szRcv = (char*)BC28_Wrap_Memory_Alloc(UART_RCV_BUF_SIZE);

	for(i=0; i<times; i++)
	{
		int num = (size - count) > MAX_SOCKET_PACKET_SIZE ? MAX_SOCKET_PACKET_SIZE : (size - count);
		if(num > 0)
		{
			sprintf(szCmd, "AT+NSORF=%d,%d\r", socket, num);
			if(BC28_SendATCmdWaitRcv(szCmd, szRcv, UART_RCV_BUF_SIZE, 5000) == 1)
			{
				char *p = FindField(szRcv, ',', 3);
				if(p != NULL)
				{
					char *p1 = strchr(p, ',');
					if(p1 != NULL)
					{
						int digits;

						num = 0;
						digits = p1 - p;
						for(i=0; i<digits; i++)
						{
							num = num*10 + (*p - '0');
							p++;
						}

						if(num > 0)
						{
							p1++;

							for(i=0; i<num; i++)
							{
								uint8_t val = 0;
								int j;

								for(j=0; j<2; j++)
								{
									uint8_t a;

									if(*p1 < 'A')
										a = *p1 - '0';
									else
										a = *p1 - 'A' + 10;

									val = (val << 4) + a;

									p1++;
								}

								PushSocketRcvQ(socket, &val, 1);
							}
						}
					}
				}
			}
			else
			{
				break;
			}
		}
	}

	BC28_Wrap_Memory_Free(szRcv);
}

static char* FindField(const char *str, char separator, int index)
{
	char *p = (char *)str;
	int i;

	for(i=0; i<index; i++)
	{
		char *p1 = strchr(p, separator);
		if(p1 != NULL)
		{
			p = p1 + 1;
		}
		else
		{
			return NULL;
		}
	}

	return p;
}

static int BC28_SendATCmd(const char *cmd)
{
	return BC28_Wrap_Send((uint8_t*)cmd, strlen(cmd));
}
