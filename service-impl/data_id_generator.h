/*
 * data_id_generator.h
 *
 *  Created on: Dec 10, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#pragma once

#include <glib.h>

#include "configuration_store.h"

typedef struct _DataIdGenerator DataIdGenerator;

DataIdGenerator *DataIdGenerator_new(ConfigurationStore *configuration_store);
void DataIdGenerator_destroy(DataIdGenerator *self);

gint DataIdGenerator_get_next_data_id(DataIdGenerator *self);
gint DataIdGenerator_get_next_data_id_no_persist(DataIdGenerator *self);
void DataIdGenerator_persist_data_id(DataIdGenerator *self);
