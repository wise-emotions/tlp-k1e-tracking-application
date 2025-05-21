#--------------------------------------------------------------
#               K1 Make framework
# Author: Andrea Ravalli (andrea.ravalli@nttdata.com)
#--------------------------------------------------------------


#---------------------
# service configuration
#---------------------
SUBDIRS = service-impl

#---------------------
# platform config
#---------------------
ARCH := armv7
MKFRW = ./service-framework/makefrw

#---------------------
# platform config
#---------------------
#src: interfaces

include $(MKFRW)/build.mk 
