/*
 * tolling_gnss_sm_data.h
 *
 *  Created on: May 26, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#pragma once

typedef struct _TollingGnssSm TollingGnssSm;
typedef struct _ServiceActivationSm ServiceActivationSm;
typedef struct _GnssFixesTransmissionQueue GnssFixesTrasmissionQueue;
typedef struct _GnssFixFilter GnssFixFilter;
typedef struct _MessageComposer MessageComposer;
typedef struct _ConnectionSm ConnectionSm;
typedef struct _Odometer Odometer;
typedef struct _ConfigurationStore ConfigurationStore;
typedef struct _Mqtt_Client Mqtt_Client;
typedef struct _TripIdManager TripIdManager;
typedef struct _TollingManagerProxy TollingManagerProxy;
typedef struct _PositioningServiceProxy PositioningServiceProxy;
typedef struct _NetworkManagerProxy NetworkManagerProxy;
typedef struct _ApplicationEvents ApplicationEvents;
typedef struct _ApplicationNotifications ApplicationNotifications;
typedef struct _EventsLogic EventsLogic;
typedef struct _AlarmAndAlertNotificationFacade AlarmAndAlertNotificationFacade;
typedef struct _GString GString;
typedef struct _HmiProxy HmiProxy;
typedef struct _DomainSpecificData DomainSpecificData;
typedef struct _ICustomActivationChecker ICustomActivationChecker;
typedef struct _TollingGnssCoreFactoryInterface TollingGnssCoreFactoryInterface;
typedef struct _DsrcGoNogoStatus DsrcGoNogoStatus;

typedef struct _Tolling_Gnss_Sm_Data
{
    ICustomActivationChecker        *activation_checker;
	TollingGnssCoreFactoryInterface *factory;
	TollingGnssSm                   *tolling_gnss_sm;
	ServiceActivationSm             *service_activation_sm;
	ConnectionSm                    *connection_sm;
	GnssFixesTrasmissionQueue       *gnss_fixes_transmision_queue;
	GnssFixFilter                   *gnss_fix_filter;
	MessageComposer                 *message_composer;
	Odometer                        *odometer;
	ConfigurationStore              *configuration_store;
	Mqtt_Client                     *mqtt_client;
	TripIdManager                   *trip_id_manager;
	TollingManagerProxy             *tolling_manager_proxy;
	PositioningServiceProxy         *positioning_service_proxy;
	NetworkManagerProxy             *network_manager_proxy;
	ApplicationEvents               *application_events;
	ApplicationNotifications        *application_notifications;
	EventsLogic                     *events_logic;
	AlarmAndAlertNotificationFacade	*alarm_and_alert_notification_facade;
	GString                         *obu_id;
	HmiProxy                        *hmi_proxy;

	gboolean 						first_fix;
} Tolling_Gnss_Sm_Data;

Tolling_Gnss_Sm_Data *Tolling_Gnss_Sm_Data_new(const DomainSpecificData *domain_specific_data);
void Tolling_Gnss_Sm_Data_destroy(Tolling_Gnss_Sm_Data *self);
