/*
 * dsrc_go_nogo_status.c
 *
 *  Created on: May 26, 2022
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#include "logger.h"

#include "tolling_gnss_sm_data.h"
#include "application_events.h"
#include "application_events_common_names.h"
#include "dsrc_service_proxy.h"

#include "dsrc_go_nogo_status.h"

typedef struct _DsrcGoNogoStatus {
	guint dsrc_go_nogo_flags;
	GoNogo_t dsrc_go_nogo_status;
	GoNogo_t app_go_nogo_status;
	gboolean app_nogo_service;
	gboolean app_nogo_network;
	gboolean app_nogo_gnss;
	ApplicationEvents *application_events;
	DsrcServiceProxy *dsrc_service_proxy;
} DsrcGoNogoStatus;

// Forward declarations
static void DsrcGoNogoStatus_set_go(gpointer gpointer_self, guint go_nogo_flags);
static void DsrcGoNogoStatus_set_nogo(gpointer gpointer_self, guint go_nogo_flags);
static void DsrcGoNogoStatus_set_current_app_status(DsrcGoNogoStatus *self);


DsrcGoNogoStatus *DsrcGoNogoStatus_new(ApplicationEvents *application_events, DsrcServiceProxy *dsrc_service_proxy)
{
	DsrcGoNogoStatus* self = g_new0(DsrcGoNogoStatus, 1);

	self->application_events = application_events;
	self->dsrc_service_proxy = dsrc_service_proxy;
	self->dsrc_go_nogo_status = go;
	self->dsrc_go_nogo_flags = 0;	
	self->app_go_nogo_status = go;
	self->app_nogo_service = FALSE;
	self->app_nogo_network = FALSE;
	self->app_nogo_gnss = FALSE;

	ApplicationEvents_register_event_ex(
		self->application_events,
		EVENT_DSRC_SERVICE_PROXY_GO,
		G_CALLBACK(DsrcGoNogoStatus_set_go),
		self,
		1,
		G_TYPE_UINT);
	ApplicationEvents_register_event_ex(
		self->application_events,
		EVENT_DSRC_SERVICE_PROXY_NOGO,
		G_CALLBACK(DsrcGoNogoStatus_set_nogo),
		self,
		1,
		G_TYPE_UINT);


	return self;
}

void DsrcGoNogoStatus_destroy(DsrcGoNogoStatus *self)
{
	if (self) {
		g_free(self);
	}
}

static void DsrcGoNogoStatus_set_go(gpointer gpointer_self, guint go_nogo_flags)
{
	/*DsrcGoNogoStatus *self = (DsrcGoNogoStatus *)gpointer_self;
	self->dsrc_go_nogo_status = go;
	self->dsrc_go_nogo_flags = go_nogo_flags;	
	//logdbg("Fixes will be marked as 'go'");
	DsrcGoNogoStatus_set_current_app_status(self);*/
}

static void DsrcGoNogoStatus_set_nogo(gpointer gpointer_self, guint go_nogo_flags)
{
	/*DsrcGoNogoStatus *self = (DsrcGoNogoStatus *)gpointer_self;
	self->dsrc_go_nogo_status = nogo;
	self->dsrc_go_nogo_flags = go_nogo_flags;	
	//logdbg("Fixes will be marked as 'nogo'");
	DsrcGoNogoStatus_set_current_app_status(self);*/
}

GoNogo_t DsrcGoNogoStatus_get_current_dsrc_status(const DsrcGoNogoStatus *self)
{
	return self->dsrc_go_nogo_status;
}

guint DsrcGoNogoStatus_get_current_dsrc_status_flags(const DsrcGoNogoStatus *self)
{
	return self->dsrc_go_nogo_flags;
}

static void DsrcGoNogoStatus_set_current_app_status(DsrcGoNogoStatus *self)
{
	self->app_go_nogo_status = (self->app_nogo_service || self->app_nogo_network || self->app_nogo_gnss) ? nogo : go;

	if (self->dsrc_go_nogo_status != self->app_go_nogo_status) {
		logwarn("dsrc (%d) and app (%d) go nogo differ", self->dsrc_go_nogo_status, self->app_go_nogo_status);

		if (DsrcServiceProxy_get_other_anomalies_present(self->dsrc_service_proxy)) {
			logwarn("there are other anomalies");

			self->app_go_nogo_status = nogo;
		}
	}

	if  (self->app_go_nogo_status == go)
		logdbg("Fixes will be marked as 'go'");
	else
		logdbg("Fixes will be marked as 'nogo'");
}

GoNogo_t DsrcGoNogoStatus_get_current_app_status(const DsrcGoNogoStatus *self)
{
	return self->app_go_nogo_status;
}

void DsrcGoNogoStatus_set_app_no_go_service(DsrcGoNogoStatus *self, gboolean is_active)
{
	self->app_nogo_service = is_active;
	DsrcGoNogoStatus_set_current_app_status(self);
}

void DsrcGoNogoStatus_set_app_no_go_network(DsrcGoNogoStatus *self, gboolean is_active)
{
	self->app_nogo_network = is_active;
	DsrcGoNogoStatus_set_current_app_status(self);
}

void DsrcGoNogoStatus_set_app_no_go_gnss(DsrcGoNogoStatus *self, gboolean is_active)
{
	self->app_nogo_gnss = is_active;
	DsrcGoNogoStatus_set_current_app_status(self);
}
