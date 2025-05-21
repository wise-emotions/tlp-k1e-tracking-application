/*
 * trip_id_manager.c
 *
 * Created on: Mar 26, 2021
 * Author: Gianni Manzo (gianni.manzo@nttdata.com)
 */

#include "trip_id_manager.h"

#include "logger.h"

struct _TripIdManager {
	GString *trip_id;
};

TripIdManager *trip_id_manager_create() {
	TripIdManager *self = (TripIdManager *) g_malloc0(sizeof(TripIdManager));

	self->trip_id = g_string_new("");

	return self;
}

void trip_id_manager_destroy(TripIdManager *self) {
	if(self != NULL) {
		g_string_free(self->trip_id, TRUE);

		g_free(self);
	}
}

void trip_id_manager_generate_trip_id(TripIdManager *self, GString *obu_id) {
	logdbg("");

	GDateTime *now_utc = g_date_time_new_now_utc();

	guint64	now_utc_microseconds = g_date_time_to_unix(now_utc) * G_USEC_PER_SEC +
			g_date_time_get_microsecond(now_utc);

	logdbg("now_utc_microseconds %" G_GUINT64_FORMAT, now_utc_microseconds);

	g_string_printf(self->trip_id, "%s%" G_GUINT64_FORMAT, obu_id->str, now_utc_microseconds);

	logdbg("trip_id: %s", self->trip_id->str);

	GChecksum *checksum = g_checksum_new(G_CHECKSUM_SHA256);
	if (checksum == NULL) {
		logerr("g_checksum_new() failed");
	} else {
		g_checksum_update(checksum, (const guchar *)self->trip_id->str, self->trip_id->len);

		g_string_assign(self->trip_id, g_checksum_get_string(checksum));

		g_checksum_free(checksum);

		logdbg("sha256(trip_id): %s", self->trip_id->str);
	}
}

GString *trip_id_manager_get_trip_id(TripIdManager *self) {
	return self->trip_id;
}
