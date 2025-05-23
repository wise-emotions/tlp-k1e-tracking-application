/*
 * gnss_fix_data.c
 *
 *  Created on: Nov 5, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#include "positioning_service_types.h"
#include "logger.h"
#include "json_mapper.h"
#include "utils.h"
#include "shared_types.h"

#include "odometer.h"
#include "tolling_gnss_sm_data.h"
#include "trip_id_manager.h"
#include "tolling_manager_proxy.h"
#include "fix_and_status.h"
#include "trip_id_manager.h"
#include "fix_data_json.h"

#include "gnss_fix_data.h"


void gnss_fix_data_fill_remaining_fields(GnssFixData *fix_data, Tolling_Gnss_Sm_Data  *tolling_gnss_sm_data);
void gnss_fix_data_fill_fix_and_status(GnssFixData *fix_data, Tolling_Gnss_Sm_Data *tolling_gnss_sm_data);
void fix_and_status_data_to_json_mapper(const GnssFixData *self, JsonMapper *json_mapper);


GnssFixData *gnss_fix_data_new(void)
{
	GnssFixData *self = (GnssFixData*)g_malloc0(sizeof(GnssFixData));
	self->data_id = g_string_new("");
	self->fix_and_status = fix_and_status_new();

	return self;
}


void gnss_fix_data_destroy(GnssFixData *self)
{
	if (self) {
		if (self->fix_and_status) {
			fix_and_status_destroy(self->fix_and_status);
		}
		if (self->data_id) {
			g_string_free(self->data_id, TRUE);
		}
		g_free(self);
	} else {
		logerr("Freeing NULL GnssFixData");
	}
}

gboolean gnss_fix_data_are_equal(const GnssFixData *first, const GnssFixData *second)
{
	return
	g_string_equal(          first->data_id,                second->data_id)             &&
	                         first->latitude             == second->latitude             &&
	                         first->longitude            == second->longitude            &&
	                         first->altitude             == second->altitude             &&
	                         first->fix_time_epoch       == second->fix_time_epoch       &&
	                         first->gps_speed            == second->gps_speed            &&
	                         first->accuracy             == second->accuracy             &&
	                         first->gps_heading          == second->gps_heading          &&
	                         first->satellites_for_fix   == second->satellites_for_fix   &&
	fix_and_status_are_equal(first->fix_and_status,         second->fix_and_status)      &&
	                         first->total_distance_m     == second->total_distance_m     &&
	                         first->prg_trip             == second->prg_trip             &&
	                         first->current_axis_trailer == second->current_axis_trailer &&
                             first->current_train_weight == second->current_train_weight &&
                             first->fix_validity         == second->fix_validity         &&
                             first->fix_type             == second->fix_type             &&
                             first->pdop                 == second->pdop                 &&
	                         first->hdop                 == second->hdop                 &&
	                         first->vdop                 == second->vdop;
}



void gnss_fix_data_copy_from_position_data(GnssFixData *fix_data, const PositionData *position, Tolling_Gnss_Sm_Data  *tolling_gnss_sm_data)
{

	logdbg("******************************************************");
	fix_data->latitude           = position->latitude;
	fix_data->longitude          = position->longitude;
	fix_data->altitude           = position->altitude;

	fix_data->fix_time_epoch     = position->date_time;
	fix_data->gps_speed          = position->speed;
	fix_data->accuracy           = position->accuracy;
	fix_data->gps_heading        = position->heading;
	fix_data->satellites_for_fix = position->sat_for_fix->len;

	fix_data->fix_validity       = position->fix_validity;
	fix_data->fix_type           = position->fix_type;
	fix_data->pdop               = position->pdop;
	fix_data->vdop               = position->vdop;
	fix_data->hdop               = position->hdop;

	gnss_fix_data_fill_remaining_fields(fix_data, tolling_gnss_sm_data);

}

void gnss_fix_data_fill_remaining_fields(GnssFixData *fix_data, Tolling_Gnss_Sm_Data *tolling_gnss_sm_data)
{
	//fix_data->prg_trip          = TripIdManager_get_current_trip_id(tolling_gnss_sm_data->trip_id_manager);
	fix_data->total_distance_m  = (gint) (odometer_get_trip_distance(tolling_gnss_sm_data->odometer)* KM_TO_M);

	TollingManagerProxy_get_current_axles(tolling_gnss_sm_data->tolling_manager_proxy, &fix_data->current_axis_trailer);
	TollingManagerProxy_get_current_weight(tolling_gnss_sm_data->tolling_manager_proxy, &fix_data->current_train_weight);
	TollingManagerProxy_get_current_actual_weight(tolling_gnss_sm_data->tolling_manager_proxy, &fix_data->current_actual_weight);
        TollingManagerProxy_get_current_trailer_type(tolling_gnss_sm_data->tolling_manager_proxy,&fix_data->current_trailer_type);
}

guint64 gnss_fix_data_get_timestamp_in_milliseconds(const GnssFixData *fix_data)
{
	return fix_data->fix_time_epoch;
}

gdouble gnss_fix_data_get_latitude(const GnssFixData *fix_data)
{
	return fix_data->latitude;
}

gdouble gnss_fix_data_get_longitude(const GnssFixData *fix_data)
{
	return fix_data->longitude;
}

gdouble gnss_fix_data_get_altitude(const GnssFixData *fix_data)
{
	return fix_data->altitude;
}

gdouble gnss_fix_data_get_heading(const GnssFixData *fix_data)
{
	return fix_data->gps_heading;
}

gdouble gnss_fix_data_get_odometer(const GnssFixData *fix_data)
{
	return fix_data->total_distance_m;
}

gboolean gnss_fix_data_get_nogo(const GnssFixData *fix_data)
{
	return fix_data->fix_and_status->nogo;
}

void fix_and_status_data_to_json_mapper(const GnssFixData *self, JsonMapper *json_mapper)
{
	json_mapper_add_property(json_mapper, (const guchar*)JSON_TAMPERING,            json_type_boolean,      (const gpointer) &self->fix_and_status->tampering);
	json_mapper_add_property(json_mapper, (const guchar*)JSON_NOGO,                 json_type_boolean,      (const gpointer) &self->fix_and_status->nogo);
	json_mapper_add_property(json_mapper, (const guchar*)JSON_BLOCKED,              json_type_boolean,      (const gpointer) &self->fix_and_status->blocked);
	json_mapper_add_property(json_mapper, (const guchar*)JSON_TOLLING_ELIGIBILITY,  json_type_boolean,      (const gpointer) &self->fix_and_status->tolling_eligibility);
}

void posdata_identifier_to_json_mapper(const GnssFixData *self, JsonMapper *json_mapper)
{

	GString *create_time = date_time_unix_utc_to_iso8601_ms(self->fix_time_epoch); // milliseconds
	json_mapper_add_property(json_mapper, (const guchar*)JSON_TIME,         json_type_string,   (const gpointer) create_time->str);
	g_string_free(create_time, TRUE);
}

void position_data_to_json_mapper(const GnssFixData *self, JsonMapper *json_mapper)
{
	json_mapper_add_property(json_mapper, (const guchar*)JSON_LATITUDE,     json_type_double,   (const gpointer) &self->latitude);
	json_mapper_add_property(json_mapper, (const guchar*)JSON_LONGITUDE,    json_type_double,   (const gpointer) &self->longitude);
	json_mapper_add_property(json_mapper, (const guchar*)JSON_ALTITUDE,     json_type_double,   (const gpointer) &self->altitude);
	json_mapper_add_property(json_mapper, (const guchar*)JSON_SPEED,        json_type_double,   (const gpointer) &self->gps_speed);
	json_mapper_add_property(json_mapper, (const guchar*)JSON_DIRECTION,    json_type_double,   (const gpointer) &self->gps_heading);
	json_mapper_add_property(json_mapper, (const guchar*)JSON_HDOP,         json_type_double,   (const gpointer) &self->hdop);
	json_mapper_add_property(json_mapper, (const guchar*)JSON_SAT_FOR_FIX,  json_type_int,      (const gpointer) &self->satellites_for_fix);
}


void computed_data_to_json_mapper(const GnssFixData *self, JsonMapper *json_mapper)
{
	gdouble total = self->total_distance_m/1000.0;
	json_mapper_add_property(json_mapper, (const guchar*)JSON_ODOMETER,     json_type_double,      (const gpointer)&total );
}


void vehicle_data_to_json_mapper(const GnssFixData *self, JsonMapper *json_mapper)
{
	json_mapper_add_property(json_mapper, (const guchar*)JSON_VEHICLE_TRAILER_AXLES, json_type_int,      (const gpointer)&self->current_axis_trailer);
	json_mapper_add_property(json_mapper, (const guchar*)JSON_VEHICLE_TRAIN_WEIGHT,  json_type_int,      (const gpointer)&self->current_train_weight);
	json_mapper_add_property(json_mapper, (const guchar*)JSON_VEHICLE_ACTUAL_TRAIN_WEIGHT,  json_type_int,      (const gpointer)&self->current_actual_weight);
	json_mapper_add_property(json_mapper, (const guchar*)JSON_VEHICLE_TRAILER_TYPE,  json_type_int,      (const gpointer)&self->current_trailer_type);

}

JsonMapper *gnss_fix_data_to_json_mapper(const GnssFixData *self)
{
	JsonMapper *json_mapper = json_mapper_create((guchar*)"{}");

	posdata_identifier_to_json_mapper(self, json_mapper);
	position_data_to_json_mapper(self, json_mapper);
//	fix_and_status_data_to_json_mapper(self, json_mapper);
	computed_data_to_json_mapper(self, json_mapper);
//	vehicle_data_to_json_mapper(self, json_mapper);
	logdbg("json string (mapper)   : '%s'", json_mapper_to_string(json_mapper));

	return json_mapper;
}
