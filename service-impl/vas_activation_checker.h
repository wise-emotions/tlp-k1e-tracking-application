/*  vas_activation_checker.h
 */
#ifndef __VAS_ACTIVATION_CHECKER_H__
#define __VAS_ACTIVATION_CHECKER_H__

#include <glib.h>
#include "icustom_activation_checker.h"

typedef struct _VasActivationChecker VasActivationChecker;
typedef struct _VasActivationCheckerPriv  VasActivationCheckerPriv;

struct _VasActivationChecker {
    ICustomActivationChecker impl;
    VasActivationCheckerPriv *priv;

    void (*ctor)(
        VasActivationChecker            *self,
        guint                            service_id
    );
    
    void (*dtor)(
        VasActivationChecker            *self
    );
};
extern VasActivationChecker *VasActivationChecker_new(void);


#endif /*__VAS_ACTIVATION_CHECKER_H__*/
