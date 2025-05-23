/*
 * service_activation_sm.c
 *
 *  Created on: Mar 22, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#include "service_activation_sm.h"

#include <stdlib.h>
#include <stdio.h>

#include "logger.h"
#include "hmi_proxy.h"

#include "connection_sm.h"
#include "gnss_fixes_transmission_queue.h"
#include "gnss_fix_filter.h"
#include "tolling_gnss_sm_data.h"
#include "odometer.h"
#include "trip_id_manager.h"
#include "network_manager_proxy.h"
#include "application_notifications.h"
#include "gnss_fix_data.h"


typedef enum _ServiceActivationStateId {
	service_activation_not_active = 0,
	service_activation_no_go,
	service_activation_active,
	SERVICE_ACTIVATION_STATES_NUM
} ServiceActivationStateId;

typedef struct _ServiceActivationSm {
	State *states[SERVICE_ACTIVATION_STATES_NUM];
	ServiceActivationStateId  curr_state_id;
	gboolean started;
	gboolean first_fix;
} ServiceActivationSm;

typedef struct _Actions {
	void (*enter_state)(ServiceActivationSm* service_activation_sm);
	void (*exit_state)(ServiceActivationSm* service_activation_sm);

	void (*tolling_active)(ServiceActivationSm* service_activation_sm);
	void (*tolling_no_go)(ServiceActivationSm* service_activation_sm);
	void (*tolling_not_active)(ServiceActivationSm* service_activation_sm);

	void (*not_connected)(ServiceActivationSm* service_activation_sm);
	void (*connected)(ServiceActivationSm* service_activation_sm);

	void (*anomaly)(ServiceActivationSm* service_activation_sm);
	void (*no_anomaly)(ServiceActivationSm* service_activation_sm);

	void (*position_update)(ServiceActivationSm* service_activation_sm, const PositionData *position);
	void (*send_tolling_data)(ServiceActivationSm* service_activation_sm);
} Actions;

State* service_activation_not_active_state_new(Tolling_Gnss_Sm_Data* data);
State* service_activation_no_go_state_new(Tolling_Gnss_Sm_Data* data);
State* service_activation_active_state_new(Tolling_Gnss_Sm_Data* data);
void service_activation_state_destroy(State *self);
const gchar *convert_service_activation_state_to_string(const ServiceActivationStateId state_id);

ServiceActivationSm *service_activation_sm_new(Tolling_Gnss_Sm_Data* data)
{
	ServiceActivationSm *self = (ServiceActivationSm *)g_malloc0(sizeof(ServiceActivationSm));

	data->connection_sm = connection_sm_new(data);

	self->states[service_activation_not_active] = service_activation_not_active_state_new(data);
	self->states[service_activation_no_go]      = service_activation_no_go_state_new(data);
	self->states[service_activation_active]     = service_activation_active_state_new(data);

	self->curr_state_id = service_activation_not_active;
	self->started = FALSE;
	self->first_fix = FALSE;
	return self;
}

void service_activation_sm_destroy(ServiceActivationSm *self)
{
	Tolling_Gnss_Sm_Data* data = (Tolling_Gnss_Sm_Data*)self->states[0]->data;

	service_activation_state_destroy(self->states[service_activation_not_active]);
	service_activation_state_destroy(self->states[service_activation_no_go]);
	service_activation_state_destroy(self->states[service_activation_active]);

	connection_sm_destroy(data->connection_sm);
	g_free(self);
}

void service_activation_state_destroy(State *self)
{
	free(self->actions);
	free(self);
}

void service_activation_sm_change_state(ServiceActivationSm *self, const ServiceActivationStateId next_state_id)
{
	if (self->curr_state_id == next_state_id) {
		logdbg("Already in target state %s, nothing to do", convert_service_activation_state_to_string(next_state_id));
		return;
	}

	self->states[self->curr_state_id]->actions->exit_state(self);
	self->curr_state_id = next_state_id;
	self->states[self->curr_state_id]->actions->enter_state(self);
}

void service_activation_not_implemented(ServiceActivationSm* self)
{
	logdbg("Service activation action not implemented");
}

void service_activation_enter_state_not_active(ServiceActivationSm* self)
{
	logdbg("Service activation enter state not active");
}

void service_activation_enter_state_no_go(ServiceActivationSm* self)
{
	logdbg("Service activation enter state no go");
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	ApplicationNotifications_notify_no_go_service(curr_state_data->application_notifications, TRUE);
}

void service_activation_exit_state_no_go(ServiceActivationSm* self)
{
	logdbg("Service activation exit state no go");
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	ApplicationNotifications_notify_no_go_service(curr_state_data->application_notifications, FALSE);
}

void service_activation_enter_state_active(ServiceActivationSm* self)
{
	logdbg("Service activation enter state active");
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	odometer_reset(curr_state_data->odometer);

	GnssFixFilter_reset_last_fix(curr_state_data->gnss_fix_filter);

}

void service_activation_exit_state_active(ServiceActivationSm* self)
{
	logdbg("Service activation exit state active");
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	connection_sm_send_tolling_data(curr_state_data->connection_sm);
}

void service_activation_tolling_active(ServiceActivationSm *self)
{
	logdbg("Service activation tolling active");
	service_activation_sm_change_state(self, service_activation_active);
}

void service_activation_tolling_no_go(ServiceActivationSm *self)
{
	logdbg("Service activation tolling no go");
	service_activation_sm_change_state(self, service_activation_no_go);
}

void service_activation_tolling_not_active(ServiceActivationSm *self)
{
	logdbg("Service activation tolling not active");
	service_activation_sm_change_state(self, service_activation_not_active);
}

void service_activation_not_connected(ServiceActivationSm *self)
{
	logdbg("Service activation not connected");

	// Forward the event to the inner state machine
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	connection_sm_not_connected(curr_state_data->connection_sm);
}

void service_activation_connected(ServiceActivationSm *self)
{
	logdbg("Service activation connected");

	// Forward the event to the inner state machine
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	connection_sm_connected(curr_state_data->connection_sm);
}

void service_activation_anomaly(ServiceActivationSm *self)
{
	logdbg("Service activation anomaly (no actions yet)");
}

void service_activation_no_anomaly(ServiceActivationSm *self)
{
	logdbg("Service activation no anomaly (no actions yet)");
}

void service_activation_position_updated_not_implemented(ServiceActivationSm *self, const PositionData *position)
{
	logdbg("Service activation position updated not implemented");
}

void service_activation_position_updated(ServiceActivationSm *self, const PositionData *position)
{
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	odometer_position_received(curr_state_data->odometer, position);
	GnssFixesTrasmissionQueue *txq = curr_state_data->gnss_fixes_transmision_queue;
	txq->push(txq, position);
}

void service_activation_send_tolling_data(ServiceActivationSm *self)
{
	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	connection_sm_send_tolling_data(curr_state_data->connection_sm);
}

State* service_activation_not_active_state_new(Tolling_Gnss_Sm_Data *data)
{
	State   *_this   = (State*)calloc(1, sizeof(State));
	Actions *actions = (Actions*)calloc(1, sizeof(Actions));

	actions->enter_state     = service_activation_enter_state_not_active;
	actions->exit_state      = service_activation_not_implemented;

	actions->tolling_active     = service_activation_tolling_active;
	actions->tolling_no_go      = service_activation_tolling_no_go;
	actions->tolling_not_active = service_activation_not_implemented;

	actions->not_connected = service_activation_not_connected;
	actions->connected     = service_activation_connected;

	actions->anomaly       = service_activation_not_implemented;
	actions->no_anomaly    = service_activation_not_implemented;

	actions->position_update   = service_activation_position_updated_not_implemented;
	actions->send_tolling_data = service_activation_not_implemented;

	_this->actions = actions;
	_this->data = (Data*) data;

	return _this;
}

State *service_activation_no_go_state_new(Tolling_Gnss_Sm_Data *data)
{
	State   *_this   = (State*)calloc(1, sizeof(State));
	Actions *actions = (Actions*)calloc(1, sizeof(Actions));

	actions->enter_state     = service_activation_enter_state_no_go;
	actions->exit_state      = service_activation_exit_state_no_go;

	actions->tolling_active     = service_activation_tolling_active;
	actions->tolling_no_go      = service_activation_not_implemented;
	actions->tolling_not_active = service_activation_tolling_not_active;

	actions->not_connected = service_activation_not_connected;
	actions->connected     = service_activation_connected;

	actions->anomaly       = service_activation_not_implemented;
	actions->no_anomaly    = service_activation_not_implemented;

	actions->position_update   = service_activation_position_updated;
	actions->send_tolling_data = service_activation_not_implemented;

	_this->actions = actions;
	_this->data = (Data*) data;

	return _this;
}

State *service_activation_active_state_new(Tolling_Gnss_Sm_Data *data)
{
	State   *_this   = (State*)calloc(1, sizeof(State));
	Actions *actions = (Actions*)calloc(1, sizeof(Actions));

	actions->enter_state     = service_activation_enter_state_active;
	actions->exit_state      = service_activation_exit_state_active;

	actions->tolling_active     = service_activation_not_implemented;
	actions->tolling_no_go      = service_activation_tolling_no_go;
	actions->tolling_not_active = service_activation_tolling_not_active;

	actions->not_connected = service_activation_not_connected;
	actions->connected     = service_activation_connected;

	actions->anomaly       = service_activation_anomaly;
	actions->no_anomaly    = service_activation_no_anomaly;

	actions->position_update   = service_activation_position_updated;
	actions->send_tolling_data = service_activation_send_tolling_data;

	_this->actions = actions;
	_this->data = (Data*) data;

	return _this;
}

const gchar *convert_service_activation_state_to_string(const ServiceActivationStateId state_id)
{
	static const gchar *state_name[] = {
		"service_activation_not_active",
		"service_activation_no_go",
		"service_activation_active"
	};

	return state_name[state_id];
}

void service_activation_sm_stop(ServiceActivationSm *self)
{
	self->states[self->curr_state_id]->actions->exit_state(self);
	self->started = FALSE;
}

void service_activation_sm_start(ServiceActivationSm *self)
{
    if (self->curr_state_id == service_activation_no_go) self->states[self->curr_state_id]->actions->enter_state(self);
}

void service_activation_sm_not_active(ServiceActivationSm *self)
{
	if (self->started) {
		self->states[self->curr_state_id]->actions->tolling_not_active(self);
	} else {
		self->started = TRUE;
		self->curr_state_id = service_activation_not_active;
		logdbg("State machine starting in state %s", convert_service_activation_state_to_string(self->curr_state_id));
		service_activation_enter_state_not_active(self);
	}
}

void service_activation_sm_no_go(ServiceActivationSm *self)
{
	if (self->started) {
		self->states[self->curr_state_id]->actions->tolling_no_go(self);
	} else {
		self->started = TRUE;
		self->curr_state_id = service_activation_no_go;
		logdbg("State machine starting in state %s", convert_service_activation_state_to_string(self->curr_state_id));
		service_activation_enter_state_no_go(self);
	}
}

void service_activation_sm_active(ServiceActivationSm *self)
{
	if (self->started) {
		self->states[self->curr_state_id]->actions->tolling_active(self);
	} else {
		self->started = TRUE;
		self->curr_state_id = service_activation_active;
		logdbg("State machine starting in state %s", convert_service_activation_state_to_string(self->curr_state_id));
		service_activation_enter_state_active(self);
	}
}

void service_activation_sm_not_connected(ServiceActivationSm *self)
{
	self->states[self->curr_state_id]->actions->not_connected(self);
}

void service_activation_sm_connected(ServiceActivationSm *self)
{
	self->states[self->curr_state_id]->actions->connected(self);
}

void service_activation_sm_anomaly(ServiceActivationSm *self)
{
	self->states[self->curr_state_id]->actions->anomaly(self);
}

void service_activation_sm_no_anomaly(ServiceActivationSm *self)
{
	self->states[self->curr_state_id]->actions->no_anomaly(self);
}

void service_activation_sm_position_updated(ServiceActivationSm *self, const PositionData *position)
{
	self->states[self->curr_state_id]->actions->position_update(self, position);
}

void service_activation_sm_send_tolling_data(ServiceActivationSm *self)
{
	self->states[self->curr_state_id]->actions->send_tolling_data(self);
}
