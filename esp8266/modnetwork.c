/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Paul Sokolovsky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "py/nlr.h"
#include "py/objlist.h"
#include "py/runtime.h"
#include "netutils.h"
#include "queue.h"
#include "user_interface.h"
#include "espconn.h"
#include "spi_flash.h"
#include "utils.h"

#define MODNETWORK_INCLUDE_CONSTANTS (1)

typedef struct _wlan_if_obj_t {
    mp_obj_base_t base;
    int if_id;
} wlan_if_obj_t;

void error_check(bool status, const char *msg);
const mp_obj_type_t wlan_if_type;

STATIC const wlan_if_obj_t wlan_objs[] = {
    {{&wlan_if_type}, STATION_IF},
    {{&wlan_if_type}, SOFTAP_IF},
};

STATIC void require_if(mp_obj_t wlan_if, int if_no) {
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(wlan_if);
    if (self->if_id != if_no) {
        error_check(false, "STA required");
    }
}

STATIC mp_obj_t get_wlan(mp_uint_t n_args, const mp_obj_t *args) {
    int idx = 0;
    if (n_args > 0) {
        idx = mp_obj_get_int(args[0]);
    }
    return MP_OBJ_FROM_PTR(&wlan_objs[idx]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(get_wlan_obj, 0, 1, get_wlan);

STATIC mp_obj_t esp_active(mp_uint_t n_args, const mp_obj_t *args) {
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    uint32_t mode = wifi_get_opmode();
    if (n_args > 1) {
        int mask = self->if_id == STATION_IF ? STATION_MODE : SOFTAP_MODE;
        if (mp_obj_get_int(args[1]) != 0) {
            mode |= mask;
        } else {
            mode &= ~mask;
        }
        error_check(wifi_set_opmode(mode), "Cannot update i/f status");
        return mp_const_none;
    }

    // Get active status
    if (self->if_id == STATION_IF) {
        return mp_obj_new_bool(mode & STATION_MODE);
    } else {
        return mp_obj_new_bool(mode & SOFTAP_MODE);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(esp_active_obj, 1, 2, esp_active);

STATIC mp_obj_t esp_connect(mp_uint_t n_args, const mp_obj_t *args) {
    require_if(args[0], STATION_IF);
    struct station_config config = {{0}};
    mp_uint_t len;
    const char *p;

    p = mp_obj_str_get_data(args[1], &len);
    memcpy(config.ssid, p, len);
    p = mp_obj_str_get_data(args[2], &len);
    memcpy(config.password, p, len);

    error_check(wifi_station_set_config(&config), "Cannot set STA config");
    error_check(wifi_station_connect(), "Cannot connect to AP");

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(esp_connect_obj, 3, 7, esp_connect);

STATIC mp_obj_t esp_disconnect(mp_obj_t self_in) {
    require_if(self_in, STATION_IF);
    error_check(wifi_station_disconnect(), "Cannot disconnect from AP");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp_disconnect_obj, esp_disconnect);

STATIC mp_obj_t esp_status(mp_obj_t self_in) {
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->if_id == STATION_IF) {
        return MP_OBJ_NEW_SMALL_INT(wifi_station_get_connect_status());
    }
    return MP_OBJ_NEW_SMALL_INT(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp_status_obj, esp_status);

STATIC void esp_scan_cb(scaninfo *si, STATUS status) {
    struct bss_info *bs;
    if (si->pbss) {
        STAILQ_FOREACH(bs, si->pbss, next) {
            mp_obj_tuple_t *t = mp_obj_new_tuple(6, NULL);
            t->items[0] = mp_obj_new_bytes(bs->ssid, strlen((char*)bs->ssid));
            t->items[1] = mp_obj_new_bytes(bs->bssid, sizeof(bs->bssid));
            t->items[2] = MP_OBJ_NEW_SMALL_INT(bs->channel);
            t->items[3] = MP_OBJ_NEW_SMALL_INT(bs->rssi);
            t->items[4] = MP_OBJ_NEW_SMALL_INT(bs->authmode);
            t->items[5] = MP_OBJ_NEW_SMALL_INT(bs->is_hidden);
            call_function_1_protected(MP_STATE_PORT(scan_cb_obj), t);
        }
    }
}

STATIC mp_obj_t esp_scan(mp_obj_t self_in, mp_obj_t cb_in) {
    MP_STATE_PORT(scan_cb_obj) = cb_in;
    if (wifi_get_opmode() == SOFTAP_MODE) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, 
            "Scan not supported in AP mode"));
    }
    wifi_station_scan(NULL, (scan_done_cb_t)esp_scan_cb);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(esp_scan_obj, esp_scan);

/// \method isconnected()
/// Return True if connected to an AP and an IP address has been assigned,
///     false otherwise.
STATIC mp_obj_t esp_isconnected(mp_obj_t self_in) {
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->if_id == STATION_IF) {
        if (wifi_station_get_connect_status() == STATION_GOT_IP) {
            return mp_const_true;
        }
    } else {
        if (wifi_softap_get_station_num() > 0) {
            return mp_const_true;
        }
    }
    return mp_const_false;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp_isconnected_obj, esp_isconnected);

STATIC mp_obj_t esp_mac(mp_uint_t n_args, const mp_obj_t *args) {
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    uint8_t mac[6];
    if (n_args == 1) {
        wifi_get_macaddr(self->if_id, mac);
        return mp_obj_new_bytes(mac, sizeof(mac));
    } else {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);

        if (bufinfo.len != 6) {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError,
                "invalid buffer length"));
        }

        wifi_set_macaddr(self->if_id, bufinfo.buf);
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(esp_mac_obj, 1, 2, esp_mac);

STATIC mp_obj_t esp_ifconfig(mp_obj_t self_in) {
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(self_in);
    struct ip_info info;
    wifi_get_ip_info(self->if_id, &info);
    mp_obj_t ifconfig[4] = {
            netutils_format_ipv4_addr((uint8_t*)&info.ip, NETUTILS_BIG),
            netutils_format_ipv4_addr((uint8_t*)&info.netmask, NETUTILS_BIG),
            netutils_format_ipv4_addr((uint8_t*)&info.gw, NETUTILS_BIG),
            MP_OBJ_NEW_QSTR(MP_QSTR_), // no DNS server
    };
    return mp_obj_new_tuple(4, ifconfig);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp_ifconfig_obj, esp_ifconfig);

STATIC mp_obj_t esp_config(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    if (n_args != 1 && kwargs->used != 0) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError,
            "either pos or kw args are allowed"));
    }

    wlan_if_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    union {
        struct station_config sta;
        struct softap_config ap;
    } cfg;

    if (self->if_id == STATION_IF) {
        error_check(wifi_station_get_config(&cfg.sta), "can't get STA config");
    } else {
        error_check(wifi_softap_get_config(&cfg.ap), "can't get AP config");
    }

    if (kwargs->used != 0) {

        for (mp_uint_t i = 0; i < kwargs->alloc; i++) {
            if (MP_MAP_SLOT_IS_FILLED(kwargs, i)) {
                #define QS(x) (uintptr_t)MP_OBJ_NEW_QSTR(x)
                switch ((uintptr_t)kwargs->table[i].key) {
                    case QS(MP_QSTR_essid): {
                        mp_uint_t len;
                        const char *s = mp_obj_str_get_data(kwargs->table[i].value, &len);
                        len = MIN(len, sizeof(cfg.ap.ssid));
                        memcpy(cfg.ap.ssid, s, len);
                        cfg.ap.ssid_len = len;
                        break;
                    }
                    default:
                        goto unknown;
                }
                #undef QS
            }
        }

        if (self->if_id == STATION_IF) {
            error_check(wifi_station_set_config(&cfg.sta), "can't set STA config");
        } else {
            error_check(wifi_softap_set_config(&cfg.ap), "can't set AP config");
        }

        return mp_const_none;
    }

    // Get config

    if (n_args != 2) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError,
            "can query only one param"));
    }

    #define QS(x) (uintptr_t)MP_OBJ_NEW_QSTR(x)
    switch ((uintptr_t)args[1]) {
        case QS(MP_QSTR_essid):
            return mp_obj_new_str((char*)cfg.ap.ssid, cfg.ap.ssid_len, false);
    }
    #undef QS

unknown:
    nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError,
        "unknown config param"));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(esp_config_obj, 1, esp_config);

STATIC const mp_map_elem_t wlan_if_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_active), (mp_obj_t)&esp_active_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_connect), (mp_obj_t)&esp_connect_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_disconnect), (mp_obj_t)&esp_disconnect_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_status), (mp_obj_t)&esp_status_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_scan), (mp_obj_t)&esp_scan_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_isconnected), (mp_obj_t)&esp_isconnected_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_mac), (mp_obj_t)&esp_mac_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_config), (mp_obj_t)&esp_config_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ifconfig), (mp_obj_t)&esp_ifconfig_obj },
};

STATIC MP_DEFINE_CONST_DICT(wlan_if_locals_dict, wlan_if_locals_dict_table);

const mp_obj_type_t wlan_if_type = {
    { &mp_type_type },
    .name = MP_QSTR_WLAN,
    .locals_dict = (mp_obj_t)&wlan_if_locals_dict,
};

STATIC mp_obj_t esp_wifi_mode(mp_uint_t n_args, const mp_obj_t *args) {
    if (n_args == 0) {
        return mp_obj_new_int(wifi_get_opmode());
    } else {
        wifi_set_opmode(mp_obj_get_int(args[0]));
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(esp_wifi_mode_obj, 0, 1, esp_wifi_mode);

STATIC mp_obj_t esp_phy_mode(mp_uint_t n_args, const mp_obj_t *args) {
    if (n_args == 0) {
        return mp_obj_new_int(wifi_get_phy_mode());
    } else {
        wifi_set_phy_mode(mp_obj_get_int(args[0]));
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(esp_phy_mode_obj, 0, 1, esp_phy_mode);

STATIC const mp_map_elem_t mp_module_network_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_network) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_WLAN), (mp_obj_t)&get_wlan_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_wifi_mode), (mp_obj_t)&esp_wifi_mode_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_phy_mode), (mp_obj_t)&esp_phy_mode_obj },

#if MODNETWORK_INCLUDE_CONSTANTS
    { MP_OBJ_NEW_QSTR(MP_QSTR_STAT_IDLE),
        MP_OBJ_NEW_SMALL_INT(STATION_IDLE)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_STAT_CONNECTING),
        MP_OBJ_NEW_SMALL_INT(STATION_CONNECTING)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_STAT_WRONG_PASSWORD),
        MP_OBJ_NEW_SMALL_INT(STATION_WRONG_PASSWORD)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_STAT_NO_AP_FOUND),
        MP_OBJ_NEW_SMALL_INT(STATION_NO_AP_FOUND)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_STAT_CONNECT_FAIL),
        MP_OBJ_NEW_SMALL_INT(STATION_CONNECT_FAIL)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_STAT_GOT_IP),
        MP_OBJ_NEW_SMALL_INT(STATION_GOT_IP)},
#endif
};

STATIC MP_DEFINE_CONST_DICT(mp_module_network_globals, mp_module_network_globals_table);

const mp_obj_module_t network_module = {
    .base = { &mp_type_module },
    .name = MP_QSTR_network,
    .globals = (mp_obj_dict_t*)&mp_module_network_globals,
};
