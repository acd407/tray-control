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
        std::visit(
            [&key](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    fmt::printf(", %s: %s", key, arg);
                } else if constexpr (std::is_same_v<T, bool>) {
                    fmt::printf(", %s: %s", key, arg ? "true" : "false");
                } else if constexpr (std::is_same_v<T, int32_t>) {
                    fmt::printf(", %s: %d", key, arg);
                }
            },
            value
        );
    }

    std::cout << std::endl;

    // 递归打印子菜单项
    for (const auto &child : item.children) {
        printMenuItems(child, indent + 1);
    }
}

int main(int argc, char **argv) {
    cxxopts::Options optionsDecl("tray-trigger", "Trigger a menu item of a system tray item");
    optionsDecl.add_options()("h,help", "Print help and exit", cxxopts::value<bool>()->default_value("false"))("i,id", "Find items by id", cxxopts::value<std::string>())("t,title", "Find items by title", cxxopts::value<std::string>())("a,addr", "Directly specify the address of the item", cxxopts::value<std::string>())("p,path", "Directly specify the path of the item", cxxopts::value<std::string>())("m,menu-id", "Menu item ID to click", cxxopts::value<int32_t>())(
        "l,list", "List menu items instead of clicking", cxxopts::value<bool>()->default_value("false")
    );

    const auto options = optionsDecl.parse(argc, argv);
    if (options["help"].as<bool>()) {
        std::cout << optionsDecl.help();
        return 0;
    }

    std::string id, title, addr, path;
    bool found = false;

    auto countId = options.count("id");
    auto countTitle = options.count("title");
    auto countAddr = options.count("addr");
    auto countPath = options.count("path");

    // 直接指定模式：需要同时提供addr和path
    if (countAddr && countPath) {
        addr = options["addr"].as<std::string>();
        path = options["path"].as<std::string>();
        found = true;
    }
    // 错误情况：只提供了addr或path中的一个
    else if (countAddr || countPath) {
        exitWithMsg("Both addr and path must be provided together", 0);
    }
    // 传统搜索模式：使用id或title
    else if (countId || countTitle) {
        if (countId && countTitle || !countId && !countTitle) {
            exitWithMsg("Please specify either id or title (and not both)", 0);
        }

        if (countId) {
            id = options["id"].as<std::string>();
        } else {
            title = options["title"].as<std::string>();
        }
    } else {
        exitWithMsg("Please specify either addr/path or id/title", 0);
    }

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

    std::string targetAddr, targetPath;
    bool foundTarget = false;

    // 直接指定模式：直接使用指定的地址
    if (!addr.empty() && !path.empty()) {
        targetAddr = addr;
        targetPath = path;
        foundTarget = true;
    }
    // 传统搜索模式：遍历查找匹配项
    else if (auto maybeAddrs = watcher.getRegisteredAddresses()) {
        for (const auto &fullAddr : maybeAddrs.value()) {
            auto [itemAddr, itemPath] = splitAddress(fullAddr);
            StatusNotifierItem item(itemAddr, itemPath);
            if (!item.connect())
                continue;

            bool found = false;
            if (!title.empty()) {
                ifExpected(item.getTitle(), [&title, &found](const std::string &ctitle) {
                    if (ctitle == title) {
                        found = true;
                    }
                });
            } else if (!id.empty()) {
                ifExpected(item.getId(), [&id, &found](const std::string &cid) {
                    if (cid == id) {
                        found = true;
                    }
                });
            }

            if (found) {
                targetAddr = itemAddr;
                targetPath = itemPath;
                foundTarget = true;
                break; // 找到匹配项后立即退出循环
            }
        }
    }

    // 统一处理找到的目标项
    if (foundTarget) {
        StatusNotifierItem item(targetAddr, targetPath);
        if (!item.connect()) {
            exitWithMsg("Could not connect to the StatusNotifierItem at " + targetAddr + ":" + targetPath, -1);
        }

        // 获取菜单路径
        std::string menuPath;
        ifExpected(item.getMenu(), [&menuPath](const sdbus::ObjectPath &path) { menuPath = path; });

        // 添加调试信息
        fmt::printf("Menu path: %s\n", menuPath);

        if (!menuPath.empty()) {
            // 创建DBusMenu对象
            DBusMenu dbusMenu(targetAddr, menuPath);
            if (auto connRes = dbusMenu.connect()) {
                // 获取版本
                ifExpected(dbusMenu.getVersion(), [](uint32_t version) { fmt::printf("Menu version: %d\n", version); });

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
                                fmt::printf(
                                    "Failed to click menu item with ID: %d, error: %s\n", foundId,
                                    clickRes.error().show().c_str()
                                );
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
    } else {
        std::cerr << "No matching system tray item found\n";
    }

    return 0;
}