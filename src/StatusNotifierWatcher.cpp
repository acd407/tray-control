//
// Created by andrew on 11/16/22.
//

#include "StatusNotifierWatcher.h"
#include <sdbus-c++/sdbus-c++.h>

#include "Utils.h"
#include "DBusUtils.h"

StatusNotifierWatcher::StatusNotifierWatcher() = default;

StatusNotifierWatcher::~StatusNotifierWatcher() = default;

std::expected<void, Error> StatusNotifierWatcher::connect() {
    return safelyExec([this] -> std::expected<void, Error> {
        proxy_ = sdbus::createProxy(
            sdbus::createSessionBusConnection(), sdbus::ServiceName{"org.kde.StatusNotifierWatcher"},
            sdbus::ObjectPath{"/StatusNotifierWatcher"}
        );
        if (proxy_)
            return {};
        else
            return makeError(ErrorKind::ConnectionError);
    });
}

std::expected<std::vector<std::string>, Error> StatusNotifierWatcher::getRegisteredAddresses() {
    auto result = safelyGetProperty<std::vector<std::string>>(
        proxy_, "org.kde.StatusNotifierWatcher", "RegisteredStatusNotifierItems"
    );

    if (result) {
        return std::move(result.value());
    } else {
        return std::unexpected(result.error());
    }
}
