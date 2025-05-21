#pragma once

#include "events_logic.h"

#include "application_notifications.h"

#include <glib.h>

struct _EventsLogic {
	ApplicationEvents *application_events;
	ApplicationNotifications *application_notifications;
	TollingManagerProxy *tolling_manager_proxy;
	gboolean is_gnss_anomaly_standing;
	gboolean is_network_anomaly_standing;
	gboolean is_network_connected;
	gboolean is_service_configured;
	gboolean is_vehicle_configured;
	gboolean is_service_active;
	gboolean is_another_service_on_shared_domain_active;
	gboolean is_inside_gnss_domain;
	gboolean is_inside_ccc_domain;
	gboolean is_axles_config_approved;
	gboolean on_hold;
	guint gnss_anomaly_timer;
	guint network_anomaly_timer;

	// "virtual" methods begin
	void (*dtor)(EventsLogic *self);

	void (*check_service_status)(EventsLogic *self);

	void (*on_service_activated_from_remote)(gpointer gpointer_self);

	void (*on_service_deactivated_from_remote)(gpointer gpointer_self);

	void (*on_exit_gnss_domain)(gpointer gpointer_self);

    void (*on_exit_ccc_domain)(gpointer gpointer_self);
	// "virtual" methods end
};

// methods accessed from derived classes begin
gboolean EventsLogic_get_service_activation(EventsLogic *self);
void EventsLogic_check_service_status(EventsLogic *self);
void EventsLogic_update_service_status(EventsLogic *self);
void EventsLogic_on_exit_gnss_domain(gpointer gpointer_self);
void EventsLogic_on_exit_ccc_domain(gpointer gpointer_self);



EventsLogic *EventsLogic_initialize(EventsLogic *self,
		ApplicationEvents *application_events,
		TollingManagerProxy *tolling_manager_proxy);
void EventsLogic_deinitialize(EventsLogic *self);
// methods accessed from derived classes end
