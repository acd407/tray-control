//
// Created by tray-control on 2023/11/24.
//
#pragma once

#include <sdbus-c++/Types.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <variant>
#include <expected>
#include <functional>

#include "Errors.h"

namespace sdbus {
class IProxy;
}

// 定义菜单项属性类型
using MenuPropertyMap = std::map<
    std::string, std::variant<
                     bool,                                 // enabled, visible
                     int32_t,                              // toggle-state
                     std::string,                          // type, label, icon-name, toggle-type, children-display
                     std::vector<uint8_t>,                 // icon-data
                     std::vector<std::vector<std::string>> // shortcut
                     >>;

// 菜单项结构
struct MenuItem {
    int32_t id;
    MenuPropertyMap properties;
};

// 菜单布局项结构
struct MenuLayoutItem {
    int32_t id;
    MenuPropertyMap properties;
    std::vector<MenuLayoutItem> children;
};

// 事件类型枚举
enum class MenuEventType { Clicked, Hovered };

// DBusMenu 类，包装 com.canonical.dbusmenu 接口
class DBusMenu {
  public:
    explicit DBusMenu(const std::string &service, const std::string &path);
    ~DBusMenu();

    // 连接到 DBus 服务
    std::expected<void, Error> connect();

    // 获取版本
    std::expected<uint32_t, Error> getVersion() const;

    // 获取状态
    std::expected<std::string, Error> getStatus() const;

    // 获取菜单布局
    std::expected<std::pair<uint32_t, MenuLayoutItem>, Error>
    getLayout(int32_t parentId, int32_t recursionDepth, const std::vector<std::string> &propertyNames = {});

    // 获取一组菜单项的属性
    std::expected<std::vector<MenuItem>, Error>
    getGroupProperties(const std::vector<int32_t> &ids, const std::vector<std::string> &propertyNames = {});

    // 获取单个菜单项的属性
    std::expected<std::variant<bool, int32_t, std::string>, Error> getProperty(int32_t id, const std::string &name);

    // 发送事件到菜单项
    std::expected<void, Error> sendEvent(
        int32_t id, const std::string &eventId, const std::variant<bool, int32_t, std::string> &data, uint32_t timestamp
    );

    // 通知菜单即将显示
    std::expected<bool, Error> aboutToShow(int32_t id);

    // 注册菜单项属性更新回调
    void registerItemsPropertiesUpdatedCallback(
        std::function<
            void(const std::vector<MenuItem> &, const std::vector<std::pair<int32_t, std::vector<std::string>>> &)>
            callback
    );

    // 注册布局更新回调
    void registerLayoutUpdatedCallback(std::function<void(uint32_t, int32_t)> callback);

    // 注册菜单项激活请求回调
    void registerItemActivationRequestedCallback(std::function<void(int32_t, uint32_t)> callback);

  private:
    std::string service_;
    std::string path_;
    std::unique_ptr<sdbus::IProxy> proxy_;

    // 回调函数
    std::function<
        void(const std::vector<MenuItem> &, const std::vector<std::pair<int32_t, std::vector<std::string>>> &)>
        itemsPropertiesUpdatedCallback_;
    std::function<void(uint32_t, int32_t)> layoutUpdatedCallback_;
    std::function<void(int32_t, uint32_t)> itemActivationRequestedCallback_;

    // 注册信号处理
    void registerSignalHandlers();

    // 辅助函数：解析 DBus 返回的菜单布局
    MenuLayoutItem parseLayout(const sdbus::Struct<int32_t, MenuPropertyMap, std::vector<sdbus::Variant>> &item);
};