/**
  *********************************************************
  * @file	MQTT.h
  * @brief  MQTT message composer include file
  * @ver	0.01
  *********************************************************
  * 
  */

#ifndef _MQTT_H_
#define _MQTT_H_

#define MQTT_BROKER_IP_FET			"150.117.126.25"
#define MQTT_USER_NAME_FET			"fet/fet"
#define MQTT_PASSWD_FET				"fet@1234"
#define MQTT_BROKER_IP_HIVEMQ		"52.29.72.194"
#define MQTT_BROKER_IP_ECLIPSE		"198.41.30.241"

#define DEFAULT_MQTT_BROKER_IP		MQTT_BROKER_IP_ECLIPSE
#define DEFAULT_MQTT_BROKER_PORT	"1883"
#define DEFAULT_MQTT_CLIENT_ID		""
#define DEFAULT_MQTT_USER_NAME		""
#define DEFAULT_MQTT_PASSWORD		""
#define DEFAULT_MQTT_PUBLISH_TOPIC	"HLX-IoT/pub"
#define DEFAULT_MQTT_PUBLISH_MSG	"test"
#define DEFAULT_MQTT_SUBSCRIBE_TOPIC	"HLX-IoT/pub"

#define MQTT_MSG_SIZE_CONNACK		4
#define MQTT_MSG_SIZE_SUBACK		5

enum {
	MQTT_MSG_TYPE_RESERVED,
	MQTT_MSG_TYPE_CONNECT,
	MQTT_MSG_TYPE_CONNACK,
	MQTT_MSG_TYPE_PUBLISH,
	MQTT_MSG_TYPE_PUBACK,
	MQTT_MSG_TYPE_PUBREC,
	MQTT_MSG_TYPE_PUBREL,
	MQTT_MSG_TYPE_PUBCOMP,
	MQTT_MSG_TYPE_SUBSCRIBE,
	MQTT_MSG_TYPE_SUBACK,
	MQTT_MSG_TYPE_UNSUBSCRIBE,
	MQTT_MSG_TYPE_UNSUBACK,
	MQTT_MSG_TYPE_PINGREQ,
	MQTT_MSG_TYPE_PINGRESP,
	MQTT_MSG_TYPE_DISCONNECT
};

enum {
	MQTT_QOS_AT_MOST_ONCE,
	MQTT_QOS_AT_LEAST_ONCE,
	MQTT_QOS_EXACTLY_ONCE,
	MQTT_QOS_RESERVED
};

enum {
	MQTT_CONNACK_ACCEPTED,
	MQTT_CONNACK_REFUSED_UNACCEPTABLE_VERSION,
	MQTT_CONNACK_REFUSED_IDENTIFIER_REJECTED,
	MQTT_CONNACK_REFUSED_SERVER_UNAVAILABLE,
	MQTT_CONNACK_REFUSED_BAD_USER_NAME_OR_PASSWORD,
	MQTT_CONNACK_REFUSED_NOT_AUTHORIZED
};

int MQTT_GetMessageType(unsigned char *msg);

int MQTT_ConnectMessage(unsigned char *msg, int size, const char *IP, const char *port, 
				   const char *Client_ID, const char *user_name, const char *passwd,
				   int con_timeout, int keep_alive, int clean_session);

int MQTT_DisconnectMessage(unsigned char *msg, int size);

int MQTT_PublishMessage(unsigned char *msg, int size, int dup, int qos, int retain,
						const char* topic, const char* message, int msg_id);

int MQTT_SubscribeMessage(unsigned char *msg, int size, const char* topic);

int MQTT_PingRequestMessage(unsigned char *msg, int size);

int MQTT_ParsePublishMessage(unsigned char *msg, int size, char *topic, char *message);

//-1 means not CONNACK, 0 means OK, else means FAILED
int MQTT_CheckConnectAck(const unsigned char* msg);

#endif
