/*
 * Author: Fabio Turati (fabio.turati@nttdata.com)
 */
#include <glib.h>
#include <string.h>

#include "events_manager.h"
#include "configuration.h"
#include "logger.h"
#include "mqtt_client.h"
#include "data_store.h"

#include "tolling-gnss-sm.h"
#include "tolling_gnss_sm_data.h"
#include "domain_specific_data.h"
#include "tolling_manager_proxy.h"

#include "application_message_composer.h"
#include "application_gnss_fix_filter.h"

#include "vas_activation_checker.h"


#define GNSS_DOMAIN_NAME "TRACKING"
#define GNSS_SERVICE_ACTIVATION_DOMAIN_ID 101
//#define OTHER_SERVICE_ON_SHARED_DOMAIN 6

// Polonia ETOLL: C_SER=73
// Polonia EETS:  C_SER=67

typedef struct _Service {
	Tolling_Gnss_Sm_Data *c_data;
	GMainLoop *app_loop;
	EventsManager_listener_t obu_available_listener_id;
} Service;


void get_obu_id(Service* self) {
	//retrieve obu_id from data_store
	guchar* obu_id = NULL;
	gint get_ret = data_store_get_text_data((guchar*)OBU_ID_DATA_STORE_KEY, &obu_id);
	if (get_ret != DATA_STORE_NO_ERROR)
	{
		logerr("OBU ID get from data store failed");
		return;
	} else {
		g_string_assign(self->c_data->obu_id, (char*)obu_id);
		g_free(obu_id);
	}

}

static void obu_id_available_callback(Service* self)
{
	/*
	// This block *must* be before Tolling_Gnss_Sm_Data_new
	other_services_on_this_domain = g_array_new(FALSE, FALSE, sizeof(gint));
	gint other_service_on_shared_domain = OTHER_SERVICE_ON_SHARED_DOMAIN;
	g_array_append_val(other_services_on_this_domain, other_service_on_shared_domain);
	*/
    VasActivationChecker *activation_checker = VasActivationChecker_new();
    activation_checker->ctor(activation_checker, GNSS_SERVICE_ACTIVATION_DOMAIN_ID);

	DomainSpecificData *domain_specific_data = g_malloc0(sizeof(DomainSpecificData));
	domain_specific_data->gnss_domain_name             = GNSS_DOMAIN_NAME;
	domain_specific_data->service_activation_domain_id = GNSS_SERVICE_ACTIVATION_DOMAIN_ID;
	domain_specific_data->activation_checker           = (ICustomActivationChecker *)activation_checker;
	self->c_data = Tolling_Gnss_Sm_Data_new(domain_specific_data);
	g_free(domain_specific_data);
	self->c_data->message_composer = ApplicationMessageComposer_new(self->c_data);
	self->c_data->gnss_fix_filter = ApplicationGnssFixFilter_new(self->c_data);

	logdbg("OBU ID AVAILABLE: init mqtt client and subscribe topic");

	//retrieve obu_id from data_store
	get_obu_id(self);

	events_manager_listener_remove(self->obu_available_listener_id);
	self->obu_available_listener_id = 0;

	tolling_gnss_sm_run(self->c_data->tolling_gnss_sm);
	return;
}

void service_destroy(Service* self)
{
	logdbg("");
	if (self != NULL) 
	{
		if (self->obu_available_listener_id != 0)
		{
			events_manager_listener_remove(self->obu_available_listener_id);
		}
		if (self->c_data != NULL)
		{
			ApplicationMessageComposer_destroy(self->c_data->message_composer);
			ApplicationGnssFixFilter_destroy(self->c_data->gnss_fix_filter);
			Tolling_Gnss_Sm_Data_destroy(self->c_data);
		}		
		g_free(self);
	}
}

int service_deinit(Service* self)
{
	logdbg("");
	if(self->c_data != NULL){
		tolling_gnss_sm_stop(self->c_data->tolling_gnss_sm);
	}
	return 0;
}

int service_init(Service* self, GMainLoop * loop)
{
	logdbg("");
	self->app_loop = loop;
	self->obu_available_listener_id = events_manager_listen_obu_id_available(
		(EventsManager_obu_id_available_cb_t)obu_id_available_callback,
		(gpointer)self);
	logdbg(" obu id available listener identifier = %d", self->obu_available_listener_id);



	return 0;
}

Service* service_new()
{
	logdbg("");
	Service *self = g_try_new0(Service, 1);
	self->app_loop = NULL;
	self->obu_available_listener_id = 0;

	return self;
}

int service_on_hold(Service* self)
{
	logdbg("");
	if(self->c_data != NULL){
		tolling_gnss_sm_on_hold(self->c_data->tolling_gnss_sm);

	}

	return 0;
}

int service_resume(Service* self)
{
	logdbg("");
	self->c_data->first_fix = FALSE;
	if(self->c_data != NULL)
		tolling_gnss_sm_run(self->c_data->tolling_gnss_sm);

	return 0;
}
