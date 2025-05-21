/*
 * data_id_generator.c
 *
 *  Created on: Dec 10, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#include <glib.h>

#include "logger.h"

#include "configuration_store.h"

#include "data_id_generator.h"

#define INVALID_DATA_ID 0

typedef struct _DataIdGenerator
{
	ConfigurationStore* configuration_store;
	gint current_data_id;
} DataIdGenerator;

DataIdGenerator *DataIdGenerator_new(ConfigurationStore *configuration_store)
{
	DataIdGenerator *self = g_new0(DataIdGenerator, 1);
	self->configuration_store = configuration_store;
	self->current_data_id = INVALID_DATA_ID;
	return self;
}

void DataIdGenerator_destroy(DataIdGenerator *self)
{
	g_free(self);
}

gint DataIdGenerator_get_next_data_id(DataIdGenerator *self)
{
	if (self->current_data_id == INVALID_DATA_ID)
	{
		ConfigurationStore_get_int(self->configuration_store, (guchar*)"data_id", &(self->current_data_id));
	}

	(self->current_data_id)++;
	return self->current_data_id;
}

gint DataIdGenerator_get_next_data_id_no_persist(DataIdGenerator *self)
{
	(self->current_data_id)++;
	return self->current_data_id;
}

void DataIdGenerator_persist_data_id(DataIdGenerator *self)
{
	ConfigurationStore_set_int(self->configuration_store, (guchar*)"data_id", self->current_data_id);
}
