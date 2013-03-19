#ifndef _EVENTS_REMAP_H_
#define _EVENTS_REMAP_H_

#define EVENTS_REMAP_MAX_KEYS 16

struct events_remap_key {
    int id;
    int override;
};

typedef struct events_remap_key events_remap_map[EVENTS_REMAP_MAX_KEYS];

events_remap_map* events_remap_load(const char *filename);
events_remap_map* events_remap_find(const char *device, int eventfd);

#ifdef EVENTS_REMAP_DEBUG_ON
#define EVENTS_REMAP_DEBUG(fmt, args...) printf("[EVENTS_REMAP D] "fmt, ## args)
#else
#define EVENTS_REMAP_DEBUG(fmt, args...) ({})
#endif

#define EVENTS_REMAP_INFO(fmt, args...) printf("[EVENTS_REMAP I] "fmt, ## args)

#endif
