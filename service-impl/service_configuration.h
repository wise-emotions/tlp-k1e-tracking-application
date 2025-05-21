/*
 * service_configuration.h
 *
 * Created on: Dec 4, 2019
 * Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#pragma once

#include "configuration.h"


#define CONFIGURATION_PERSISTENCE_PAR "configuration_persistence_file"
#define CONFIGURATION_PERSISTENCE_DESCR "Name of the JSON configuration persistence file"
#define CONFIGURATION_PERSISTENCE_RANGE NULL

#define MQTT_TOLLING_PL_POSDATA_TOPIC_PAR "tolling_pl_posdata_topic"
#define MQTT_TOLLING_PL_POSDATA_TOPIC_PAR_DESCR "MQTT topic for tolling PL"
#define MQTT_TOLLING_PL_POSDATA_TOPIC_PAR_RANGE NULL

#define MQTT_TOLLING_AXLES_CONFIGURATION_TOPIC_PAR "tolling_axles_configuration_topic"
#define MQTT_TOLLING_AXLES_CONFIGURATION_TOPIC_PAR_DESCR "MQTT topic to receive axles configuration changes ACKs for tolling PL"
#define MQTT_TOLLING_AXLES_CONFIGURATION_TOPIC_PAR_RANGE NULL


void add_custom_parameters();
