//
// Created by andrew on 11/17/22.
//

#include "StatusNotifierItem.h"
#include <sdbus-c++/sdbus-c++.h>

#include "DBusUtils.h"

template <typename T>
static constexpr auto safelyGetSNIProperty =
    std::bind(safelyGetProperty<T>, std::placeholders::_1, "org.kde.StatusNotifierItem", std::placeholders::_2);

StatusNotifierItem::StatusNotifierItem(std::string_view destination) : destination_(destination) {}

StatusNotifierItem::~StatusNotifierItem() = default;

std::expected<void, Error> StatusNotifierItem::connect() {
    return safelyExec([this] -> std::expected<void, Error> {
        proxy_ = sdbus::createProxy(
            sdbus::createSessionBusConnection(), sdbus::ServiceName{std::string(destination_)},
            sdbus::ObjectPath{"/StatusNotifierItem"}
        );
        if (proxy_)
            return {};
        else
            return makeError(ErrorKind::ConnectionError);
    });
}

std::expected<std::string, Error> StatusNotifierItem::getCategory() {
    return safelyGetSNIProperty<std::string>(proxy_, "Category");
}

std::expected<std::string, Error> StatusNotifierItem::getId() {
    return safelyGetSNIProperty<std::string>(proxy_, "Id");
}

std::expected<std::string, Error> StatusNotifierItem::getTitle() {
    return safelyGetSNIProperty<std::string>(proxy_, "Title");
}

std::expected<std::string, Error> StatusNotifierItem::getStatus() {
    return safelyGetSNIProperty<std::string>(proxy_, "Status");
}

std::expected<uint32_t, Error> StatusNotifierItem::getWindowId() {
    return safelyGetSNIProperty<uint32_t>(proxy_, "WindowId");
}

std::expected<std::string, Error> StatusNotifierItem::getIconName() {
    return safelyGetSNIProperty<std::string>(proxy_, "IconName");
}

std::expected<std::string, Error> StatusNotifierItem::getOverlayIconName() {
    return safelyGetSNIProperty<std::string>(proxy_, "OverlayIconName");
}

std::expected<std::string, Error> StatusNotifierItem::getAttentionIconName() {
    return safelyGetSNIProperty<std::string>(proxy_, "AttentionIconName");
}

std::expected<std::string, Error> StatusNotifierItem::getAttentionMovieName() {
    return safelyGetSNIProperty<std::string>(proxy_, "AttentionMovieName");
}

std::expected<std::string, Error> StatusNotifierItem::getToolTip() const {
        return safelyGetSNIProperty<std::string>(proxy_, "ToolTip");
    }

std::expected<std::string, Error> StatusNotifierItem::getIconThemePath() {
    return safelyGetSNIProperty<std::string>(proxy_, "IconThemePath");
}

std::expected<sdbus::ObjectPath, Error> StatusNotifierItem::getMenu() const {
    auto result = safelyGetSNIProperty<sdbus::ObjectPath>(proxy_, "Menu");
    if (!result) {
        // 如果获取菜单路径失败，返回默认路径 "/MenuBar"
        return sdbus::ObjectPath{"/MenuBar"};
    }
    return result;
}

std::expected<bool, Error> StatusNotifierItem::getItemIsMenu() {
    return safelyGetSNIProperty<bool>(proxy_, "ItemIsMenu");
}

std::expected<void, Error> StatusNotifierItem::contextMenu(int x, int y) {
    return safelyCallMethod<void>(proxy_, "org.kde.StatusNotifierItem", "ContextMenu", x, y);
}

std::expected<void, Error> StatusNotifierItem::activate(int x, int y) {
    return safelyCallMethod<void>(proxy_, "org.kde.StatusNotifierItem", "Activate", x, y);
}

std::expected<void, Error> StatusNotifierItem::secondaryActivate(int x, int y) {
    return safelyCallMethod<void>(proxy_, "org.kde.StatusNotifierItem", "SecondaryActivate", x, y);
}

std::expected<void, Error> StatusNotifierItem::scroll(int delta, const std::string &orientation) {
    return safelyCallMethod<void>(proxy_, "org.kde.StatusNotifierItem", "Scroll", delta, orientation);
}

std::expected<void, Error> StatusNotifierItem::provideXdgActivationToken(const std::string &token) {
    return safelyCallMethod<void>(proxy_, "org.kde.StatusNotifierItem", "ProvideXdgActivationToken", token);
}