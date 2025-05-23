/*
 * service_monitor.c
 *
 *  Created on: 18 Oct 2022
 *      Author: balde
 */

#include "math.h"
#include "glib.h"
#include "logger.h"
#include "utils.h"
#include "service_monitor.h"
#include "positioning_service_types.h"

#if !defined(SERVICE_MONITOR_REPORT_INTERVAL)
    #define SERVICE_MONITOR_REPORT_INTERVAL 1000
#endif


/* monitor status
*/
static struct {
    guint64  stime;
    gulong   timer;

    gulong   fix_interval; /* usec */

    gdouble   odometer;
    gboolean  latest_nogo;
    guint64   latest_mqtt_transmission_epoch_ms;

}
gnss_activity;

/* timed report callback
*/
static gboolean report(__attribute__((unused)) gpointer);

void service_monitor_set_fix_interval(gulong fix_interval){
	gboolean was_started  = FALSE;

	if(gnss_activity.timer > 0)
		was_started = TRUE;

	service_monitor_stop_tracking();

	logdbg("updating fix interval -> %d msec for service_monitor %s started",(gint)fix_interval,was_started==FALSE?"not":"already");

	gnss_activity.fix_interval   = fix_interval * 1000UL; /* usec */

	service_monitor_start_tracking();

}


void service_monitor_init(gulong fix_interval) {
    service_monitor_stop_tracking();
    gnss_activity.fix_interval   = fix_interval * 1000UL; /* usec */
}


void service_monitor_start_tracking(void) {
	  if (gnss_activity.timer ==  0UL){

		gnss_activity.stime         			           = g_get_real_time();
		gnss_activity.latest_nogo  				           = FALSE;
		gnss_activity.latest_mqtt_transmission_epoch_ms    = 0;
		gnss_activity.odometer       			  		   = 0.0;

		gnss_activity.timer = g_timeout_add_seconds(SERVICE_MONITOR_REPORT_INTERVAL, report, NULL);

	  }else
		  logdbg("service monitor already tracking");

}


void service_monitor_stop_tracking(void) {
    if (gnss_activity.timer > 0)
        g_source_remove(gnss_activity.timer);

    gnss_activity.timer          					   = 0UL;
    gnss_activity.odometer                             = 0UL;
    gnss_activity.latest_mqtt_transmission_epoch_ms    = 0UL;
    gnss_activity.latest_nogo       				   = FALSE;

}


void service_monitor_update_metrics(const GnssFixData * const latest_fix) {

	if (gnss_activity.timer > 0) {

	   gnss_activity.odometer = latest_fix->total_distance_m;
	   gnss_activity.latest_mqtt_transmission_epoch_ms = gnss_fix_data_get_timestamp_in_milliseconds(latest_fix);
	   gnss_activity.latest_nogo = 1;
   }
}


static inline gulong ulmax(gulong a, gulong b)
    { return a > b ? a : b; }

static gboolean report(gpointer unused)
{

	GString* to_report = date_time_unix_utc_to_iso8601_ms(gnss_activity.latest_mqtt_transmission_epoch_ms);

	loginfo("service_monitoring: nogo=%s, odometer=%f m, latest_fix_time= %s",
			gnss_activity.latest_nogo==TRUE?"true":"false",
			gnss_activity.odometer,(to_report->str)
		);


    return G_SOURCE_CONTINUE;
}
