#ifndef MIDDLEWARES_PUBLIC_H
#define MIDDLEWARES_PUBLIC_H

#ifdef __cplusplus
extern "C" {
#endif

/* ===== Algorithm ===== */
#include "algorithm/controller/gimbal_pid.h"
#include "algorithm/controller/pid.h"
#include "algorithm/crc.h"
#include "algorithm/filter/filter.h"
#include "algorithm/joint_pos.h"
#include "algorithm/line_hall_pll.h"
#include "algorithm/math/maths.h"
#include "algorithm/math/user_lib.h"
#include "algorithm/math/utils.h"
#include "algorithm/math/utils_math.h"

/* ===== HMI ===== */
#include "hmi/key_base.h"

/* ===== Logic ===== */
#include "logic/fsm.h"

/* ===== Message Center ===== */
#include "message_center/message_center.h"

/* ===== Protocol ===== */
#include "protocol/protocol_packer.h"
#include "protocol/protocol_parser.h"

/* ===== Services ===== */
#include "services/can_comm/can_comm.h"
#include "services/daemon/daemon.h"
#include "services/debug/debug.h"
#include "services/led/led.h"

/* ===== Utils ===== */
#include "utils/kfifo.h"
#include "utils/memory_pool.h"
#include "utils/toolkit/toolkit.h"

#ifdef __cplusplus
}
#endif

#endif /* MIDDLEWARES_PUBLIC_H */
