/*
 * main.c
 *
 *  Created on: 09.08.2017
 *      Author: thomas
 */
#include <stdio.h>
#define _BSD_SOURCE
#include <lib_thread.h>
#include <mqtt_adapter.hpp>
#include <signal.h>
#include <string.h>


static void sigHandler(int _sig);

int main (void)
{
	int i;
	unsigned int count = 0;
	char buffer_1[20] = {0};
	char buffer_2[20] = {0};

	mqtt_client *client;
	mqtt_provide *provide, *provide2;
	mqtt_subscribe *consume;
	int rc;

	for(i =1;i <NSIG; i++)
	{
		printf("%u_%s\n",i,strsignal(i));

	}

//	if(signal(32,sigHandler) == SIG_ERR) {
//		return 0;
//	}


	client = new mqtt_client("MQ_CLI_3", "tomhp",1883);

	provide = client->create_mqtt_provide("ID12",MQTT_QOS_0, false);
	provide2 = client->create_mqtt_provide("ID13",MQTT_QOS_0, false);
	provide->provide("hello_1_msg");
	consume = client->create_mqtt_subscribe("ID20",MQTT_QOS_0);
	consume = client->create_mqtt_subscribe("ID21",MQTT_QOS_0);




	while(count < 500){
		count++;

		snprintf(&buffer_1[0],sizeof(buffer_1),"%u",count);
		snprintf(&buffer_2[0],sizeof(buffer_2),"hello_2_%u",count);

//		provide->provide(&buffer_1[0]);
//		provide2->provide(&buffer_2[0]);
		lib_thread__msleep(2000);

	}
	client->disconnect();
	delete(client);
}


static void sigHandler(int _sig)
{
	printf("%s: %i\n",__func__,_sig);
}


