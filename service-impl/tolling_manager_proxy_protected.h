#pragma once

#include "tolling_manager_proxy.h"

#include <gio/gio.h>

typedef struct _Tolling Tolling;

struct _TollingManagerProxy
{
	const char *gnss_domain_name;
	guint service_activation_domain_id;
	GHashTable *other_services_on_shared_domain;
	const gchar *geofencing_domain_name;
	Tolling *dbus_proxy;
	guint watcher_id;
	ConfigurationStore *configuration_store;
	ApplicationEvents *application_events;
	Tolling_Gnss_Sm_Data *tolling_gnss_sm_data;
	guint current_axles;
	guint current_weight;
	gint  current_trailer_type;
	guint current_actual_weight;
	gboolean unknown_gnss_domain;

	// "virtual" methods begin
	void (*dtor)(TollingManagerProxy *self);

	void (*get_initial_gnss_domains)(TollingManagerProxy *self);

	void (*on_interface_available)(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer gpointer_self);

	void (*on_domain_enter)(Tolling *object, const gchar *arg_domain_id, TollingManagerProxy *self);

	void (*on_domain_exit)(Tolling *object, const gchar *arg_domain_id, TollingManagerProxy *self);
	// "virtual" methods end
};

// methods accessed from derived classes begin
void TollingManagerProxy_on_interface_available(
	GDBusConnection *connection,
	const gchar *name,
	const gchar *name_owner,
	gpointer gpointer_self);

TollingManagerProxy *TollingManagerProxy_initialize(TollingManagerProxy *self,
		const gchar *gnss_domain_name,
		const guint service_activation_domain_id,
		ConfigurationStore *configuration_store,
		ApplicationEvents *application_events,
		Tolling_Gnss_Sm_Data *data);
void TollingManagerProxy_deinitialize(TollingManagerProxy *self);
// methods accessed from derived classes end
