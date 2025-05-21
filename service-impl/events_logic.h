#pragma once

#include <glib.h>


typedef struct _ApplicationNotifications ApplicationNotifications;
typedef struct _EventsLogic EventsLogic;
typedef struct _ApplicationEvents ApplicationEvents;
typedef struct _TollingManagerProxy TollingManagerProxy;

EventsLogic *EventsLogic_new(ApplicationEvents *application_events,
		TollingManagerProxy *tolling_manager_proxy,
		ApplicationNotifications *application_notifications);
void EventsLogic_delete(EventsLogic *self);

void EventsLogic_on_enter_ccc_domain(EventsLogic *self,
		guint go_nogo_flags);
void EventsLogic_on_tolling_gnss_sm_on_hold(gpointer gpointer_self);
void EventsLogic_on_tolling_gnss_sm_start(gpointer gpointer_self);
void EventsLogic_on_axles_change_requested(gpointer gpointer_self);
void EventsLogic_on_axles_change_approved(gpointer gpointer_self);
void EventsLogic_on_axles_change_rejected(gpointer gpointer_self);
void EventsLogic_deallocate(EventsLogic *self);
void EventsLogic_destroy(EventsLogic *self);
EventsLogic *EventsLogic_allocate();
void EventsLogic_on_sm_start(EventsLogic *self);
void EventsLogic_on_sm_on_hold(EventsLogic *self);


