/*****************************************************************************
* Model: bwgpsrecorder.qm
* File:  qmcode/qgps.c
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
/* @(/3/13) ................................................................*/
#include <qp_port.h>
#include <qevents.h>
#include <bsp.h>
#include <gps.h>
#include <protocol.h>
#include <utility.h>
#include <error.h>
#include <exception.h>
#include "type.h"

Q_DEFINE_THIS_MODULE("qgps.c")

#define TIMEOUT_PWR_ON_DELAY   (BSP_TICKS_PER_SEC)  //开机延时，0.5s
#define TIMEOUT_RST_DELAY      (BSP_TICKS_PER_SEC >> 5)  //复位延时，25ms
#define TIMEOUT_GPS_RPT        (BSP_TICKS_PER_SEC * 30)  //GPS点位上报周期
#define TIMEOUT_RETRIEVE       (BSP_TICKS_PER_SEC)       //提取周期
#define TIMEOUT_DISALLOCATED   (5 * 60) //不定位时长，5分钟

/* Active object class -----------------------------------------------------*/
/* @(/1/12) ................................................................*/
typedef struct QGpsTag {
/* protected: */
    QActive super;

/* private: */
    QTimeEvt m_Timer;
    QTimeEvt m_rptTimer;
    uint8_t m_ticks;
    uint8_t m_disallocatedTicks;
} QGps;

/* protected: */
static QState QGps_initial(QGps * const me, QEvt const * const e);
static QState QGps_servicing(QGps * const me, QEvt const * const e);
static QState QGps_activing(QGps * const me, QEvt const * const e);
static QState QGps_handling(QGps * const me, QEvt const * const e);
static QState QGps_pwron_delay(QGps * const me, QEvt const * const e);
static QState QGps_reset_delay(QGps * const me, QEvt const * const e);
static QState QGps_Sleep_Mode(QGps * const me, QEvt const * const e);



/* Local objects -----------------------------------------------------------*/
static QGps l_Gps; /* the single instance of the Table active object */

/* Global-scope objects ----------------------------------------------------*/
QActive * const AO_Gps = &l_Gps.super; /* "opaque" AO pointer */

/*..........................................................................*/
/* @(/1/22) ................................................................*/
void QGps_ctor(void) {
    QGps *me = &l_Gps;
    QActive_ctor(&me->super, Q_STATE_CAST(&QGps_initial));
}
/* @(/1/12) ................................................................*/
/* @(/1/12/4) ..............................................................*/
/* @(/1/12/4/0) */
static QState QGps_initial(QGps * const me, QEvt const * const e) {
    QActive_subscribe(&me->super, ACC_ON_SIG);
    QActive_subscribe(&me->super, ACC_OFF_SIG);
    QActive_subscribe(&me->super, WAKEUP_REQ_SIG);
    QActive_subscribe(&me->super, SLEEP_REQ_SIG);

    GPS_Init();
    GPS_LED_OFF;

    me->m_ticks = 0;
    me->m_disallocatedTicks = 0;
    return Q_TRAN(&QGps_servicing);
}
/* @(/1/12/4/1) ............................................................*/
static QState QGps_servicing(QGps * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /* @(/1/12/4/1/0) */
        case Q_INIT_SIG: {
            status_ = Q_TRAN(&QGps_activing);
            break;
        }
        /* @(/1/12/4/1/1) */
        case GPS_ANTENA_ONCHANGE_SIG: {
            //检测GPS天线
            u8 state = (u8)GPS_GETANTENASTATE();
            if(state == 0)
            {
                //天线正常，清除天线故障类型
                if(CheckSpecifiedErrTypeExisted(GPS_CLASS, GPS_FAULT_ANTENA))
                {
                    ClearErrType(GPS_CLASS, GPS_FAULT_ANTENA);
                }

                GPS_SetAntenaState(ANTENA_OK);
            }
            else
            {
                //设置故障类型
                SetErrType(GPS_CLASS, GPS_FAULT_ANTENA);
                GPS_SetAntenaState(ANTENA_ERR);
            }

            TRACE_(QS_USER, NULL, "[GPS] antena state is %d", state);
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/* @(/1/12/4/1/2) ..........................................................*/
static QState QGps_activing(QGps * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /* @(/1/12/4/1/2) */
        case Q_ENTRY_SIG: {
            GPS_POWER_ON; //开电
            GPS_RST_PULLUP;
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/12/4/1/2/0) */
        case Q_INIT_SIG: {
            status_ = Q_TRAN(&QGps_pwron_delay);
            break;
        }
        /* @(/1/12/4/1/2/1) */
        case SLEEP_REQ_SIG: {
            GPS_POWER_OFF;
            GPS_LED_OFF;
            TRACE_(QS_USER, NULL, "[GPS] ----- PowerMgr: SLEEP -----");
            status_ = Q_TRAN(&QGps_Sleep_Mode);
            break;
        }
        default: {
            status_ = Q_SUPER(&QGps_servicing);
            break;
        }
    }
    return status_;
}
/* @(/1/12/4/1/2/2) ........................................................*/
static QState QGps_handling(QGps * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /* @(/1/12/4/1/2/2) */
        case Q_ENTRY_SIG: {
            QTimeEvt_postEvery(&me->m_Timer, &me->super, TIMEOUT_RETRIEVE);
            QTimeEvt_postEvery(&me->m_rptTimer, &me->super, TIMEOUT_GPS_RPT);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/12/4/1/2/2) */
        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->m_Timer);
            QTimeEvt_disarm(&me->m_rptTimer);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/12/4/1/2/2/0) */
        case Q_TIMEOUT_SIG: {
            if(me->m_ticks++ % 3 == 0)
            {
                if(GPS_IsLocated())
                {
                    GPS_FLASH_LED;
                    me->m_disallocatedTicks = 0;
                }
                else
                {
                    GPS_LED_OFF;

                    me->m_disallocatedTicks ++;
                    if(me->m_disallocatedTicks > TIMEOUT_DISALLOCATED)
                    {
                        SetErrType(GPS_CLASS, GPS_FAULT_INVALID);
                    }
                }
            }

            GPS_HandleMsg(); //GPS信息解析处理

            GPS_DumpInfo(); //GPS信息存储到SD卡
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/12/4/1/2/2/1) */
        case GPS_RPT_TIMEOUT_SIG: {
            GPS_ReportGpsInfo(); //上报GPS点位信息
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&QGps_activing);
            break;
        }
    }
    return status_;
}
/* @(/1/12/4/1/2/3) ........................................................*/
static QState QGps_pwron_delay(QGps * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /* @(/1/12/4/1/2/3) */
        case Q_ENTRY_SIG: {
            QTimeEvt_postIn(&me->m_Timer, &me->super, TIMEOUT_PWR_ON_DELAY);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/12/4/1/2/3) */
        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->m_Timer);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/12/4/1/2/3/0) */
        case Q_TIMEOUT_SIG: {
            TRACE_(QS_USER, NULL, "[GPS] poweron_delay timeout");
            status_ = Q_TRAN(&QGps_reset_delay);
            break;
        }
        default: {
            status_ = Q_SUPER(&QGps_activing);
            break;
        }
    }
    return status_;
}
/* @(/1/12/4/1/2/4) ........................................................*/
static QState QGps_reset_delay(QGps * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /* @(/1/12/4/1/2/4) */
        case Q_ENTRY_SIG: {
            GPS_RST_PULLDOWN;
            QTimeEvt_postIn(&me->m_Timer, &me->super, TIMEOUT_RST_DELAY);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/12/4/1/2/4) */
        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->m_Timer);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/12/4/1/2/4/0) */
        case Q_TIMEOUT_SIG: {
            GPS_RST_PULLUP;
            TRACE_(QS_USER, NULL, "[GPS] RST delay timeout");
            status_ = Q_TRAN(&QGps_handling);
            break;
        }
        default: {
            status_ = Q_SUPER(&QGps_activing);
            break;
        }
    }
    return status_;
}
/* @(/1/12/4/1/3) ..........................................................*/
static QState QGps_Sleep_Mode(QGps * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /* @(/1/12/4/1/3) */
        case Q_ENTRY_SIG: {
            GPS_POWER_OFF; //开电
            SetSysSleepState(MODULE_SLEEP_STATE_GPS);//设置模块睡眠态标志
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/12/4/1/3/0) */
        case WAKEUP_REQ_SIG: {
            TRACE_(QS_USER, NULL, "[GPS] ----- PowerMgr: WAKEUP -----");
            ClearSysSleepState(MODULE_SLEEP_STATE_GPS);//清除模块休眠态
            status_ = Q_TRAN(&QGps_activing);
            break;
        }
        default: {
            status_ = Q_SUPER(&QGps_servicing);
            break;
        }
    }
    return status_;
}

