#--------------------------------------------------------------
#               K1 Make framework
# Author: Andrea Ravalli (andrea.ravalli@nttdata.com)
#--------------------------------------------------------------

#---------------------
# Source code files
#---------------------
APPLICATION := tracking-application
PACKAGE_ID := 16
PACKAGE_TYPE := software
WITH_OBU_ID := true

SOURCES :=  tolling-gnss-sm.c \
	    service_monitor.c \
	    tolling_gnss_sm_data.c \
	    service_activation_sm.c \
	    connection_sm.c \
	    configuration_store.c \
	    odometer.c \
	    fix_and_status.c \
	    gnss_fixes_transmission_queue.c \
	    message_composer.c \
	    tolling_manager_proxy.c \
	    positioning_service_proxy.c \
	    network_manager_proxy.c \
	    icc_service_proxy.c \
	    application_events.c \
	    application_notifications.c \
	    events_logic.c \
	    gnss_fix_filter.c \
	    tolling_gnss_core_default_factory.c \
        gnss_fix_data.c  \
        trip_id_manager.c \
        tolling_application.c \
        service_configuration.c \
        application_message_composer.c \
        application_gnss_fix_filter.c \
        vas_activation_checker.c

SUBDIRS := #tolling-gnss-core

CPPFLAGS += -DGNSS_TOLLING_CFG=tracking-application.cfg

HAS_CUSTOM_CONFIG := true
WITH_JSON_MAPPER := true
WITH_MQTT_CLIENT := true
WITH_SQLITE := true
WITH_LCM_STATE_MACHINE := true
WITH_HMI_PROXY := true
WITH_ANOMALY_AND_ALERT_NOTIFICATION_FACADE := true

SERVICE_IF_LIBS = 
SERVICE_IF_LIBS_PATH = 
SERVICE_IF_INC_PATH = ./tolling-gnss-core

EXT_IF_LIBS :=  networkmanager \
                positioning \
                vasmanager \
                tollingmanager \
                iccif \
                dsrc \
                powermanager


EXT_IF_LIBS_PATH := ../../network-manager/lib-$(ARCH) \
                    ../../positioning-service/lib-$(ARCH) \
                    ../../vas-manager/lib-$(ARCH) \
                    ../../tolling-manager/lib-$(ARCH) \
                    ../../icc-service/lib-$(ARCH) \
                    ../../dsrc-service/lib-$(ARCH) \
                    ../../power-manager/lib-$(ARCH)


EXT_IF_INC_PATH :=  ../../network-manager/generated \
                    ../../network-manager/include \
                    ../../positioning-service/generated \
                    ../../positioning-service/include \
                    ../../vas-manager/generated \
                    ../../vas-manager/include \
                    ../../tolling-manager/generated \
                    ../../tolling-manager/include \
                    ../../icc-service/generated \
                    ../../icc-service/include \
                    ../../dsrc-service/generated \
                    ../../dsrc-service/include \
                    ../../power-manager/generated \
                    ../../power-manager/include \
                    ../../../hmi-manager/include


EXTERNAL_LIBS := crypto curl m ssl

MKFRW = ../service-framework/makefrw

$(info USE_FRW_DEFAULT_IMPL is $(USE_FRW_DEFAULT_IMPL) )

include $(MKFRW)/build-targets.mk
