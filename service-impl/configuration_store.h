/*
 * configuration_store.h
 *
 * Created on: Apr 15, 2020
 * Author: Gianni Manzo (gianni.manzo@nttdata.com)
 */

#ifndef CONFIGURATION_STORE_H_
#define CONFIGURATION_STORE_H_

#include <glib.h>

#include "json_mapper.h"

typedef struct _ConfigurationStore ConfigurationStore;

gboolean ConfigurationStore_get_int16_ex(ConfigurationStore* self, const guchar *key, gint16* value, gboolean log_error_on_missing);
gboolean ConfigurationStore_get_int_ex(ConfigurationStore* self, const guchar *key, gint* value, gboolean log_error_on_missing);
gboolean ConfigurationStore_get_double_ex(ConfigurationStore* self, const guchar *key, gdouble* value, gboolean log_error_on_missing);
gboolean ConfigurationStore_get_string_ex(ConfigurationStore* self, const guchar *key, const gchar** value, gboolean log_error_on_missing);

gboolean ConfigurationStore_get_int16(ConfigurationStore* self, const guchar *key, gint16* value);
gboolean ConfigurationStore_get_int(ConfigurationStore* self, const guchar *key, gint* value);
gboolean ConfigurationStore_get_double(ConfigurationStore* self, const guchar *key, gdouble* value);
gboolean ConfigurationStore_get_string(ConfigurationStore* self, const guchar *key, const gchar** value);

gboolean ConfigurationStore_set_int(ConfigurationStore* self, const guchar *key, gint value);
gboolean ConfigurationStore_set_double(ConfigurationStore* self, const guchar *key, gdouble value);
gboolean ConfigurationStore_set_string(ConfigurationStore *self, const guchar *key, const gchar* value);

gboolean ConfigurationStore_update_begin(ConfigurationStore* self, JsonMapper** updated_config);
gboolean ConfigurationStore_update_end(ConfigurationStore* self, JsonMapper* updated_config, gboolean update_ok);

ConfigurationStore* ConfigurationStore_new();
void ConfigurationStore_destroy(ConfigurationStore* config);

#endif
