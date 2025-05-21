#include <glib.h>

#include "dsrc.h"
#include "shared_types.h"

#include "logger.h"

#include "configuration_store.h"
#include "application_events.h"
#include "application_events_common_names.h"

#include "dsrc_service_proxy.h"

#define OUR_ANOMALIES_MASK (ANOMALY_MASK(ANOMALY_LTE) | ANOMALY_MASK(ANOMALY_GNSS) | ANOMALY_MASK(ANOMALY_SERVICE))

typedef struct _DsrcServiceProxy
{
	Dsrc *dbus_proxy;
	guint watcher_id;
	ConfigurationStore *configuration_store;
	ApplicationEvents *application_events;
	gboolean other_anomalies_present;
} DsrcServiceProxy;

static void DsrcServiceProxy_set_other_anomalies_present(DsrcServiceProxy *self, const gboolean go_nogo_bool, const guint go_nogo_flags)
{
	gboolean other_anomalies_present = FALSE;
	if (go_nogo_bool == FALSE)
	{
		loginfo("go_nogo_flags = 0x%x", go_nogo_flags);
		guint remaining_flags = go_nogo_flags & ANOMALY_GONOGO_MASK & ~OUR_ANOMALIES_MASK;
		if (remaining_flags != 0)
		{
			logwarn("there are other anomalies (remaining_flags = 0x%x)", remaining_flags);
			other_anomalies_present = TRUE;
		}
	}
	self->other_anomalies_present = other_anomalies_present;
}

static void DsrcServiceProxy_emit_go_nogo_event(DsrcServiceProxy *self, const gboolean go_nogo_bool, const guint go_nogo_flags)
{
	DsrcServiceProxy_set_other_anomalies_present(self, go_nogo_bool, go_nogo_flags);
	if (go_nogo_bool)
	{
		ApplicationEvents_emit_event_ex(self->application_events, EVENT_DSRC_SERVICE_PROXY_GO, go_nogo_flags);
	}
	else
	{
		ApplicationEvents_emit_event_ex(self->application_events, EVENT_DSRC_SERVICE_PROXY_NOGO, go_nogo_flags);
	}
}

static void DsrcServiceProxy_get_initial_go_nogo_status(DsrcServiceProxy *self)
{
	gboolean go_nogo_bool = FALSE;
	guint go_nogo_flags = 0;
	gint return_err_val;
	GError *call_error = NULL;
	gboolean call_result = dsrc_call_dsrc_get_go_nogo_flags_sync(self->dbus_proxy, &go_nogo_bool, &go_nogo_flags, &return_err_val, NULL, &call_error);
	if (call_result == TRUE && return_err_val == 0)
	{
		logdbg("Called DsrcGetGoNogoFlags: go_nogo_bool = %d, go_nogo_flags = %u", go_nogo_bool, go_nogo_flags);
	}
	else
	{
		logerr("dsrc_call_dsrc_get_go_nogo_flags_sync failed");
		if (call_error)
		{
			logerr("Error code = %d, error message = '%s'", call_error->code, call_error->message);
			g_error_free(call_error);
		}
	}
	DsrcServiceProxy_emit_go_nogo_event(self, go_nogo_bool, go_nogo_flags);
}

static void DsrcServiceProxy_on_interface_available(
	GDBusConnection *connection,
	const gchar *name,
	const gchar *name_owner,
	gpointer gpointer_self)
{
	logdbg("Interface '%s' is available from owner '%s'", name, name_owner);
	DsrcServiceProxy *self = (DsrcServiceProxy *)gpointer_self;
	DsrcServiceProxy_get_initial_go_nogo_status(self);
	return;
}

static void DsrcServiceProxy_on_interface_not_available(GDBusConnection *connection, const gchar *name, gpointer data)
{
	logdbg("Interface '%s' is not available", name);
	return;
}

static void DsrcServiceProxy_on_DsrcGoNogoEvent(Dsrc *object, gboolean go_nogo_bool, guint go_nogo_flags, DsrcServiceProxy *self)
{
	logdbg("Received DsrcGoNogoEvent: go_nogo_bool = %d, go_nogo_flags = %u", go_nogo_bool, go_nogo_flags);
	DsrcServiceProxy_emit_go_nogo_event(self, go_nogo_bool, go_nogo_flags);
}

gboolean DsrcServiceProxy_get_other_anomalies_present(const DsrcServiceProxy *self)
{
	return self->other_anomalies_present;
}

DsrcServiceProxy *DsrcServiceProxy_new(ConfigurationStore *configuration_store, ApplicationEvents *application_events)
{
	loginfo("");
	DsrcServiceProxy *self = g_try_new0(DsrcServiceProxy, 1);
	g_return_val_if_fail(self != NULL, NULL);
	self->configuration_store = configuration_store;
	self->application_events = application_events;
	self->other_anomalies_present = FALSE;
	GError *g_error = NULL;
	self->dbus_proxy = dsrc_proxy_new_for_bus_sync(
		G_BUS_TYPE_SYSTEM,
		G_DBUS_PROXY_FLAGS_NONE,
		dsrc_service_bus_name,
		dsrc_service_obj_path,
		NULL,
		&g_error);
	if (self->dbus_proxy == NULL)
	{
		logerr("proxy_new_for_bus_sync error: %d - %s", g_error->code, g_error->message);
	}
	else
	{
		self->watcher_id = g_bus_watch_name(
			G_BUS_TYPE_SYSTEM,
			dsrc_service_bus_name,
			G_BUS_NAME_WATCHER_FLAGS_NONE,
			DsrcServiceProxy_on_interface_available,
			DsrcServiceProxy_on_interface_not_available,
			self,
			NULL);
		g_signal_connect(
			self->dbus_proxy,
			"dsrc-go-nogo-event",
			G_CALLBACK(DsrcServiceProxy_on_DsrcGoNogoEvent),
			self);
	}
	return self;
}

void DsrcServiceProxy_destroy(DsrcServiceProxy *self)
{
	loginfo("");
	if (self != NULL)
	{
		if (self->watcher_id)
		{
			g_bus_unwatch_name(self->watcher_id);
		}
		if (self->dbus_proxy)
		{
			g_object_unref(self->dbus_proxy);
		}
		g_free(self);
	}
	return;
}

