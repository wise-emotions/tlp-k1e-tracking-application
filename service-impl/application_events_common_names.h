#pragma once

#define EVENT_TOLLING_MANAGER_PROXY_CONFIGURED_FROM_REMOTE              "TollingManagerProxy_condfigured_from_remote"
#define EVENT_TOLLING_MANAGER_PROXY_VEHICLE_CONFIGURED_FROM_REMOTE      "TollingManagerProxy_vehicle_configured_from_remote"
#define EVENT_TOLLING_MANAGER_PROXY_SERVICE_ACTIVATED_FROM_REMOTE       "TollingManagerProxy_service_activated_from_remote"
#define EVENT_TOLLING_MANAGER_PROXY_SERVICE_DEACTIVATED_FROM_REMOTE     "TollingManagerProxy_service_deactivated_from_remote"
#define EVENT_TOLLING_MANAGER_PROXY_OTHER_ACTIVE_SERVICES_PRESENT       "TollingManagerProxy_other_active_services_present"
#define EVENT_TOLLING_MANAGER_PROXY_OTHER_ACTIVE_SERVICES_ABSENT        "TollingManagerProxy_other_active_services_absent"
#define EVENT_TOLLING_MANAGER_PROXY_ENTER_GNSS_DOMAIN                   "TollingManagerProxy_enter_gnss_domain"
#define EVENT_TOLLING_MANAGER_PROXY_EXIT_GNSS_DOMAIN                    "TollingManagerProxy_exit_gnss_domain"
#define EVENT_TOLLING_MANAGER_PROXY_ENTER_CCC_DOMAIN                    "TollingManagerProxy_enter_ccc_domain"
#define EVENT_TOLLING_MANAGER_PROXY_EXIT_CCC_DOMAIN                     "TollingManagerProxy_exit_ccc_domain"
#define EVENT_TOLLING_MANAGER_PROXY_MAX_PACKET_PROPERTY_UPDATE          "TollingManagerProxy_max_packet_property_update"

#define EVENT_POSITIONING_SERVICE_PROXY_POSITION_UPDATED                "PositioningServiceProxy_position_updated"

#define EVENT_NETWORK_MANAGER_PROXY_NETWORK_CONNECTED                   "NetworkManager_network_connected"
#define EVENT_NETWORK_MANAGER_PROXY_NETWORK_NOT_CONNECTED               "NetworkManager_network_not_connected"

#define EVENT_ICC_SERVICE_PROXY_TAMPERING_ATTEMPTED                     "IccServiceProxy_tampering_attempted"
#define EVENT_ICC_SERVICE_PROXY_TAMPERING_RESET                         "IccServiceProxy_tampering_reset"

#define EVENT_DSRC_SERVICE_PROXY_GO                                     "DsrcServiceProxy_go"
#define EVENT_DSRC_SERVICE_PROXY_NOGO                                   "DsrcServiceProxy_nogo"

#define EVENT_EVENTS_LOGIC_SERVICE_STATUS_ACTIVE                        "EventsLogic_service_status_active"
#define EVENT_EVENTS_LOGIC_SERVICE_STATUS_NOT_ACTIVE                    "EventsLogic_service_status_not_active"
#define EVENT_EVENTS_LOGIC_SERVICE_STATUS_NO_GO                         "EventsLogic_service_status_no_go"

#define EVENT_EVENTS_LOGIC_GENERIC_ANOMALY_PRESENT                      "EventsLogic_generic_anomaly_present"
#define EVENT_EVENTS_LOGIC_GENERIC_ANOMALY_ABSENT                       "EventsLogic_generic_anomaly_absent"
#define EVENT_EVENTS_LOGIC_NETWORK_ANOMALY_PRESENT                      "EventsLogic_network_anomaly_present"
#define EVENT_EVENTS_LOGIC_NETWORK_ANOMALY_ABSENT                       "EventsLogic_network_anomaly_absent"
#define EVENT_EVENTS_LOGIC_GNSS_ANOMALY_PRESENT                         "EventsLogic_gnss_anomaly_present"
#define EVENT_EVENTS_LOGIC_GNSS_ANOMALY_ABSENT                          "EventsLogic_gnss_anomaly_absent"

#define EVENT_TOLLING_GNSS_SM_START                                     "tolling_gnss_sm_start"
#define EVENT_TOLLING_GNSS_SM_ON_HOLD                                   "tolling_gnss_sm_on_hold"
#define EVENT_TOLLING_GNSS_SM_ON_STOP                                   "tolling_gnss_sm_on_stop"

#define EVENT_GNSS_FIXES_CUT_OFF                                        "GnssFixesTrasmissionQueue_cut_off"

