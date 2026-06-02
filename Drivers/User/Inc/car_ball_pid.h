#ifndef __CAR_BALL_PID_H
#define __CAR_BALL_PID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void car_ball_pid_init(void);
void car_ball_pid_track_from_uart(void);

#ifdef __cplusplus
}
#endif

#endif
