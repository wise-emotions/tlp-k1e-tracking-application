/*
 * connection_sm.c
 *
 * Created on: Dec 4, 2019
 * Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#include "connection_sm.h"

#include <stdio.h>
#include <stdlib.h>

#include "logger.h"
#include "mqtt_client.h"
#include "hmi_proxy.h"

#include "tolling_gnss_sm_data.h"

typedef enum _ConnectionStateId {
	not_connected,
	connected,
	anomaly,
	CONNECTION_STATES_NUM
} ConnectionStateId;

typedef struct _ConnectionSm {
	State              *states[CONNECTION_STATES_NUM];
	ConnectionStateId   curr_state_id;
} ConnectionSm;

typedef struct _Actions {
	void (*enter_state)(ConnectionSm* connection_sm);
	void (*exit_state)(ConnectionSm* connection_sm);

	void (*connected)(ConnectionSm* connection_sm);
	void (*not_connected)(ConnectionSm* connection_sm);
	void (*anomaly)(ConnectionSm* connection_sm);

	void (*send_tolling_data)(ConnectionSm* connection_sm);
} Actions;

State* connection_not_connected_state_new(Tolling_Gnss_Sm_Data* data);
State* connection_connected_state_new(Tolling_Gnss_Sm_Data* data);
State* connection_anomaly_state_new(Tolling_Gnss_Sm_Data* data);
void connection_state_destroy(State *self);
const gchar *convert_connection_state_to_string(const ConnectionStateId state_id);

ConnectionSm *connection_sm_new(Tolling_Gnss_Sm_Data* data)
{
	ConnectionSm *self = (ConnectionSm *)g_malloc0(sizeof(ConnectionSm));

	self->states[not_connected] = connection_not_connected_state_new(data);
	self->states[connected]     = connection_connected_state_new(data);
	self->states[anomaly]       = connection_anomaly_state_new(data);

	self->curr_state_id = not_connected;

	return self;
}

void connection_sm_destroy(ConnectionSm *self)
{
	connection_state_destroy(self->states[not_connected]);
	connection_state_destroy(self->states[connected]);
	connection_state_destroy(self->states[anomaly]);

	g_free(self);
}

void connection_state_destroy(State *self)
{
	free(self->actions);
	free(self);
}

void connection_sm_change_state(ConnectionSm *self, const ConnectionStateId next_state_id)
{
	if (self->curr_state_id == next_state_id) {
		logdbg("Already in target state %s, nothing to do", convert_connection_state_to_string(next_state_id));
		return;
	}

	self->states[self->curr_state_id]->actions->exit_state(self);
	self->curr_state_id = next_state_id;
	self->states[self->curr_state_id]->actions->enter_state(self);
}

static void connection_nop(ConnectionSm *self)
{
}

void connection_not_implemented(ConnectionSm *self)
{
	logdbg("Connection action not implemented");
}

void connection_enter_state_not_connected(ConnectionSm *self)
{
	logdbg("Connection enter state not connected");
}

void connection_enter_state_connected(ConnectionSm *self)
{
	logdbg("Connection enter state connected");

	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	mqtt_client_start(curr_state_data->mqtt_client);
}

void connection_exit_state_connected(ConnectionSm *self)
{
	logdbg("Connection exit state connected");

	Tolling_Gnss_Sm_Data *curr_state_data = (Tolling_Gnss_Sm_Data*)(self->states[self->curr_state_id]->data);
	mqtt_client_stop(curr_state_data->mqtt_client);
}

void connection_enter_state_anomaly(ConnectionSm *self)
{
	logdbg("Connection enter state anomaly");
}

void connection_connected(ConnectionSm *self)
{
	logdbg("Connection connected");

	connection_sm_change_state(self, connected);
}

void connection_not_connected(ConnectionSm *self)
{
	logdbg("Connection not connected");

	connection_sm_change_state(self, not_connected);
}

void connection_anomaly(ConnectionSm *self)
{
	logdbg("Connection anomaly");

	connection_sm_change_state(self, anomaly);
}


State* connection_not_connected_state_new(Tolling_Gnss_Sm_Data *data)
{
	State   *_this   = (State*)calloc(1, sizeof(State));
	Actions *actions = (Actions*)calloc(1, sizeof(Actions));

	actions->enter_state     = connection_enter_state_not_connected;
	actions->exit_state      = connection_not_implemented;

	actions->not_connected = connection_not_implemented;
	actions->connected     = connection_connected;
	actions->anomaly       = connection_anomaly;

	actions->send_tolling_data = connection_nop;

	_this->actions = actions;
	_this->data = (Data*) data;

	return _this;
}

State* connection_connected_state_new(Tolling_Gnss_Sm_Data *data)
{
	State   *_this   = (State*)calloc(1, sizeof(State));
	Actions *actions = (Actions*)calloc(1, sizeof(Actions));

	actions->enter_state     = connection_enter_state_connected;
	actions->exit_state      = connection_exit_state_connected;

	actions->not_connected = connection_not_connected;
	actions->connected     = connection_not_implemented;
	actions->anomaly       = connection_anomaly;

	actions->send_tolling_data = connection_nop;

	_this->actions = actions;
	_this->data = (Data*) data;

	return _this;
}

State* connection_anomaly_state_new(Tolling_Gnss_Sm_Data *data)
{
	State   *_this   = (State*)calloc(1, sizeof(State));
	Actions *actions = (Actions*)calloc(1, sizeof(Actions));

	actions->enter_state     = connection_enter_state_anomaly;
	actions->exit_state      = connection_not_implemented;

	actions->not_connected = connection_not_connected;
	actions->connected     = connection_connected;
	actions->anomaly       = connection_not_implemented;

	actions->send_tolling_data = connection_nop;

	_this->actions = actions;
	_this->data = (Data*) data;

	return _this;
}

const gchar *convert_connection_state_to_string(const ConnectionStateId state_id)
{
	static const gchar *state_name[] = {
		"not_connected",
		"connected",
		"anomaly"
	};

	return state_name[state_id];
}

void connection_sm_not_connected(ConnectionSm *self)
{
	self->states[self->curr_state_id]->actions->not_connected(self);
}

void connection_sm_connected(ConnectionSm *self)
{
	self->states[self->curr_state_id]->actions->connected(self);
}

void connection_sm_send_tolling_data(ConnectionSm *self)
{
	self->states[self->curr_state_id]->actions->send_tolling_data(self);
}

