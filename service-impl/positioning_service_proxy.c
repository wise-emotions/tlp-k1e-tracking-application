#include <glib.h>
#include <string.h>

#include "position.h"
#include "positioning_service_types.h"
#include  "gnss_fix_data.h"
#include "logger.h"

#include "configuration_store.h"
#include "application_events.h"
#include "application_events_common_names.h"

#include "positioning_service_proxy.h"

typedef struct _PositioningServiceProxy
{
	Position *dbus_proxy;
	guint watcher_id;
	ConfigurationStore *configuration_store;
	ApplicationEvents *application_events;
	PositionData current_position;
} PositioningServiceProxy;

static void PositioningServiceProxy_on_interface_available(
	GDBusConnection *connection,
	const gchar *name,
	const gchar *name_owner,
	gpointer gpointer_self)
{
	logdbg("Interface '%s' is available from owner '%s'", name, name_owner);
	// TODO: get initial value?
}

static void PositioningServiceProxy_on_interface_not_available(GDBusConnection *connection, const gchar *name, gpointer data)
{
	logdbg("Interface '%s' is not available", name);
}

static void PositioningServiceProxy_on_position_update(
	Position *object,
	guint arg_fix_validity,
	guchar arg_fix_type,
	gdouble arg_pdop,
	gdouble arg_hdop,
	gdouble arg_vdop,
	gdouble arg_altitude,
	gdouble arg_latitude,
	gdouble arg_longitude,
	GVariant *arg_sat_for_fix,
	gdouble arg_heading,
	gdouble arg_speed,
	gdouble arg_accuracy,
	const gchar *arg_gps_time,
	guint64 arg_date_time,
	gdouble arg_total_dist,
	PositioningServiceProxy *self)
{
	logdbg("");
	self->current_position.fix_validity = arg_fix_validity;
	self->current_position.fix_type = arg_fix_type;
	self->current_position.pdop = arg_pdop;
	self->current_position.hdop = arg_hdop;
	self->current_position.vdop = arg_vdop;
	self->current_position.altitude = arg_altitude;
	self->current_position.latitude = arg_latitude;
	self->current_position.longitude = arg_longitude;


	// Clear the content of the GArray
	g_array_remove_range(self->current_position.sat_for_fix, 0, self->current_position.sat_for_fix->len);

	// Read the satellite IDs from the GVariant, and append to the GArray those that are not 0
	gsize sat_count;
	const guchar *sat_array = (const guchar*)g_variant_get_fixed_array(arg_sat_for_fix, &sat_count, sizeof(guchar));
	//logdbg("The GVariant contains %u satellites", sat_count);
	for (gsize sat_index = 0; sat_index < sat_count; sat_index++) {
		if (sat_array[sat_index] != 0) {
			gushort satellite_id = (gushort)(sat_array[sat_index]);
			g_array_append_val(self->current_position.sat_for_fix, satellite_id);
			//logdbg("Appending satellite %u", satellite_id);
		}
	}

	self->current_position.heading = arg_heading;
	self->current_position.speed = arg_speed;
	self->current_position.accuracy = arg_accuracy;
	strncpy(self->current_position.gps_time, arg_gps_time, DATE_TIME_LEN);
	self->current_position.date_time = arg_date_time;
	self->current_position.total_dist = arg_total_dist;
	ApplicationEvents_emit_event(self->application_events, EVENT_POSITIONING_SERVICE_PROXY_POSITION_UPDATED);
}

PositionData *PositioningServiceProxy_get_current_position(PositioningServiceProxy *self)
{
	return &self->current_position;
}


PositionData PositioningServiceProxy_get_last_position(PositioningServiceProxy *self)
{

	GError *error = NULL;

	gboolean result;
	logdbg("calling position_call_get_last_position_sync");
	error = NULL;
	guint fix_validity;
	guchar fix_type;
	gdouble pdop;
	gdouble hdop;
	gdouble vdop;
	gdouble altitude;
	gdouble latitude;
	gdouble longitude;
	GVariant *sat_for_fix;
	gdouble heading;
	gdouble speed;
	gdouble accuracy;
	gchar *gps_time;
	guint64 date_time;
	gdouble total_dist;

	PositionData fix;
	memset(&fix, 0, sizeof(fix));
	fix.sat_for_fix = g_array_sized_new(FALSE, TRUE, sizeof(gushort), MAX_SAT_FOR_FIX);
    fix.gps_time[0] = '\0';

	result = position_call_get_last_position_sync(self->dbus_proxy,
				&fix_validity,
				&fix_type,
				&pdop,
				&hdop,
				&vdop,
				&altitude,
				&latitude,
				&longitude,
				&sat_for_fix,
				&heading,
				&speed,
				&accuracy,
				&gps_time,
				&date_time,
				&total_dist,
				NULL,
				&error);

	if (result) {

		fix.fix_validity = fix_validity;
		fix.fix_type = fix_type;
		fix.pdop = pdop;
		fix.hdop = hdop;
		fix.vdop = vdop;
		fix.altitude = altitude;
		fix.latitude = latitude;
		fix.longitude = longitude;


		// Read the satellite IDs from the GVariant, and append to the GArray those that are not 0
		gsize sat_count;
		const guchar *sat_array = (const guchar*)g_variant_get_fixed_array(sat_for_fix, &sat_count, sizeof(guchar));
		//logdbg("The GVariant contains %u satellites", sat_count);
		for (gsize sat_index = 0; sat_index < sat_count; sat_index++) {
			if (sat_array[sat_index] != 0) {
				gushort satellite_id = (gushort)(sat_array[sat_index]);
				g_array_append_val(fix.sat_for_fix, satellite_id);

				//logdbg("Appending satellite %u", satellite_id);
			}
		}

		fix.heading = heading;
		fix.speed = speed;
		fix.accuracy = accuracy;
		strncpy(fix.gps_time, gps_time, DATE_TIME_LEN);
		fix.date_time = date_time;
		fix.total_dist = total_dist;

		g_variant_unref(sat_for_fix);
		g_free(gps_time);

	} else {

		if (error != NULL) {
			if (error->code == 2) {

				fix = self->current_position;
				logdbg("\terror: %d - %s", error->code, error->message);
			} else {
				logerr("\terror: %d - %s", error->code, error->message);
			}
			g_error_free(error);
		} else {
			logerr("Unknown error getting fix from Positioning service");
		}

	}

	if(fix.latitude == 0 && fix.longitude == 0 ){

		fix = self->current_position;

	}

	return fix;
}


PositioningServiceProxy *PositioningServiceProxy_new(ConfigurationStore *configuration_store, ApplicationEvents *application_events)
{
	loginfo("");
	PositioningServiceProxy *self = g_try_new0(PositioningServiceProxy, 1);
	g_return_val_if_fail(self != NULL, NULL);
	self->configuration_store = configuration_store;
	self->application_events = application_events;
	self->current_position.fix_validity = 0;
	self->current_position.fix_type = 0;
	self->current_position.pdop = 0.0;
	self->current_position.hdop = 0.0;
	self->current_position.vdop = 0.0;
	self->current_position.altitude = 0.0;
	self->current_position.latitude = 0.0;
	self->current_position.longitude = 0.0;
	self->current_position.sat_for_fix = g_array_sized_new(FALSE, TRUE, sizeof(gushort), MAX_SAT_FOR_FIX);
	self->current_position.heading = 0.0;
	self->current_position.speed = 0.0;
	self->current_position.accuracy = 0.0;
	self->current_position.gps_time[0] = '\0';
	self->current_position.date_time = 0;
	self->current_position.total_dist = 0.0;
	GError *g_error = NULL;
	self->dbus_proxy =  position_proxy_new_for_bus_sync(
		G_BUS_TYPE_SYSTEM,
		G_DBUS_PROXY_FLAGS_NONE,
		positioning_service_bus_name,
		positioning_service_obj_path,
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
			positioning_service_bus_name,
			G_BUS_NAME_WATCHER_FLAGS_NONE,
			PositioningServiceProxy_on_interface_available,
			PositioningServiceProxy_on_interface_not_available,
			self,
			NULL);
		g_signal_connect(
			self->dbus_proxy,
			"position_update",
			G_CALLBACK(PositioningServiceProxy_on_position_update),
			self);
	}
	return self;
}

void PositioningServiceProxy_destroy(PositioningServiceProxy *self)
{
	loginfo("");
	if (self != NULL)
	{
		if (self->current_position.sat_for_fix)
		{
			g_array_free(self->current_position.sat_for_fix, TRUE);
		}
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
}
