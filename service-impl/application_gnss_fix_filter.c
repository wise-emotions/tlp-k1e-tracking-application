/*
 * application_gnss_fix_filter.c
 *
 *  Created on: Jan 28, 2022
 *  Author: Gianni Manzo (gianni.manzo@nttdata.com)
 */

#include "application_gnss_fix_filter.h"


GnssFixFilter* ApplicationGnssFixFilter_new(Tolling_Gnss_Sm_Data* data)
{
	GnssFixFilter *self = GnssFixFilter_new(data);
	return self;
}

void ApplicationGnssFixFilter_destroy(GnssFixFilter *self)
{
	GnssFixFilter_destroy(self);
}

gboolean GnssFixFilter_filter(GnssFixFilter *self, const GnssFixData *current_fix)
{
	return GnssFixFilter_filter_by_time(self, current_fix);
}
