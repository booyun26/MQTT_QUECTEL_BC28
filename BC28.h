/**
  *********************************************************
  * @file	BC28.h
  * @brief  BC28 driver include file
  * @ver	0.01
  *********************************************************
  * 
  */

#ifndef _BC28_H_
#define _BC28_H_

typedef	unsigned char uint8_t;
typedef	signed char int8_t;
typedef	unsigned short uint16_t;
typedef	short int16_t;
typedef	unsigned int uint32_t;
typedef	int int32_t;

typedef void (*BC28_TASK)(int,int);

#ifndef NULL
#define NULL (0)
#endif

/**
 * MUST implement wrapper functions.
 **/

/**
  * @brief  Wrapper function to sleep for miliseconds.
  * @param  miliseconds
  * @retval None
  */
void BC28_Wrap_Sleep(int ms);

/**
  * @brief  Wrapper function to send data via UART.
  * @param  data: pointer to data, size: number of bytes
  * @retval numbers of sent bytes
  */
int BC28_Wrap_Send(const uint8_t *data, int size);

/**
  * @brief  Wrapper function to post task to main thread.
  * @param  function pointer and two parameters
  * @retval None
  */
void BC28_Wrap_PostTask(BC28_TASK, int, int);

/**
  * @brief  Wrapper function to allocate memory.
  * @param  number of bytes
  * @retval pointer to the allocated memory, NULL means error.
  */
void *BC28_Wrap_Memory_Alloc(uint32_t size);

/**
  * @brief  Wrapper function to free allocate memory.
  * @param  pointer to the allocated memory
  * @retval None
  */
void BC28_Wrap_Memory_Free(void *ptr);


/**
 * Public functions.
 **/
int BC28_Init(void);
const char* BC28_GetIMSI(void);
const char* BC28_GetIMEI(void);
void BC28_Reboot(void);
void BC28_PushReceivedByte(uint8_t b);
int BC28_WaitReady(int timeout);
int BC28_SendATCmdWaitRcv(const char* cmd, char *rcv, int rcv_size, int timeout);
int BC28_OpenTcpSocket(const char *ip, const char *port);
int BC28_WriteTcpSocket(int socket, uint8_t *data, int size);
int BC28_ReadTcpSocket(int socket, uint8_t *data, int size);
int BC28_CloseTcpSocket(int socket);
void BC28_SetSocketListener(BC28_TASK listener);

#endif
