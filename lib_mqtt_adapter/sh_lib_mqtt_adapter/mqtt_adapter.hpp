/*
 * mqtt_adapter.hpp
 *
 *  Created on: 08.08.2017
 *      Author: thomas
 */

#ifndef MQTT_ADAPTER_HPP_
#define MQTT_ADAPTER_HPP_

/* c-runtime */
#include <list>
#include <vector>
#include <atomic>
/* frame */
#include <mosquittopp.h>
#include <spinlock.h>
#include <inherit_prio_mutex.h>

#include <mutex>


enum client_state {
	CLIENT_STATE_disconnect = 100,
	CLIENT_STATE_success = 0,
	CLIENT_STATE_connection_refused_protocol = 1,
	CLIENT_STATE_connection_identifier_rejected = 2,
	CLIENT_STATE_no_broker =3
};

enum mqtt_qos
{
	MQTT_QOS_0 = 0,
	MQTT_QOS_1 = 1,
	MQTT_QOS_2 = 2
};

class mqtt_provide;
class mqtt_subscribe;

class mqtt_client : protected mosqpp::mosquittopp
{
	public:
		mqtt_client(const char *id, const char *host, int port);
		~mqtt_client();
		int disconnect();
		int reconnect();
		int provide(mqtt_provide* _inst, int *_mid, const char *_topic, const char *_payload, enum mqtt_qos _qos, bool _retain);
		int consumer(mqtt_subscribe* _inst, int *_mid, const char *_sub, int _qos);


		mqtt_provide *create_mqtt_provide(char * const _topic, enum mqtt_qos _qos, bool _retain);
		mqtt_subscribe *create_mqtt_subscribe(char * const _sub, enum mqtt_qos _qos);

	protected:
		void on_connect(int _rc);
		void on_disconnect(int rc);
		void on_publish(int mid);
		void on_message(const struct mosquitto_message *message);
		void on_subscribe(int mid, int qos_count, const int *granted_qos);
		void on_unsubscribe(int _mid);
		void on_log(int _level, const char *_str);
		void on_error();


	private:
		enum client_state m_state;
		std::vector<mqtt_provide*>	m_provide_list;
		std::mutex m_provide_lock_list;
		os_system::spinlock m_provide_spinlock_list;

};


class mqtt_provide
{
	public:
		mqtt_provide(mqtt_client *_mqtt_client,char *_topic, enum mqtt_qos _qos, bool _retain);
		~mqtt_provide();
		int get_msg_id();
		int provide(const char * _payload);
		char *m_topic;
	private:
		int m_id;
		enum mqtt_qos m_qos;
		bool m_retain;
		mqtt_client *m_mqtt_client;
		os_system::inherit_prio_mutex m_mutex;
};


class mqtt_subscribe
{
	public:
		mqtt_subscribe (mqtt_client *_mqtt_client, char *_topic, enum mqtt_qos _qos);
		~mqtt_subscribe();
	private:
		int m_id;
		enum mqtt_qos m_qos;
		bool m_retain;
		mqtt_client *m_mqtt_client;
		os_system::inherit_prio_mutex m_mutex;
};




#endif /* MQTT_ADAPTER_HPP_ */
