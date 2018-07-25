/**
  *********************************************************
  * @file	MQTT.c
  * @brief  MQTT message composer
  * @ver	0.01
  *********************************************************
  * 
  */

#include <string.h>
#include "MQTT.h"

static unsigned char ComposeHeaderFlags(int msg_type, int dup_flag, int qos_level, int retain)
{
	return (unsigned char)(((msg_type & 0xF) << 4) | ((dup_flag & 1) << 3) |
		((qos_level & 3) << 1) | (retain & 1));
}

static unsigned char ComposeConnectFlags(int usr_name, int usr_pw, int will_retain,
										 int will_qos, int will_flag, int clean_session)
{
	return (unsigned char)(((usr_name & 1) << 7) | ((usr_pw & 1) << 6) |
		((will_retain & 1) << 5) | ((will_qos & 3) << 3) | 
		((will_flag & 1) << 2) | ((clean_session & 1) << 1));
}


/**
  * @brief  To get message type from message.
  * @param  pointer to message
  * @retval enum MQTT_MSG_TYPE
  */
int MQTT_GetMessageType(unsigned char *msg)
{
	return (int)((*msg) >> 4); 
}


/**
  * @brief  To compose connect message.
  * @param  msg: pointer to message, size: max number of bytes, 
  *			IP: IPV4, port: port number,
  *			Client_ID: pointer to client ID,
  *			usr_name: pointer to user name, passwd: pointer to password,
  *			con_timeout: connection timeout in seconds,
  *			keep_alive: keep alive period in seconds
  *			clean_session: 0 for disable, 1 for enable
  * @retval number of bytes
  */
int MQTT_ConnectMessage(unsigned char *msg, int size, const char *IP, const char *port, 
				   const char *Client_ID, const char *usr_name, const char *passwd,
				   int con_timeout, int keep_alive, int clean_session)
{
	int count = 0, len;
	int flag_usr_name = 0, flag_passwd = 0;

	if(msg == NULL || size < 2)
		return 0;

	//Header Flags
	msg[count++] = ComposeHeaderFlags(MQTT_MSG_TYPE_CONNECT, 0, 0, 0);

	//set message length later
	msg[count++] = 0;

	//set protocol name
	msg[count++] = 0;
	msg[count++] = 4;
	msg[count++] = 'M';
	msg[count++] = 'Q';
	msg[count++] = 'T';
	msg[count++] = 'T';
	msg[count++] = 4;	//version: 3.1.1

	//Connect flags
	msg[count++] = ComposeConnectFlags(
		(usr_name != NULL && usr_name[0] != 0) ? 1 : 0, 
		(passwd != NULL && passwd[0] != 0) ? 1 : 0, 
		0, 0, 0, (clean_session & 1));

	//keep-alive
	if(keep_alive > 0)
	{
		msg[count++] = (keep_alive >> 8);
		msg[count++] = (keep_alive & 0xFF);
	}
	else
	{
		msg[count++] = 0;
		msg[count++] = 0;
	}

	//Client ID
	if(Client_ID)
	{
		len = strlen(Client_ID);
		if(count + len > size)
			return 0;

		msg[count++] = (len >> 8);
		msg[count++] = (len & 0xFF);
		memcpy(&msg[count], (unsigned char*)Client_ID, len);
		count += len;
	}

	//skip will message

	//user name
	if(usr_name && usr_name[0] != 0)
	{
		len = strlen(usr_name);
		if(count + len > size)
			return 0;

		msg[count++] = (len >> 8);
		msg[count++] = (len & 0xFF);
		memcpy(&msg[count], (unsigned char*)usr_name, len);
		count += len;
	}

	//password
	if(passwd && passwd[0] != 0)
	{
		len = strlen(passwd);
		if(count + len > size)
			return 0;

		msg[count++] = (len >> 8);
		msg[count++] = (len & 0xFF);
		memcpy(&msg[count], (unsigned char*)passwd, len);
		count += len;
	}

	//set message length
	msg[1] = count - 2;

	return count;
}


/**
  * @brief  To compose disconnect message.
  * @param  msg: pointer to message, size: max number of bytes
  * @retval number of bytes
  */
int MQTT_DisconnectMessage(unsigned char *msg, int size)
{
	if(msg == NULL || size < 2)
		return 0;

	msg[0] = ComposeHeaderFlags(MQTT_MSG_TYPE_DISCONNECT, 0, 0, 0);
	msg[1] = 0;

	return 2;
}


/**
  * @brief  To compose publish message.
  * @param  msg: pointer to message, size: max number of bytes, 
  *			dup: flag of re-deliver, qos: enum MQTT_QOS,
  *			retain: flag of RETAIN,
  *			topic: pointer to topic, message: pointer to message,
  *			msg_id: if qos is greater than 0, should give message id starts with 0.
  * @retval number of bytes
  */
int MQTT_PublishMessage(unsigned char *msg, int size, int dup, int qos, int retain,
						const char* topic, const char* message, int msg_id)
{
	int count = 0, len;

	if(msg == NULL || size < 4 || 
		topic == NULL || message == NULL || 
		topic[0] == 0 || message[0] == 0)
		return 0;

	len = 2 + strlen(topic) + strlen(message);
	if(qos != MQTT_QOS_AT_MOST_ONCE)
		len += 2;

	//Header Flags
	msg[count++] = ComposeHeaderFlags(MQTT_MSG_TYPE_PUBLISH, dup, qos, retain);

	//set message length later
	msg[count++] = 0;
	if(len > 127)
		msg[count++] = 0;

	//topic
	if(topic)
	{
		len = strlen(topic);
		if(count + len > size)
			return 0;

		msg[count++] = (len >> 8);
		msg[count++] = (len & 0xFF);
		memcpy(&msg[count], (unsigned char*)topic, len);
		count += len;
	}

	//message ID
	if(qos != MQTT_QOS_AT_MOST_ONCE)
		msg[count++] = msg_id;

	//message
	if(message)
	{
		len = strlen(message);
		if(count + len > size)
			return 0;

		memcpy(&msg[count], (unsigned char*)message, len);
		count += len;
	}

	//set message length
	len = count - 2;
	msg[1] = (len & 0x7F);
	if(len > 127)
	{
		msg[1] |= 0x80;
		msg[2] = ((len - (len & 0x7F)) >> 7);
	}

	return count;
}


/**
  * @brief  To compose subscribe message.
  * @param  msg: pointer to message, size: max number of bytes,
  *			topic: pointer to topic, message: pointer to message
  * @retval number of bytes
  */
int MQTT_SubscribeMessage(unsigned char *msg, int size, const char* topic)
{
	int count = 0, len;

	if(msg == NULL || size < 2)
		return 0;

	//Header Flags
	msg[count++] = ComposeHeaderFlags(MQTT_MSG_TYPE_SUBSCRIBE, 0, 1, 0);

	//set message length later
	msg[count++] = 0;

	//message identifier
	msg[count++] = 0;
	msg[count++] = 1;

	//topic
	if(topic)
	{
		len = strlen(topic);
		if(count + len > size)
			return 0;

		msg[count++] = (len >> 8);
		msg[count++] = (len & 0xFF);
		memcpy(&msg[count], (unsigned char*)topic, len);
		count += len;
	}

	//qos
	msg[count++] = 0;

	//set message length
	msg[1] = count - 2;

	return count;
}


/**
  * @brief  To compose ping request message.
  * @param  msg: pointer to message, size: max number of bytes
  * @retval number of bytes
  */
int MQTT_PingRequestMessage(unsigned char *msg, int size)
{
	int count = 0;

	if(msg == NULL || size < 2)
		return 0;

	//Header Flags
	msg[count++] = ComposeHeaderFlags(MQTT_MSG_TYPE_PINGREQ, 0, 0, 0);

	//set message length later
	msg[count++] = 0;

	return count;
}


/**
  * @brief  To parse publish message received.
  * @param  msg: pointer to message, size: max number of bytes,
  *			topic: pointer to topic, message: pointer to message
  * @retval 1: OK, 0: not publish message
  */
int MQTT_ParsePublishMessage(unsigned char *msg, int size, char *topic, char *message)
{
	if(MQTT_GetMessageType((unsigned char*)msg) == MQTT_MSG_TYPE_PUBLISH)
	{
		unsigned char *p = &msg[1];
		int msg_len = (msg[1] & 0x7F);
		int factor = 1;

		while(p[0] & 0x80)
		{
			p++;
			factor *= 128;
			msg_len += ((p[0] & 0x7F) * factor);
		}

		p++;

		//get topic
		int topic_len = ((int)p[0] << 8) + p[1];

		p += 2;

		memcpy(topic, p, topic_len);
		topic[topic_len] = 0;

		p += topic_len;

		//get message
		int message_len = msg_len - topic_len - 2;

		memcpy(message, p, message_len);
		message[message_len] = 0;

		return 1;
	}

	return 0;
}


/**
  * @brief  To check CONNACK message.
  * @param  msg: pointer to message
  * @retval -1: not CONNACK, others: enum MQTT_CONNACK
  */
int MQTT_CheckConnectAck(const unsigned char* msg)
{
	if(MQTT_GetMessageType((unsigned char*)msg) == MQTT_MSG_TYPE_CONNACK)
	{
		return msg[3];
	}

	return -1;
}

