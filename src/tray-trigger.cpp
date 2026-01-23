//
// Created by tray-control on 2023/11/24.
//
#include <cxxopts.hpp>
#include <iostream>
#include <fmt/printf.h>

#include "StatusNotifierWatcher.h"
#include "StatusNotifierItem.h"
#include "DBusMenu.h"
#include "Utils.h"

void exitWithMsg(std::string_view msg, int code = -1) {
    std::cerr << msg << std::endl;
    exit(code);
}

// 递归查找菜单项
bool findMenuItem(const MenuLayoutItem &item, int32_t targetId, int32_t &foundId) {
    if (item.id == targetId) {
        foundId = item.id;
        return true;
    }
    
    // 递归查找子菜单项
    for (const auto &child : item.children) {
        if (findMenuItem(child, targetId, foundId)) {
            return true;
        }
    }
    
    return false;
}

// 递归打印菜单项
void printMenuItems(const MenuLayoutItem &item, int indent = 0) {
    // 打印缩进
    for (int i = 0; i < indent; ++i) {
        std::cout << "  ";
    }

    // 打印菜单项ID
    fmt::printf("ID: %d", item.id);

    // 打印菜单项属性
    for (const auto &[key, value] : item.properties) {
        std::visit([&key](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>) {
                fmt::printf(", %s: %s", key, arg);
            } else if constexpr (std::is_same_v<T, bool>) {
                fmt::printf(", %s: %s", key, arg ? "true" : "false");
            } else if constexpr (std::is_same_v<T, int32_t>) {
                fmt::printf(", %s: %d", key, arg);
            }
        }, value);
    }

    std::cout << std::endl;

    // 递归打印子菜单项
    for (const auto &child : item.children) {
        printMenuItems(child, indent + 1);
    }
}

int main(int argc, char **argv) {
    cxxopts::Options optionsDecl("tray-trigger", "Trigger a menu item of a system tray item");
    optionsDecl.add_options()("h,help", "Print help and exit", cxxopts::value<bool>()->default_value("false"))(
        "i,id", "Find items by id", cxxopts::value<std::string>()
    )(
        "t,title", "Find items by title", cxxopts::value<std::string>()
    )(
        "m,menu-id", "Menu item ID to click", cxxopts::value<int32_t>()
    )(
        "l,list", "List menu items instead of clicking", cxxopts::value<bool>()->default_value("false")
    );

    const auto options = optionsDecl.parse(argc, argv);
    if (options["help"].as<bool>()) {
        std::cout << optionsDecl.help();
        return 0;
    }

    std::optional<std::string> id;
    std::optional<std::string> title;
    if (auto [countId, countTitle] = std::make_tuple(options.count("id"), options.count("title"));
        countId != 0 && countTitle != 0 || countId == 0 && countTitle == 0) {
        exitWithMsg("Please specify either id or title (and not both)", 0);
    } else if (countId != 0)
        id = options["id"].as<std::string>();
    else if (countTitle != 0)
        title = options["title"].as<std::string>();

    const bool listMode = options["list"].as<bool>();
    
    std::optional<int32_t> menuId;
    if (!listMode && options.count("menu-id") == 0) {
        exitWithMsg("Please specify menu item ID to click (or use --list to list menu items)", 0);
    } else if (!listMode) {
        menuId = options["menu-id"].as<int32_t>();
    }

    StatusNotifierWatcher watcher;
    if (auto connRes = watcher.connect(); !connRes)
        exitWithMsg("Could not connect to the StatusNotifierWatcher with error: " + connRes.error().show(), -1);

    if (auto maybeAddrs = watcher.getRegisteredStatusNotifierItemAddresses()) {
        for (const auto &addr : maybeAddrs.value()) {
            StatusNotifierItem item(addr);
            if (!item.connect())
                continue;

            bool found = false;
            if (title) {
                ifExpected(item.getTitle(), [&title, &found](const std::string &ctitle) {
                    if (ctitle == *title) {
                        found = true;
                    }
                });
            } else if (id) {
                ifExpected(item.getId(), [&id, &found](const std::string &cid) {
                    if (cid == *id) {
                        found = true;
                    }
                });
            }

            if (found) {
                // 获取菜单路径
                std::string menuPath;
                ifExpected(item.getMenu(), [&menuPath](const sdbus::ObjectPath &path) {
                    menuPath = path;
                });

                // 添加调试信息
                fmt::printf("Menu path: %s\n", menuPath);

                if (!menuPath.empty()) {
                    // 获取服务名称
                    std::string serviceName = addr;  // 地址已经是服务名称，不需要再处理

                    // 创建DBusMenu对象
                    DBusMenu dbusMenu(serviceName, menuPath);
                    if (auto connRes = dbusMenu.connect()) {
                        // 获取版本
                        ifExpected(dbusMenu.getVersion(), [](uint32_t version) {
                            fmt::printf("Menu version: %d\n", version);
                        });

                        // 获取状态
                        ifExpected(dbusMenu.getStatus(), [](const std::string &status) {
                            fmt::printf("Menu status: %s\n", status);
                        });

                        // 获取菜单布局
                        ifExpected(dbusMenu.getLayout(0, -1), [listMode, menuId, &dbusMenu](const auto &layoutResult) {
                            const auto &[revision, rootItem] = layoutResult;
                            fmt::printf("Menu revision: %d\n", revision);
                            
                            if (listMode) {
                                fmt::printf("Menu items:\n");
                                printMenuItems(rootItem);
                            } else {
                                // 查找菜单项
                                int32_t foundId = -1;
                                if (findMenuItem(rootItem, *menuId, foundId)) {
                                    fmt::printf("Found menu item with ID: %d\n", foundId);
                                    
                                    // 发送点击事件
                                    std::variant<bool, int32_t, std::string> data = static_cast<int32_t>(0);
                                    if (auto clickRes = dbusMenu.sendEvent(foundId, "clicked", data, 0)) {
                                        fmt::printf("Successfully clicked menu item with ID: %d\n", foundId);
                                    } else {
                                        fmt::printf("Failed to click menu item with ID: %d, error: %s\n", 
                                                  foundId, clickRes.error().show().c_str());
                                    }
                                } else {
                                    fmt::printf("Menu item with ID: %d not found\n", *menuId);
                                }
                            }
                        });
                    } else {
                        std::cerr << "Could not connect to the DBusMenu with error: " << connRes.error().show() << '\n';
                    }
                } else {
                    std::cerr << "No menu available for this item\n";
                }
                
                // 找到了匹配项，退出循环
                break;
            }
        }
    }

    return 0;
}