/*
 * configuration_store.c
 *
 * Created on: Apr 15, 2020
 * Author: Gianni Manzo (gianni.manzo@nttdata.com)
 */

#include "logger.h"
#include "resource_access_mediator.h"
#include "service_configuration.h"

#include "configuration_store.h"

typedef struct _ConfigurationStore
{
	JsonMapper* saved_config;
	gboolean is_empty;

} ConfigurationStore;

static GString *ConfigurationStore_read_resource_as_text()
{
	char* resource_file_name = NULL;
	ParameterTypes ptype = get_parameter(CONFIGURATION_PERSISTENCE_PAR, (const void**)&resource_file_name);
	if (ptype == pt_invalid)
	{
		return NULL;
	}
	else
	{
		logdbg("Reading from file %s", resource_file_name);
		return rm_read_resource_as_text(RM_PSIST_DATA, resource_file_name);
	}
}

static gboolean ConfigurationStore_write_resource_as_text(GString *text) {
	char* resource_file_name = NULL;
	ParameterTypes ptype = get_parameter(CONFIGURATION_PERSISTENCE_PAR, (const void**)&resource_file_name);
	if (ptype == pt_invalid)
	{
		return FALSE;
	}
	else
	{
		logdbg("Writing to file %s", resource_file_name);
		int result = rm_write_resource_as_text(RM_PSIST_DATA, resource_file_name, text);
		return (result >= 0);
	}
}

gboolean ConfigurationStore_get_int16_ex(ConfigurationStore *self, const guchar *key, gint16* value, gboolean log_error_on_missing)
{
	if (self == NULL || self->is_empty == TRUE)
	{
		logdbg("ConfigurationStore not available, cannot get value");
		return FALSE;
	}
	gint int_value;
	gint result = json_mapper_get_property_as_int(self->saved_config, key, &int_value);
	if (result != JSON_MAPPER_NO_ERROR)
	{
		if (log_error_on_missing)
		{
			logerr("Cannot get property %s", key);
		}
		return FALSE;
	}
	else
	{
		*value = int_value;
	}
	return TRUE;
}

gboolean ConfigurationStore_get_int_ex(ConfigurationStore *self, const guchar *key, gint* value, gboolean log_error_on_missing)
{
	if (self == NULL || self->is_empty == TRUE)
	{
		logdbg("ConfigurationStore not available, cannot get value");
		return FALSE;
	}
	gint result = json_mapper_get_property_as_int(self->saved_config, key, value);
	if (result != JSON_MAPPER_NO_ERROR)
	{
		if (log_error_on_missing)
		{
			logerr("Cannot get property %s", key);
		}
		return FALSE;
	}
	return TRUE;
}

gboolean ConfigurationStore_get_double_ex(ConfigurationStore *self, const guchar *key, gdouble* value, gboolean log_error_on_missing)
{
	if (self == NULL || self->is_empty == TRUE)
	{
		logdbg("ConfigurationStore not available, cannot get value");
		return FALSE;
	}
	gint result = json_mapper_get_property_as_double(self->saved_config, key, value);
	if (result != JSON_MAPPER_NO_ERROR)
	{
		if (log_error_on_missing)
		{
			logerr("Cannot get property %s", key);
		}
		return FALSE;
	}
	return TRUE;
}

gboolean ConfigurationStore_get_string_ex(ConfigurationStore *self, const guchar *key, const gchar** value, gboolean log_error_on_missing)
{
	if (self == NULL || self->is_empty == TRUE)
	{
		logdbg("ConfigurationStore not available, cannot get value");
		return FALSE;
	}
	gint result = json_mapper_get_property_as_string(self->saved_config, key, value);
	if (result != JSON_MAPPER_NO_ERROR)
	{
		if (log_error_on_missing)
		{
			logerr("Cannot get property %s", key);
		}
		return FALSE;
	}
	return TRUE;
}

gboolean ConfigurationStore_get_int16(ConfigurationStore *self, const guchar *key, gint16* value)
{
	return ConfigurationStore_get_int16_ex(self, key, value, TRUE);
}

gboolean ConfigurationStore_get_int(ConfigurationStore *self, const guchar *key, gint* value)
{
	return ConfigurationStore_get_int_ex(self, key, value, TRUE);
}

gboolean ConfigurationStore_get_double(ConfigurationStore *self, const guchar *key, gdouble* value)
{
	return ConfigurationStore_get_double_ex(self, key, value, TRUE);
}

gboolean ConfigurationStore_get_string(ConfigurationStore *self, const guchar *key, const gchar** value)
{
	return ConfigurationStore_get_string_ex(self, key, value, TRUE);
}

gboolean ConfigurationStore_set_int(ConfigurationStore *self, const guchar *key, gint value)
{
	JsonMapper* updated_config = NULL;
	gboolean update_successful = ConfigurationStore_update_begin(self, &updated_config);
	if (update_successful)
	{
		gint ret_add = json_mapper_add_property(updated_config, key, json_type_int, &value);
		gboolean update_ok = (ret_add == JSON_MAPPER_NO_ERROR);
		update_successful = ConfigurationStore_update_end(self, updated_config, update_ok);
	}
	return update_successful;
}

gboolean ConfigurationStore_set_double(ConfigurationStore *self, const guchar *key, gdouble value)
{
	JsonMapper* updated_config = NULL;
	gboolean update_successful = ConfigurationStore_update_begin(self, &updated_config);
	if (update_successful)
	{
		gint ret_add = json_mapper_add_property(updated_config, key, json_type_double, &value);
		gboolean update_ok = (ret_add == JSON_MAPPER_NO_ERROR);
		update_successful = ConfigurationStore_update_end(self, updated_config, update_ok);
	}
	return update_successful;
}

gboolean ConfigurationStore_set_string(ConfigurationStore *self, const guchar *key, const gchar *value)
{
	JsonMapper* updated_config = NULL;
	gboolean update_successful = ConfigurationStore_update_begin(self, &updated_config);
	if (update_successful)
	{
		gint ret_add = json_mapper_add_property(updated_config, key, json_type_string, (const gpointer)value);
		gboolean update_ok = (ret_add == JSON_MAPPER_NO_ERROR);
		update_successful = ConfigurationStore_update_end(self, updated_config, update_ok);
	}
	return update_successful;
}

gboolean ConfigurationStore_update_begin(ConfigurationStore *self, JsonMapper** updated_config)
{
	*updated_config = NULL;
	if (self == NULL)
	{
		logdbg("ConfigurationStore not available");
		return FALSE;
	}
	*updated_config = json_mapper_create_copy(self->saved_config);
	return TRUE;
}

gboolean ConfigurationStore_update_end(ConfigurationStore *self, JsonMapper *updated_config, gboolean update_ok)
{
	gboolean ret = FALSE;
	if (!update_ok)
	{
		logerr("ERROR updating data!");
		json_mapper_destroy(updated_config);
	}
	else
	{
		loginfo("configuration to save = %s", json_mapper_to_string(updated_config));
		GString *resource_content = g_string_new(json_mapper_to_string(updated_config));
		resource_content = g_string_append_c(resource_content, '\n');
		gboolean set_ok = ConfigurationStore_write_resource_as_text(resource_content);
		if (!set_ok)
		{
			logerr("ERROR saving data to file!");
			json_mapper_destroy(updated_config);
		}
		else
		{
			json_mapper_destroy(self->saved_config);
			self->saved_config = updated_config;
			self->is_empty = FALSE;
			ret = TRUE;
		}
		g_string_free(resource_content, TRUE);
	}
	return ret;
}

ConfigurationStore *ConfigurationStore_new()
{
	ConfigurationStore *self = (ConfigurationStore *)g_malloc0(sizeof(ConfigurationStore));
	self->saved_config = NULL;
	self->is_empty = TRUE;
	GString *cached_string = ConfigurationStore_read_resource_as_text();
	if(cached_string != NULL)
	{
		self->saved_config = json_mapper_create((const guchar*)cached_string->str);
		g_string_free(cached_string, TRUE);
		self->is_empty = FALSE;
	}
	if(self->saved_config == NULL)
	{
		self->saved_config = json_mapper_create((const guchar*)"{}");
	}
	return self;
}

void ConfigurationStore_destroy(ConfigurationStore *self)
{
	if(self != NULL)
	{
		json_mapper_destroy(self->saved_config);
		g_free(self);
	}
}
