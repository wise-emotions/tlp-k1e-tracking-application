/*
 * fix_and_status.h
 *
 *  Created on: Nov 4, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#ifndef FIX_AND_STATUS_H_
#define FIX_AND_STATUS_H_

#include <glib.h>

typedef struct _FixAndStatus {
	gboolean    tampering;
	gboolean    nogo;
	gboolean    blocked;
	gboolean    tolling_eligibility;
} FixAndStatus;

typedef struct _JsonMapper JsonMapper;

FixAndStatus *fix_and_status_new(void);
void fix_and_status_destroy(FixAndStatus *self);

gboolean fix_and_status_are_equal(const FixAndStatus *first, const FixAndStatus *second);

JsonMapper *fix_and_status_to_json_mapper(const FixAndStatus *self);


#endif /* FIX_AND_STATUS_H_ */
