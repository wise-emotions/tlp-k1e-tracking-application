/*
 * fix_and_status.c
 *
 *  Created on: Nov 4, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#include "fix_and_status.h"

#include "logger.h"
#include "json_mapper.h"

FixAndStatus *fix_and_status_new(void)
{
	FixAndStatus *self = (FixAndStatus *)g_malloc0(sizeof(FixAndStatus));

	self->tampering           = FALSE;
	self->nogo                = TRUE;     // From P.ERTA.S.013.v01.rev16_IFS-01-OBU PROXY.docx: 0 = NO-GO, 1 = GO
	self->blocked             = FALSE;
	self->tolling_eligibility = TRUE;

	return self;
}

void fix_and_status_destroy(FixAndStatus *self)
{
	g_free(self);
}

gboolean fix_and_status_are_equal(const FixAndStatus *first, const FixAndStatus *second)
{
	return
	first->tampering            == second->tampering           &&
	first->nogo                 == second->nogo                &&
	first->blocked              == second->blocked             &&
	first->tolling_eligibility  == second->tolling_eligibility;
}

JsonMapper *fix_and_status_to_json_mapper(const FixAndStatus *self)
{
	JsonMapper *json_mapper = json_mapper_create((guchar*)"{}");

	json_mapper_add_property(json_mapper, (const guchar*)"tampering",           json_type_boolean, (const gpointer)&self->tampering);
	json_mapper_add_property(json_mapper, (const guchar*)"nogo",                json_type_boolean, (const gpointer)&self->nogo);
	json_mapper_add_property(json_mapper, (const guchar*)"blocked",             json_type_boolean, (const gpointer)&self->blocked);
	json_mapper_add_property(json_mapper, (const guchar*)"tolling_eligibility", json_type_boolean, (const gpointer)&self->tolling_eligibility);

	return json_mapper;
}
