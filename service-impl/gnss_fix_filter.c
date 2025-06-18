/*
 * gnss_fix_filter.c
 *
 *  Created on: Jan 28, 2022
 *  Author: Gianni Manzo (gianni.manzo@nttdata.com)
 */

#include "gnss_fix_filter.h"

#include "logger.h"

#include "gnss_fix_data.h"
#include "tolling_manager_proxy.h"
#include "tolling_gnss_sm_data.h"


GnssFixFilter* GnssFixFilter_new(Tolling_Gnss_Sm_Data* data)
{
	GnssFixFilter *self = g_new0(GnssFixFilter, 1);
	self->tolling_gnss_sm_data = data;

	return self;
}

void GnssFixFilter_destroy(GnssFixFilter *self)
{
	g_free(self);
}

void GnssFixFilter_reset_last_fix(GnssFixFilter *self)
{
	self->previous_fix_present = FALSE;
	self->timestamp = 0;
	self->latitude = 0;
	self->longitude = 0;
	self->altitude = 0;
	self->heading = 0;
	self->distance = 0;
}

void GnssFixFilter_set_last_fix(GnssFixFilter *self, const GnssFixData *current_fix)
{
	self->previous_fix_present = TRUE;
	self->timestamp = gnss_fix_data_get_timestamp_in_milliseconds(current_fix);
	self->latitude = gnss_fix_data_get_latitude(current_fix);
	self->longitude = gnss_fix_data_get_longitude(current_fix);
	self->altitude = gnss_fix_data_get_altitude(current_fix);
	self->heading = gnss_fix_data_get_heading(current_fix);
	self->distance = gnss_fix_data_get_odometer(current_fix);
}

gboolean GnssFixFilter_get_filter_logic(GnssFixFilter *self, guint *value)
{
	gboolean result = TollingManagerProxy_get_filter_logic(self->tolling_gnss_sm_data->tolling_manager_proxy, value);

	logdbg("result = %d, value = %u", result, *value);

	return result;
}

gboolean GnssFixFilter_get_filter_distance(GnssFixFilter *self, gdouble *value)
{
	gboolean result = TollingManagerProxy_get_filter_distance(self->tolling_gnss_sm_data->tolling_manager_proxy, value);

	logdbg("result = %d, value = %f", result, *value);

	return result;
}

gboolean GnssFixFilter_get_filter_heading(GnssFixFilter *self, gdouble *value)
{
	gboolean result = TollingManagerProxy_get_filter_heading(self->tolling_gnss_sm_data->tolling_manager_proxy, value);

	logdbg("result = %d, value = %f", result, *value);

	return result;
}

gboolean GnssFixFilter_get_filter_time(GnssFixFilter *self, guint *value)
{
	gboolean result = TollingManagerProxy_get_filter_time(self->tolling_gnss_sm_data->tolling_manager_proxy, value);

	logdbg("result = %d, value = %u", result, *value);

	return result;
}


gdouble GnssFixFilter_get_distance(GnssFixFilter *self, const GnssFixData *current_fix) {
	gdouble delta_distance = gnss_fix_data_get_odometer(current_fix) - self->distance;

	logdbg("trip distance of current fix = %f; trip distance of last added fix = %f",
			gnss_fix_data_get_odometer(current_fix), self->distance);
	logdbg("delta_distance: %f", delta_distance);

	return delta_distance;
}

gdouble calculate_heading_difference(const gdouble new_heading, const gdouble old_heading)
{
	gdouble difference = new_heading - old_heading;

	// The absolute value ensures that right-turns and left-turns are treated the same way
	gdouble absolute_value = (difference > 0.0) ? difference : -difference;

	// If the heading changes from 1° to 359°, instead of treating it as a difference of 358°, we treat it as 2°.
	gdouble difference_capped_at_180 = (absolute_value <= 180.0) ? absolute_value : 360.0 - absolute_value;

	return difference_capped_at_180;
}

gdouble GnssFixFilter_get_heading(GnssFixFilter *self, const GnssFixData *current_fix) {

	gdouble new_heading = gnss_fix_data_get_heading(current_fix);
	gdouble old_heading = self->heading;
	gdouble angle = calculate_heading_difference(new_heading, old_heading);

	logdbg("heading of current fix = %f; heading of last added fix = %f",
			new_heading, old_heading);
	logdbg("angle: %f", angle);

	return angle;
}

guint GnssFixFilter_get_time(GnssFixFilter *self, const GnssFixData *current_fix) {
	gint64 delta_time = gnss_fix_data_get_timestamp_in_milliseconds(current_fix) - self->timestamp;

	guint64 elapsed_time = ABS(delta_time);

	logdbg("timestamp of current fix = %" G_GUINT64_FORMAT "; timestamp of last added fix = %" G_GUINT64_FORMAT,
			gnss_fix_data_get_timestamp_in_milliseconds(current_fix), self->timestamp);
	logdbg("elapsed time: %" G_GUINT64_FORMAT, elapsed_time);

	return elapsed_time / 1000;
}


gboolean GnssFixFilter_filter_by_position(GnssFixFilter *self, const GnssFixData *current_fix)
{
	if (self->previous_fix_present == FALSE)
		return TRUE;

	gboolean result = self->latitude != gnss_fix_data_get_latitude(current_fix)
			|| self->longitude != gnss_fix_data_get_longitude(current_fix)
			|| self->altitude != gnss_fix_data_get_altitude(current_fix);

	logdbg("result = %d", result);

	return result;
}

gboolean GnssFixFilter_filter_by_distance(GnssFixFilter *self, const GnssFixData *current_fix)
{
	if (self->previous_fix_present == FALSE)
		return TRUE;

	gdouble filter_distance = 0.0;

	gboolean result = (GnssFixFilter_get_filter_distance(self, &filter_distance) && GnssFixFilter_get_distance(self, current_fix) >= filter_distance);

	logdbg("result = %d", result);

	return result;
}

gboolean GnssFixFilter_filter_by_heading(GnssFixFilter *self, const GnssFixData *current_fix)
{
	if (self->previous_fix_present == FALSE)
		return TRUE;

	gdouble filter_heading = 0.0;

	gboolean result = (GnssFixFilter_get_filter_heading(self, &filter_heading) && GnssFixFilter_get_heading(self, current_fix) >= filter_heading);

	logdbg("result = %d", result);

	return result;
}

gboolean GnssFixFilter_filter_by_time(GnssFixFilter *self, const GnssFixData *current_fix)
{
	if (self->previous_fix_present == FALSE)
		return TRUE;

	guint filter_time = 0;

	gboolean result = (GnssFixFilter_get_filter_time(self, &filter_time) && GnssFixFilter_get_time(self, current_fix) >= filter_time);

	logdbg("result = %d", result);

	return result;
}

gboolean GnssFixFilter_filter_by_all(GnssFixFilter *self, const GnssFixData *current_fix)
{
	if (self->previous_fix_present == FALSE)
		return TRUE;

	gboolean result = GnssFixFilter_filter_by_position(self, current_fix) && (
		GnssFixFilter_filter_by_time(self, current_fix) ||
		GnssFixFilter_filter_by_heading(self, current_fix) ||
		GnssFixFilter_filter_by_distance(self, current_fix));

	logdbg("result = %d", result);

	return result;
}
