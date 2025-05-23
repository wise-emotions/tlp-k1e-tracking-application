#include "events_logic_protected.h"

#include "events_manager.h"
#include "configuration.h"
#include "logger.h"
#include "shared_types.h"

#include "application_events.h"
#include "application_events_common_names.h"
#include "tolling_manager_proxy.h"

#define DEFAULT_GNSS_ANOMALY_TIMEOUT_MINUTES 20
#define DEFAULT_NETWORK_ANOMALY_TIMEOUT_MINUTES 30

typedef enum
{
	active,
	not_active,
	no_go,
}
activation_status_t;

static char *activation_status_str[] =
{
	"active",
	"not-active",
	"no-go"
};

static gboolean inside_gnss_domain(EventsLogic *self)
{
	return self->is_inside_gnss_domain;
}

static gboolean inside_ccc_domain(EventsLogic *self)
{
	return self->is_inside_ccc_domain;
}

static gboolean tolling_service_active(EventsLogic *self)
{
	return self->is_service_active;
}

static gboolean any_service_active(EventsLogic *self)
{
	return self->is_service_active || self->is_another_service_on_shared_domain_active;
}

static gboolean no_service_active(EventsLogic *self)
{
	return !any_service_active(self);
}

static gboolean properly_configured(EventsLogic *self)
{
	return self->is_service_configured && self->is_vehicle_configured && self->is_axles_config_approved;
}

static gboolean EventsLogic_service_active_inside_its_ccc_domain(EventsLogic *self) {
	if (inside_ccc_domain(self) && tolling_service_active(self) && properly_configured(self))
	{
		return TRUE;
	}
	return FALSE;
}

static activation_status_t EventsLogic_service_activation_status(EventsLogic *self)
{
	if (inside_gnss_domain(self) && tolling_service_active(self) && properly_configured(self))
	{
		return active;
	}

	// (no_service_active(self) || !properly_configured(self)) == !(tolling_service_active(self) && properly_configured(self))

	else if (inside_ccc_domain(self) && (no_service_active(self) || !properly_configured(self)))
	{
		return no_go;
	}
	else
	{
		return not_active;
	}
}

gboolean EventsLogic_get_service_activation(EventsLogic *self){

	return self->is_service_active;

}


void EventsLogic_check_service_status(EventsLogic *self)
{
	loginfo(
		"is_inside_gnss_domain = %d, is_inside_ccc_domain = %d, "
		"is_service_active = %d, is_another_service_on_shared_domain_active = %d, "
		"is_service_configured = %d, is_vehicle_configured  = %d, is_axles_config_approved = %d",
		self->is_inside_gnss_domain,
		self->is_inside_ccc_domain,
		self->is_service_active,
		self->is_another_service_on_shared_domain_active,
		self->is_service_configured,
		self->is_vehicle_configured,
		self->is_axles_config_approved);
	return;
}

void EventsLogic_update_service_status(EventsLogic *self)
{
	const gchar * event_name;
	activation_status_t status;
	self->check_service_status(self);
	status = EventsLogic_service_activation_status(self);
	logdbg("service activation status: %s", activation_status_str[status]);
	switch(status)
	{
		case active:
		{
			event_name = EVENT_EVENTS_LOGIC_SERVICE_STATUS_ACTIVE;
			break;
		}
		case not_active:
		{
			event_name = EVENT_EVENTS_LOGIC_SERVICE_STATUS_NOT_ACTIVE;
			break;
		}
		case no_go:
		{
			event_name = EVENT_EVENTS_LOGIC_SERVICE_STATUS_NO_GO;
			break;
		}
		default:
		{
			event_name = "";
			break;
		}
	}
	ApplicationEvents_emit_event(self->application_events, event_name);
	ApplicationEvents_emit_event_ex(self->application_events, EVENT_GNSS_FIXES_CUT_OFF, !tolling_service_active(self));
	return;
}

static void EventsLogic_emit_generic_anomaly_event(EventsLogic *self)
{
	if (self->is_network_anomaly_standing || self->is_gnss_anomaly_standing)
	{
		ApplicationEvents_emit_event(self->application_events, EVENT_EVENTS_LOGIC_GENERIC_ANOMALY_PRESENT);
	}
	else
	{
		ApplicationEvents_emit_event(self->application_events, EVENT_EVENTS_LOGIC_GENERIC_ANOMALY_ABSENT);
	}
	return;
}

static void EventsLogic_emit_network_anomaly_event(EventsLogic *self)
{
	// Workaround to cope with the single Anomaly state: test being Active as a condition, not being in the SM Active State
	if (self->is_network_anomaly_standing)
	{
		ApplicationEvents_emit_event(self->application_events, EVENT_EVENTS_LOGIC_NETWORK_ANOMALY_PRESENT);
	}
	else
	{
		ApplicationEvents_emit_event(self->application_events, EVENT_EVENTS_LOGIC_NETWORK_ANOMALY_ABSENT);
	}
	return;
}

static void EventsLogic_emit_gnss_anomaly_event(EventsLogic *self)
{
	// Workaround to cope with the single Anomaly state: test being Active as a condition, not being in the SM Active State
	if (self->is_gnss_anomaly_standing)
	{
		ApplicationEvents_emit_event(self->application_events, EVENT_EVENTS_LOGIC_GNSS_ANOMALY_PRESENT);
	}
	else
	{
		ApplicationEvents_emit_event(self->application_events, EVENT_EVENTS_LOGIC_GNSS_ANOMALY_ABSENT);
	}
	return;
}


static gboolean EventsLogic_gnss_unavailable_for_too_long(gpointer gpointer_self)
{
	EventsLogic *self = (EventsLogic *)gpointer_self;

	activation_status_t status = EventsLogic_service_activation_status(self);
	if (status == active || status == no_go) {
		logwarn("gnss_unavailable_for_too_long");
		self->is_gnss_anomaly_standing = TRUE;
		EventsLogic_emit_generic_anomaly_event(self);
		EventsLogic_emit_gnss_anomaly_event(self);
	}
	else
	{
		logdbg("gnss_unavailable_for_too_long, ignored because the status is %s", activation_status_str[status]);
	}
	self->gnss_anomaly_timer = 0;
	return G_SOURCE_REMOVE;
}

static void EventsLogic_start_gnss_anomaly_timer(EventsLogic *self)
{
    if (self->on_hold) return;

	guint gnss_anomaly_timeout = DEFAULT_GNSS_ANOMALY_TIMEOUT_MINUTES;
	TollingManagerProxy_get_gps_anomaly_timeout(self->tolling_manager_proxy, &gnss_anomaly_timeout);
	logdbg("start gnss anomaly timer");
	self->gnss_anomaly_timer = g_timeout_add_seconds(
		gnss_anomaly_timeout * 60,
		EventsLogic_gnss_unavailable_for_too_long,
		self);
	return;
}

static void EventsLogic_stop_gnss_anomaly_timer(EventsLogic *self)
{
	if (self->gnss_anomaly_timer)
	{
		logdbg("stop gnss anomaly timer");
		g_source_remove(self->gnss_anomaly_timer);
		self->gnss_anomaly_timer = 0;
	}
	return;
}

static void EventsLogic_clear_gnss_anomaly(EventsLogic *self)
{
	if (self->is_gnss_anomaly_standing)
	{
		self->is_gnss_anomaly_standing = FALSE;
		EventsLogic_emit_generic_anomaly_event(self);
		EventsLogic_emit_gnss_anomaly_event(self);
	}
	return;
}

static void EventsLogic_reset_gnss_anomaly_timer(EventsLogic *self)
{
	EventsLogic_stop_gnss_anomaly_timer(self);
	EventsLogic_start_gnss_anomaly_timer(self);
	return;
}

static gboolean EventsLogic_network_unavailable_for_too_long(gpointer gpointer_self)
{
	EventsLogic *self = (EventsLogic *)gpointer_self;

	activation_status_t status = EventsLogic_service_activation_status(self);
	if (status == active || status == no_go) {
		logwarn("network_unavailable_for_too_long");
		self->is_network_anomaly_standing = TRUE;
		EventsLogic_emit_generic_anomaly_event(self);
		EventsLogic_emit_network_anomaly_event(self);
	}
	else
	{
		logdbg("network_unavailable_for_too_long, ignored because the status is %s", activation_status_str[status]);
	}
	self->network_anomaly_timer = 0;
	return G_SOURCE_REMOVE;
}

static void EventsLogic_start_network_anomaly_timer(EventsLogic *self)
{
    if (self->on_hold) return;

	guint network_anomaly_timeout = DEFAULT_NETWORK_ANOMALY_TIMEOUT_MINUTES;
	TollingManagerProxy_get_network_anomaly_timeout(self->tolling_manager_proxy, &network_anomaly_timeout);
	logdbg("start network anomaly timer");
	self->network_anomaly_timer = g_timeout_add_seconds(
		network_anomaly_timeout * 60,
		EventsLogic_network_unavailable_for_too_long,
		self);
	return;
}

static void EventsLogic_stop_network_anomaly_timer(EventsLogic *self)
{
	if (self->network_anomaly_timer)
	{
		logdbg("stop network anomaly timer");
		g_source_remove(self->network_anomaly_timer);
		self->network_anomaly_timer = 0;
	}
	return;
}

static void EventsLogic_clear_network_anomaly(EventsLogic *self)
{
	if (self->is_network_anomaly_standing)
	{
		self->is_network_anomaly_standing = FALSE;
		EventsLogic_emit_generic_anomaly_event(self);
		EventsLogic_emit_network_anomaly_event(self);
	}
	return;
}

static void EventsLogic_force_clear_network_anomaly(EventsLogic *self)
{
	self->is_network_anomaly_standing = FALSE;
	EventsLogic_emit_generic_anomaly_event(self);
	EventsLogic_emit_network_anomaly_event(self);
}

static void EventsLogic_log_network_anomaly_status(EventsLogic *self)
{
	logdbg("is_network_connected = %d, network_anomaly_timer = %d, is_network_anomaly_standing = %d, is_inside_ccc_domain = %d",
		self->is_network_connected,
		self->network_anomaly_timer,
		self->is_network_anomaly_standing,
		self->is_inside_ccc_domain);
	return;
}

static void EventsLogic_update_network_anomaly_timer_status(EventsLogic *self)
{
	logdbg("");
	EventsLogic_log_network_anomaly_status(self);
	if (self->is_network_connected)
	{
		EventsLogic_stop_network_anomaly_timer(self);
		EventsLogic_clear_network_anomaly(self);
	}
	else
	{
		// If the tolling application raises an anomaly, notifying the DSRC, and then crashes, it will restart and not know
		// about the anomaly, and therefore it won't clear it. This means that the driver won't see anything wrong, but if
		// the police check the OBU with CCC they'll find the anomaly and issue a ticket.
		// To prevent this scenario we explicitly clear the anomaly. This must be done under two conditions:
		// 1) the OBU must be inside the CCC domain of the current application (an application cannot clear the anomaly raised by another application), and
		// 2) according to that tolling application there is no anomaly (so that if for the DSRC there is no anomaly it's a noop, and if there is one, then it's
		// exactly the scenario that requires it to be cleared)
		if (self->is_inside_ccc_domain && self->is_network_anomaly_standing == FALSE)
		{
			EventsLogic_force_clear_network_anomaly(self);
			if (self->network_anomaly_timer == 0)
			{
				EventsLogic_start_network_anomaly_timer(self);
			}
		}
	}
	return;
}

static void EventsLogic_on_position_updated(gpointer gpointer_self)
{
	EventsLogic *self = (EventsLogic *)gpointer_self;
	if (self->is_inside_ccc_domain)
	{
		activation_status_t status = EventsLogic_service_activation_status(self);
		if (status == active || status == no_go)
		{
			EventsLogic_clear_gnss_anomaly(self);
			EventsLogic_reset_gnss_anomaly_timer(self);
		}
	}
}

static void EventsLogic_on_network_connected(gpointer gpointer_self)
{
	logdbg("");
	EventsLogic *self = (EventsLogic *)gpointer_self;
	self->is_network_connected = TRUE;
	EventsLogic_update_network_anomaly_timer_status(self);
	return;
}

static void EventsLogic_on_network_not_connected(gpointer gpointer_self)
{
	logdbg("");
	EventsLogic *self = (EventsLogic *)gpointer_self;
	self->is_network_connected = FALSE;
	EventsLogic_update_network_anomaly_timer_status(self);
	return;
}

static void EventsLogic_on_configured_from_remote(gpointer gpointer_self)
{
	logdbg("");
	EventsLogic *self = (EventsLogic *)gpointer_self;
	self->is_service_configured = TRUE;
	EventsLogic_update_service_status(self);
	return;
}

static void EventsLogic_on_vehicle_configured_from_remote(gpointer gpointer_self)
{
	logdbg("");
	EventsLogic *self = (EventsLogic *)gpointer_self;
	self->is_vehicle_configured = TRUE;
	EventsLogic_update_service_status(self);
	return;
}

static void EventsLogic_on_service_activated_from_remote(gpointer gpointer_self)
{
	logdbg("");
	EventsLogic *self = (EventsLogic *)gpointer_self;
	self->is_service_active = TRUE;
	EventsLogic_update_service_status(self);
}
static void EventsLogic_on_service_deactivated_from_remote(gpointer gpointer_self)
{
	logdbg("");
	EventsLogic *self = (EventsLogic *)gpointer_self;
	self->is_service_active = FALSE;
	EventsLogic_update_service_status(self);
	return;
}

static void EventsLogic_on_other_active_services_present(gpointer gpointer_self)
{
	logdbg("");
	EventsLogic *self = (EventsLogic *)gpointer_self;
	self->is_another_service_on_shared_domain_active = TRUE;
	EventsLogic_update_service_status(self);
	return;
}

static void EventsLogic_on_other_active_services_absent(gpointer gpointer_self)
{
	logdbg("");
	EventsLogic *self = (EventsLogic *)gpointer_self;
	self->is_another_service_on_shared_domain_active = FALSE;
	EventsLogic_update_service_status(self);
	return;
}

static void EventsLogic_stop_all_anomaly_timers(EventsLogic *self)
{
	logdbg("");
	EventsLogic_stop_gnss_anomaly_timer(self);
	EventsLogic_stop_network_anomaly_timer(self);
	return;
}

static void EventsLogic_on_enter_gnss_domain(gpointer gpointer_self)
{
	logdbg("");
	EventsLogic *self = (EventsLogic *)gpointer_self;
	self->is_inside_gnss_domain = TRUE;
	EventsLogic_update_service_status(self);
	return;
}

void EventsLogic_on_exit_gnss_domain(gpointer gpointer_self)
{
	logdbg("");
	EventsLogic *self = (EventsLogic *)gpointer_self;
	// There should be no need to clear anomalies and stop timers here, as it's already done in function on_exit_ccc_domain;
	// but if the OBU "teleports" outside of the polygons, it's possible this function is called before on_exit_ccc_domain,
	// and that can lead to an inconsistency. Clearing everything here solves the problem.
	EventsLogic_stop_all_anomaly_timers(self);
	self->is_inside_gnss_domain = FALSE;
	EventsLogic_update_service_status(self);
	return;
}


static void EventsLogic_clean_up_managed_anomalies_set_by_others(EventsLogic *self, guint go_nogo_flags)
{
    if (self->is_inside_ccc_domain) {
        if ((go_nogo_flags & ANOMALY_MASK_GNSS) || (go_nogo_flags & ANOMALY_MASK_LTE)) {
            /* lets clean up any pending anomaly under our control we don't know about.
            */
            if ( ((go_nogo_flags & ANOMALY_MASK_GNSS) && !self->is_gnss_anomaly_standing)
              || ((go_nogo_flags & ANOMALY_MASK_LTE) && !self->is_network_anomaly_standing)
            )
                logdbg("perform just in time anomaly cleanup");

            if (!(self->is_gnss_anomaly_standing || self->is_network_anomaly_standing))
                ApplicationEvents_emit_event(self->application_events, EVENT_EVENTS_LOGIC_GENERIC_ANOMALY_ABSENT);

            if ((go_nogo_flags & ANOMALY_MASK_GNSS) && !self->is_gnss_anomaly_standing)
                ApplicationEvents_emit_event(self->application_events, EVENT_EVENTS_LOGIC_GNSS_ANOMALY_ABSENT);

            if ((go_nogo_flags & ANOMALY_MASK_LTE) && !self->is_network_anomaly_standing)
                ApplicationEvents_emit_event(self->application_events, EVENT_EVENTS_LOGIC_NETWORK_ANOMALY_ABSENT);
        }
    }

    if ((go_nogo_flags & ANOMALY_MASK_SERVICE)) {
    	if (EventsLogic_service_active_inside_its_ccc_domain(self)) {
    		// CLEAR
    		ApplicationNotifications_notify_no_go_service(
    				self->application_notifications, FALSE);
    	}
    }

}


void EventsLogic_on_enter_ccc_domain(EventsLogic *self, guint go_nogo_flags)
{
	logdbg("");

	self->is_inside_ccc_domain = TRUE;
	EventsLogic_clean_up_managed_anomalies_set_by_others(self, go_nogo_flags);

	EventsLogic_update_service_status(self);
	// If the connection is down, this will start the network anomaly timer
	EventsLogic_update_network_anomaly_timer_status(self);
	// Inside the CCC domain the status is either active or nogo: in either case we want to clear the anomaly and reset the timer
	EventsLogic_clear_gnss_anomaly(self);
	EventsLogic_reset_gnss_anomaly_timer(self);
	return;
}

void EventsLogic_on_exit_ccc_domain(gpointer gpointer_self)
{
	logdbg("");
	EventsLogic *self = (EventsLogic *)gpointer_self;
	EventsLogic_stop_all_anomaly_timers(self);
	self->is_inside_ccc_domain = FALSE;
	EventsLogic_update_service_status(self);
	return;
}

void EventsLogic_on_tolling_gnss_sm_on_hold(gpointer gpointer_self)
{
	EventsLogic *self = (EventsLogic *)gpointer_self;
	EventsLogic_stop_all_anomaly_timers(self);
}

void EventsLogic_on_tolling_gnss_sm_start(gpointer gpointer_self)
{
	logdbg("");
	EventsLogic *self = (EventsLogic *)gpointer_self;
	// No specific actions, just recalculate the status
	EventsLogic_update_service_status(self);
	activation_status_t status = EventsLogic_service_activation_status(self);
	if (status == active || status == no_go)
	{
		if (self->is_network_anomaly_standing || self->is_gnss_anomaly_standing)
		{
			ApplicationEvents_emit_event(self->application_events, EVENT_EVENTS_LOGIC_GENERIC_ANOMALY_PRESENT);
		}
		if (self->is_network_anomaly_standing)
		{
			ApplicationEvents_emit_event(self->application_events, EVENT_EVENTS_LOGIC_NETWORK_ANOMALY_PRESENT);
		}
		if (self->is_gnss_anomaly_standing)
		{
			ApplicationEvents_emit_event(self->application_events, EVENT_EVENTS_LOGIC_GNSS_ANOMALY_PRESENT);
		}
	}
	if (self->is_inside_ccc_domain)
		EventsLogic_start_gnss_anomaly_timer(self);
	return;
}

void EventsLogic_deallocate(EventsLogic *self)
{
	logdbg("");
	if (self != NULL)
	{
		g_free(self);
	}
	return;
}

void EventsLogic_destroy(EventsLogic *self)
{
	logdbg("");
	EventsLogic_deinitialize(self);
	EventsLogic_deallocate(self);
	return;
}

EventsLogic *EventsLogic_allocate()
{
	logdbg("");
	EventsLogic *self = g_try_new0(EventsLogic, 1);
	g_return_val_if_fail(self != NULL, NULL);
	self->dtor = EventsLogic_destroy;
	self->check_service_status = EventsLogic_check_service_status;
	self->on_service_activated_from_remote = EventsLogic_on_service_activated_from_remote;
	self->on_service_deactivated_from_remote = EventsLogic_on_service_deactivated_from_remote;
	self->on_exit_gnss_domain = EventsLogic_on_exit_gnss_domain;
	self->on_exit_ccc_domain = EventsLogic_on_exit_ccc_domain;
	return self;
}

EventsLogic *EventsLogic_new(ApplicationEvents *application_events, TollingManagerProxy *tolling_manager_proxy,
		ApplicationNotifications *application_notifications)
{
	logdbg("");
	EventsLogic *self = EventsLogic_allocate();
	g_return_val_if_fail(self != NULL, NULL);
	EventsLogic *initialize_result = EventsLogic_initialize(
		self,
		application_events,
		tolling_manager_proxy);
	self->application_notifications = application_notifications;
	if (initialize_result == NULL)
	{
		EventsLogic_deallocate(self);
	}

	g_return_val_if_fail(initialize_result != NULL, NULL);
	return self;
}

void EventsLogic_delete(EventsLogic *self)
{
	if (self != NULL)
	{
		self->dtor(self);
	}
	return;
}

void EventsLogic_on_sm_start(EventsLogic *self) { self->on_hold = FALSE; }
void EventsLogic_on_sm_on_hold(EventsLogic *self) { self->on_hold = TRUE; }


EventsLogic *EventsLogic_initialize(EventsLogic *self,
		ApplicationEvents *application_events,
		TollingManagerProxy *tolling_manager_proxy)
{
	loginfo("");
	g_return_val_if_fail(self != NULL, NULL);
	self->application_events = application_events;
	self->tolling_manager_proxy = tolling_manager_proxy;
	self->is_gnss_anomaly_standing = FALSE;
	self->is_network_anomaly_standing = FALSE;
	self->is_network_connected = TRUE;
	self->is_service_configured = FALSE;
	self->is_vehicle_configured = FALSE;
	self->is_service_active = FALSE;
	self->is_another_service_on_shared_domain_active = FALSE;
	self->is_inside_gnss_domain = FALSE;
	self->is_inside_ccc_domain = FALSE;
	self->is_axles_config_approved = TRUE;
	self->gnss_anomaly_timer = 0;
	self->network_anomaly_timer = 0;
	self->on_hold = TRUE;

    ApplicationEvents_register_event(
        self->application_events,
        EVENT_TOLLING_GNSS_SM_START,
        (ApplicationEvents_callback_type)EventsLogic_on_sm_start,
        self);

    ApplicationEvents_register_event(
        self->application_events,
        EVENT_TOLLING_GNSS_SM_ON_HOLD,
        (ApplicationEvents_callback_type)EventsLogic_on_sm_on_hold,
        self);

	ApplicationEvents_register_event(
		self->application_events,
		EVENT_POSITIONING_SERVICE_PROXY_POSITION_UPDATED,
		EventsLogic_on_position_updated,
		self);
	ApplicationEvents_register_event(
		self->application_events,
		EVENT_NETWORK_MANAGER_PROXY_NETWORK_CONNECTED,
		EventsLogic_on_network_connected,
		self);
	ApplicationEvents_register_event(
		self->application_events,
		EVENT_NETWORK_MANAGER_PROXY_NETWORK_NOT_CONNECTED,
		EventsLogic_on_network_not_connected,
		self);
	ApplicationEvents_register_event(
		self->application_events,
		EVENT_TOLLING_MANAGER_PROXY_CONFIGURED_FROM_REMOTE,
		EventsLogic_on_configured_from_remote,
		self);
	ApplicationEvents_register_event(
		self->application_events,
		EVENT_TOLLING_MANAGER_PROXY_VEHICLE_CONFIGURED_FROM_REMOTE,
		EventsLogic_on_vehicle_configured_from_remote,
		self);
	ApplicationEvents_register_event(
		self->application_events,
		EVENT_TOLLING_MANAGER_PROXY_SERVICE_ACTIVATED_FROM_REMOTE,
		self->on_service_activated_from_remote,
		self);
	ApplicationEvents_register_event(
		self->application_events,
		EVENT_TOLLING_MANAGER_PROXY_SERVICE_DEACTIVATED_FROM_REMOTE,
		self->on_service_deactivated_from_remote,
		self);
	ApplicationEvents_register_event(
		self->application_events,
		EVENT_TOLLING_MANAGER_PROXY_OTHER_ACTIVE_SERVICES_PRESENT,
		EventsLogic_on_other_active_services_present,
		self);
	ApplicationEvents_register_event(
		self->application_events,
		EVENT_TOLLING_MANAGER_PROXY_OTHER_ACTIVE_SERVICES_ABSENT,
		EventsLogic_on_other_active_services_absent,
		self);
	ApplicationEvents_register_event(
		self->application_events,
		EVENT_TOLLING_MANAGER_PROXY_ENTER_GNSS_DOMAIN,
		EventsLogic_on_enter_gnss_domain,
		self);
	ApplicationEvents_register_event(
		self->application_events,
		EVENT_TOLLING_MANAGER_PROXY_EXIT_GNSS_DOMAIN,
		self->on_exit_gnss_domain,
		self);
	ApplicationEvents_register_event_ex(
		self->application_events,
		EVENT_TOLLING_MANAGER_PROXY_ENTER_CCC_DOMAIN,
		G_CALLBACK(EventsLogic_on_enter_ccc_domain),
		self, 1, G_TYPE_UINT);
	ApplicationEvents_register_event(
		self->application_events,
		EVENT_TOLLING_MANAGER_PROXY_EXIT_CCC_DOMAIN,
		self->on_exit_ccc_domain,
		self);
	ApplicationEvents_register_event(
		self->application_events,
		EVENT_TOLLING_GNSS_SM_START,
		EventsLogic_on_tolling_gnss_sm_start,
		self);
	ApplicationEvents_register_event(
		self->application_events,
		EVENT_TOLLING_GNSS_SM_ON_HOLD,
		EventsLogic_on_tolling_gnss_sm_on_hold,
		self);
	return self;
}

void EventsLogic_deinitialize(EventsLogic *self)
{
	loginfo("");
	return;
}


