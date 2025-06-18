#include "tolling_gnss_core_default_factory.h"

#include "logger.h"

#include "events_logic.h"
#include "tolling_manager_proxy.h"


static void                             TollingGnssCoreDefaultFactory_dtor(TollingGnssCoreFactoryInterface *);

static EventsLogic                     *TollingGnssCoreDefaultFactory_create_events_logic(
                                            TollingGnssCoreFactoryInterface *,
                                            ApplicationEvents *,
                                            TollingManagerProxy *,
                                    		ApplicationNotifications*
                                        );

static TollingManagerProxy             *TollingGnssCoreDefaultFactory_create_tolling_manager_proxy(
                                            TollingGnssCoreFactoryInterface *,
                                            const gchar *,
                                            const guint,
                                            ConfigurationStore *,
                                            ApplicationEvents *,
                                            Tolling_Gnss_Sm_Data *
                                        );


TollingGnssCoreFactoryInterface *
TollingGnssCoreDefaultFactory_new()
{
	TollingGnssCoreFactoryInterface *obj;

	obj = g_try_new0(TollingGnssCoreFactoryInterface, 1);
	g_return_val_if_fail(obj != NULL, NULL);

	obj->dtor                           = TollingGnssCoreDefaultFactory_dtor;
	obj->create_events_logic            = TollingGnssCoreDefaultFactory_create_events_logic;
	obj->create_tolling_manager_proxy   = TollingGnssCoreDefaultFactory_create_tolling_manager_proxy;

	return obj;
}


static void
TollingGnssCoreDefaultFactory_dtor(
		TollingGnssCoreFactoryInterface     *self
)
{
	g_return_if_fail(self != NULL);

	g_free(self);
}


static EventsLogic *
TollingGnssCoreDefaultFactory_create_events_logic(
		TollingGnssCoreFactoryInterface     *self,
		ApplicationEvents                   *application_events,
		TollingManagerProxy                 *tolling_manager_proxy,
		ApplicationNotifications 			*application_notifications
)
{
	g_return_val_if_fail(self != NULL, NULL);

	return EventsLogic_new(application_events,tolling_manager_proxy,
			application_notifications);
}

static TollingManagerProxy *
TollingGnssCoreDefaultFactory_create_tolling_manager_proxy(
		TollingGnssCoreFactoryInterface     *self,
		const gchar                         *gnss_domain_name,
		const guint                         service_activation_domain_id,
		ConfigurationStore                  *configuration_store,
		ApplicationEvents                   *application_events,
		Tolling_Gnss_Sm_Data                *data
)
{
	g_return_val_if_fail(self != NULL, NULL);

	return TollingManagerProxy_new(gnss_domain_name,
			service_activation_domain_id,
			configuration_store,
			application_events,
			data);
}
