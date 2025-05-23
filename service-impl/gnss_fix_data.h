/*
 * gnss_fix_data.h
 *
 *  Created on: Nov 26, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#ifndef GNSS_FIX_DATA_H_
#define GNSS_FIX_DATA_H_

#include <glib.h>

#define KM_TO_M  1000.0

typedef struct _JsonMapper JsonMapper;


typedef struct _GnssFixData {
	GString        *data_id;
	gdouble         latitude;
	gdouble         longitude;
	gdouble         altitude;
	guint64         fix_time_epoch;
	gdouble         gps_speed;
	gdouble         accuracy;
	gdouble         gps_heading;
	guint           satellites_for_fix;

	gint            total_distance_m;
	guint           prg_trip;

	guint           current_actual_weight;
	gint 		current_trailer_type;
	guint           current_axis_trailer;
	guint           current_train_weight;

	guint           fix_validity;
	guchar          fix_type;
	gdouble         pdop;
	gdouble         hdop;
	gdouble         vdop;

} GnssFixData;

typedef struct _PositionData PositionData;
typedef struct _Tolling_Gnss_Sm_Data Tolling_Gnss_Sm_Data;

GnssFixData *gnss_fix_data_new(void);
void gnss_fix_data_destroy(GnssFixData *self);

gboolean gnss_fix_data_are_equal(const GnssFixData *first, const GnssFixData *second);

void gnss_fix_data_copy_from_position_data(GnssFixData *fix_data, const PositionData *position, Tolling_Gnss_Sm_Data *tolling_gnss_sm_data);
void gnss_fix_data_fill_data_id(GnssFixData *fix_data, Tolling_Gnss_Sm_Data *tolling_gnss_sm_data);
guint64 gnss_fix_data_get_timestamp_in_milliseconds(const GnssFixData *fix_data);
gdouble gnss_fix_data_get_latitude(const GnssFixData *fix_data);
gdouble gnss_fix_data_get_longitude(const GnssFixData *fix_data);
gdouble gnss_fix_data_get_altitude(const GnssFixData *fix_data);
gdouble gnss_fix_data_get_heading(const GnssFixData *fix_data);
gdouble gnss_fix_data_get_odometer(const GnssFixData *fix_data);
gboolean gnss_fix_data_get_nogo(const GnssFixData *fix_data);

// The returned JsonMapper is owned by the caller, who is in charge of freeing it
JsonMapper *gnss_fix_data_to_json_mapper(const GnssFixData *self);



#endif /* GNSS_FIX_DATA_H_ */
