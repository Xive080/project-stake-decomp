// Tablas de las funciones pendientes de jefes 3 y 4 (generadas por tools/gen_tables.py).
#pragma once
#include <cstdint>
namespace vn { namespace bosspend {
struct B4ShoukanSlot { float x, y, z; int32_t angle; };   // D_009D3D30 (24 B por entrada; solo x,y,z y el angulo +0x10)
struct B3MotionEntry { int32_t anim, length; };           // D_009D0880 (8 B)
extern const uint32_t kB4StateRows[6][3];
extern const int8_t kB4IwaCount[2][17];
extern const float kB4IwaSpeed[2][17];
extern const B4ShoukanSlot kB4ShoukanSlot[7];
extern const float kB4PathShift[7][3];
extern const float kB4IwaOrigin[25][3];
extern const float kB4BunshinSpread[3][2];
extern const int32_t kB4KiriPreAngle[2][2];
extern const int32_t kB4KiriPreAngleAdd[2];
extern const int32_t kB4KiriPreMotion[2];
extern const float kB4KiriY[3];
extern const int32_t kB4KiriAngle[3];
extern const int32_t kB4KiriShoukanMotion[3];
extern const uint8_t kB4BunshinMotAction[19];
extern const int32_t kB4BunshinMotRows[3][2][5];
extern const uint8_t kB4BunshinB2Class[19];
extern const B3MotionEntry kB3MotionFull[13];
}}
