#ifndef _DRV_H_
#define _DRV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>

#define BR_DEVICE_PRIVATE
#define BR_OUTPUT_FACILITY_PRIVATE
#define BR_DEVICE_PIXELMAP_PRIVATE
#define BR_RENDERER_FACILITY_PRIVATE
#define BR_RENDERER_PRIVATE
#define BR_RENDERER_STATE_STORED_PRIVATE
#define BR_BUFFER_STORED_PRIVATE
#define BR_GEOMETRY_V1_MODEL_PRIVATE
#define BR_GEOMETRY_STORED_PRIVATE
#define BR_DEVICE_CLUT_PRIVATE

#include "brddi.h"
#include "fw_p.h"
#include "brvkrend.h"
#include "formats.h"
#include "pm.h"

#include "vkshaders.h"
#include "video.h"
#include "template.h"
#include "device.h"
#include "devclut.h"
#include "outfcty.h"
#include "devpixmp.h"
#include "rendfcty.h"
#include "state.h"
#include "renderer.h"
#include "sstate.h"
#include "sbuffer.h"
#include "gv1model.h"
#include "gstored.h"
#include "gpu.h"

#define BRT(t) BRT_##t, 0
#define DEV(t) 0, #t

#ifndef _NO_PROTOTYPES
#include "drv_ip.h"
#endif

#ifdef __cplusplus
};
#endif
#endif
