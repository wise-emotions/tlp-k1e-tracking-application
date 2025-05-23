#pragma once

#include <glib.h>


typedef struct _ApplicationEvents ApplicationEvents;
typedef struct _ConfigurationStore ConfigurationStore;
typedef struct _TollingManagerProxy TollingManagerProxy;
typedef struct _Tolling_Gnss_Sm_Data Tolling_Gnss_Sm_Data;
typedef struct _GArray GArray;

// This must be set by the tolling applications which need it. Those which don't can simply ignore it.
extern GArray *other_services_on_this_domain;

gboolean TollingManagerProxy_get_tr_id(const TollingManagerProxy *self, const gchar **value);
gboolean TollingManagerProxy_get_block_behaviour(const TollingManagerProxy *self, guint16 *value);
gboolean TollingManagerProxy_get_transmission_timeout(const TollingManagerProxy *self, guint *value);
gboolean TollingManagerProxy_get_max_packet(const TollingManagerProxy *self, guint16 *value);
gboolean TollingManagerProxy_get_filter_distance(const TollingManagerProxy *self, gdouble *value);
gboolean TollingManagerProxy_get_filter_time(const TollingManagerProxy *self, guint *value);
gboolean TollingManagerProxy_get_filter_logic(const TollingManagerProxy *self, guint *value);
gboolean TollingManagerProxy_get_filter_heading(const TollingManagerProxy *self, gdouble *value);
gboolean TollingManagerProxy_get_network_anomaly_timeout(const TollingManagerProxy *self, guint *value);
gboolean TollingManagerProxy_get_gps_anomaly_timeout(const TollingManagerProxy *self, guint *value);
gboolean TollingManagerProxy_get_license_plate_nationality(const TollingManagerProxy *self, const gchar **value);
gboolean TollingManagerProxy_get_license_plate(const TollingManagerProxy *self, const gchar **value);
gboolean TollingManagerProxy_get_vehicle_max_laden_weight(const TollingManagerProxy *self, guint *value);
gboolean TollingManagerProxy_get_vehicle_max_train_weight(const TollingManagerProxy *self, guint *value);
gboolean TollingManagerProxy_get_fap_presence(const TollingManagerProxy *self, guint *value);
gboolean TollingManagerProxy_get_euro_weight_class(const TollingManagerProxy *self, guint *value);
gboolean TollingManagerProxy_get_default_axis(const TollingManagerProxy *self, guint *value);

gboolean TollingManagerProxy_get_current_axles(const TollingManagerProxy *self, guint *value);
gboolean TollingManagerProxy_get_current_weight(const TollingManagerProxy *self, guint *value);
gboolean TollingManagerProxy_get_current_actual_weight(const TollingManagerProxy *self, guint *value);
gboolean TollingManagerProxy_get_current_trailer_type(const TollingManagerProxy *self, guint *value);

TollingManagerProxy *TollingManagerProxy_new(const gchar *gnss_domain_name, const guint service_activation_domain_id,
                                             ConfigurationStore *configuration_store, ApplicationEvents *application_events,
                                             Tolling_Gnss_Sm_Data *data);
void TollingManagerProxy_delete(TollingManagerProxy *self);
