/*
 * fix_data_json.h
 *
 *  Created on: 21 Feb 2022
 *      Author: Andrea Baldecchi
 */

#ifndef SERVICE_IMPL_TOLLING_GNSS_CORE_FIX_DATA_JSON_H_
#define SERVICE_IMPL_TOLLING_GNSS_CORE_FIX_DATA_JSON_H_


//FixData
#define JSON_DATA_ID                  "data_id"
#define JSON_TIME                     "timestamp"
#define JSON_LATITUDE                 "latitude"
#define JSON_LONGITUDE                "longitude"
#define JSON_ALTITUDE                 "altitude"
#define JSON_SPEED                    "speed"
#define JSON_DIRECTION                "direction"
#define JSON_ACCURACY                 "accuracy"
#define JSON_HDOP                     "hdop"
#define JSON_VDOP                     "vdop"
#define JSON_PDOP                     "pdop"
#define JSON_FIX_VAL                  "fix_validity"
#define JSON_FIX_TYPE                 "fix_type"
#define JSON_SAT_FOR_FIX              "satellitesNumber"

//fix and status
#define JSON_TAMPERING                "tampering"
#define JSON_NOGO                     "nogo"
#define JSON_BLOCKED                  "blocked"
#define JSON_TOLLING_ELIGIBILITY      "tolling_eligibility"

#define JSON_ODOMETER                 "odometer_km"

//vehicle
#define JSON_VEHICLE_TRAILER_AXLES        "vehicle_trailer_axles"
#define JSON_VEHICLE_TRAIN_WEIGHT         "vehicle_train_weight"
#define JSON_VEHICLE_ACTUAL_TRAIN_WEIGHT  "vehicle_actual_weight_kg"
#define JSON_VEHICLE_TRAILER_TYPE         "vehicle_trailer_type"


#endif /* SERVICE_IMPL_TOLLING_GNSS_CORE_FIX_DATA_JSON_H_ */
