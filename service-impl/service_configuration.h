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

#define MQTT_TRACKING_TOPIC_PAR "tracking_topic"
#define MQTT_TRACKING_TOPIC_PAR_DESCR "MQTT topic for tracking"
#define MQTT_TRACKING_TOPIC_PAR_RANGE NULL

#define MQTT_EVENTS_TOPIC_PAR "tracking_events_topic"
#define MQTT_EVENTS_TOPIC_PAR_DESCR "MQTT topic for events"
#define MQTT_EVENTS_TOPIC_PAR_RANGE NULL

#define TRACKING_EVENTS_WAIT_CONFIRMATION_TIMEOUT_PAR         "tracking_event_wait_confirmation_timeout"
#define TRACKING_EVENTS_WAIT_CONFIRMATION_TIMEOUT_PAR_DESCR   "tracking event wait confirmation timeout"
#define TRACKING_EVENTS_WAIT_CONFIRMATION_TIMEOUT_PAR_DFLT    (default_val){.nval = 10}
#define TRACKING_EVENTS_WAIT_CONFIRMATION_TIMEOUT_PAR_RANGE   "[0,]"

extern gint           iparam(const gchar *name);
extern gint           fparam(const gchar *name);
extern gboolean       bparam(const gchar *name);
extern const gchar   *sparam(const gchar *name);
extern const gchar  **svparam(const gchar *name);

void add_custom_parameters();
