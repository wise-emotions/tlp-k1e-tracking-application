#include "tolling_manager_proxy_protected.h"

#include "tolling.h"
#include "tolling_manager_types.h"

#include "logger.h"

#include "configuration_store.h"
#include "application_events.h"
#include "application_events_common_names.h"
#include "tolling_gnss_sm_data.h"
#include "service_monitor.h"
#include "icustom_activation_checker.h"

#define FALLBACK_BLOCK_BEHAVIOUR       0
#define FALLBACK_FILTER_DISTANCE       2.0
#define FALLBACK_FILTER_TIME           300
#define FALLBACK_TRANSMISSION_TIME     600
#define FALLBACK_FILTER_LOGIC          1
#define FALLBACK_FILTER_HEADING        90.0
#define FALLBACK_GPS_ANOMALY_TIMEOUT   1440



// Forward declarations
static void TollingManagerProxy_update_activation_status_of_other_services(const TollingManagerProxy *self, const guint service_id, const guint service_status);
static void TollingManagerProxy_check_if_other_services_are_active(const TollingManagerProxy *self);



static gboolean TollingManagerProxy_update_gnss_config(
	TollingManagerProxy *self,
	const gchar * const tr_id,
	const guint16 block_behaviour_guint16,
	const guint transmission_timeout,
	const guint16 max_packet_guint16,
	const gdouble filter_distance,
	const guint filter_time,
	const guint filter_logic,
	const gdouble filter_heading,
	const guint network_anomaly_timeout,
	const guint gps_anomaly_timeout)
{
	JsonMapper* updated_config = NULL;
	gboolean configuration_update_successful = ConfigurationStore_update_begin(self->configuration_store, &updated_config);
	// Convert 16-bit int types to 32 bits, because json_mapper_add_property treats the passed pointer as (int*) and reads 32 bit,
	// which would be garbage if the original variable is a 16-bit one.
	const guint block_behaviour = (const guint)block_behaviour_guint16;
	const guint max_packet = (const guint)max_packet_guint16;
	if (configuration_update_successful)
	{
		gint ret_add_1 = json_mapper_add_property(updated_config, (const guchar*)"tr_id",                   json_type_string, (const gpointer)tr_id);
		gint ret_add_2 = json_mapper_add_property(updated_config, (const guchar*)"block_behaviour",         json_type_int,    (const gpointer)&block_behaviour);
		gint ret_add_3 = json_mapper_add_property(updated_config, (const guchar*)"transmission_timeout",    json_type_int,    (const gpointer)&transmission_timeout);
		gint ret_add_4 = json_mapper_add_property(updated_config, (const guchar*)"max_packet",              json_type_int,    (const gpointer)&max_packet);
		gint ret_add_5 = json_mapper_add_property(updated_config, (const guchar*)"filter_distance",         json_type_double, (const gpointer)&filter_distance);
		gint ret_add_6 = json_mapper_add_property(updated_config, (const guchar*)"filter_time",             json_type_int,    (const gpointer)&filter_time);
		gint ret_add_7 = json_mapper_add_property(updated_config, (const guchar*)"filter_logic",            json_type_int,    (const gpointer)&filter_logic);
		gint ret_add_8 = json_mapper_add_property(updated_config, (const guchar*)"filter_heading",          json_type_double, (const gpointer)&filter_heading);
		gint ret_add_9 = json_mapper_add_property(updated_config, (const guchar*)"network_anomaly_timeout", json_type_int,    (const gpointer)&network_anomaly_timeout);
		gint ret_add_A = json_mapper_add_property(updated_config, (const guchar*)"gps_anomaly_timeout",     json_type_int,    (const gpointer)&gps_anomaly_timeout);
		gboolean update_ok = (
			ret_add_1 == JSON_MAPPER_NO_ERROR &&
			ret_add_2 == JSON_MAPPER_NO_ERROR &&
			ret_add_3 == JSON_MAPPER_NO_ERROR &&
			ret_add_4 == JSON_MAPPER_NO_ERROR &&
			ret_add_5 == JSON_MAPPER_NO_ERROR &&
			ret_add_6 == JSON_MAPPER_NO_ERROR &&
			ret_add_7 == JSON_MAPPER_NO_ERROR &&
			ret_add_8 == JSON_MAPPER_NO_ERROR &&
			ret_add_9 == JSON_MAPPER_NO_ERROR &&
			ret_add_A == JSON_MAPPER_NO_ERROR);
		configuration_update_successful = ConfigurationStore_update_end(self->configuration_store, updated_config, update_ok);
	}
	if (configuration_update_successful)
	{
		//service_monitor fix interval updating
		service_monitor_set_fix_interval(filter_time*1000UL);
		ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_CONFIGURED_FROM_REMOTE);
		ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_MAX_PACKET_PROPERTY_UPDATE);
	}
	return configuration_update_successful;
}

static gboolean TollingManagerProxy_update_vehicle_config(
	TollingManagerProxy *self,
	const gchar *license_plate_nationality,
	const gchar *license_plate,
	const guint vehicle_max_laden_weight,
	const guint vehicle_max_train_weight,
	const guint fap_presence,
	const guint euro_weight_class,
	const guint default_axis)
{
	JsonMapper* updated_config = NULL;
	gboolean configuration_update_successful = ConfigurationStore_update_begin(self->configuration_store, &updated_config);
	if (configuration_update_successful)
	{
		gint ret_add_1 = json_mapper_add_property(updated_config, (guchar*)"license_plate_nationality", json_type_string, (const gpointer)license_plate_nationality);
		gint ret_add_2 = json_mapper_add_property(updated_config, (guchar*)"license_plate",             json_type_string, (const gpointer)license_plate);
		gint ret_add_3 = json_mapper_add_property(updated_config, (guchar*)"vehicle_max_laden_weight",  json_type_int,    (const gpointer)&vehicle_max_laden_weight);
		gint ret_add_4 = json_mapper_add_property(updated_config, (guchar*)"vehicle_max_train_weight",  json_type_int,    (const gpointer)&vehicle_max_train_weight);
		gint ret_add_5 = json_mapper_add_property(updated_config, (guchar*)"fap_presence",              json_type_int,    (const gpointer)&fap_presence);
		gint ret_add_6 = json_mapper_add_property(updated_config, (guchar*)"euro_weight_class",         json_type_int,    (const gpointer)&euro_weight_class);
		gint ret_add_7 = json_mapper_add_property(updated_config, (guchar*)"default_axis",              json_type_int,    (const gpointer)&default_axis);
		gboolean update_ok = (
			ret_add_1 == JSON_MAPPER_NO_ERROR &&
			ret_add_2 == JSON_MAPPER_NO_ERROR &&
			ret_add_3 == JSON_MAPPER_NO_ERROR &&
			ret_add_4 == JSON_MAPPER_NO_ERROR &&
			ret_add_5 == JSON_MAPPER_NO_ERROR &&
			ret_add_6 == JSON_MAPPER_NO_ERROR &&
			ret_add_7 == JSON_MAPPER_NO_ERROR);
		configuration_update_successful = ConfigurationStore_update_end(self->configuration_store, updated_config, update_ok);
	}
	
	if (configuration_update_successful)
	{
		ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_VEHICLE_CONFIGURED_FROM_REMOTE);
	}
				
	return configuration_update_successful;
}


static void TollingManagerProxy_get_initial_vehicle_config(TollingManagerProxy *self)
{
	GError *error = NULL;

	gchar *license_plate_nationality;
	gchar *license_plate;
	guint vehicle_max_laden_weight;
	guint vehicle_max_train_weight;
	guint fap_presence;
	guint euro_weight_class;
	guint default_axles;
	
	gboolean result_1 = tolling_call_get_license_plate_nationality_sync(
		self->dbus_proxy,
		&license_plate_nationality,
		NULL,
		&error);
	if (result_1 == FALSE)
	{
		logerr("tolling_call_get_license_plate_nationality_sync failed");
		if (error)
		{
			logerr("error code: %d, error message: %s\n", error->code, error->message);
			g_clear_error(&error);
		}
	}

	gboolean result_2 = tolling_call_get_license_plate_sync(
		self->dbus_proxy,
		&license_plate,
		NULL,
		&error);
	if (result_2 == FALSE)
	{
		logerr("tolling_call_get_license_plate_sync failed");
		if (error)
		{
			logerr("error code: %d, error message: %s\n", error->code, error->message);
			g_clear_error(&error);
		}
	}

	gboolean result_3 = tolling_call_get_vehicle_max_laden_weight_sync(
		self->dbus_proxy,
		&vehicle_max_laden_weight,
		NULL,
		&error);
	if (result_3 == FALSE)
	{
		logerr("tolling_call_get_vehicle_max_laden_weight_sync failed");
		if (error)
		{
			logerr("error code: %d, error message: %s\n", error->code, error->message);
			g_clear_error(&error);
		}
	}

	gboolean result_4 = tolling_call_get_vehicle_max_train_weight_sync(
		self->dbus_proxy,
		&vehicle_max_train_weight,
		NULL,
		&error);
	if (result_4 == FALSE)
	{
		logerr("tolling_call_get_vehicle_max_train_weight_sync failed");
		if (error)
		{
			logerr("error code: %d, error message: %s\n", error->code, error->message);
			g_clear_error(&error);
		}
	}

	gboolean result_5 = tolling_call_get_fap_presence_sync(
		self->dbus_proxy,
		&fap_presence,
		NULL,
		&error);
	if (result_5 == FALSE)
	{
		logerr("tolling_call_get_fap_presence_sync failed");
		if (error)
		{
			logerr("error code: %d, error message: %s\n", error->code, error->message);
			g_clear_error(&error);
		}
	}

	gboolean result_6 = tolling_call_get_euro_weight_class_sync(
		self->dbus_proxy,
		&euro_weight_class,
		NULL,
		&error);
	if (result_6 == FALSE)
	{
		logerr("tolling_call_get_euro_weight_class_sync failed");
		if (error)
		{
			logerr("error code: %d, error message: %s\n", error->code, error->message);
			g_clear_error(&error);
		}
	}

	gboolean result_7 = tolling_call_get_default_axles_sync(
		self->dbus_proxy,
		&default_axles,
		NULL,
		&error);
	if (result_7 == FALSE)
	{
		logerr("tolling_call_get_default_axles_sync failed");
		if (error)
		{
			logerr("error code: %d, error message: %s\n", error->code, error->message);
			g_clear_error(&error);
		}
	}

	gboolean all_values_available = (
		result_1 &&
		result_2 &&
		result_3 &&
		result_4 &&
		result_5 &&
		result_6 &&
		result_7);

	if (all_values_available)
	{
		logdbg("license_plate_nationality = %s", license_plate_nationality ? license_plate_nationality : "");
		logdbg("license_plate = %s", license_plate ? license_plate : "");
		logdbg("vehicle_max_laden_weight = %u", vehicle_max_laden_weight);
		logdbg("vehicle_max_train_weight = %u", vehicle_max_train_weight);
		logdbg("fap_presence = %u", fap_presence);
		logdbg("euro_weight_class = %u", euro_weight_class);
		logdbg("default_axles = %u", default_axles);

		gboolean config_updated = TollingManagerProxy_update_vehicle_config(
			self,
			license_plate_nationality,
			license_plate,
			vehicle_max_laden_weight,
			vehicle_max_train_weight,
			fap_presence,
			euro_weight_class,
			default_axles);

		if (config_updated == FALSE)
		{
			// TODO: catastrophic. Raise anomaly and quit?
		}
	}
	return;
}

static gboolean TollingManagerProxy_get_activation_for_service(TollingManagerProxy *self, const guint service_id, guint *active)
{
	GError *error = NULL;
	//guint active;
	gchar service_name[sizeof("tolling_XX")];
	g_snprintf(service_name, sizeof(service_name), "tolling_%s", self->gnss_domain_name);
	gboolean result = tolling_call_get_service_activation_status_sync(
		self->dbus_proxy,
		service_id,
		service_name,
		active,
		NULL,
		&error);
	if (!result)
	{
		logerr("tolling_call_get_service_activation_status_sync for service %u failed", service_id);
		if (error)
		{
			logerr("error code: %d, error message: %s\n", error->code, error->message);
			g_error_free(error);
		}
	}

	return result;
}

static void TollingManagerProxy_get_initial_service_activation(TollingManagerProxy *self)
{
	guint active;
	gboolean result;

	if (!self->tolling_gnss_sm_data->activation_checker)
        result = TollingManagerProxy_get_activation_for_service(self, self->service_activation_domain_id, &active);
	else
	    result = self->tolling_gnss_sm_data->activation_checker->get_service_activation(self->tolling_gnss_sm_data->activation_checker, &active);

	if (result)
	{
		logdbg("Initial service activation value = %s", active ? "true" : "false");
		if (active == 0)
		{
			ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_SERVICE_DEACTIVATED_FROM_REMOTE);
		}
		else
		{
			ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_SERVICE_ACTIVATED_FROM_REMOTE);
		}
	}

	return;
}

static void TollingManagerProxy_get_initial_activation_of_other_services(TollingManagerProxy *self)
{
	GHashTableIter iter;
	gpointer key, value;

	g_hash_table_iter_init (&iter, self->other_services_on_shared_domain);
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		gint service_id = *(gint*)key;

		guint active;
		gboolean result = TollingManagerProxy_get_activation_for_service(self, service_id, &active);

		if (result)
		{
			gboolean *is_active = g_new(gboolean, 1);
			*is_active = (gboolean)active;
			g_hash_table_iter_replace(&iter, is_active);
			loginfo("Service %d is %s", service_id, (*is_active) ? "active" : "not active");
		}
	}

	/*
	// DEBUG
	GHashTableIter debug_iter;
	gpointer debug_key, debug_value;

	g_hash_table_iter_init (&debug_iter, self->other_services_on_shared_domain);
	while (g_hash_table_iter_next (&debug_iter, &debug_key, &debug_value))
	{
		gint service_id = *(gint*)debug_key;
		gboolean active = *(gboolean*)debug_value;
		logdbg("Content of hashmap: service %d is %s", service_id, (active) ? "active" : "not active");
	}
	*/

	TollingManagerProxy_check_if_other_services_are_active(self);
}

static void TollingManagerProxy_get_initial_gnss_config(TollingManagerProxy *self)
{
	GError *error = NULL;
	gchar* trn_id = NULL;
	guint16 block_behaviour;
	guint transmission_timeout;
	guint16 max_packet;
	gdouble filter_distance;
	guint filter_time;
	guint filter_logic;
	gdouble filter_heading;
	guint network_anomaly_timeout;
	guint gps_anomaly_timeout;
	gboolean result = tolling_call_get_current_gnss_config_sync(
		self->dbus_proxy,
		self->gnss_domain_name,
		&trn_id,
		&block_behaviour,
		&transmission_timeout,
		&max_packet,
		&filter_distance,
		&filter_time,
		&filter_logic,
		&filter_heading,
		&network_anomaly_timeout,
		&gps_anomaly_timeout,
		NULL,
		&error);
	if (result)
	{
		logdbg("trn_id = %s", trn_id ? trn_id : "");
		logdbg("block_behaviour = %u", block_behaviour);
		logdbg("transmission_timeout = %u", transmission_timeout);
		logdbg("max_packet = %u", max_packet);
		logdbg("filter_distance = %lf", filter_distance);
		logdbg("filter_time = %u", filter_time);
		logdbg("filter_logic = %u", filter_logic);
		logdbg("filter_heading = %lf", filter_heading);
		logdbg("network_anomaly_timeout = %u", network_anomaly_timeout);
		logdbg("gps_anomaly_timeout = %u", gps_anomaly_timeout);
		TollingManagerProxy_update_gnss_config(
			self,
			trn_id,
			block_behaviour,
			transmission_timeout,
			max_packet,
			filter_distance,
			filter_time,
			filter_logic,
			filter_heading,
			network_anomaly_timeout,
			gps_anomaly_timeout);
	}
	else
	{
		logerr("tolling_call_get_current_gnss_config_sync failed");
		if (error)
		{
			logerr("error code: %d, error message: %s\n", error->code, error->message);
			g_error_free(error);
		}

		TollingManagerProxy_update_gnss_config(
					self,
					"default_config",
					0,					//block_behaviour
					FALLBACK_TRANSMISSION_TIME,
					60,
					FALLBACK_FILTER_DISTANCE,
					FALLBACK_FILTER_TIME,
					FALLBACK_FILTER_LOGIC,
					FALLBACK_FILTER_HEADING,
					FALLBACK_GPS_ANOMALY_TIMEOUT,
					FALLBACK_GPS_ANOMALY_TIMEOUT);


		guint16 max_packet;
		gboolean max_packet_available = TollingManagerProxy_get_max_packet(self, &max_packet);
		if (max_packet_available)
		{
			ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_CONFIGURED_FROM_REMOTE);
			ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_MAX_PACKET_PROPERTY_UPDATE);
		}
	}
	return;
}

static void TollingManagerProxy_set_current_trailer_axles(TollingManagerProxy *self, guint current_axles)
{
	logdbg("current_axles = %u", current_axles);
	guint tractor_axles;
	gboolean got_tractor_axles = TollingManagerProxy_get_default_axis(self, &tractor_axles);
	if (got_tractor_axles) {
		guint trailer_axles = current_axles - tractor_axles;
		logdbg("trailer_axles = %u", trailer_axles);
		self->current_axles = trailer_axles;
	} else {
		logerr("error getting default (trailer) axles value");
		self->current_axles = 0;
	}
}

static void TollingManagerProxy_get_initial_axles(TollingManagerProxy *self)
{
	GError *error = NULL;
	guint current_axles;
	gboolean result = tolling_call_get_current_axles_sync(
		self->dbus_proxy,
		&current_axles,
		NULL,
		&error);
	if (result)
	{
		logdbg("current_axles = %u", current_axles);
		TollingManagerProxy_set_current_trailer_axles(self, current_axles);
	}
	else
	{
		logdbg("tolling_call_get_current_axles_sync failed");
		if (error)
		{
			logdbg("error code: %d, error message: %s\n", error->code, error->message);
			g_error_free(error);
		}
		self->current_axles = 0;
	}
	return;
}

static void TollingManagerProxy_get_initial_weight(TollingManagerProxy *self)
{
	GError *error = NULL;
	guint current_weight;
	gboolean result = tolling_call_get_current_weight_sync(
		self->dbus_proxy,
		&current_weight,
		NULL,
		&error);
	if (result)
	{
		logdbg("current_weight = %u", current_weight);
		self->current_weight = current_weight;
	}
	else
	{
		logdbg("tolling_call_get_current_weight_sync failed");
		if (error)
		{
			logdbg("error code: %d, error message: %s\n", error->code, error->message);
			g_error_free(error);
		}
		self->current_weight = 0;
	return;
	}
}

static void TollingManagerProxy_get_initial_actual_weight(TollingManagerProxy *self)
{
	GError *error = NULL;
	guint current_actual_weight;
	gboolean result = tolling_call_get_actual_current_weight_sync(
		self->dbus_proxy,
		&current_actual_weight,
		NULL,
		&error);
	if (result)
	{
		logdbg("current_actual_weight = %u", current_actual_weight);
		self->current_actual_weight = current_actual_weight;
	}
	else
	{
		logdbg("tolling_call_get_actual_current_weight_sync failed");
		if (error)
		{
			logdbg("error code: %d, error message: %s\n", error->code, error->message);
			g_error_free(error);
		}
		self->current_actual_weight = 0;
	return;
	}
}

static void TollingManagerProxy_get_initial_trailer_type(TollingManagerProxy *self)
{
	GError *error = NULL;
	guint current_trailer_type;
	gboolean result = tolling_call_get_current_trailer_type_sync(
		self->dbus_proxy,
		&current_trailer_type,
		NULL,
		&error);
	if (result)
	{
		logdbg("current_trailer_type = %u", current_trailer_type);
		self->current_trailer_type = current_trailer_type;
	}
	else
	{
		logdbg("tolling_call_get_current_trailer_type failed");
		if (error)
		{
			logdbg("error code: %d, error message: %s\n", error->code, error->message);
			g_error_free(error);
		}
		self->current_trailer_type = 0;
	return;
	}
}


static void TollingManagerProxy_get_initial_gnss_domains(TollingManagerProxy *self)
{
	GError *error = NULL;
	gchar **out_gnss_domain_names;

	gboolean result = tolling_call_get_current_domains_sync(
		self->dbus_proxy,
		&out_gnss_domain_names,
		NULL,
		&error);
	if (result)
	{
		int index = 0;
		// "local" domain means: DE for tolling-de, PL for tolling-pl, etc
		gboolean local_domain_found = FALSE;
		while (out_gnss_domain_names[index])
		{
			if (g_strcmp0(out_gnss_domain_names[index], self->geofencing_domain_name) == 0)
			{
				logdbg("Inside GNSS domain %s", self->geofencing_domain_name);
				local_domain_found = TRUE;
				ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_ENTER_GNSS_DOMAIN);
				break;
			}
			index++;
		}
		// If the "local" domain isn't in the list of current domains, then the exit_domain event must be explicitly emitted.
		// This helps in one case: the tolling application is active, inside its domain, and then, very close to the border,
		// the Tolling Manager crashes. The border is crossed when the Tolling Manager is not running. When it is restarted,
		// its interface reappears, and this function calls the Tolling Manager to have the list of current domains, but at
		// that point the local domain is not listed. In that case, the tolling application would not find out that the OBU
		// is now outside of the domain, and it would keep collecting positions and sending tolling messages. This fixes it:
		// if the local domain isn't listed, then the EXIT_GNSS_DOMAIN signal is emitted.
		if (local_domain_found == FALSE)
		{
			logdbg("Outside GNSS domain %s", self->geofencing_domain_name);
			ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_EXIT_GNSS_DOMAIN);
		}
	}
	else
	{
		logerr("tolling_call_get_current_domains_sync failed");
		if (error)
		{
			logerr("error code: %d, error message: %s\n", error->code, error->message);
			g_error_free(error);
		}
	}
	g_strfreev(out_gnss_domain_names);
}

static void TollingManagerProxy_get_initial_ccc_domains(TollingManagerProxy *self)
{
	GError *error = NULL;
	GVariant *out_ccc_domain_ids_and_data;

	gboolean result = tolling_call_get_current_ccc_domains_sync(
		self->dbus_proxy,
		&out_ccc_domain_ids_and_data,
		NULL,
		&error);
	if (result)
	{
		GVariantIter iter;
		GVariantIter* domain_ids_iter;
		GVariantIter* data_iter;
		g_variant_iter_init(&iter, out_ccc_domain_ids_and_data);

		// "local" domain means: DE for tolling-de, PL for tolling-pl, etc
		gboolean local_domain_found = FALSE;

		while(g_variant_iter_loop(&iter, "(auau)", &domain_ids_iter, &data_iter)) {
			guint domain_id;
			while(g_variant_iter_loop(domain_ids_iter, "u", &domain_id)) {
				if (domain_id == self->service_activation_domain_id)
				{
					logdbg("Inside CCC domain %u", self->service_activation_domain_id);
					local_domain_found = TRUE;

					guint dsrc_flags = 0;
					ApplicationEvents_emit_event_ex(self->application_events, EVENT_TOLLING_MANAGER_PROXY_ENTER_CCC_DOMAIN, dsrc_flags);

					break;
				}
			}
		}

		if (local_domain_found == FALSE)
		{
			logdbg("Outside CCC domain %u", self->service_activation_domain_id);
			ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_EXIT_CCC_DOMAIN);
		}

		g_variant_unref(out_ccc_domain_ids_and_data);
	}
	else
	{
		logerr("tolling_call_get_current_ccc_domains_sync failed");
		if (error)
		{
			logerr("error code: %d, error message: %s\n", error->code, error->message);
			g_error_free(error);
		}
	}
}

void TollingManagerProxy_on_interface_available(
	GDBusConnection *connection,
	const gchar *name,
	const gchar *name_owner,
	gpointer gpointer_self)
{
	logdbg("Interface '%s' is available from owner '%s'", name, name_owner);
	TollingManagerProxy *self = (TollingManagerProxy*)gpointer_self;
	TollingManagerProxy_get_initial_vehicle_config(self);
	TollingManagerProxy_get_initial_service_activation(self);
	TollingManagerProxy_get_initial_activation_of_other_services(self);
	TollingManagerProxy_get_initial_gnss_config(self);
	self->get_initial_gnss_domains(self);
	TollingManagerProxy_get_initial_ccc_domains(self);

	return;
}

static void TollingManagerProxy_on_interface_not_available(GDBusConnection *connection, const gchar *name, gpointer data)
{
	logdbg("Interface '%s' is not available", name);
	return;
}

static void TollingManagerProxy_on_domain_enter(
	Tolling *object,
	const gchar *arg_domain_id,
	TollingManagerProxy *self)
{
	if (g_strcmp0(arg_domain_id, self->geofencing_domain_name) == 0)
	{
        logdbg("%s", arg_domain_id);
		ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_ENTER_GNSS_DOMAIN);
	}
	return;
}

static void TollingManagerProxy_on_domain_exit(
	Tolling *object,
	const gchar *arg_domain_id,
	TollingManagerProxy *self)
{
	if (g_strcmp0(arg_domain_id, self->geofencing_domain_name) == 0)
	{
		logdbg("%s", arg_domain_id);
		ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_EXIT_GNSS_DOMAIN);
	}
	return;
}

static void TollingManagerProxy_on_ccc_domain_enter(
	Tolling *object,
	GVariant *arg_ccc_domain_ids,
	GVariant *arg_ccc_context_data,
	TollingManagerProxy *self)
{
	GVariantIter domain_ids_iter;
	g_variant_iter_init(&domain_ids_iter, arg_ccc_domain_ids);
	guint32 domain_id_item;
	while (g_variant_iter_loop(&domain_ids_iter, "u", &domain_id_item))
	{
		if (domain_id_item == self->service_activation_domain_id)
		{
			logdbg("%d", domain_id_item);
            guint dsrc_flags = 0;
			ApplicationEvents_emit_event_ex(self->application_events, EVENT_TOLLING_MANAGER_PROXY_ENTER_CCC_DOMAIN, dsrc_flags);

			break;
		}
	}
	return;
}

static void TollingManagerProxy_on_ccc_domain_exit(
	Tolling *object,
	GVariant *arg_ccc_domain_ids,
	GVariant *arg_ccc_context_data,
	TollingManagerProxy *self)
{
	GVariantIter domain_ids_iter;
	g_variant_iter_init(&domain_ids_iter, arg_ccc_domain_ids);
	guint32 domain_id_item;
	while (g_variant_iter_loop(&domain_ids_iter, "u", &domain_id_item))
	{
		if (domain_id_item == self->service_activation_domain_id)
		{
			logdbg("%d", domain_id_item);
			ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_EXIT_CCC_DOMAIN);
			break;
		}
	}
	return;
}

static void TollingManagerProxy_on_service_activation_status_changed(
	Tolling *object,
	guint arg_service_id,
	const gchar *arg_service_name,
	guint arg_service_status,
	TollingManagerProxy *self)
{
	logdbg("service = %u, status = %u", arg_service_id, arg_service_status);
	if (arg_service_id == self->service_activation_domain_id)
	{
		if (arg_service_status == 0)
		{
			ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_SERVICE_DEACTIVATED_FROM_REMOTE);
		}
		else
		{
			ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_SERVICE_ACTIVATED_FROM_REMOTE);


		}
	} else {
		if (self->other_services_on_shared_domain)
		{
			TollingManagerProxy_update_activation_status_of_other_services(self, arg_service_id, arg_service_status);
		}
		else
		{
			//logdbg("No other service on shared domain");
		}

	}
	return;
}

static void TollingManagerProxy_on_gnss_config_update(
	Tolling *object,
	const gchar *arg_tr_id,
	const gchar *arg_tolling_domain,
	guint16 arg_block_behaviour,
	guint arg_transmission_timeout,
	guint16 arg_max_packet,
	gdouble arg_filter_distance,
	guint arg_filter_time,
	guint arg_filter_logic,
	gdouble arg_filter_heading,
	guint arg_network_anomaly_timeout,
	guint arg_gps_anomaly_timeout,
	TollingManagerProxy *self)
{
	if (g_strcmp0(arg_tolling_domain, self->gnss_domain_name) != 0)
	{
		logdbg("ignoring message for another tolling domain:", arg_tolling_domain);
		return;
	}
	logdbg("%s", arg_tolling_domain);
	logdbg("arg_tr_id = %s", arg_tr_id ? arg_tr_id : "");
	logdbg("arg_block_behaviour = %u", arg_block_behaviour);
	logdbg("arg_transmission_timeout = %u", arg_transmission_timeout);
	logdbg("arg_max_packet = %u", arg_max_packet);
	logdbg("arg_filter_distance = %lf", arg_filter_distance);
	logdbg("arg_filter_time = %u", arg_filter_time);
	logdbg("arg_filter_logic = %u", arg_filter_logic);
	logdbg("arg_filter_heading = %lf", arg_filter_heading);
	logdbg("arg_network_anomaly_timeout = %u",arg_network_anomaly_timeout);
	logdbg("arg_gps_anomaly_timeout = %u", arg_gps_anomaly_timeout);
	TollingManagerProxy_update_gnss_config(
		self,
		arg_tr_id,
		arg_block_behaviour,
		arg_transmission_timeout,
		arg_max_packet,
		arg_filter_distance,
		arg_filter_time,
		arg_filter_logic,
		arg_filter_heading,
		arg_network_anomaly_timeout,
		arg_gps_anomaly_timeout);
	return;
}

static void TollingManagerProxy_on_vehicle_config_update(
	Tolling *object,
	const gchar *arg_license_plate_nationality,
	const gchar *arg_license_plate,
	guint arg_vehicle_max_laden_weight,
	guint arg_vehicle_max_train_weight,
	guint arg_fap_presence,
	guint arg_euro_weight_class,
	guint arg_default_axis,
	TollingManagerProxy *self)
{
	logdbg("");
	if (arg_license_plate_nationality != NULL)
	{
		logdbg("arg_license_plate_nationality = %s", arg_license_plate_nationality);
	}
	if (arg_license_plate != NULL)
	{
		logdbg("arg_license_plate = %s", arg_license_plate);
	}
	logdbg("arg_vehicle_max_laden_weight = %u", arg_vehicle_max_laden_weight);
	logdbg("arg_vehicle_max_train_weight = %u", arg_vehicle_max_train_weight);
	logdbg("arg_fap_presence = %u", arg_fap_presence);
	logdbg("arg_euro_weight_class = %u", arg_euro_weight_class);
	logdbg("arg_default_axis = %u", arg_default_axis);
	TollingManagerProxy_update_vehicle_config(
		self,
		arg_license_plate_nationality,
		arg_license_plate,
		arg_vehicle_max_laden_weight,
		arg_vehicle_max_train_weight,
		arg_fap_presence,
		arg_euro_weight_class,
		arg_default_axis);

	return;
}

static void TollingManagerProxy_on_current_axles_changed(
	Tolling *object,
	guint new_axles,
	const gchar *trn_id,
	TollingManagerProxy *self)
{
	loginfo("new_axles = %u, trn_id = %s", new_axles, trn_id);

	TollingManagerProxy_set_current_trailer_axles(self, new_axles);

	return;
}

static void TollingManagerProxy_on_current_weight_changed(
	Tolling *object,
	guint current_weight,
	TollingManagerProxy *self)
{
	loginfo("%u", current_weight);
	self->current_weight = current_weight;
	return;
}

static void TollingManagerProxy_on_current_trailer_type_changed(
	Tolling *object,
	guint current_trailer_type,
	TollingManagerProxy *self)
{
	loginfo("%u", current_trailer_type);
	self->current_trailer_type = current_trailer_type;
	return;
}

static void TollingManagerProxy_on_current_actual_weight_changed(
	Tolling *object,
	guint current_actual_weight,
	TollingManagerProxy *self)
{
	loginfo("%u", current_actual_weight);
	self->current_actual_weight = current_actual_weight;
	return;
}

gboolean TollingManagerProxy_get_tr_id(const TollingManagerProxy *self, const gchar **value)
{
	return ConfigurationStore_get_string(self->configuration_store, (const guchar*)"tr_id", value);
}

gboolean TollingManagerProxy_get_block_behaviour(const TollingManagerProxy *self, guint16 *value)
{
	gboolean found = ConfigurationStore_get_int16_ex(self->configuration_store, (const guchar*)"block_behaviour", (gint16*)value, FALSE);
	if (found == FALSE)
	{
		logwarn("using fall back value %d", FALLBACK_BLOCK_BEHAVIOUR);
		*value = FALLBACK_BLOCK_BEHAVIOUR;
	}
	return TRUE;
}

gboolean TollingManagerProxy_get_transmission_timeout(const TollingManagerProxy *self, guint *value)
{
	return ConfigurationStore_get_int(self->configuration_store, (const guchar*)"transmission_timeout", (gint*)value);
}

gboolean TollingManagerProxy_get_max_packet(const TollingManagerProxy *self, guint16 *value)
{
	return ConfigurationStore_get_int16(self->configuration_store, (const guchar*)"max_packet", (gint16*)value);
}

gboolean TollingManagerProxy_get_filter_distance(const TollingManagerProxy *self, gdouble *value)
{
	gboolean found = ConfigurationStore_get_double_ex(self->configuration_store, (const guchar*)"filter_distance", (gdouble*)value, FALSE);
	if (found == FALSE)
	{
		logwarn("using fall back value %f", FALLBACK_FILTER_DISTANCE);
		*value = FALLBACK_FILTER_DISTANCE;
	}
	return TRUE;
}

gboolean TollingManagerProxy_get_filter_time(const TollingManagerProxy *self, guint *value)
{
	gboolean found = ConfigurationStore_get_int_ex(self->configuration_store, (const guchar*)"filter_time", (gint*)value, FALSE);
	if (found == FALSE)
	{
		logdbg("using fall back value %d", FALLBACK_FILTER_TIME);
		*value = FALLBACK_FILTER_TIME;
	}
	return TRUE;
}

gboolean TollingManagerProxy_get_filter_logic(const TollingManagerProxy *self, guint *value)
{
	gboolean found = ConfigurationStore_get_int_ex(self->configuration_store, (const guchar*)"filter_logic", (gint*)value, FALSE);
	if (found == FALSE)
	{
		logwarn("using fall back value %d", FALLBACK_FILTER_LOGIC);
		*value = FALLBACK_FILTER_LOGIC;
	}
	return TRUE;
}

gboolean TollingManagerProxy_get_filter_heading(const TollingManagerProxy *self, gdouble *value)
{
	gboolean found = ConfigurationStore_get_double_ex(self->configuration_store, (const guchar*)"filter_heading", (gdouble*)value, FALSE);
	if (found == FALSE)
	{
		logwarn("using fall back value %f", FALLBACK_FILTER_HEADING);
		*value = FALLBACK_FILTER_HEADING;
	}
	return TRUE;
}

gboolean TollingManagerProxy_get_network_anomaly_timeout(const TollingManagerProxy *self, guint *value)
{
	return ConfigurationStore_get_int(self->configuration_store, (const guchar*)"network_anomaly_timeout", (gint*)value);
}

gboolean TollingManagerProxy_get_gps_anomaly_timeout(const TollingManagerProxy *self, guint *value)
{
	gboolean found = ConfigurationStore_get_int_ex(self->configuration_store, (const guchar*)"gps_anomaly_timeout", (gint*)value, FALSE);
	if (found == FALSE)
	{
		logdbg("using fall back value %d", FALLBACK_GPS_ANOMALY_TIMEOUT);
		*value = FALLBACK_GPS_ANOMALY_TIMEOUT;
	}
	return TRUE;
}

gboolean TollingManagerProxy_get_license_plate_nationality(const TollingManagerProxy *self, const gchar **value)
{
	return ConfigurationStore_get_string(self->configuration_store, (const guchar*)"license_plate_nationality", value);
}

gboolean TollingManagerProxy_get_license_plate(const TollingManagerProxy *self, const gchar **value)
{
	return ConfigurationStore_get_string(self->configuration_store, (const guchar*)"license_plate", value);
}

gboolean TollingManagerProxy_get_vehicle_max_laden_weight(const TollingManagerProxy *self, guint *value)
{
	return ConfigurationStore_get_int(self->configuration_store, (const guchar*)"vehicle_max_laden_weight", (gint*)value);
}

gboolean TollingManagerProxy_get_vehicle_max_train_weight(const TollingManagerProxy *self, guint *value)
{
	return ConfigurationStore_get_int(self->configuration_store, (const guchar*)"vehicle_max_train_weight", (gint*)value);
}

gboolean TollingManagerProxy_get_fap_presence(const TollingManagerProxy *self, guint *value)
{
	return ConfigurationStore_get_int(self->configuration_store, (const guchar*)"fap_presence", (gint*)value);
}

gboolean TollingManagerProxy_get_euro_weight_class(const TollingManagerProxy *self, guint *value)
{
	return ConfigurationStore_get_int(self->configuration_store, (const guchar*)"euro_weight_class", (gint*)value);
}

gboolean TollingManagerProxy_get_default_axis(const TollingManagerProxy *self, guint *value)
{
	return ConfigurationStore_get_int(self->configuration_store, (const guchar*)"default_axis", (gint*)value);
}

gboolean TollingManagerProxy_get_current_axles(const TollingManagerProxy *self, guint *value)
{
	*value = self->current_axles;
	return TRUE;
}

gboolean TollingManagerProxy_get_current_weight(const TollingManagerProxy *self, guint *value)
{
	*value = self->current_weight;
	return TRUE;
}

gboolean TollingManagerProxy_get_current_actual_weight(const TollingManagerProxy *self, guint *value){

	*value = self->current_actual_weight;
	return TRUE;
}

gboolean TollingManagerProxy_get_current_trailer_type(const TollingManagerProxy *self, guint *value){

	*value = self->current_trailer_type;
	return TRUE;
}


GHashTable *TollingManagerProxy_set_other_services(TollingManagerProxy *self, const GArray * const other_services_array)
{
	GHashTable *other_services_hashtable = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, g_free);

	if (other_services_array == NULL || other_services_array->len == 0)
	{
		loginfo("There are no other services which share the domain with this applications's service (%u)", self->service_activation_domain_id);
		return other_services_hashtable;
	}

	for (int i = 0; i < other_services_array->len; i++)
	{
		// Key
		gint *service_id = g_new(gint, 1);
		*service_id = g_array_index(other_services_array, gint, i);

		// Value
		gboolean *active = g_new(gboolean, 1);
		*active = FALSE;

		g_hash_table_insert(other_services_hashtable, (gpointer)service_id, (gpointer)active);
		loginfo("Service %d shares the domain with this application's service", *service_id);
	}

	return other_services_hashtable;
}

static void TollingManagerProxy_update_activation_status_of_other_services(const TollingManagerProxy *self, const guint service_id, const guint service_status)
{
	gpointer value = g_hash_table_lookup(self->other_services_on_shared_domain, &service_id);
	if (value)
	{
		loginfo("Service %u is among the other services for this domain", service_id);
		gboolean *bool_active = (gboolean*)value;
		*bool_active = (gboolean)service_status;

		TollingManagerProxy_check_if_other_services_are_active(self);
	}
	else
	{
		logdbg("Service %u is not among the other services for this domain", service_id);
	}
}

static void TollingManagerProxy_check_if_other_services_are_active(const TollingManagerProxy *self)
{
	gboolean is_at_least_another_service_active = FALSE;

	GHashTableIter iter;
	gpointer key, value;

	g_hash_table_iter_init (&iter, self->other_services_on_shared_domain);
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		gboolean active = *(gboolean*)value;
		if (active)
		{
			is_at_least_another_service_active = TRUE;
			break;
		}
	}

	if (is_at_least_another_service_active)
	{
		ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_OTHER_ACTIVE_SERVICES_PRESENT);
	}
	else
	{
		ApplicationEvents_emit_event(self->application_events, EVENT_TOLLING_MANAGER_PROXY_OTHER_ACTIVE_SERVICES_ABSENT);
	}
}



// Initialised to NULL here, so that an application that doesn't need it can simply ignore it
GArray *other_services_on_this_domain = NULL;


static void TollingManagerProxy_destroy(TollingManagerProxy *self);
static TollingManagerProxy *TollingManagerProxy_allocate();
static void TollingManagerProxy_deallocate(TollingManagerProxy *self);


TollingManagerProxy *TollingManagerProxy_new(
	const gchar *gnss_domain_name,
	const guint service_activation_domain_id,
	ConfigurationStore *configuration_store,
	ApplicationEvents *application_events,
	Tolling_Gnss_Sm_Data *data)
{
	logdbg("");

	TollingManagerProxy *self = TollingManagerProxy_allocate();
	g_return_val_if_fail(self != NULL, NULL);

	TollingManagerProxy *initialize_result = TollingManagerProxy_initialize(self,
			gnss_domain_name,
			service_activation_domain_id,
			configuration_store,
			application_events,
			data);
	if (initialize_result == NULL) TollingManagerProxy_deallocate(self);
	g_return_val_if_fail(initialize_result != NULL, NULL);

	return self;
}

void TollingManagerProxy_delete(TollingManagerProxy *self)
{
	if (self != NULL) {
		self->dtor(self);
	}
}

static void TollingManagerProxy_destroy(TollingManagerProxy *self)
{
	logdbg("");

	TollingManagerProxy_deinitialize(self);

	TollingManagerProxy_deallocate(self);
}

static TollingManagerProxy *TollingManagerProxy_allocate()
{
    logdbg("");

	TollingManagerProxy *self = g_try_new0(TollingManagerProxy, 1);

	g_return_val_if_fail(self != NULL, NULL);

	self->dtor                      = TollingManagerProxy_destroy;
	self->get_initial_gnss_domains  = TollingManagerProxy_get_initial_gnss_domains;
	self->on_interface_available    = TollingManagerProxy_on_interface_available;
	self->on_domain_enter           = TollingManagerProxy_on_domain_enter;
	self->on_domain_exit            = TollingManagerProxy_on_domain_exit;

	return self;
}

static void TollingManagerProxy_deallocate(TollingManagerProxy *self)
{
	logdbg("");

	if (self != NULL)
	{
		g_free(self);
	}
}

TollingManagerProxy *TollingManagerProxy_initialize(TollingManagerProxy *self,
		const gchar *gnss_domain_name,
		const guint service_activation_domain_id,
		ConfigurationStore *configuration_store,
		ApplicationEvents *application_events,
		Tolling_Gnss_Sm_Data *data)
{
	loginfo("");

	g_return_val_if_fail(self != NULL, NULL);

	self->configuration_store = configuration_store;
	self->application_events = application_events;
	self->tolling_gnss_sm_data = data;
	self->gnss_domain_name = gnss_domain_name;
	self->geofencing_domain_name  = gnss_domain_name;
	self->service_activation_domain_id = service_activation_domain_id;
	self->other_services_on_shared_domain = TollingManagerProxy_set_other_services(self, other_services_on_this_domain);


	GError *g_error = NULL;
	self->dbus_proxy =  tolling_proxy_new_for_bus_sync(
		G_BUS_TYPE_SYSTEM,
		G_DBUS_PROXY_FLAGS_NONE,
		tolling_manager_bus_name,
		tolling_manager_obj_path,
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
			tolling_manager_bus_name,
			G_BUS_NAME_WATCHER_FLAGS_NONE,
			self->on_interface_available,
			TollingManagerProxy_on_interface_not_available,
			self,
			NULL);
		g_signal_connect(
			self->dbus_proxy,
			"domain_enter",
			G_CALLBACK(self->on_domain_enter),
			self);
		g_signal_connect(
			self->dbus_proxy,
			"domain_exit",
			G_CALLBACK(self->on_domain_exit),
			self);
		g_signal_connect(
			self->dbus_proxy,
			"ccc_domain_enter",
			G_CALLBACK(TollingManagerProxy_on_ccc_domain_enter),
			self);
		g_signal_connect(
			self->dbus_proxy,
			"ccc_domain_exit",
			G_CALLBACK(TollingManagerProxy_on_ccc_domain_exit),
			self);
		g_signal_connect(
			self->dbus_proxy,
			"gnss_config_update",
			G_CALLBACK(TollingManagerProxy_on_gnss_config_update),
			self);
		g_signal_connect(
			self->dbus_proxy,
			"vehicle_config_update",
			G_CALLBACK(TollingManagerProxy_on_vehicle_config_update),
			self);
		g_signal_connect(
			self->dbus_proxy,
			"current_axles_changed",
			G_CALLBACK(TollingManagerProxy_on_current_axles_changed),
			self);
		g_signal_connect(
			self->dbus_proxy,
			"current_weight_changed",
			G_CALLBACK(TollingManagerProxy_on_current_weight_changed),
			self);
		g_signal_connect(
			self->dbus_proxy,
			"current_trailer_type_changed",
			G_CALLBACK(TollingManagerProxy_on_current_trailer_type_changed),
			self);
		g_signal_connect(
			self->dbus_proxy,
			"actual_current_weight_changed",
			G_CALLBACK(TollingManagerProxy_on_current_actual_weight_changed),
			self);

        if (!self->tolling_gnss_sm_data->activation_checker) {
            g_signal_connect(
                self->dbus_proxy,
                "service_activation_status_changed",
                G_CALLBACK(TollingManagerProxy_on_service_activation_status_changed),
                self);
        }
        else {
            self->tolling_gnss_sm_data->activation_checker->set_service_activation_changed_callbak(
                self->tolling_gnss_sm_data->activation_checker,
                G_CALLBACK(TollingManagerProxy_on_service_activation_status_changed),
                self
            );
        }
	}
	return self;
}

void TollingManagerProxy_deinitialize(TollingManagerProxy *self)
{
	loginfo("");

	if (self != NULL)
	{
		if (self->other_services_on_shared_domain)
		{
			g_hash_table_destroy(self->other_services_on_shared_domain);
		}
		if (self->watcher_id)
		{
			g_bus_unwatch_name(self->watcher_id);
		}
		if (self->dbus_proxy)
		{
			g_object_unref(self->dbus_proxy);
		}
	}
}
