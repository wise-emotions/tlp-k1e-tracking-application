/*
 * axles_change_manager.c
 *
 *  Created on: Mar 2, 2022
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#include "logger.h"
#include "json_mapper.h"
#include "mqtt_client.h"

#include "tolling_gnss_sm_data.h"
#include "configuration_store.h"
#include "application_events.h"
#include "application_events_common_names.h"
#include "tolling_manager_proxy.h"
#include "service_configuration.h"

#include "axles_change_manager.h"

#define INVALID_AXLES_CHANGE_TRN_ID "0"
#define RESPONSE_OK 0

typedef enum {
	requested = 1,
	approved,
	rejected,
}
axles_change_status_t;

typedef struct _AxlesChangeManager
{
	Tolling_Gnss_Sm_Data *data;
	GString *current_axles_change_trn_id;
	axles_change_status_t axles_change_status;
	guint 	current_axles;


} AxlesChangeManager;

// Forward declarations
void     AxlesChangeManager_init_trn_id(AxlesChangeManager *self);
void     AxlesChangeManager_init_current_total_axles(AxlesChangeManager *self);
void     AxlesChangeManager_init_axles_change_status_and_emit_event(AxlesChangeManager *self);
void     AxlesChangeManager_init_axles_change_status(AxlesChangeManager *self);
void     AxlesChangeManager_persist_axles_change_status(AxlesChangeManager *self);
void     AxlesChangeManager_persist_last_current_axles(AxlesChangeManager *self);
void     AxlesChangeManager_set_and_persist_trn_id(AxlesChangeManager *self, const gchar *trn_id);
void     AxlesChangeManager_set_and_persist_current_total_axles(AxlesChangeManager *self, guint new_axles);
gboolean AxlesChangeManager_extract_response_from_ack_message(const char *message, const gchar **received_trn_id, int *response);
gboolean AxlesChangeManager_does_response_match_request(const gchar *response_trn_id_string, const gchar *saved_axles_change_trn_id);

AxlesChangeManager *AxlesChangeManager_new(Tolling_Gnss_Sm_Data *data)
{
	AxlesChangeManager* self = g_new0(AxlesChangeManager, 1);

	self->data = data;

	AxlesChangeManager_init_trn_id(self);
	AxlesChangeManager_init_current_total_axles(self);
	AxlesChangeManager_init_axles_change_status_and_emit_event(self);

	return self;
}

void AxlesChangeManager_destroy(AxlesChangeManager *self)
{
	g_string_free(self->current_axles_change_trn_id, TRUE);
	g_free(self);
}

void AxlesChangeManager_init_trn_id(AxlesChangeManager *self)
{
	self->current_axles_change_trn_id = g_string_new("");

	const gchar *temp_trn_id = INVALID_AXLES_CHANGE_TRN_ID;
	gboolean value_found = ConfigurationStore_get_string(self->data->configuration_store, (const guchar*)"axles_change_trn_id", &temp_trn_id);
	logdbg("current_axles_change_trn_id %s in configuration store", value_found ? "found" : "not found");
	g_string_printf(self->current_axles_change_trn_id, "%s", temp_trn_id);
	loginfo("current_axles_change_trn_id = %s", self->current_axles_change_trn_id->str);
}

void AxlesChangeManager_init_current_total_axles(AxlesChangeManager *self){

	gint curr_ax_trailer = 0;
	gint curr_default_ax = 0;

	//trailer axles
	TollingManagerProxy_get_current_axles(self->data->tolling_manager_proxy,&curr_ax_trailer);

	//default axles
	gboolean value_found = ConfigurationStore_get_int(self->data->configuration_store, (const guchar*)"default_axis", &curr_default_ax);

	self->current_axles = curr_ax_trailer + curr_default_ax;

	loginfo("current_total_axles = %d", self->current_axles);

}

void AxlesChangeManager_init_axles_change_status(AxlesChangeManager *self){

	gint int_status = approved;
	gboolean value_found = ConfigurationStore_get_int(self->data->configuration_store, (const guchar*)"axles_change_status", &int_status);

	logdbg("axles_change_status %s in configuration store", value_found ? "found" : "not found");
	self->axles_change_status = int_status;

	loginfo("axles_change_status = %d", self->axles_change_status);

}

void AxlesChangeManager_init_axles_change_status_and_emit_event(AxlesChangeManager *self)
{
	gint int_status = approved;
	gboolean value_found = ConfigurationStore_get_int(self->data->configuration_store, (const guchar*)"axles_change_status", &int_status);

	logdbg("axles_change_status %s in configuration store", value_found ? "found" : "not found");
	self->axles_change_status = int_status;

	loginfo("axles_change_status = %d", self->axles_change_status);

	switch(self->axles_change_status)
	{
	case requested:
		AxlesChangeManager_notify_last_axles_change(self);
		AxlesChangeManager_persist_last_current_axles(self);
		ApplicationEvents_emit_event(self->data->application_events, EVENT_AXLES_CHANGE_MANAGER_AXLES_CHANGE_REQUESTED);
		break;
	case approved:
		ApplicationEvents_emit_event(self->data->application_events, EVENT_AXLES_CHANGE_MANAGER_AXLES_CHANGE_APPROVED);
		break;
	case rejected:
		self->axles_change_status = requested;
		AxlesChangeManager_notify_last_axles_change(self);
		AxlesChangeManager_persist_last_current_axles(self);
		ApplicationEvents_emit_event(self->data->application_events, EVENT_AXLES_CHANGE_MANAGER_AXLES_CHANGE_REQUESTED);
	break;
	default:
		logerr("Unknown axles_change_status");
		break;
	}
}

void AxlesChangeManager_notify_axles_change(AxlesChangeManager *self, const guint new_axles, const gchar *trn_id){

	EventsLogic* events_logic = self->data->events_logic;

	if(EventsLogic_get_service_activation(events_logic)){
		loginfo("sending trn_id:%s axles:%d",trn_id, new_axles);
		JsonMapper* payload_mapper = json_mapper_create((const guchar*)"{}");
		json_mapper_add_property(payload_mapper,(const guchar*)"data",json_type_string,(gpointer)"axles_change");
		json_mapper_add_property(payload_mapper,(const guchar*)"trn_id",json_type_string,(gpointer)trn_id);

		json_object *value = json_object_new_object();
		json_object_object_add(value, "axles", json_object_new_int(new_axles));
		json_mapper_add_property(payload_mapper,(const guchar*)"value",json_type_object,(gpointer)value);

		mqtt_client_publish_message_on_topic_from_param(self->data->mqtt_client,json_mapper_to_string_pretty(payload_mapper),MQTT_TOLLING_AXLES_CONFIGURATION_TOPIC_PAR);

	}
}

void AxlesChangeManager_notify_last_axles_change(AxlesChangeManager *self){

	if(self->axles_change_status != approved){

		AxlesChangeManager_init_current_total_axles(self);

		guint current_total_axles = 0;

		if(self->current_axles != 0 )
			AxlesChangeManager_notify_axles_change(self,self->current_axles,self->current_axles_change_trn_id->str);

	}
}

void AxlesChangeManager_axles_change_requested(AxlesChangeManager *self, const guint new_axles, const gchar *trn_id)
{
	loginfo("Received Axles change request with new_axles = %u, trn_id = '%s'", new_axles, trn_id);
	gint curr_total_ax = 0;

	gboolean value_found = ConfigurationStore_get_int(self->data->configuration_store, (const guchar*)"current_total_axles", &curr_total_ax);

	logdbg("current_total_axles %s in configuration store", value_found ? "found" : "not found");
	loginfo("curr total axles %d new axles %d",curr_total_ax,new_axles);
	if (g_strcmp0(trn_id, "") != 0 && (curr_total_ax != new_axles )) {
		loginfo("Changing to requested state");
		AxlesChangeManager_notify_axles_change(self,new_axles,trn_id);
		AxlesChangeManager_set_and_persist_current_total_axles(self, new_axles);
		self->axles_change_status = requested;
		AxlesChangeManager_persist_axles_change_status(self);
		AxlesChangeManager_set_and_persist_trn_id(self, trn_id);
		ApplicationEvents_emit_event(self->data->application_events, EVENT_AXLES_CHANGE_MANAGER_AXLES_CHANGE_REQUESTED);
	} else {
		loginfo("This change doesn't require explicit approval from the tolling charger");
	//	self->axles_change_status = approved;
		AxlesChangeManager_set_and_persist_current_total_axles(self, new_axles);
		AxlesChangeManager_persist_axles_change_status(self);
	//	ApplicationEvents_emit_event(self->data->application_events, EVENT_AXLES_CHANGE_MANAGER_AXLES_CHANGE_APPROVED);
	}
}

void AxlesChangeManager_persist_axles_change_status(AxlesChangeManager *self)
{
	gboolean success = ConfigurationStore_set_int(self->data->configuration_store, (const guchar*)"axles_change_status", self->axles_change_status);
	if (success == FALSE)
	{
		logerr("Could not persist axles_change_status");
	}
}

void AxlesChangeManager_persist_last_current_axles(AxlesChangeManager *self)
{
	gboolean success = ConfigurationStore_set_int(self->data->configuration_store, (const guchar*)"current_total_axles", self->current_axles);
	if (success == FALSE)
	{
		logerr("Could not persist current_total_axles");
	}
}

void AxlesChangeManager_set_and_persist_trn_id(AxlesChangeManager *self, const gchar *trn_id)
{
	g_string_printf(self->current_axles_change_trn_id, "%s", trn_id);
	gboolean success = ConfigurationStore_set_string(self->data->configuration_store, (const guchar*)"axles_change_trn_id", trn_id);
	if (success == FALSE)
	{
		logerr("Could not persist trn_id");
	}
}

void AxlesChangeManager_set_and_persist_current_total_axles(AxlesChangeManager *self, guint new_axles)
{
	self->current_axles = new_axles;
	AxlesChangeManager_persist_last_current_axles(self);
}

void AxlesChangeManager_axles_change_ack_callback(AxlesChangeManager *self, const char *message)
{
	logdbg("Received message: %s", message);

	gchar *received_trn_id = NULL;
	int response = -1;

	gboolean is_message_valid = AxlesChangeManager_extract_response_from_ack_message(message, (const gchar **)&received_trn_id, &response);
	if (is_message_valid == FALSE)
	{
		logerr("The received response is not valid");
		return;
	} else {
		logdbg("received_trn_id = %s, response = %d", received_trn_id, response);
	}

	if (self->axles_change_status != requested)
	{
		logwarn("The received response is unexpected, it will be ignored");
		return;
	}

	gboolean response_matches_request = AxlesChangeManager_does_response_match_request(received_trn_id, self->current_axles_change_trn_id->str);
	g_free(received_trn_id);
	if (response_matches_request == FALSE)
	{
		logdbg("The received response does not match the current request");
		return;
	}

	if (response == RESPONSE_OK)
	{
		loginfo("Axles change approved by tolling charger");
		self->axles_change_status = approved;
		AxlesChangeManager_persist_axles_change_status(self);
		ApplicationEvents_emit_event(self->data->application_events, EVENT_AXLES_CHANGE_MANAGER_AXLES_CHANGE_APPROVED);
	} else {
		logwarn("Axles change rejected by tolling charger with response: %d", response);
		logwarn("nothing to do in this case, keep mantaining to_be_approved status");

	}
}

gboolean AxlesChangeManager_extract_response_from_ack_message(const char *message, const gchar **received_trn_id, int *response)
{
	GString* payload;
	mqtt_client_get_payload(message, &payload);
	if (payload == NULL) {
		logerr("No payload found");
		return FALSE;
	}
	logdbg("payload = %s", payload->str);

	JsonMapper *payload_json_mapper = json_mapper_create((const guchar *)payload->str);
	g_string_free(payload, TRUE);

	if (payload_json_mapper == NULL) {
		logerr("Invalid json payload");
		return FALSE;
	}

	gboolean is_message_valid = TRUE;
	gint property_found;

	const char *trn_id_from_json;
	property_found = json_mapper_get_property_as_string(payload_json_mapper, (const guchar*)"trn_id", &trn_id_from_json);
	if (property_found != JSON_MAPPER_NO_ERROR)
	{
		logerr("Malformed JSON: missing property 'trn_id'");
		is_message_valid = FALSE;
	}

	property_found = json_mapper_get_property_as_int(payload_json_mapper, (const guchar*)"response", response);
	if (property_found != JSON_MAPPER_NO_ERROR)
	{
		logerr("Malformed JSON: missing property 'response'");
		is_message_valid = FALSE;
	}

	if (is_message_valid)
	{
		// This is required because destroying the json_mapper also destroys the string trn_id_from_json
		*received_trn_id = g_strdup((const gchar *)trn_id_from_json);
	}

	json_mapper_destroy(payload_json_mapper);

	return is_message_valid;
}

gboolean AxlesChangeManager_does_response_match_request(const gchar *response_trn_id_string, const gchar *saved_axles_change_trn_id)
{
	logdbg("response_trn_id_string = '%s', saved_trn_id_string = '%s'", response_trn_id_string, saved_axles_change_trn_id);

	gboolean response_matches_request;
	if (g_strcmp0(response_trn_id_string, saved_axles_change_trn_id) == 0)
	{
		response_matches_request = TRUE;
		//logdbg("Ok: current transaction");
	} else {
		response_matches_request = FALSE;
		//logdbg("Different transaction");
	}

	return response_matches_request;
}

