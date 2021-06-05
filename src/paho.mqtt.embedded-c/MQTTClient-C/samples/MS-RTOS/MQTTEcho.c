/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* MS-RTOS includes. */
#include <ms_rtos.h>

#include "MQTTClient.h"

static void messageArrived(MessageData* data)
{
    char tmpbuf[80];

    memcpy(tmpbuf, data->topicName->lenstring.data, data->topicName->lenstring.len);
    tmpbuf[data->topicName->lenstring.len] = 0;
    ms_printf("Message arrived on topic %s", tmpbuf);

    memcpy(tmpbuf, data->message->payload, data->message->payloadlen);
    tmpbuf[data->message->payloadlen] = 0;
    ms_printf(": %s\n", tmpbuf);
}

int main(int argc, char **argv)
{
	/* connect to 192.168.128.1, subscribe to a topic, send and receive messages regularly every 10 sec */
	MQTTClient client;
	Network network;
	unsigned char sendbuf[80], readbuf[80];
	int rc = 0, count = 0;
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

	NetworkInit(&network);
	MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

	char* address = "192.168.128.1";
	if ((rc = NetworkConnect(&network, address, 1883)) != 0) {
		ms_printf("Return code from network connect is %d\n", rc);
		exit(0);
	}

#if defined(MQTT_TASK)
	if ((rc = MQTTStartTask(&client)) != MS_ERR_NONE) {
		ms_printf("Return code from start tasks is %d\n", rc);
        exit(0);
	}
#endif

	connectData.MQTTVersion = 3;
	connectData.clientID.cstring = "MS-RTOS_sample";
	connectData.username.cstring = "user";
    connectData.password.cstring = "passwd";

	if ((rc = MQTTConnect(&client, &connectData)) != 0) {
		ms_printf("Return code from MQTT connect is %d\n", rc);
        exit(0);
	}

	ms_printf("MQTT Connected\n");

	if ((rc = MQTTSubscribe(&client, "MS-RTOS/sample/a", 2, messageArrived)) != 0) {
		ms_printf("Return code from MQTT subscribe is %d\n", rc);
        exit(0);
	}

	while (++count) {
		MQTTMessage message;
		char payload[30];

		message.qos = 1;
		message.retained = 0;
		message.payload = payload;
		ms_snprintf(payload, sizeof(payload), "message number %d", count);
		message.payloadlen = strlen(payload);

		if ((rc = MQTTPublish(&client, "MS-RTOS/sample/a", &message)) != 0) {
			ms_printf("Return code from MQTT publish is %d\n", rc);
			break;
		}
#if !defined(MQTT_TASK)
		if ((rc = MQTTYield(&client, 10000)) != 0) {
			ms_printf("Return code from yield is %d\n", rc);
            break;
		}
#else
		ms_thread_sleep_s(10);
#endif
	}

	ms_printf("MQTT example exit!\n");

    MQTTDisconnect(&client);
    NetworkDisconnect(&network);

    return 0;
}
