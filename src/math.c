#include "focus_math.h"
#include <math.h>
void fm_quaternion_from_yaw(float yaw_radians, float out_xyzw[4]) {
    float half = yaw_radians * 0.5f;
    out_xyzw[0] = 0.0f;
    out_xyzw[1] = sinf(half);
    out_xyzw[2] = 0.0f;
    out_xyzw[3] = cosf(half);
}
