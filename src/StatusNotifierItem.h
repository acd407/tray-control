//
// Created by andrew on 11/17/22.
//
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <expected>
#include "Errors.h"
#include <sdbus-c++/sdbus-c++.h>

namespace sdbus {
class IProxy;
}

class StatusNotifierItem {
  public:
    // 工具提示结构体，对应(sa(iiay)ss)
    struct ToolTip {
        std::string iconName;
        std::string title;
        std::string description;
    };

    StatusNotifierItem(std::string_view destination);
    ~StatusNotifierItem();

    std::expected<void, Error> connect();

    /**
     * @name Properties
     * @brief Properties interface for StatusNotifierItem:
     * https://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/StatusNotifierItem/
     * @return
     */
    ///@{
    std::expected<std::string, Error> getCategory();

    std::expected<std::string, Error> getId();

    std::expected<std::string, Error> getTitle();

    std::expected<std::string, Error> getStatus();

    std::expected<uint32_t, Error> getWindowId();

    std::expected<std::string, Error> getIconName();

    std::expected<std::string, Error> getOverlayIconName();

    std::expected<std::string, Error> getAttentionIconName();

    std::expected<std::string, Error> getAttentionMovieName();

    std::expected<std::string, Error> getToolTip() const;

    std::expected<std::string, Error> getIconThemePath();

    std::expected<sdbus::ObjectPath, Error> getMenu() const;

    std::expected<bool, Error> getItemIsMenu();

    // menu
    ///@}

    /**
     * @name Methods
     * @brief Methods interface for StatusNotifierItem:
     * https://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/StatusNotifierItem/
     * @return
     */
    ///@{
    std::expected<void, Error> contextMenu(int x, int y);

    std::expected<void, Error> activate(int x, int y);

    std::expected<void, Error> secondaryActivate(int x, int y);

    std::expected<void, Error> scroll(int delta, const std::string &orientation);

    std::expected<void, Error> provideXdgActivationToken(const std::string &token);
    ///@}

  private:
    std::unique_ptr<sdbus::IProxy> proxy_;
    std::string_view destination_;
};