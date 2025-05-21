#include <glib.h>

#include "connection.h"
#include "network-manager-types.h"

#include "logger.h"

#include "configuration_store.h"
#include "application_events.h"
#include "application_events_common_names.h"

#include "network_manager_proxy.h"

void NetworkManagerProxy_get_initial_connection_status(NetworkManagerProxy *self);

typedef struct _NetworkManagerProxy
{
	Connection *dbus_proxy;
	guint watcher_id;
	gulong connection_status_changed_handler;
	ConfigurationStore *configuration_store;
	ApplicationEvents *application_events;
} NetworkManagerProxy;

static void NetworkManagerProxy_on_interface_available(
	GDBusConnection *connection,
	const gchar *name,
	const gchar *name_owner,
	gpointer gpointer_self)
{
	logdbg("Interface '%s' is available from owner '%s'", name, name_owner);
	NetworkManagerProxy_get_initial_connection_status((NetworkManagerProxy *)gpointer_self);
}

void NetworkManagerProxy_get_initial_connection_status(NetworkManagerProxy *self)
{
	GError *error = NULL;
	gint16 connection_status;
	gboolean result = connection_call_status_sync(self->dbus_proxy, &connection_status, NULL, &error);
	if (result)
	{
		if (connection_status == 1)
		{
			ApplicationEvents_emit_event(self->application_events,  EVENT_NETWORK_MANAGER_PROXY_NETWORK_CONNECTED);
		}
		else
		{
			ApplicationEvents_emit_event(self->application_events, EVENT_NETWORK_MANAGER_PROXY_NETWORK_NOT_CONNECTED);
		}
	}
	else
	{
		logerr("connection_call_status_sync failed");
		if (error)
		{
			logerr("error: %d", error->code);
			g_error_free(error);
		}
		ApplicationEvents_emit_event(self->application_events, EVENT_NETWORK_MANAGER_PROXY_NETWORK_NOT_CONNECTED);
	}
}

static void NetworkManagerProxy_on_interface_not_available(GDBusConnection *connection, const gchar *name, gpointer data)
{
	logdbg("Interface '%s' is not available", name);
}

void NetworkManagerProxy_on_connection_status_changed(Connection *proxy, gint16 status, NetworkManagerProxy *self)
{
	logdbg("on_connection_status_changed");
	logdbg("status = %d", status);
	if (status == 1)
	{
		ApplicationEvents_emit_event(self->application_events,  EVENT_NETWORK_MANAGER_PROXY_NETWORK_CONNECTED);
	}
	else
	{
		ApplicationEvents_emit_event(self->application_events, EVENT_NETWORK_MANAGER_PROXY_NETWORK_NOT_CONNECTED);
	}
}

void NetworkManagerProxy_start_dbus(NetworkManagerProxy *self)
{
	GError *g_error = NULL;
	self->dbus_proxy =  connection_proxy_new_for_bus_sync(
		G_BUS_TYPE_SYSTEM,
		G_DBUS_PROXY_FLAGS_NONE,
		NETWORK_MANAGER_BUS_NAME,
		NETWORK_MANAGER_OBJ_PATH,
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
			NETWORK_MANAGER_BUS_NAME,
			G_BUS_NAME_WATCHER_FLAGS_NONE,
			NetworkManagerProxy_on_interface_available,
			NetworkManagerProxy_on_interface_not_available,
			self,
			NULL);

		self->connection_status_changed_handler = g_signal_connect(
			self->dbus_proxy,
			"connection_status_changed",
			G_CALLBACK(NetworkManagerProxy_on_connection_status_changed),
			self);
	}
}

void NetworkManagerProxy_stop_dbus(NetworkManagerProxy *self)
{
	if (self)
	{
		if (self->connection_status_changed_handler)
		{
			g_signal_handler_disconnect(self->dbus_proxy, self->connection_status_changed_handler);
			self->connection_status_changed_handler = 0;
		}
		if (self->watcher_id)
		{
			g_bus_unwatch_name(self->watcher_id);
			self->watcher_id = 0;
		}
		if (self->dbus_proxy)
		{
			g_object_unref(self->dbus_proxy);
			self->dbus_proxy = NULL;
		}
	}
}


NetworkManagerProxy *NetworkManagerProxy_new(ConfigurationStore *configuration_store, ApplicationEvents *application_events)
{
	loginfo("");
	NetworkManagerProxy *self = g_try_new0(NetworkManagerProxy, 1);
	g_return_val_if_fail(self != NULL, NULL);
	self->configuration_store = configuration_store;
	self->application_events = application_events;

	NetworkManagerProxy_start_dbus(self);

	return self;
}

void NetworkManagerProxy_destroy(NetworkManagerProxy *self)
{
	loginfo("");

	NetworkManagerProxy_stop_dbus(self);

	if (self)
	{
		g_free(self);
	}
}

void NetworkManagerProxy_on_hold(NetworkManagerProxy *self)
{
	logdbg("");

	NetworkManagerProxy_stop_dbus(self);
}


void NetworkManagerProxy_resume(NetworkManagerProxy *self)
{
	logdbg("");

	NetworkManagerProxy_start_dbus(self);
}
