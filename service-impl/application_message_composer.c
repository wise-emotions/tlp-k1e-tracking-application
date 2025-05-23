/*
 *  Created on: Nov 5, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#include <glib.h>

#include "logger.h"
#include "mqtt_client.h"

#include "service_configuration.h"

#include "tolling_gnss_sm_data.h"
#include "data_id_generator.h"

#include "application_message_composer.h"

JsonMapper *MessageComposer_create_payload_json_mapper_pos(MessageComposer *self, const PositionData fix, const gchar* msg_type);



void MessageComposer_send_message(MessageComposer *self, const GArray* fixes_to_send)
{
	GString *json_string = MessageComposer_create_message_from_fixes(self, fixes_to_send);
	logdbg("MQTT message to send:\n%s", json_string->str);
	//logdbg("Topic parameter: %s", MQTT_TOLLING_PL_POSDATA_TOPIC_PAR);

	mqtt_client_publish_message_on_topic_from_param(self->tolling_gnss_sm_data->mqtt_client, json_string->str, MQTT_TRACKING_TOPIC_PAR);
	g_string_free(json_string, TRUE);
}

MessageComposer *ApplicationMessageComposer_new(Tolling_Gnss_Sm_Data* data)
{
	MessageComposer *self = MessageComposer_new(data);
	return self;
}

GString *MessageComposer_create_event_message(const MessageComposer *self, const GArray* start_fix, const gchar* msg_type)
{

	JsonMapper *payload_json_mapper = MessageComposer_create_payload_json_mapper(self, start_fix,msg_type);

	GString *json_string = g_string_new(json_mapper_to_string(payload_json_mapper));

	mqtt_client_publish_message_on_topic_from_param(self->tolling_gnss_sm_data->mqtt_client, json_string->str, MQTT_TRACKING_TOPIC_PAR);
    g_string_free(json_string, TRUE);


	json_mapper_destroy(payload_json_mapper);

	return json_string;

}

GString *MessageComposer_create_event_message_pos(const MessageComposer *self, const PositionData fix, const gchar* msg_type)
{

	JsonMapper *payload_json_mapper = MessageComposer_create_payload_json_mapper_pos(self, fix, msg_type);

	GString *json_string = g_string_new(json_mapper_to_string(payload_json_mapper));
	loginfo("\n %s \n %s",msg_type, json_string->str);
	mqtt_client_publish_message_on_topic_from_param(self->tolling_gnss_sm_data->mqtt_client, json_string->str, MQTT_EVENTS_TOPIC_PAR);
    g_string_free(json_string, TRUE);


	json_mapper_destroy(payload_json_mapper);

	return json_string;

}


void ApplicationMessageComposer_destroy(MessageComposer *self)
{
	MessageComposer_destroy(self);
}
