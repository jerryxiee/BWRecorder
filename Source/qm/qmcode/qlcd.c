/*****************************************************************************
* Model: bwgpsrecorder.qm
* File:  qmcode/qlcd.c
*
* This code has been generated by QM tool (see state-machine.com/qm).
* DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*****************************************************************************/
/* @(/3/2) .................................................................*/
#include "qp_port.h"
#include "qevents.h"
#include "bsp.h"
#include "type.h"

Q_DEFINE_THIS_MODULE("qlcd.c")



/* Active object class -----------------------------------------------------*/
/* @(/1/1) .................................................................*/
typedef struct QLCDTag {
/* protected: */
    QActive super;
} QLCD;

/* protected: */
static QState QLCD_initial(QLCD * const me, QEvt const * const e);
static QState QLCD_serving(QLCD * const me, QEvt const * const e);



/* Local objects -----------------------------------------------------------*/
static QLCD l_QLCD; /* the single instance of the Table active object */

/* Global-scope objects ----------------------------------------------------*/
QActive * const AO_LCD = &l_QLCD.super; /* "opaque" AO pointer */

/*..........................................................................*/
/* @(/1/28) ................................................................*/
void QLCD_ctor(void) {
    QLCD *me = &l_QLCD;
    QActive_ctor(&me->super, Q_STATE_CAST(&QLCD_initial));
}
/* @(/1/1) .................................................................*/
/* @(/1/1/0) ...............................................................*/
/* @(/1/1/0/0) */
static QState QLCD_initial(QLCD * const me, QEvt const * const e) {
    return Q_TRAN(&QLCD_serving);
}
/* @(/1/1/0/1) .............................................................*/
static QState QLCD_serving(QLCD * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}

