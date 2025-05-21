/*  gnss_monitor.h
 */
#ifndef __GNSS_MONITOR_H__
#define __GNSS_MONITOR_H__

#include <glib-object.h>

#include "gnss_fix_data.h"

typedef struct _PositionData PositionData;

void service_monitor_set_fix_interval(gulong fix_interval/*msec*/);
void service_monitor_init(gulong fix_interval /*msec*/);
void service_monitor_start_tracking(void);
void service_monitor_stop_tracking(void);
void service_monitor_update_metrics(const GnssFixData * const latest_fix);

#endif /*__GNSS_MONITOR_H__*/
