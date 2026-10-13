#ifndef _DEVICE_P_H_
#define _DEVICE_P_H_

/*
    This file was added by dethrace.
    Nasty hack to provide device-specific types to external consumers of BRender
*/

#include "compiler.h"
#include "pixelmap.h"

// For use with `virtualframebuffer` device.
typedef void br_device_virtualfb_swapbuffers_cbfn(br_pixelmap* pm);
typedef void br_device_virtualfb_palette_changed_cbfn(br_uint_32* palette_entries);

typedef struct br_device_virtualfb_callback_procs {
    br_device_virtualfb_palette_changed_cbfn      *palette_changed;
    br_device_virtualfb_swapbuffers_cbfn          *swap_buffers;
} br_device_virtualfb_callback_procs;

// For use with `glrend` device.
typedef void BR_CALLBACK br_device_gl_swapbuffers_cbfn(br_pixelmap* pm);
typedef void* BR_CALLBACK br_device_gl_getprocaddress_cbfn(const char* name);
typedef void BR_CALLBACK br_device_gl_free_cbfn(br_pixelmap* pm, void* user);
typedef void BR_CALLBACK br_device_gl_getviewport_cbfn(int *x, int *y, float *width_multiplier, float *height_multiplier);

typedef struct br_device_gl_callback_procs {
    br_device_gl_getprocaddress_cbfn      *get_proc_address;
    br_device_gl_getviewport_cbfn         *get_viewport;
    br_device_gl_swapbuffers_cbfn         *swap_buffers;
    br_device_gl_free_cbfn                *free;
} br_device_gl_callback_procs;

typedef br_boolean BR_CALLBACK br_device_vkrend_load_library_cbfn(const char* path);
typedef void BR_CALLBACK br_device_vkrend_unload_library_cbfn(void);
typedef void* BR_CALLBACK br_device_vkrend_get_vk_get_instance_proc_addr_cbfn(void);
typedef const char* BR_CALLBACK br_device_vkrend_get_error_cbfn(void);
typedef br_boolean BR_CALLBACK br_device_vkrend_get_instance_extensions_cbfn(void* window, br_uint_32* count, const char** names, br_uint_32 max_names);
typedef br_boolean BR_CALLBACK br_device_vkrend_create_surface_cbfn(void* window, void* instance, void** surface);

typedef struct br_device_vkrend_callback_procs {
    br_uint_32 size;
    void* window;
    br_device_vkrend_load_library_cbfn* load_library;
    br_device_vkrend_unload_library_cbfn* unload_library;
    br_device_vkrend_get_vk_get_instance_proc_addr_cbfn* get_vk_get_instance_proc_addr;
    br_device_vkrend_get_error_cbfn* get_error;
    br_device_vkrend_get_instance_extensions_cbfn* get_instance_extensions;
    br_device_vkrend_create_surface_cbfn* create_surface;
} br_device_vkrend_callback_procs;

#endif
