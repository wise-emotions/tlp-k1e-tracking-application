/*
 * application_gnss_fix_filter.h
 *
 *  Created on: Jan 28, 2022
 *  Author: Gianni Manzo (gianni.manzo@nttdata.com)
 */

#ifndef APPLICATION_GNSS_FIX_FILTER_H_
#define APPLICATION_GNSS_FIX_FILTER_H_


#include "gnss_fix_filter.h"


GnssFixFilter* ApplicationGnssFixFilter_new(Tolling_Gnss_Sm_Data* data);
void ApplicationGnssFixFilter_destroy(GnssFixFilter *self);


#endif /* APPLICATION_GNSS_FIX_FILTER_H_ */
