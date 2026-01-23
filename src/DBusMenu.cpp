//
// Created by tray-control on 2023/11/24.
//

#include "DBusMenu.h"
#include <sdbus-c++/sdbus-c++.h>
#include "DBusUtils.h"
#include <iostream>

DBusMenu::DBusMenu(const std::string &service, const std::string &path) : service_(service), path_(path) {}

DBusMenu::~DBusMenu() = default;

std::expected<void, Error> DBusMenu::connect() {
    return safelyExec([this] -> std::expected<void, Error> {
        proxy_ = sdbus::createProxy(
            sdbus::createSessionBusConnection(), sdbus::ServiceName{service_}, sdbus::ObjectPath{path_}
        );

        if (!proxy_) {
            return makeError(ErrorKind::ConnectionError, "Failed to create DBus proxy");
        }

        // 注册信号处理
        registerSignalHandlers();

        return {};
    });
}

std::expected<uint32_t, Error> DBusMenu::getVersion() const {
    return safelyGetProperty<uint32_t>(proxy_, "com.canonical.dbusmenu", "Version");
}

std::expected<std::string, Error> DBusMenu::getStatus() const {
    return safelyGetProperty<std::string>(proxy_, "com.canonical.dbusmenu", "Status");
}

std::expected<std::pair<uint32_t, MenuLayoutItem>, Error>
DBusMenu::getLayout(int32_t parentId, int32_t recursionDepth, const std::vector<std::string> &propertyNames) {

    return safelyExec(
        [this, parentId, recursionDepth,
         &propertyNames]() -> std::expected<std::pair<uint32_t, MenuLayoutItem>, Error> {
            if (!proxy_) {
                return makeError(ErrorKind::ConnectionError, "DBus proxy not initialized");
            }

            try {
                // 直接按签名接收结果，不使用Variant
                uint32_t revision;
                sdbus::Struct<int32_t, MenuPropertyMap, std::vector<sdbus::Variant>> layout;

                proxy_->callMethod("GetLayout")
                    .onInterface("com.canonical.dbusmenu")
                    .withArguments(parentId, recursionDepth, propertyNames)
                    .storeResultsTo(revision, layout);

                // 使用递归函数解析布局
                MenuLayoutItem rootItem = parseLayout(layout);

                return std::make_pair(revision, rootItem);
            } catch (const sdbus::Error &err) {
                return makeError(ErrorKind::DBusError, err.what());
            } catch (const std::exception &e) {
                return makeError(ErrorKind::UnknownError, e.what());
            }
        }
    );
}

std::expected<std::vector<MenuItem>, Error>
DBusMenu::getGroupProperties(const std::vector<int32_t> &ids, const std::vector<std::string> &propertyNames) {

    return safelyExec([this, &ids, &propertyNames]() -> std::expected<std::vector<MenuItem>, Error> {
        if (!proxy_) {
            return makeError(ErrorKind::ConnectionError, "DBus proxy not initialized");
        }

        try {
            // 调用 GetGroupProperties 方法并直接获取结果
            std::vector<sdbus::Variant> itemsData;

            proxy_->callMethod("GetGroupProperties")
                .onInterface("com.canonical.dbusmenu")
                .withArguments(ids, propertyNames)
                .storeResultsTo(itemsData);

            // 转换为 MenuItem 结构
            std::vector<MenuItem> items;
            for (const auto &itemVariant : itemsData) {
                try {
                    // 简化处理，只获取基本结构
                    MenuItem item;
                    item.id = 0; // 默认值

                    // 尝试获取属性
                    if (itemVariant.containsValueOfType<sdbus::Struct<int32_t, MenuPropertyMap>>()) {
                        auto itemStruct = itemVariant.get<sdbus::Struct<int32_t, MenuPropertyMap>>();
                        item.id = std::get<0>(itemStruct);
                        item.properties = std::get<1>(itemStruct);
                    }

                    items.push_back(std::move(item));
                } catch (const std::exception &e) {
                    // 忽略单个项的错误，继续处理其他项
                    continue;
                }
            }

            return items;
        } catch (const sdbus::Error &err) {
            return makeError(ErrorKind::DBusError, err.what());
        } catch (const std::exception &e) {
            return makeError(ErrorKind::UnknownError, e.what());
        }
    });
}

std::expected<std::variant<bool, int32_t, std::string>, Error>
DBusMenu::getProperty(int32_t id, const std::string &name) {

    return safelyExec([this, id, &name]() -> std::expected<std::variant<bool, int32_t, std::string>, Error> {
        if (!proxy_) {
            return makeError(ErrorKind::ConnectionError, "DBus proxy not initialized");
        }

        try {
            // 调用 GetProperty 方法并直接获取结果
            sdbus::Variant value;

            proxy_->callMethod("GetProperty")
                .onInterface("com.canonical.dbusmenu")
                .withArguments(id, name)
                .storeResultsTo(value);

            // 根据类型返回值
            if (value.containsValueOfType<bool>()) {
                return value.get<bool>();
            } else if (value.containsValueOfType<int32_t>()) {
                return value.get<int32_t>();
            } else if (value.containsValueOfType<std::string>()) {
                return value.get<std::string>();
            } else {
                return makeError(ErrorKind::TypeError, "Unsupported property type");
            }
        } catch (const sdbus::Error &err) {
            return makeError(ErrorKind::DBusError, err.what());
        } catch (const std::exception &e) {
            return makeError(ErrorKind::UnknownError, e.what());
        }
    });
}

std::expected<void, Error> DBusMenu::sendEvent(
    int32_t id, const std::string &eventId, const std::variant<bool, int32_t, std::string> &data, uint32_t timestamp
) {

    return safelyExec([this, id, &eventId, &data, timestamp]() -> std::expected<void, Error> {
        if (!proxy_) {
            return makeError(ErrorKind::ConnectionError, "DBus proxy not initialized");
        }

        // 调用 Event 方法
        proxy_->callMethod("Event")
            .onInterface("com.canonical.dbusmenu")
            .withArguments(id, eventId, data, timestamp)
            .dontExpectReply();

        return {};
    });
}

std::expected<bool, Error> DBusMenu::aboutToShow(int32_t id) {
    return safelyExec([this, id]() -> std::expected<bool, Error> {
        if (!proxy_) {
            return makeError(ErrorKind::ConnectionError, "DBus proxy not initialized");
        }

        try {
            // 调用 AboutToShow 方法并直接获取结果
            bool needUpdate;

            proxy_->callMethod("AboutToShow")
                .onInterface("com.canonical.dbusmenu")
                .withArguments(id)
                .storeResultsTo(needUpdate);

            return needUpdate;
        } catch (const sdbus::Error &err) {
            return makeError(ErrorKind::DBusError, err.what());
        } catch (const std::exception &e) {
            return makeError(ErrorKind::UnknownError, e.what());
        }
    });
}

void DBusMenu::registerItemsPropertiesUpdatedCallback(
    std::function<
        void(const std::vector<MenuItem> &, const std::vector<std::pair<int32_t, std::vector<std::string>>> &)>
        callback
) {
    itemsPropertiesUpdatedCallback_ = std::move(callback);
}

void DBusMenu::registerLayoutUpdatedCallback(std::function<void(uint32_t, int32_t)> callback) {
    layoutUpdatedCallback_ = std::move(callback);
}

void DBusMenu::registerItemActivationRequestedCallback(std::function<void(int32_t, uint32_t)> callback) {
    itemActivationRequestedCallback_ = std::move(callback);
}

void DBusMenu::registerSignalHandlers() {
    if (!proxy_) {
        return;
    }

    // 注册 ItemsPropertiesUpdated 信号处理
    try {
        proxy_->uponSignal("ItemsPropertiesUpdated")
            .onInterface("com.canonical.dbusmenu")
            .call([this](
                      const std::vector<sdbus::Struct<int32_t, MenuPropertyMap>> &updatedProps,
                      const std::vector<sdbus::Struct<int32_t, std::vector<std::string>>> &removedProps
                  ) {
                if (!itemsPropertiesUpdatedCallback_) {
                    return;
                }

                try {
                    // 转换为 MenuItem 结构
                    std::vector<MenuItem> updatedItems;
                    for (const auto &itemStruct : updatedProps) {
                        MenuItem item;
                        item.id = std::get<0>(itemStruct);
                        item.properties = std::get<1>(itemStruct);
                        updatedItems.push_back(std::move(item));
                    }

                    // 转换移除的属性
                    std::vector<std::pair<int32_t, std::vector<std::string>>> removedItems;
                    for (const auto &itemStruct : removedProps) {
                        removedItems.emplace_back(std::get<0>(itemStruct), std::get<1>(itemStruct));
                    }

                    // 调用回调
                    itemsPropertiesUpdatedCallback_(updatedItems, removedItems);
                } catch (const sdbus::Error &err) {
                    // 错误处理
                }
            });
    } catch (const sdbus::Error &err) {
        // 信号注册失败
    }

    // 注册 LayoutUpdated 信号处理
    try {
        proxy_->uponSignal("LayoutUpdated")
            .onInterface("com.canonical.dbusmenu")
            .call([this](uint32_t revision, int32_t parent) {
                if (!layoutUpdatedCallback_) {
                    return;
                }

                try {
                    // 调用回调
                    layoutUpdatedCallback_(revision, parent);
                } catch (const sdbus::Error &err) {
                    // 错误处理
                }
            });
    } catch (const sdbus::Error &err) {
        // 信号注册失败
    }

    // 注册 ItemActivationRequested 信号处理
    try {
        proxy_->uponSignal("ItemActivationRequested")
            .onInterface("com.canonical.dbusmenu")
            .call([this](int32_t id, uint32_t timestamp) {
                if (!itemActivationRequestedCallback_) {
                    return;
                }

                try {
                    // 调用回调
                    itemActivationRequestedCallback_(id, timestamp);
                } catch (const sdbus::Error &err) {
                    // 错误处理
                }
            });
    } catch (const sdbus::Error &err) {
        // 信号注册失败
    }
}

// 递归解析布局项
MenuLayoutItem
DBusMenu::parseLayout(const sdbus::Struct<int32_t, MenuPropertyMap, std::vector<sdbus::Variant>> &layout) {
    MenuLayoutItem item;
    item.id = std::get<0>(layout);
    item.properties = std::get<1>(layout);

    // 递归解析子菜单项
    const auto &children = std::get<2>(layout);
    for (const auto &childVariant : children) {
        // 子菜单项也是相同的结构，递归解析
        auto childLayout = childVariant.get<sdbus::Struct<int32_t, MenuPropertyMap, std::vector<sdbus::Variant>>>();
        MenuLayoutItem childItem = parseLayout(childLayout);
        item.children.push_back(childItem);
    }

    return item;
}