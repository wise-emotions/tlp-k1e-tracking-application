/*
 * tolling-gnss-sm.c
 *
 *  Created on: Mar 16, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#include <stdlib.h>

#include "logger.h"
#include "configuration_store.h"

#include "positioning_service_proxy.h"
#include "network_manager_proxy.h"
#include "service_activation_sm.h"
#include "tolling_gnss_sm_data.h"
#include "application_events.h"
#include "application_events_common_names.h"
#include "application_notifications.h"
#include "service_monitor.h"
#include "connection_sm.h"
#include "axles_change_manager.h"

#include "tolling-gnss-sm.h"

typedef enum _TollingGnssStateId {
	tollingGnss_stopped = 0,
	tollingGnss_on_hold,
	tollingGnss_running,
	TOLLING_GNSS_STATES_NUM // This must always be the last one!
} TollingGnssStateId;

typedef struct _TollingGnssSm {
	State                  *states[TOLLING_GNSS_STATES_NUM];
	TollingGnssStateId      curr_state_id;
} TollingGnssSm;

typedef struct _Actions {
	void (*enter_state)(TollingGnssSm* tolling_gnss_sm);
	void (*exit_state)(TollingGnssSm* tolling_gnss_sm);

	void (*stop)(TollingGnssSm* tolling_gnss_sm);
	void (*on_hold)(TollingGnssSm* tolling_gnss_sm);
	void (*run)(TollingGnssSm* tolling_gnss_sm);

	void (*not_active)(TollingGnssSm* tolling_gnss_sm);
	void (*no_go)(TollingGnssSm* tolling_gnss_sm);
	void (*active)(TollingGnssSm* tolling_gnss_sm);

	void (*not_connected)(TollingGnssSm* tolling_gnss_sm);
	void (*connected)(TollingGnssSm* tolling_gnss_sm);

	void (*anomaly)(TollingGnssSm* tolling_gnss_sm);
	void (*no_anomaly)(TollingGnssSm* tolling_gnss_sm);

	void (*position_update)(TollingGnssSm* tolling_gnss_sm, const PositionData *position);
	void (*send_tolling_data)(TollingGnssSm* tolling_gnss_sm);
} Actions;

State* tolling_gnss_stopped_state_new(Tolling_Gnss_Sm_Data* data);
State* tolling_gnss_on_hold_state_new(Tolling_Gnss_Sm_Data* data);
State* tolling_gnss_running_state_new(Tolling_Gnss_Sm_Data* data);
void tolling_gnss_state_destroy(State *self);
const gchar *convert_tolling_gnss_state_to_string(const TollingGnssStateId state_id);

void tolling_gnss_sm_change_state(TollingGnssSm *self, const TollingGnssStateId next_state_id)
{
	if (self->curr_state_id == next_state_id) {
		logdbg("Already in target state %s, nothing to do", convert_tolling_gnss_state_to_string(next_state_id));
		return;
	}

	self->states[self->curr_state_id]->actions->exit_state(self);
	self->curr_state_id = next_state_id;
	self->states[self->curr_state_id]->actions->enter_state(self);
}

void tolling_gnss_not_implemented(TollingGnssSm* self)
{
	logdbg("Tolling GNSS action not implemented");
}

void tolling_gnss_enter_state_stopped(TollingGnssSm* self)
{
	logdbg("Tolling GNSS enter state stopped");

	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	ApplicationEvents_emit_event(curr_state_data->application_events, EVENT_TOLLING_GNSS_SM_ON_STOP);
}

void tolling_gnss_enter_state_on_hold(TollingGnssSm* self)
{
	logdbg("Tolling GNSS enter state on_hold");

	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	ApplicationEvents_emit_event(curr_state_data->application_events, EVENT_TOLLING_GNSS_SM_ON_HOLD);
	NetworkManagerProxy_on_hold(curr_state_data->network_manager_proxy);
	connection_sm_not_connected(curr_state_data->connection_sm);

}

void tolling_gnss_exit_state_on_hold(TollingGnssSm* self)
{
	logdbg("Tolling GNSS exit state on_hold");

	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	NetworkManagerProxy_resume(curr_state_data->network_manager_proxy);
}

void tolling_gnss_enter_state_running(TollingGnssSm* self)
{
	logdbg("Tolling GNSS enter state running");

    Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	ApplicationEvents_emit_event(curr_state_data->application_events, EVENT_TOLLING_GNSS_SM_START);
    service_activation_sm_start(curr_state_data->service_activation_sm);
	if(curr_state_data->axles_change_manager)
		AxlesChangeManager_notify_last_axles_change(curr_state_data->axles_change_manager);

}

void tolling_gnss_exit_state_running(TollingGnssSm* self)
{
	logdbg("Tolling GNSS exit state running");

	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
}

void tolling_gnss_stop(TollingGnssSm* self)
{
	logdbg("Tolling GNSS stop");
	tolling_gnss_sm_change_state(self, tollingGnss_stopped);
}

void tolling_gnss_on_hold(TollingGnssSm* self)
{
	logdbg("Tolling GNSS on_hold");
	tolling_gnss_sm_change_state(self, tollingGnss_on_hold);
}

void tolling_gnss_run(TollingGnssSm* self)
{
	logdbg("Tolling GNSS run");
	tolling_gnss_sm_change_state(self, tollingGnss_running);
}

void tolling_gnss_not_active(TollingGnssSm* self)
{
	logdbg("Tolling GNSS not active");

	// Forward the event to the inner state machine
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	service_activation_sm_not_active(curr_state_data->service_activation_sm);
	service_monitor_stop_tracking();
}

void tolling_gnss_no_go(TollingGnssSm* self)
{
	logdbg("Tolling GNSS no go");

	// Forward the event to the inner state machine
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	service_activation_sm_no_go(curr_state_data->service_activation_sm);
}

void tolling_gnss_active(TollingGnssSm* self)
{
	logdbg("Tolling GNSS active");

	// Forward the event to the inner state machine
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	service_activation_sm_active(curr_state_data->service_activation_sm);

	service_monitor_start_tracking();
}

void tolling_gnss_not_connected(TollingGnssSm* self)
{
	// Forward the event to the inner state machine
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	service_activation_sm_not_connected(curr_state_data->service_activation_sm);
}

void tolling_gnss_connected(TollingGnssSm* self)
{
	// Forward the event to the inner state machine
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	service_activation_sm_connected(curr_state_data->service_activation_sm);
}

void tolling_gnss_anomaly(TollingGnssSm* self)
{
	// Forward the event to the inner state machine
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	service_activation_sm_anomaly(curr_state_data->service_activation_sm);
}

void tolling_gnss_no_anomaly(TollingGnssSm* self)
{
	// Forward the event to the inner state machine
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	service_activation_sm_no_anomaly(curr_state_data->service_activation_sm);
}

void tolling_gnss_position_update_not_implemented(TollingGnssSm* self, const PositionData *position)
{
	logdbg("Tolling GNSS position update not implemented");
}

void tolling_gnss_position_update(TollingGnssSm* self, const PositionData *position)
{
	logdbg("Tolling GNSS position update");

	// Forward the event to the inner state machine
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	service_activation_sm_position_updated(curr_state_data->service_activation_sm, position);
}

void tolling_gnss_send_tolling_data(TollingGnssSm* self)
{
	logdbg("Tolling GNSS send tolling data");

	// Forward the event to the inner state machine
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	service_activation_sm_send_tolling_data(curr_state_data->service_activation_sm);
}

State* tolling_gnss_stopped_state_new(Tolling_Gnss_Sm_Data *data)
{
	State   *_this   = (State*)calloc(1, sizeof(State));
	Actions *actions = (Actions*)calloc(1, sizeof(Actions));

	actions->enter_state       = tolling_gnss_enter_state_stopped;
	actions->exit_state        = tolling_gnss_not_implemented;

	actions->stop              = tolling_gnss_not_implemented;
	actions->on_hold           = tolling_gnss_on_hold;
	actions->run               = tolling_gnss_run;

	actions->not_active        = tolling_gnss_not_implemented;
	actions->no_go             = tolling_gnss_not_implemented;
	actions->active            = tolling_gnss_not_implemented;

	actions->not_connected     = tolling_gnss_not_implemented;
	actions->connected         = tolling_gnss_not_implemented;

	actions->anomaly           = tolling_gnss_not_implemented;
	actions->no_anomaly        = tolling_gnss_not_implemented;

	actions->position_update   = tolling_gnss_position_update_not_implemented;
	actions->send_tolling_data = tolling_gnss_not_implemented;

	_this->actions = actions;
	_this->data = (Data*) data;

	return _this;
}

State* tolling_gnss_on_hold_state_new(Tolling_Gnss_Sm_Data* data)
{
	State   *_this   = (State*)calloc(1, sizeof(State));
	Actions *actions = (Actions*)calloc(1, sizeof(Actions));

	actions->enter_state       = tolling_gnss_enter_state_on_hold;
	actions->exit_state        = tolling_gnss_exit_state_on_hold;

	actions->stop              = tolling_gnss_stop;
	actions->on_hold           = tolling_gnss_not_implemented;
	actions->run               = tolling_gnss_run;

	actions->not_active        = tolling_gnss_not_implemented;
	actions->no_go             = tolling_gnss_not_implemented;
	actions->active            = tolling_gnss_not_implemented;

	actions->not_connected     = tolling_gnss_not_implemented;
	actions->connected         = tolling_gnss_not_implemented;

	actions->anomaly           = tolling_gnss_not_implemented;
	actions->no_anomaly        = tolling_gnss_not_implemented;

	actions->position_update   = tolling_gnss_position_update_not_implemented;
	actions->send_tolling_data = tolling_gnss_not_implemented;

	_this->actions = actions;
	_this->data = (Data*) data;

	return _this;
}

State* tolling_gnss_running_state_new(Tolling_Gnss_Sm_Data* data)
{
	State   *_this   = (State*)calloc(1, sizeof(State));
	Actions *actions = (Actions*)calloc(1, sizeof(Actions));

	actions->enter_state       = tolling_gnss_enter_state_running;
	actions->exit_state        = tolling_gnss_exit_state_running;

	actions->stop              = tolling_gnss_stop;
	actions->on_hold           = tolling_gnss_on_hold;
	actions->run               = tolling_gnss_not_implemented;

	actions->not_active        = tolling_gnss_not_active;
	actions->no_go             = tolling_gnss_no_go;
	actions->active            = tolling_gnss_active;

	actions->not_connected     = tolling_gnss_not_connected;
	actions->connected         = tolling_gnss_connected;

	actions->anomaly           = tolling_gnss_anomaly;
	actions->no_anomaly        = tolling_gnss_no_anomaly;

	actions->position_update   = tolling_gnss_position_update;
	actions->send_tolling_data = tolling_gnss_send_tolling_data;

	_this->actions = actions;
	_this->data = (Data*) data;

	return _this;
}

const gchar *convert_tolling_gnss_state_to_string(const TollingGnssStateId state_id)
{
	static const gchar *state_name[] = {
		"tollingGnss_stopped",
		"tollingGnss_on_hold",
		"tollingGnss_running"
	};

	return state_name[state_id];
}

void tolling_gnss_sm_stop(TollingGnssSm *self)
{
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	PositionData *position = PositioningServiceProxy_get_current_position(curr_state_data->positioning_service_proxy);
	position->total_dist =  odometer_get_trip_distance(curr_state_data);
 	JsonMapper* payload_json_mapper = MessageComposer_create_event_message_pos(curr_state_data->message_composer, *position,"stop");
 	curr_state_data->first_fix = FALSE;

	self->states[self->curr_state_id]->actions->stop(self);
}

void tolling_gnss_sm_on_hold(TollingGnssSm *self)
{

	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	PositionData *position = PositioningServiceProxy_get_current_position(curr_state_data->positioning_service_proxy);
	position->total_dist =  odometer_get_trip_distance(curr_state_data);
	JsonMapper* payload_json_mapper = MessageComposer_create_event_message_pos(curr_state_data->message_composer, *position,"stop");

	self->states[self->curr_state_id]->actions->on_hold(self);

}

void tolling_gnss_sm_run(TollingGnssSm *self)
{
	self->states[self->curr_state_id]->actions->run(self);
}

static void tolling_gnss_sm_not_active(gpointer gpointer_self)
{
	TollingGnssSm* self = (TollingGnssSm *)gpointer_self;
	self->states[self->curr_state_id]->actions->not_active(self);
}

static void tolling_gnss_sm_no_go(gpointer gpointer_self)
{
	TollingGnssSm* self = (TollingGnssSm *)gpointer_self;
	self->states[self->curr_state_id]->actions->no_go(self);
}

static void tolling_gnss_sm_active(gpointer gpointer_self)
{
	TollingGnssSm* self = (TollingGnssSm *)gpointer_self;
	self->states[self->curr_state_id]->actions->active(self);
}

static void tolling_gnss_sm_connected(gpointer gpointer_self)
{
	TollingGnssSm* self = (TollingGnssSm *)gpointer_self;
	self->states[self->curr_state_id]->actions->connected(self);
}

static void tolling_gnss_sm_not_connected(gpointer gpointer_self)
{
	TollingGnssSm* self = (TollingGnssSm *)gpointer_self;
	self->states[self->curr_state_id]->actions->not_connected(self);
}

static void tolling_gnss_sm_anomaly(gpointer gpointer_self)
{
	TollingGnssSm* self = (TollingGnssSm *)gpointer_self;
	self->states[self->curr_state_id]->actions->anomaly(self);
}

static void tolling_gnss_sm_no_anomaly(gpointer gpointer_self)
{
	TollingGnssSm* self = (TollingGnssSm *)gpointer_self;
	self->states[self->curr_state_id]->actions->no_anomaly(self);
}

static void tolling_gnss_sm_notify_network_anomaly(gpointer gpointer_self)
{
	logdbg("Workaround to cope with the single Anomaly state: no transition here, as it is triggered by 'generic anomaly'");
	TollingGnssSm* self = (TollingGnssSm *)gpointer_self;
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	ApplicationNotifications_notify_anomaly_network(curr_state_data->application_notifications, TRUE);
}

static void tolling_gnss_sm_notify_no_network_anomaly(gpointer gpointer_self)
{
	logdbg("Workaround to cope with the single Anomaly state: no transition here, as it is triggered by 'generic anomaly'");
	TollingGnssSm* self = (TollingGnssSm *)gpointer_self;
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	ApplicationNotifications_notify_anomaly_network(curr_state_data->application_notifications, FALSE);
}

static void tolling_gnss_sm_notify_gnss_anomaly(gpointer gpointer_self)
{
	TollingGnssSm* self = (TollingGnssSm *)gpointer_self;
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	ApplicationNotifications_notify_anomaly_gnss(curr_state_data->application_notifications, TRUE);
}

static void tolling_gnss_sm_notify_no_gnss_anomaly(gpointer gpointer_self)
{
	TollingGnssSm* self = (TollingGnssSm *)gpointer_self;
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	ApplicationNotifications_notify_anomaly_gnss(curr_state_data->application_notifications, FALSE);
}

static void tolling_gnss_sm_position_updated(gpointer gpointer_self)
{
	TollingGnssSm* self = (TollingGnssSm *)gpointer_self;
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	PositionData *position = PositioningServiceProxy_get_current_position(curr_state_data->positioning_service_proxy);
	self->states[self->curr_state_id]->actions->position_update(self, position);

	//invio messaggio start
	if(curr_state_data->first_fix == FALSE){
		position->total_dist = 0.0;
 	    JsonMapper* payload_json_mapper = MessageComposer_create_event_message_pos(curr_state_data->message_composer, *position,"start");
 	   curr_state_data->first_fix = TRUE;
	}
}

/*
void tolling_gnss_sm_send_tolling_data(TollingGnssSm *self)
{
	self->states[self->curr_state_id]->actions->send_tolling_data(self);
}
*/

TollingGnssSm *tolling_gnss_sm_new(Tolling_Gnss_Sm_Data* data)
{
	TollingGnssSm *self = g_malloc0(sizeof(TollingGnssSm));
	self->states[tollingGnss_stopped] = tolling_gnss_stopped_state_new(data);
	self->states[tollingGnss_on_hold] = tolling_gnss_on_hold_state_new(data);
	self->states[tollingGnss_running] = tolling_gnss_running_state_new(data);

	data->service_activation_sm = service_activation_sm_new(data);

	self->curr_state_id = tollingGnss_stopped;

	ApplicationEvents_register_event(
		data->application_events,
		EVENT_EVENTS_LOGIC_SERVICE_STATUS_NOT_ACTIVE,
		tolling_gnss_sm_not_active,
		self);
	ApplicationEvents_register_event(
		data->application_events,
		EVENT_EVENTS_LOGIC_SERVICE_STATUS_NO_GO,
		tolling_gnss_sm_no_go,
		self);
	ApplicationEvents_register_event(
		data->application_events,
		EVENT_EVENTS_LOGIC_SERVICE_STATUS_ACTIVE,
		tolling_gnss_sm_active,
		self);
	ApplicationEvents_register_event(
		data->application_events,
		EVENT_POSITIONING_SERVICE_PROXY_POSITION_UPDATED,
		tolling_gnss_sm_position_updated,
		self);
	ApplicationEvents_register_event(
		data->application_events,
		EVENT_NETWORK_MANAGER_PROXY_NETWORK_CONNECTED,
		tolling_gnss_sm_connected,
		self);
	ApplicationEvents_register_event(
		data->application_events,
		EVENT_NETWORK_MANAGER_PROXY_NETWORK_NOT_CONNECTED,
		tolling_gnss_sm_not_connected,
		self);
	ApplicationEvents_register_event(
		data->application_events,
		EVENT_EVENTS_LOGIC_GENERIC_ANOMALY_PRESENT,
		tolling_gnss_sm_anomaly,
		self);
	ApplicationEvents_register_event(
		data->application_events,
		EVENT_EVENTS_LOGIC_GENERIC_ANOMALY_ABSENT,
		tolling_gnss_sm_no_anomaly,
		self);
	ApplicationEvents_register_event(
		data->application_events,
		EVENT_EVENTS_LOGIC_NETWORK_ANOMALY_PRESENT,
		tolling_gnss_sm_notify_network_anomaly,
		self);
	ApplicationEvents_register_event(
		data->application_events,
		EVENT_EVENTS_LOGIC_NETWORK_ANOMALY_ABSENT,
		tolling_gnss_sm_notify_no_network_anomaly,
		self);
	ApplicationEvents_register_event(
		data->application_events,
		EVENT_EVENTS_LOGIC_GNSS_ANOMALY_PRESENT,
		tolling_gnss_sm_notify_gnss_anomaly,
		self);
	ApplicationEvents_register_event(
		data->application_events,
		EVENT_EVENTS_LOGIC_GNSS_ANOMALY_ABSENT,
		tolling_gnss_sm_notify_no_gnss_anomaly,
		self);



	return self;
}

void tolling_gnss_sm_destroy(TollingGnssSm *self)
{

	service_monitor_stop_tracking();
	// Retrieve data from one of the states, doesn't matter which one
	Tolling_Gnss_Sm_Data* data = (Tolling_Gnss_Sm_Data*)self->states[0]->data;
	service_activation_sm_destroy(data->service_activation_sm);

	tolling_gnss_state_destroy(self->states[tollingGnss_stopped]);
	tolling_gnss_state_destroy(self->states[tollingGnss_on_hold]);
	tolling_gnss_state_destroy(self->states[tollingGnss_running]);

	g_free(self);
}

void tolling_gnss_state_destroy(State *self)
{
	free(self->actions);
	free(self);
}
