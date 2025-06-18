#pragma once

#include <glib.h>

typedef struct _TollingGnssCoreFactoryInterface TollingGnssCoreFactoryInterface;

typedef struct _ApplicationEvents ApplicationEvents;
typedef struct _ApplicationNotifications ApplicationNotifications;
typedef struct _ConfigurationStore ConfigurationStore;
typedef struct _EventsLogic EventsLogic;
typedef struct _Tolling_Gnss_Sm_Data Tolling_Gnss_Sm_Data;
typedef struct _TollingManagerProxy TollingManagerProxy;

struct _TollingGnssCoreFactoryInterface {
	void
	(*dtor)(
			TollingGnssCoreFactoryInterface     *self
	);

	EventsLogic *
	(*create_events_logic)(
			TollingGnssCoreFactoryInterface     *self,
			ApplicationEvents                   *application_events,
			TollingManagerProxy                 *tolling_manager_proxy,
			ApplicationNotifications 			*application_notifications
	);

	TollingManagerProxy *
	(*create_tolling_manager_proxy)(
			TollingGnssCoreFactoryInterface     *self,
			const gchar                         *gnss_domain_name,
			const guint                         service_activation_domain_id,
			ConfigurationStore                  *configuration_store,
			ApplicationEvents                   *application_events,
			Tolling_Gnss_Sm_Data                *data
	);
};
