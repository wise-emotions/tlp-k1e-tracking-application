/*
 * service_configuration.c
 *
 * Created on: Mar 19, 2020
 * Author: Gianni Manzo (gianni.manzo@nttdata.com)
 */

#include "service_configuration.h"

void add_custom_parameters()
{
	/*
	default_val ndv;
	ndv.nval = 0;
	*/

	default_val sdv;
	sdv.sval = "";



	Parameter* configuration_persistence_file = parameter_new(
			CONFIGURATION_PERSISTENCE_PAR, "", "--configuration_persistence_file", pt_string,
			CONFIGURATION_PERSISTENCE_RANGE, CONFIGURATION_PERSISTENCE_DESCR, sdv);

	Parameter* mqtt_tolling_pl_topic = parameter_new(
			MQTT_TRACKING_TOPIC_PAR, "", "", pt_string,
			MQTT_TRACKING_TOPIC_PAR_RANGE, MQTT_TRACKING_TOPIC_PAR_DESCR, sdv);

	Parameter* mqtt_tolling_pl_axles_configuration_topic = parameter_new(
			MQTT_EVENTS_TOPIC_PAR, "", "", pt_string,
			MQTT_EVENTS_TOPIC_PAR_RANGE, MQTT_EVENTS_TOPIC_PAR_DESCR, sdv);

	Parameter* tracking_event_wait_confirmation_timeout = parameter_new(
	        TRACKING_EVENTS_WAIT_CONFIRMATION_TIMEOUT_PAR,
	        "",
	        "",
	        pt_number,
	        TRACKING_EVENTS_WAIT_CONFIRMATION_TIMEOUT_PAR_RANGE,
	        TRACKING_EVENTS_WAIT_CONFIRMATION_TIMEOUT_PAR_DESCR,
	        TRACKING_EVENTS_WAIT_CONFIRMATION_TIMEOUT_PAR_DFLT
	);

	add_parameter(configuration_persistence_file);
	add_parameter(mqtt_tolling_pl_topic);
	add_parameter(mqtt_tolling_pl_axles_configuration_topic);
    add_parameter(tracking_event_wait_confirmation_timeout);


}
gint iparam(const gchar *name)
{
float *par = NULL;
    get_parameter(name, (const void **)&par);
    return (par == NULL)? 0: (gint)*par;
}


gint fparam(const gchar *name)
{
float *par = NULL;
    get_parameter(name, (const void **)&par);
    return (par == NULL)? 0.0: *par;
}


gboolean bparam(const gchar *name)
{
gboolean *par = NULL;
    get_parameter(name, (const void **)&par);
    return (par == NULL)? FALSE: (gboolean)*par;
}


const gchar *sparam(const gchar *name)
{
const gchar *par = NULL;
    get_parameter(name, (const void **)&par);
    return (const gchar *)par;
}

const gchar **svparam(const gchar *name) {
const gchar **par = NULL;
    get_parameter(name, (const void **)&par);
    return par;
}

