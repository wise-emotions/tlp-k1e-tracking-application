#include <glib.h>

#include "events_manager.h"
#include "configuration.h"
#include "logger.h"
#include "hmi_proxy.h"
#include "mqtt_client.h"

#include "tolling-gnss-sm.h"
#include "configuration_store.h"
#include "odometer.h"
#include "trip_id_manager.h"
#include "gnss_fixes_transmission_queue.h"
#include "data_id_generator.h"
#include "domain_specific_data.h"

#include "tolling_manager_proxy.h"
#include "positioning_service_proxy.h"
#include "network_manager_proxy.h"
#include "icc_service_proxy.h"
#include "dsrc_service_proxy.h"
#include "application_events.h"
#include "dsrc_go_nogo_status.h"
#include "application_notifications.h"
#include "alarm_and_alert_notification_facade.h"
#include "events_logic.h"

#include "tolling_gnss_sm_data.h"
#include "tolling_gnss_core_default_factory.h"

Tolling_Gnss_Sm_Data *Tolling_Gnss_Sm_Data_new(const DomainSpecificData *domain_specific_data)
{
	Tolling_Gnss_Sm_Data *self = g_try_new0(Tolling_Gnss_Sm_Data, 1);
	g_return_val_if_fail(self != NULL, NULL);

	self->factory = domain_specific_data->factory;
	if (self->factory == NULL)
	{
		self->factory = TollingGnssCoreDefaultFactory_new();
		if (self->factory == NULL) g_free(self);
		g_return_val_if_fail(self->factory != NULL, NULL);
	}

	/* hmi-manager proxy initialization moved at top to be ready to signal
	 * any anomaly that may rise in the following
	*/
	self->hmi_proxy = hmi_proxy_create(self);

	self->mqtt_client = mqtt_client_new();

	char* par1;
	ParameterTypes ptype;
	ptype = get_parameter(MQTT_HOST_PAR, (const void**)&par1);
	char* host = (ptype != pt_invalid) ? par1 : (char*)"";
	float* par2;
	ptype = get_parameter(MQTT_PORT_PAR, (const void**)&par2);
	int port = (ptype != pt_invalid) ? (int)(*par2) : 0;
	ptype = get_parameter(MQTT_USER_PAR, (const void**)&par1);
	char* user = (ptype != pt_invalid) ? par1 : NULL;
	ptype = get_parameter(MQTT_PWD_PAR, (const void**)&par1);
	char* pwd = (ptype != pt_invalid) ? par1 : NULL;
	enum mqtt_states mqtt_state = mqtt_client_init(
		self->mqtt_client,
		user,
		pwd,
		host,
		port,
		self);
	if (mqtt_state != mqtt_ok)
	{
		logerr("mqtt_client_init failed with return code %d", mqtt_state);
	}




	self->configuration_store = ConfigurationStore_new();
	self->application_events = ApplicationEvents_new();
    self->dsrc_service_proxy = DsrcServiceProxy_new(
        self->configuration_store,
        self->application_events);
	self->positioning_service_proxy = PositioningServiceProxy_new(
		self->configuration_store,
		self->application_events);
	self->network_manager_proxy = NetworkManagerProxy_new(
		self->configuration_store,
		self->application_events);
	self->icc_service_proxy = IccServiceProxy_new(
		self->configuration_store,
		self->application_events);
    self->tolling_manager_proxy = self->factory->create_tolling_manager_proxy(self->factory,
        domain_specific_data->gnss_domain_name,
        domain_specific_data->service_activation_domain_id,
        self->configuration_store,
        self->application_events,
        self);

	self->gnss_fixes_transmision_queue = GnssFixesTransmissionQueue_new();
	self->gnss_fixes_transmision_queue->ctor(self->gnss_fixes_transmision_queue, self,domain_specific_data->gnss_domain_name); //NOTE: destructor called in MessageComposet_destroy()
	self->gnss_fixes_transmision_queue->delayed_activation(self->gnss_fixes_transmision_queue);

//	self->events_logic = self->factory->create_events_logic(self->factory,
//		self->application_events,
//		self->tolling_manager_proxy);
	self->dsrc_go_nogo_status = DsrcGoNogoStatus_new(self->application_events, self->dsrc_service_proxy);
	self->alarm_and_alert_notification_facade = AlarmAndAlertNotificationFacade_new(self->mqtt_client);
	self->application_notifications = ApplicationNotifications_new(self->dsrc_go_nogo_status, self->alarm_and_alert_notification_facade);
	self->events_logic = self->factory->create_events_logic(self->factory,
		self->application_events,
		self->tolling_manager_proxy,
		self->application_notifications);

	self->trip_id_manager = trip_id_manager_create();
	self->data_id_generator = DataIdGenerator_new(self->configuration_store);
	self->axles_change_manager = NULL;
	self->odometer = odometer_new();
	self->tolling_gnss_sm = tolling_gnss_sm_new(self);
	self->obu_id = g_string_new("");

	self->first_fix = FALSE;

	return self;
}

void Tolling_Gnss_Sm_Data_destroy(Tolling_Gnss_Sm_Data *self)
{
	if (self != NULL)
	{
		TollingManagerProxy_delete(self->tolling_manager_proxy);
		PositioningServiceProxy_destroy(self->positioning_service_proxy);
		NetworkManagerProxy_destroy(self->network_manager_proxy);
		IccServiceProxy_destroy(self->icc_service_proxy);
		DsrcServiceProxy_destroy(self->dsrc_service_proxy);
		ConfigurationStore_destroy(self->configuration_store);
		ApplicationEvents_destroy(self->application_events);
		ApplicationNotifications_destroy(self->application_notifications);
		DsrcGoNogoStatus_destroy(self->dsrc_go_nogo_status);
		AlarmAndAlertNotificationFacade_destroy(self->alarm_and_alert_notification_facade);
		EventsLogic_delete(self->events_logic);
		odometer_destroy(self->odometer);
		trip_id_manager_destroy(self->trip_id_manager);
		DataIdGenerator_destroy(self->data_id_generator);
		mqtt_client_deinit(self->mqtt_client);
		mqtt_client_destroy(self->mqtt_client);
		tolling_gnss_sm_destroy(self->tolling_gnss_sm);
		g_string_free(self->obu_id, TRUE);
		hmi_proxy_destroy(self->hmi_proxy);

		// Note: GnssFixesTransmissionQueue destructor called in MessageComposer_destroy()
		// due to a problematic destruction sequence in tolling applications

		g_free(self);
	}	
}
