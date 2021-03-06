/*
 * HR20 ESP Master
 * ---------------
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http:*www.gnu.org/licenses
 *
 */

#include "util.h"
#include "protocol.h"
#include "model.h"
#include "error.h"
#include "eventlog.h"
#include "json.h"

namespace hr20 {
namespace json {

ICACHE_FLASH_ATTR void append_client_attr(String &str,
                                          const HR20 &client)
{
    // this is intentionally cryptic as we have to fit in 128 bytes
    // for PubSubClient to be able to handle us
    json::Object obj(str);

    // attributes follow.
    json::kv_raw(obj, "auto", client.auto_mode.to_str());
    json::kv_raw(obj, "lock", client.menu_locked.to_str());
    json::kv_raw(obj, "window", client.mode_window.to_str());
    json::kv_raw(obj, "temp", client.temp_avg.to_str());
    json::kv_raw(obj, "bat", client.bat_avg.to_str());
    json::kv_raw(obj, "temp_wtd", client.temp_wanted.to_str());
    json::kv_raw(obj, "valve_wtd", client.cur_valve_wtd.to_str());
    json::kv_raw(obj, "error", client.ctl_err.to_str());
    json::kv_raw(obj, "last_seen", String(client.last_contact));
}

ICACHE_FLASH_ATTR void append_timer_day(String &str, const HR20 &m, uint8_t day)
{
    json::Object day_obj(str);

    for (uint8_t idx = 0; idx < TIMER_SLOTS_PER_DAY; ++idx) {
        // skip timer we don't know yet
        if (!m.timers[day][idx].remote_valid()) continue;

        // this index is synced.
        day_obj.key(idx);

        // remote timer is read
        const auto &remote = m.timers[day][idx].get_remote();

        // value is an object
        json::Object slot(day_obj);


        slot.key("time");
        json::str(str, cvt::TimeHHMM::to_str(remote.time()));
        slot.key("mode");
        json::str(str, cvt::Simple::to_str(remote.mode()));
    }
}

void append_event(String &str, const Event &ev) {
    json::Object obj(str);

    // attributes follow.
    json::kv_raw(obj, "type",  cvt::Simple::to_str((uint8_t)ev.type));
    switch (ev.type) {
    case EventType::EVENT:
        json::kv_str(
            obj, "name", event_to_str(static_cast<EventCode>(ev.code)));
        break;
    case EventType::ERROR:
        json::kv_str(obj, "name", err_to_str(static_cast<ErrorCode>(ev.code)));
        break;
    default:
        break;
    }
    json::kv_raw(obj, "value", cvt::Simple::to_str(ev.value));
    json::kv_raw(obj, "time",  cvt::Simple::to_str(ev.time));
}


} // namespace json
} // namespace hr20
