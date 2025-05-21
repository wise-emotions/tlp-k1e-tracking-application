#include <glib.h>
#include <string.h>

#include "logger.h"
#include "hmi_proxy.h"
#include "alarm_and_alert_notification_facade.h"

#include "dsrc_go_nogo_status.h"

#include "application_notifications.h"

#define APPLICATION_NOTIFICATIONS_NO_GO_SERVICE_KEY "tolling_gnss_notification_service_go"
#define APPLICATION_NOTIFICATIONS_NO_GO_NETWORK_KEY "tolling_gnss_notification_network_anomaly"
#define APPLICATION_NOTIFICATIONS_NO_GO_GNSS_KEY "tolling_gnss_notification_gnss_anomaly"

#define APPLICATION_NOTIFICATIONS_AUDIO_RESOURCE_DEFAULT_VOLUME 100
#define APPLICATION_NOTIFICATIONS_NOGO_AUDIO_RESOURCE "service-not-properly-configured.opus"
#define APPLICATION_NOTIFICATIONS_GNSS_ANOMALY_AUDIO_RESOURCE "no-gps-signal-for-too-long.opus"
#define APPLICATION_NOTIFICATIONS_NETWORK_ANOMALY_AUDIO_RESOURCE "network-disconnected-for-too-long.opus"

struct _ApplicationNotifications
{
	HmiProxy* hmi_proxy;
	AlarmAndAlertNotificationFacade *alarm_and_alert_notification_facade;
	DsrcGoNogoStatus *dsrc_go_nogo_status;
};

static void ApplicationNotifications_install_audio_pattern_for_resource_id(ApplicationNotifications *self, char *resource_id)
{
	HmiAudioPattern audio_pattern;
	memset(&audio_pattern, 0, sizeof(audio_pattern));
	audio_pattern.volume = APPLICATION_NOTIFICATIONS_AUDIO_RESOURCE_DEFAULT_VOLUME;
	audio_pattern.resource_id = resource_id;
	hmi_proxy_install_localized_audio_pattern(self->hmi_proxy, resource_id, &audio_pattern);
}

static void ApplicationNotifications_notify_with_facade(ApplicationNotifications *self, gchar *key, gboolean is_active)
{
	if (is_active)
	{
		AlarmAndAlertNotificationFacade_raise(self->alarm_and_alert_notification_facade, key);
	}
	else
	{
		AlarmAndAlertNotificationFacade_clear(self->alarm_and_alert_notification_facade, key);
	}
	return;
}

#define RAISE_SERVICE_ANOMALY_DELAY_IN_MILLISECONDS 750

static gboolean ApplicationNotifications_raise_service_anomaly(gpointer user_data)
{
	ApplicationNotifications *self = (ApplicationNotifications *) user_data;

	ApplicationNotifications_notify_with_facade(self, APPLICATION_NOTIFICATIONS_NO_GO_SERVICE_KEY, TRUE);

	return G_SOURCE_REMOVE;
}

void ApplicationNotifications_notify_no_go_service(ApplicationNotifications *self, gboolean is_active)
{
	if (is_active == FALSE) {
		ApplicationNotifications_notify_with_facade(self, APPLICATION_NOTIFICATIONS_NO_GO_SERVICE_KEY, is_active);
	} else {
		g_timeout_add(RAISE_SERVICE_ANOMALY_DELAY_IN_MILLISECONDS, ApplicationNotifications_raise_service_anomaly, self);
	}
	DsrcGoNogoStatus_set_app_no_go_service(self->dsrc_go_nogo_status, is_active);
}

void ApplicationNotifications_notify_anomaly_network(ApplicationNotifications *self, gboolean is_active)
{
	ApplicationNotifications_notify_with_facade(self, APPLICATION_NOTIFICATIONS_NO_GO_NETWORK_KEY, is_active);
	DsrcGoNogoStatus_set_app_no_go_network(self->dsrc_go_nogo_status, is_active);
}

void ApplicationNotifications_notify_anomaly_gnss(ApplicationNotifications *self, gboolean is_active)
{
	ApplicationNotifications_notify_with_facade(self, APPLICATION_NOTIFICATIONS_NO_GO_GNSS_KEY, is_active);
	DsrcGoNogoStatus_set_app_no_go_gnss(self->dsrc_go_nogo_status, is_active);
}

ApplicationNotifications *ApplicationNotifications_new(DsrcGoNogoStatus *dsrc_go_nogo_status, AlarmAndAlertNotificationFacade *alarm_and_alert_notification_facade)
{
	loginfo("");
	ApplicationNotifications *self = g_try_new0(ApplicationNotifications, 1);
	g_return_val_if_fail(self != NULL, NULL);
	self->dsrc_go_nogo_status = dsrc_go_nogo_status;
	self->alarm_and_alert_notification_facade = alarm_and_alert_notification_facade;
	self->hmi_proxy = hmi_proxy_create(self);
	ApplicationNotifications_install_audio_pattern_for_resource_id(
		self,
		APPLICATION_NOTIFICATIONS_NOGO_AUDIO_RESOURCE);
	AlarmAndAlertNotificationFacade_register_notification(
		self->alarm_and_alert_notification_facade,
		APPLICATION_NOTIFICATIONS_NO_GO_SERVICE_KEY,
		APPLICATION_NOTIFICATIONS_NO_GO_SERVICE_KEY,
		ALARM_AND_ALERT_NOTIFICATION_NOTIFICATION_TYPE_DSRC_ALARM_SERVICE,
		APPLICATION_NOTIFICATIONS_NOGO_AUDIO_RESOURCE);
	ApplicationNotifications_install_audio_pattern_for_resource_id(
		self,
		APPLICATION_NOTIFICATIONS_GNSS_ANOMALY_AUDIO_RESOURCE);
	AlarmAndAlertNotificationFacade_register_notification(
		self->alarm_and_alert_notification_facade,
		APPLICATION_NOTIFICATIONS_NO_GO_GNSS_KEY,
		APPLICATION_NOTIFICATIONS_NO_GO_GNSS_KEY,
		ALARM_AND_ALERT_NOTIFICATION_NOTIFICATION_TYPE_DSRC_ALARM_GNSS,
		APPLICATION_NOTIFICATIONS_GNSS_ANOMALY_AUDIO_RESOURCE);
	ApplicationNotifications_install_audio_pattern_for_resource_id(
		self,
		APPLICATION_NOTIFICATIONS_NETWORK_ANOMALY_AUDIO_RESOURCE);
	AlarmAndAlertNotificationFacade_register_notification(
		self->alarm_and_alert_notification_facade,
		APPLICATION_NOTIFICATIONS_NO_GO_NETWORK_KEY,
		APPLICATION_NOTIFICATIONS_NO_GO_NETWORK_KEY,
		ALARM_AND_ALERT_NOTIFICATION_NOTIFICATION_TYPE_DSRC_ALARM_LTE,
		APPLICATION_NOTIFICATIONS_NETWORK_ANOMALY_AUDIO_RESOURCE);
	return self;
}

void ApplicationNotifications_destroy(ApplicationNotifications *self)
{
	loginfo("");
	if (self != NULL)
	{
		if (self->hmi_proxy != NULL)
		{
			hmi_proxy_destroy(self->hmi_proxy);
		}
		g_free(self);
	}
}
