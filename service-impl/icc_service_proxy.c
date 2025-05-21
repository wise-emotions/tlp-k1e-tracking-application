#include <glib.h>

#include "iccsensors.h"
#include "icc_service_types.h"

#include "logger.h"

#include "configuration_store.h"
#include "application_events.h"
#include "application_events_common_names.h"

#include "icc_service_proxy.h"

typedef struct _IccServiceProxy
{
	IccSensors *dbus_proxy;
	guint watcher_id;
	ConfigurationStore *configuration_store;
	ApplicationEvents *application_events;
	gboolean tampering_status;
} IccServiceProxy;

static void IccServiceProxy_get_initial_tampering(IccServiceProxy *self)
{
	GError *error = NULL;
	guint tampering_status;
	gboolean result = icc_sensors_call_get_tamper_status_sync(
		self->dbus_proxy,
		&tampering_status,
		NULL,
		&error);
	if (result)
	{
		logdbg("tampering_status = %u", tampering_status);
		self->tampering_status = tampering_status;
	}
	else
	{
		logdbg("icc_sensors_call_get_tamper_status_sync failed");
		if (error)
		{
			logdbg("error code: %d, error message: %s\n", error->code, error->message);
			g_error_free(error);
		}
	}
}

static void IccServiceProxy_on_interface_available(
	GDBusConnection *connection,
	const gchar *name,
	const gchar *name_owner,
	gpointer gpointer_self)
{
	logdbg("Interface '%s' is available from owner '%s'", name, name_owner);
	IccServiceProxy *self = (IccServiceProxy*)gpointer_self;
	IccServiceProxy_get_initial_tampering(self);
	return;
}

static void IccServiceProxy_on_interface_not_available(GDBusConnection *connection, const gchar *name, gpointer data)
{
	logdbg("Interface '%s' is not available", name);
	return;
}

static void IccServiceProxy_on_tampering_alert(
	IccSensors *object,
	IccServiceProxy *self)
{
	logdbg("on_tampering_alert");
	self->tampering_status = TRUE;
	ApplicationEvents_emit_event(self->application_events, EVENT_ICC_SERVICE_PROXY_TAMPERING_ATTEMPTED);
	return;
}

static void IccServiceProxy_on_tampering_alert_reset(
	IccSensors *object,
	IccServiceProxy *self)
{
	logdbg("on_tampering_alert_reset");
	self->tampering_status = FALSE;
	ApplicationEvents_emit_event(self->application_events, EVENT_ICC_SERVICE_PROXY_TAMPERING_RESET);
	return;
}

gboolean IccServiceProxy_get_tampering_status(const IccServiceProxy *self, guint *value)
{
	*value = self->tampering_status;
	return TRUE;
}

IccServiceProxy *IccServiceProxy_new(ConfigurationStore *configuration_store, ApplicationEvents *application_events)
{
	loginfo("");
	IccServiceProxy *self = g_try_new0(IccServiceProxy, 1);
	g_return_val_if_fail(self != NULL, NULL);
	self->configuration_store = configuration_store;
	self->application_events = application_events;
	GError *g_error = NULL;
	self->dbus_proxy =  icc_sensors_proxy_new_for_bus_sync(
		G_BUS_TYPE_SYSTEM,
		G_DBUS_PROXY_FLAGS_NONE,
		iccif_sensors_dbus_interest,
		iccif_sensors_dbus_address,
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
			iccif_sensors_dbus_interest,
			G_BUS_NAME_WATCHER_FLAGS_NONE,
			IccServiceProxy_on_interface_available,
			IccServiceProxy_on_interface_not_available,
			self,
			NULL);
		g_signal_connect(
			self->dbus_proxy,
			"tampering-alert",
			G_CALLBACK(IccServiceProxy_on_tampering_alert),
			self);
		g_signal_connect(
			self->dbus_proxy,
			"tampering-alert-reset",
			G_CALLBACK(IccServiceProxy_on_tampering_alert_reset),
			self);
	}
	self->tampering_status = FALSE;
	return self;
}

void IccServiceProxy_destroy(IccServiceProxy *self)
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
	return;
	}
}
