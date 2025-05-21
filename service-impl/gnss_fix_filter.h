/*
 * gnss_fix_filter.h
 *
 *  Created on: Jan 28, 2022
 *  Author: Gianni Manzo (gianni.manzo@nttdata.com)
 */

#ifndef GNSS_FIX_FILTER_H_
#define GNSS_FIX_FILTER_H_


#include <glib.h>


typedef struct _Tolling_Gnss_Sm_Data Tolling_Gnss_Sm_Data;
typedef struct _GnssFixData GnssFixData;
typedef struct _GnssFixBuffer GnssFixBuffer;


typedef struct _GnssFixFilter {
	Tolling_Gnss_Sm_Data *tolling_gnss_sm_data;

	gboolean previous_fix_present;

	// used for filter by time
	guint64 timestamp;

	// used for filter by position
	gdouble latitude;
	gdouble longitude;
	gdouble altitude;

	// used for filter by heading
	gdouble heading;

	// used for filter by distance
	gdouble distance;
} GnssFixFilter;


GnssFixFilter* GnssFixFilter_new(Tolling_Gnss_Sm_Data* data);
void GnssFixFilter_destroy(GnssFixFilter *self);

void GnssFixFilter_reset_last_fix(GnssFixFilter *self);
void GnssFixFilter_set_last_fix(GnssFixFilter *self, const GnssFixData *current_fix);

gboolean GnssFixFilter_get_filter_logic(GnssFixFilter *self, guint *value);
gboolean GnssFixFilter_get_filter_distance(GnssFixFilter *self, gdouble *value);
gboolean GnssFixFilter_get_filter_heading(GnssFixFilter *self, gdouble *value);
gboolean GnssFixFilter_get_filter_time(GnssFixFilter *self, guint *value);

gdouble GnssFixFilter_get_distance(GnssFixFilter *self, const GnssFixData *current_fix);
gdouble GnssFixFilter_get_heading(GnssFixFilter *self, const GnssFixData *current_fix);
guint GnssFixFilter_get_time(GnssFixFilter *self, const GnssFixData *current_fix);

gboolean GnssFixFilter_filter_by_position(GnssFixFilter *self, const GnssFixData *current_fix);
gboolean GnssFixFilter_filter_by_distance(GnssFixFilter *self, const GnssFixData *current_fix);
gboolean GnssFixFilter_filter_by_heading(GnssFixFilter *self, const GnssFixData *current_fix);
gboolean GnssFixFilter_filter_by_time(GnssFixFilter *self, const GnssFixData *current_fix);

gboolean GnssFixFilter_filter_by_all(GnssFixFilter *self, const GnssFixData *current_fix);

gboolean GnssFixFilter_filter(GnssFixFilter *self, const GnssFixData *current_fix);


#endif /* GNSS_FIX_FILTER_H_ */
