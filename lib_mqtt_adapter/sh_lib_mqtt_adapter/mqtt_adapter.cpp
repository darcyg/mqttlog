/*
 * mqtt_client.cpp
 *
 *  Created on: 08.08.2017
 *      Author: thomas
 */

#include <iostream>
#include <string.h>
#include <mutex>

#include <lib_convention__errno.h>

#include "mqtt_adapter.hpp"

/* *******************************************************************
 * mqtt_client member functions
 * ******************************************************************/
mqtt_client::mqtt_client(const char *id, const char *host, int port) : mosquittopp(id,true), m_state(CLIENT_STATE_disconnect)
{
	int ret;
	int keepalive = 60;
	mosqpp::lib_init();
	ret = connect(host, port, keepalive);
	loop_start();
};

mqtt_client::~mqtt_client()
{
	loop_stop(true);
	m_provide_spinlock_list.lock();
	for (std::vector<mqtt_provide*>::iterator it = m_provide_list.begin() ; it != m_provide_list.end(); ++it)
	{
		if (m_provide_list.empty()){
			break;
		}
		delete(*it);
		m_provide_list.erase(it);
	}
	m_provide_spinlock_list.unlock();


	mosqpp::lib_cleanup();
	std::cout << "Client destroy" << std::endl;
}

int mqtt_client::disconnect()
{
	switch (m_state)
	{
		case CLIENT_STATE_success:
			mosquittopp::disconnect();
			return EOK;

		case CLIENT_STATE_disconnect:
		case CLIENT_STATE_connection_refused_protocol:
		case CLIENT_STATE_connection_identifier_rejected:
		case CLIENT_STATE_no_broker:
		default:
			return -ENOT_SUPP;
	}
}

int mqtt_client::reconnect()
{
	switch (m_state)
	{
		case CLIENT_STATE_disconnect:
		case CLIENT_STATE_connection_refused_protocol:
		case CLIENT_STATE_connection_identifier_rejected:
		case CLIENT_STATE_no_broker:
			mosquittopp::reconnect();
			return EOK;

		case CLIENT_STATE_success:
		default:
			return -ENOT_SUPP;
	}

}



mqtt_provide *mqtt_client::create_mqtt_provide(char *const _topic, enum mqtt_qos _qos, bool _retain)
{
	mqtt_provide *provide;
	provide = new mqtt_provide((mqtt_client*)this, _topic, _qos, _retain);

	m_provide_spinlock_list.lock();
	m_provide_list.push_back(provide);
	m_provide_spinlock_list.unlock();

	return provide;
}

mqtt_subscribe *mqtt_client::create_mqtt_subscribe(char * const _sub, enum mqtt_qos _qos)
{
	mqtt_subscribe *subscribe;
	subscribe = new mqtt_subscribe((mqtt_client*)this, _sub, _qos);


}


int mqtt_client::provide(mqtt_provide* _inst, int *_mid, const char *_topic, const char *_payload, enum mqtt_qos _qos, bool _retain)
{
	mosq_err_t mosq_ret;
	int ret, len;
	if (m_state != CLIENT_STATE_success) {
		return -ENOTCONN;
	}

	len = strlen(_payload);
	mosq_ret = (mosq_err_t)publish(_mid, _topic, len, _payload, _qos, _retain);
	switch (mosq_ret)
	{
		case MOSQ_ERR_SUCCESS:
			ret = EOK; break;
		case MOSQ_ERR_CONN_PENDING:
			ret =-EBUSY; break;
		case MOSQ_ERR_NOMEM:
			ret =-ENOMEM; break;
		case MOSQ_ERR_PROTOCOL:
			ret = -EPROTOTYPE; break;
		case MOSQ_ERR_INVAL:
			ret = -EINVAL; break;
		case MOSQ_ERR_NO_CONN:
			ret = -ENOTCONN; break;
		case MOSQ_ERR_CONN_REFUSED:
		case MOSQ_ERR_CONN_LOST:
		case MOSQ_ERR_TLS:
		case MOSQ_ERR_AUTH:
		case MOSQ_ERR_ACL_DENIED:
			ret = -ECOMM; break;
		case MOSQ_ERR_NOT_FOUND:
			ret = -EFAULT; break;
		case MOSQ_ERR_PAYLOAD_SIZE:
		case MOSQ_ERR_NOT_SUPPORTED:
		case MOSQ_ERR_UNKNOWN:
		case MOSQ_ERR_ERRNO:
		case MOSQ_ERR_EAI:
		case MOSQ_ERR_PROXY:
		default:
			ret = -EINVAL;
	}

	return ret;
}

int mqtt_client::consumer(mqtt_subscribe* _inst, int *_mid, const char *_sub, int _qos)
{
	mosq_err_t mosq_ret;
	int ret;

	mosq_ret = (mosq_err_t)mosquittopp::subscribe(_mid,_sub,_qos);
	switch (mosq_ret)
	{
		case MOSQ_ERR_SUCCESS:
			ret = EOK; break;
		case MOSQ_ERR_CONN_PENDING:
			ret =-EBUSY; break;
		case MOSQ_ERR_NOMEM:
			ret =-ENOMEM; break;
		case MOSQ_ERR_PROTOCOL:
			ret = -EPROTOTYPE; break;
		case MOSQ_ERR_INVAL:
			ret = -EINVAL; break;
		case MOSQ_ERR_NO_CONN:
			ret = -ENOTCONN; break;
		case MOSQ_ERR_CONN_REFUSED:
		case MOSQ_ERR_CONN_LOST:
		case MOSQ_ERR_TLS:
		case MOSQ_ERR_AUTH:
		case MOSQ_ERR_ACL_DENIED:
			ret = -ECOMM; break;
		case MOSQ_ERR_NOT_FOUND:
			ret = -EFAULT; break;
		case MOSQ_ERR_PAYLOAD_SIZE:
		case MOSQ_ERR_NOT_SUPPORTED:
		case MOSQ_ERR_UNKNOWN:
		case MOSQ_ERR_ERRNO:
		case MOSQ_ERR_EAI:
		case MOSQ_ERR_PROXY:
		default:
			ret = -EINVAL;
	}

	return ret;


}



void mqtt_client::on_connect(int _rc)
{
	switch (_rc)
	{
		case 0:
			m_state = CLIENT_STATE_success;
			std::cout << "Client connect" << std::endl;
			break;
		case 1:
			m_state = CLIENT_STATE_connection_refused_protocol;
			std::cout << "Client connection refused, unacceptable protocol version" << std::endl;
			break;
		case 2:
			m_state = CLIENT_STATE_connection_identifier_rejected;
			std::cout << "Client connection refused, identifier rejected" << std::endl;
			break;
		case 3:
			m_state = CLIENT_STATE_no_broker;
			std::cout << "Client connection refused, broker unavailable" << std::endl;
			break;
		default:
			m_state = CLIENT_STATE_disconnect;
			break;
	}

}

void mqtt_client::on_disconnect(int _rc)
{
	if (_rc > 0) {
		std::cout << "Client unexpected disconnected" << std::endl;
	}
	else {
		std::cout << "Client successful disconnected" << std::endl;
	}

	m_state = CLIENT_STATE_disconnect;
}


void mqtt_client::on_publish(int mid)
{
	int ret;
	mqtt_provide *provide;

}




void mqtt_client::on_message(const struct mosquitto_message *message)
{
	char* data;
	data = (char*)message->payload;


	std::cout<<message->mid<<message->topic<<" data: "<<data<<std::endl;
}

void mqtt_client::on_subscribe(int mid, int qos_count, const int *granted_qos)
{

}

void mqtt_client::on_unsubscribe(int _mid)
{

}

void mqtt_client::on_log(int _level, const char *_str)
{

}

void mqtt_client::on_error()
{
	std::cout<<"on_error"<<std::endl;
}





/* *******************************************************************
 * mqtt_provide member functions
 * ******************************************************************/
mqtt_provide::mqtt_provide(mqtt_client *_mqtt_client,char *_topic, enum mqtt_qos _qos, bool _retain) : m_mqtt_client(_mqtt_client),m_qos(_qos), m_retain(_retain), m_id(0)
{
	int len;
	len = strlen(_topic) +1;
	m_topic = (char*)calloc(len, sizeof(char));
	if (m_topic != NULL) {
		strcpy(m_topic,_topic);
	}
}

mqtt_provide::~mqtt_provide()
{
	std::cout<<"mqtt_provide"<<std::endl;
}

int mqtt_provide::provide(const char * _payload)
{
	int ret = EOK;

	m_mutex.lock();

	ret = m_mqtt_client->provide((mqtt_provide*)this, &m_id, m_topic, _payload, m_qos, m_retain);
	if(ret < EOK) {
		std::cout << "Provide Error" << ret << std::endl;
	}

	m_mutex.unlock();
	return ret;
}

/* *******************************************************************
 * mqtt_subscribe member functions
 * ******************************************************************/

mqtt_subscribe::mqtt_subscribe (mqtt_client *_mqtt_client, char *_sub, enum mqtt_qos _qos) : m_mqtt_client(_mqtt_client),m_qos(_qos), m_id(1)
{
	m_mqtt_client->consumer((mqtt_subscribe*)this, &m_id, _sub, _qos);
}

mqtt_subscribe::~mqtt_subscribe()
{

}

