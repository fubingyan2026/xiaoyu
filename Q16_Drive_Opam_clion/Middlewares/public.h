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
#include "algorithm/pll/pll.h"
#include "algorithm/math/maths.h"
#include "algorithm/math/user_lib.h"
#include "algorithm/math/utils.h"
#include "algorithm/math/utils_math.h"

/* ===== key_base ===== */
#include "key_base/key_base.h"

/* ===== fsm ===== */
#include "fsm/fsm.h"

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

/* ===== STDLIB ===== */
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
}
#endif

#endif /* MIDDLEWARES_PUBLIC_H */
