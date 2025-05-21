/*
 * domain_specific_data.h
 *
 *  Created on: May 11, 2022
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#ifndef DOMAIN_SPECIFIC_DATA_H_
#define DOMAIN_SPECIFIC_DATA_H_

#include <glib.h>

typedef struct _TollingGnssCoreFactoryInterface TollingGnssCoreFactoryInterface;

typedef struct _DomainSpecificData {
	const gchar *gnss_domain_name;
	guint service_activation_domain_id;
	TollingGnssCoreFactoryInterface *factory;
} DomainSpecificData;

#endif /* DOMAIN_SPECIFIC_DATA_H_ */
