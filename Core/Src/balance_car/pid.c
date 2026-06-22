#include "balance_car/pid.h"

void PID_Init(PID_t *p)
{
    p->Target = 0.0f;
    p->Actual = 0.0f;
    p->Actual1 = 0.0f;
    p->Out = 0.0f;
    p->Error0 = 0.0f;
    p->Error1 = 0.0f;
    p->ErrorInt = 0.0f;
}

void PID_Update(PID_t *p)
{
    p->Error1 = p->Error0;
    p->Error0 = p->Target - p->Actual;

    if (p->Ki != 0.0f) {
        p->ErrorInt += p->Error0;
        if (p->ErrorInt > p->ErrorIntMax) {
            p->ErrorInt = p->ErrorIntMax;
        }
        if (p->ErrorInt < p->ErrorIntMin) {
            p->ErrorInt = p->ErrorIntMin;
        }
    } else {
        p->ErrorInt = 0.0f;
    }

    p->Out = p->Kp * p->Error0
           + p->Ki * p->ErrorInt
           - p->Kd * (p->Actual - p->Actual1);

    if (p->Out > 0.0f) {
        p->Out += p->OutOffset;
    }
    if (p->Out < 0.0f) {
        p->Out -= p->OutOffset;
    }

    if (p->Out > p->OutMax) {
        p->Out = p->OutMax;
    }
    if (p->Out < p->OutMin) {
        p->Out = p->OutMin;
    }

    p->Actual1 = p->Actual;
}
