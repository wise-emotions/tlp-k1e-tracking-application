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

	add_parameter(configuration_persistence_file);
	add_parameter(mqtt_tolling_pl_topic);
	add_parameter(mqtt_tolling_pl_axles_configuration_topic);


}
