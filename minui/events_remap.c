/*
.keys files line format:
    <key> <DOWN|UP|LEFT|RIGHT|BACK|ENTER>
    <key> <number>

Empty lines and lines started with '#' is ignered.
Only first two parameters in line is parsed, but use '#' for comments anyway.

Examples:
    72 BACK # remap key with code 72 to KEY_BACK
    128 16  # remap key with code 72 to 15 (KEY_TAB)

The config files will be checked at:
    /sdcard/devices/${device}/${evname}.keys

    /sdcard/devices/.input/Vendor_${XXXX}_Product_${YYYY}_Version_${ZZZZ}.keys
    /sdcard/devices/.input/Vendor_${XXXX}_Product_${YYYY}.keys
    /sdcard/devices/.input/${evname}_${device}.keys
    /sdcard/devices/.input/${evname}.keys
    /sdcard/devices/.input/Generic.keys

    and same for /res/input instead /sdcard/devices/.input

where:
    ${device}   - the device name passed as DEVICE=... kernel command line option

    ${evname}   - input device name

    ${XXXX}     - input device vendor ID
    ${YYYY}     - input device product ID
    ${ZZZZ}     - input device version

See also:
    http://source.android.com/tech/input/key-layout-files.html

*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/input.h>

#include "events_remap.h"

events_remap_map* events_remap_load(const char *filename)
{
    FILE *file;
    char line[128];

    int key_id, key_override;
    char key_name[32];

    events_remap_map* map = NULL;

    EVENTS_REMAP_DEBUG("events_remap_load(%s)\n", filename);

    file = fopen(filename, "r");
    if (file != NULL) {
        map = malloc(sizeof(events_remap_map));

        int count = 0;

        while ( fgets(line, sizeof(line), file) != NULL) {
            int len = strlen(line);

            if (line[len - 1] == '\n') {
                line[--len] = 0;
            }

            if (len == 0) {
                continue;
            }

            if (strncmp(line, "#", 1) == 0) {
                continue;
            }

            if ( sscanf(line, "%d %32s", &key_id, (char *)&key_name) == 2 ) {
                EVENTS_REMAP_DEBUG("ID:%3d name='%s'\n", key_id, key_name);
                key_override = -1;

                if ( isdigit(key_name[0]) ) {
                    key_override = atoi(key_name);
                } else if ( strcmp(key_name, "NONE") == 0 ) {
                    key_override = KEY_RESERVED;
                } else if ( strcmp(key_name, "UP") == 0 ) {
                    key_override = KEY_UP;
                } else if ( strcmp(key_name, "DOWN") == 0 ) {
                    key_override = KEY_DOWN;
                } else if ( strcmp(key_name, "LEFT") == 0 ) {
                    key_override = KEY_LEFT;
                } else if ( strcmp(key_name, "RIGHT") == 0 ) {
                    key_override = KEY_RIGHT;
                } else if ( strcmp(key_name, "BACK") == 0 ) {
                    key_override = KEY_BACK;
                } else if ( strcmp(key_name, "ENTER") == 0 ) {
                    key_override = KEY_ENTER;
                }

                if (key_override >= 0) {
                    (*map)[count].id = key_id;
                    (*map)[count].override = key_override;
                    count++;
                }
            }

            if (count == EVENTS_REMAP_MAX_KEYS) {
                EVENTS_REMAP_DEBUG("Stop loading, maximum reached\n");
                break;
            }
        }

        if (count < EVENTS_REMAP_MAX_KEYS) {
            (*map)[count].id = -1;
        }

        fclose(file);

        if (count == 0) {
            free(map);
            map = NULL;
        }

        EVENTS_REMAP_INFO("Loaded %d overrides form %s\n", count, filename);
    }

    return map;
}

static const char *keys_remap_dirs[] = {
    "/sdcard/devices/.input",
    "/res/input",
//#ifdef EVENTS_REMAP_DEBUG_ON
//    "./.input",
//#endif
    0
};

events_remap_map* events_remap_find(const char *device, int eventfd)
{
    events_remap_map *map;
    char filepath[PATH_MAX];

    char evname[64] = "";
    struct input_id device_info;

    const char **dir;

    if (ioctl(eventfd, EVIOCGNAME(sizeof(evname)), evname) < 0) {
        evname[0] = 0;
    }

    if (ioctl(eventfd, EVIOCGID, &device_info) != 0) {
        device_info.version = 0x0000;
        device_info.vendor  = 0x0000;
        device_info.product = 0x0000;
    }

    if (device) {
        sprintf(filepath, "%s/%s/%s.keys", "/sdcard/devices", device, evname);
        if ((map = events_remap_load(filepath)))
            return map;
    }

    for (dir = keys_remap_dirs; *dir; dir++)
    {
        if (device_info.vendor != 0) {
            sprintf(filepath, "%s/Vendor_%04hx_Product_%04hx_Version_%04hx.keys", 
                *dir, device_info.vendor, device_info.product, device_info.version);
            if ((map = events_remap_load(filepath)))
                return map;

            sprintf(filepath, "%s/Vendor_%04hx_Product_%04hx.keys", 
                *dir, device_info.vendor, device_info.product);
            if ((map = events_remap_load(filepath)))
                return map;
        }

        if (device) {
            sprintf(filepath, "%s/%s_%s.keys", *dir, evname, device);
            if ((map = events_remap_load(filepath)))
                return map;
        }

        if (evname[0]) {
            sprintf(filepath, "%s/%s.keys", *dir, evname);
            if ((map = events_remap_load(filepath)))
                return map;
        }

        sprintf(filepath, "%s/%s.keys", *dir, "Generic");
        if ((map = events_remap_load(filepath)))
            return map;

    }

    return NULL;
}
