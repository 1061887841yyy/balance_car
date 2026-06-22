#ifndef BALANCE_CAR_PID_H
#define BALANCE_CAR_PID_H

typedef struct {
    float Target;       /* Ozone: PID目标值。角度环=目标角度；速度环=目标速度；转向环=目标左右速度差 */
    float Actual;       /* Ozone: PID实际值。角度环=当前角度；速度环=平均速度；转向环=左右速度差 */
    float Actual1;      /* Ozone: 上一次实际值，用于微分先行D项，一般只观察不用手改 */
    float Out;          /* Ozone: PID输出。角度环输出到平均PWM；速度环输出到角度目标；转向环输出到差分PWM */

    float Kp;           /* Ozone: 比例系数。先调Kp，让车有足够扶正力 */
    float Ki;           /* Ozone: 积分系数。最后少量加入，用来消除长期偏差 */
    float Kd;           /* Ozone: 微分系数。Kp能扶正后再加Kd，用来抑制来回摆动 */

    float Error0;       /* Ozone: 当前误差=Target-Actual，可观察调参方向是否合理 */
    float Error1;       /* Ozone: 上一次误差，一般只观察不用手改 */
    float ErrorInt;     /* Ozone: 误差积分。若变得很大，先把Ki设0或请求reset_pid */

    float ErrorIntMax;  /* Ozone: 积分上限，防止积分项越积越大 */
    float ErrorIntMin;  /* Ozone: 积分下限，防止积分项越积越大 */

    float OutMax;       /* Ozone: 输出最大限幅 */
    float OutMin;       /* Ozone: 输出最小限幅 */

    float OutOffset;    /* Ozone: 输出死区补偿。电机小PWM不动时少量加，一开始可设0 */
} PID_t;

void PID_Init(PID_t *p);
void PID_Update(PID_t *p);

#endif
