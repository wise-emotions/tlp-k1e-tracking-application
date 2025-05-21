/*
 * message_composer.c
 *
 *  Created on: Oct 27, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#include "message_composer.h"

#include <glib.h>

#include "logger.h"
#include "json_mapper.h"

#include "gnss_fix_data.h"
#include "trip_id_manager.h"
#include "tolling_gnss_sm_data.h"
#include "gnss_fixes_transmission_queue.h"
#include "service_monitor.h"

#include "utils.h"
#include "tolling_manager_proxy.h"

MessageComposer* MessageComposer_new(Tolling_Gnss_Sm_Data* data)
{
	MessageComposer *self = g_new0(MessageComposer, 1);
	self->tolling_gnss_sm_data = data;

	return self;
}

void MessageComposer_destroy(MessageComposer *self)
{
	// GnssFixesTransmissionQueue destructor moved here due to incorrect deallocation sequence in tolling application
	self->tolling_gnss_sm_data->gnss_fixes_transmision_queue->dtor(self->tolling_gnss_sm_data->gnss_fixes_transmision_queue);
	g_free(self);
}

JsonMapper *MessageComposer_create_payload(const MessageComposer *self, const gchar* msg_type)
{
	JsonMapper *payload_json_mapper = json_mapper_create((const guchar*)"{}");

	json_mapper_add_property(payload_json_mapper, (const guchar*)"trip_id",     json_type_string, (const gpointer) trip_id_manager_get_trip_id(self->tolling_gnss_sm_data->trip_id_manager)->str);

	json_mapper_add_property(payload_json_mapper, (const guchar*)"messageType",     json_type_string, (const gpointer) msg_type);

	json_mapper_add_property(payload_json_mapper, (const guchar*)"obuId",     json_type_string, (const gpointer) self->tolling_gnss_sm_data->obu_id->str);

	const gchar* plate;
	TollingManagerProxy_get_license_plate(self->tolling_gnss_sm_data->tolling_manager_proxy,&plate);
	json_mapper_add_property(payload_json_mapper, (const guchar*)"plate",     json_type_string, (const gpointer) plate);

	GString *create_time = date_time_now_utc_to_iso8601(); // milliseconds
    json_mapper_add_property(payload_json_mapper, (const guchar*)"eventTimestamp",         json_type_string,   (const gpointer) create_time->str);

	return payload_json_mapper;
}

JsonMapper *MessageComposer_create_fixdata_json_mapper(const GArray* fixes_to_send)
{
	const GnssFixData *fix_data = NULL;

	JsonMapper *fixdata_json_mapper = json_mapper_create((const guchar*)"[]");
	for (guint  i = 0; i < fixes_to_send->len; i++)
	{
		fix_data = g_array_index(fixes_to_send, const GnssFixData*, i);

		JsonMapper *single_fix_json_mapper = gnss_fix_data_to_json_mapper(fix_data);
		json_mapper_add_mapper_to_array(fixdata_json_mapper, single_fix_json_mapper);

		json_mapper_destroy(single_fix_json_mapper);
	}

	//update monitor with latest nogo value
	if(fix_data != NULL)
		service_monitor_update_metrics(fix_data);

	return fixdata_json_mapper;
}

JsonMapper *MessageComposer_create_fixdata_json_mapper_pos(PositionData fix)
{
	 GnssFixData fix_data;

	JsonMapper *fixdata_json_mapper = json_mapper_create((const guchar*)"[]");

	loginfo("fix lat %f",fix.latitude);

	fix_data.latitude           = fix.latitude;
	fix_data.longitude          = fix.longitude;
	fix_data.altitude           = fix.altitude;

	fix_data.fix_time_epoch     = fix.date_time;
	fix_data.gps_speed          = fix.speed;

	fix_data.gps_heading        = fix.heading;
	fix_data.satellites_for_fix = fix.sat_for_fix->len;

	fix_data.hdop               = fix.hdop;
	fix_data.total_distance_m   = fix.total_dist*1000;

	JsonMapper *single_fix_json_mapper = gnss_fix_data_to_json_mapper(&fix_data);
	json_mapper_add_mapper_to_array(fixdata_json_mapper, single_fix_json_mapper);

	json_mapper_destroy(single_fix_json_mapper);


	//update monitor with latest nogo value
	//if(fix_data != NULL)
	service_monitor_update_metrics(&fix_data);

	return fixdata_json_mapper;
}

JsonMapper *MessageComposer_create_payload_json_mapper(MessageComposer *self, const GArray* fixes_to_send, const gchar* msg_type)
{
	//creating the message payload object with default properties
	JsonMapper *payload_json_mapper = MessageComposer_create_payload(self, msg_type);

	JsonMapper *fixdata_json_mapper = MessageComposer_create_fixdata_json_mapper(fixes_to_send);
	json_mapper_add_property_from_mapper(payload_json_mapper, (const guchar*)"locations", fixdata_json_mapper);
	json_mapper_destroy(fixdata_json_mapper);

	return payload_json_mapper;
}

JsonMapper *MessageComposer_create_payload_json_mapper_pos(MessageComposer *self, const PositionData fix, const gchar* msg_type)
{
	//creating the message payload object with default properties
	JsonMapper *payload_json_mapper = MessageComposer_create_payload(self, msg_type);

	JsonMapper *fixdata_json_mapper = MessageComposer_create_fixdata_json_mapper_pos(fix);
	json_mapper_add_property_from_mapper(payload_json_mapper, (const guchar*)"locations", fixdata_json_mapper);
	json_mapper_destroy(fixdata_json_mapper);

	return payload_json_mapper;
}

GString *MessageComposer_create_message_from_fixes(MessageComposer *self, const GArray* fixes_to_send)
{
	JsonMapper *payload_json_mapper = MessageComposer_create_payload_json_mapper(self, fixes_to_send, "tracking");

	GString *json_string = g_string_new(json_mapper_to_string(payload_json_mapper));

	json_mapper_destroy(payload_json_mapper);

	return json_string;
}
