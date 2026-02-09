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

// 定义常量以提高可维护性
namespace {
constexpr int DEFAULT_COORDINATE = 0;
constexpr int EXIT_ERROR_CODE = -1;
constexpr int MENU_ITEM_NOT_FOUND = -1;
} // namespace

void exitWithMsg(std::string_view msg, int code = EXIT_ERROR_CODE) {
    std::cerr << msg << std::endl;
    std::_Exit(code); // 使用_std::Exit避免可能的清理问题
}

// 递归查找菜单项 - 优化版本
bool findMenuItem(const MenuLayoutItem &item, int32_t targetId, int32_t &foundId) {
    if (item.id == targetId) {
        foundId = item.id;
        return true;
    }

    // 使用范围for循环和早期返回优化性能
    for (const auto &child : item.children) {
        if (findMenuItem(child, targetId, foundId)) {
            return true;
        }
    }

    return false;
}

// 辅助函数：打印缩进
inline void printIndent(int indent) {
    for (int i = 0; i < indent; ++i) {
        std::cout << "  ";
    }
}

// 递归打印菜单项 - 优化版本
void printMenuItems(const MenuLayoutItem &item, int indent = 0) {
    printIndent(indent);

    // 打印菜单项ID
    fmt::printf("ID: %d", item.id);

    // 打印菜单项属性 - 修复访问方式
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
    cxxopts::Options optionsDecl(
        "tray-trigger", "Interact with system tray items (show, activate, or trigger menu items)"
    );
    optionsDecl.add_options()("h,help", "Print help and exit", cxxopts::value<bool>()->default_value("false"))
        ("i,id", "Find items by id", cxxopts::value<std::string>())
        ("t,title", "Find items by title", cxxopts::value<std::string>())
        ("a,addr", "Directly specify the address of the item", cxxopts::value<std::string>())
        ("p,path", "Directly specify the path of the item", cxxopts::value<std::string>())
        ("m,menu-id", "Menu item ID to click", cxxopts::value<int32_t>())
        ("l,list", "List menu items instead of clicking", cxxopts::value<bool>()->default_value("false"))
        ("s,show", "Show all system tray items (equivalent to tray-show)", cxxopts::value<bool>()->default_value("false"))
        ("activate", "Activate the system tray item (equivalent to tray-activate)", cxxopts::value<bool>()->default_value("false"))
        ("context-menu", "Trigger the context menu of the system tray item", cxxopts::value<bool>()->default_value("false"))
        ("x", "X coordinate for activation (default: 0)", cxxopts::value<int>()->default_value("0"))
        ("y", "Y coordinate for activation (default: 0)", cxxopts::value<int>()->default_value("0"))
        ("v,verbose", "Show full info about each item (when using --show)", cxxopts::value<bool>()->default_value("false"));

    const auto options = optionsDecl.parse(argc, argv);
    if (options["help"].as<bool>()) {
        std::cout << optionsDecl.help();
        return 0;
    }

    std::string id, title, addr, path;

    // 获取各种模式的标志
    const bool showMode = options["show"].as<bool>();
    const bool activateMode = options["activate"].as<bool>();
    const bool contextMenuMode = options["context-menu"].as<bool>();
    const bool listMode = options["list"].as<bool>();
    const bool verboseOutput = options["verbose"].as<bool>();
    const int x = options["x"].as<int>();
    const int y = options["y"].as<int>();

    auto countId = options.count("id");
    auto countTitle = options.count("title");
    auto countAddr = options.count("addr");
    auto countPath = options.count("path");

    // 如果是show模式，则不需要指定特定的项目
    if (showMode) {
        if (countId || countTitle || countAddr || countPath) {
            exitWithMsg("--show mode cannot be combined with id, title, addr, or path options", 0);
        }
    } else {
        // 非show模式下需要指定项目
        // 直接指定模式：需要同时提供addr和path
        if (countAddr && countPath) {
            addr = options["addr"].as<std::string>();
            path = options["path"].as<std::string>();
        }
        // 错误情况：只提供了addr或path中的一个
        else if (countAddr || countPath) {
            exitWithMsg("Both addr and path must be provided together", 0);
        }
        // 传统搜索模式：使用id或title
        else if (countId || countTitle) {
            if ((countId && countTitle) || (!countId && !countTitle)) {
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

        // 对于非show模式，检查是否需要菜单ID
        if (!activateMode && !contextMenuMode && !listMode) {
            if (options.count("menu-id") == 0) {
                exitWithMsg(
                    "Please specify menu item ID to click (or use --list to list menu items, --activate to activate "
                    "the item, --context-menu to trigger context menu, or --show to list all items)",
                    0
                );
            }
        }
    }

    StatusNotifierWatcher watcher;
    if (auto connRes = watcher.connect(); !connRes)
        exitWithMsg(
            "Could not connect to the StatusNotifierWatcher with error: " + connRes.error().show(), EXIT_ERROR_CODE
        );

    if (showMode) {
        // 实现tray-show的功能
        if (auto maybeAddrs = watcher.getRegisteredAddresses()) {
            for (const auto &fullAddr : maybeAddrs.value()) {
                auto [addr, path] = splitAddress(fullAddr);
                fmt::printf("Address: %s\n", addr);
                fmt::printf("Path: %s\n", path);
                StatusNotifierItem item(addr, path);
                if (auto connRes = item.connect()) {
                    ifExpected(item.getCategory(), [](const std::string &category) {
                        fmt::printf("Category: %s\n", category);
                    });

                    ifExpected(item.getTitle(), [](const std::string &title) { fmt::printf("Title: %s\n", title); });

                    if (verboseOutput) {
                        ifExpected(item.getId(), [](const std::string &id) { fmt::printf("Id: %s\n", id); });

                        ifExpected(item.getStatus(), [](const std::string &status) {
                            fmt::printf("Status: %s\n", status);
                        });

                        ifExpected(item.getWindowId(), [](uint32_t windowId) {
                            fmt::printf("WindowId: %d\n", windowId);
                        });

                        ifExpected(item.getIconName(), [](const std::string &iconName) {
                            fmt::printf("IconName: %s\n", iconName);
                        });

                        ifExpected(item.getIconThemePath(), [](const std::string &iconThemePath) {
                            fmt::printf("IconThemePath: %s\n", iconThemePath);
                        });

                        ifExpected(item.getOverlayIconName(), [](const std::string &overlayIconName) {
                            fmt::printf("OverlayIconName: %s\n", overlayIconName);
                        });

                        ifExpected(item.getAttentionIconName(), [](const std::string &attentionIconName) {
                            fmt::printf("AttentionIconName: %s\n", attentionIconName);
                        });

                        ifExpected(item.getAttentionMovieName(), [](const std::string &attentionMovieName) {
                            fmt::printf("AttentionMovieName: %s\n", attentionMovieName);
                        });

                        ifExpected(item.getMenu(), [](const sdbus::ObjectPath &menu) {
                            fmt::printf("Menu: %s\n", menu.c_str());
                        });

                        ifExpected(item.getItemIsMenu(), [](bool itemIsMenu) {
                            fmt::printf("ItemIsMenu: %s\n", itemIsMenu ? "true" : "false");
                        });

                        // ToolTip
                        ifExpected(item.getToolTip(), [](const std::string &toolTip) {
                            fmt::printf("ToolTip: %s\n", toolTip);
                        });
                    }
                } else {
                    std::cerr << "Could not connect to the StatusNotifierItem on address: " << fullAddr
                              << " with error: " << connRes.error().show() << '\n';
                }

                std::cout << '\n';
            }
        }
    } else {
        // 非show模式，需要定位到特定项目
        std::string targetAddr, targetPath;
        bool foundTarget = false;

        // 检查是否已通过命令行参数指定了目标
        if (!addr.empty() && !path.empty()) {
            // 直接指定模式：直接使用指定的地址
            targetAddr = addr;
            targetPath = path;
            foundTarget = true;
        } else {
            // 传统搜索模式：遍历查找匹配项
            if (auto maybeAddrs = watcher.getRegisteredAddresses()) {
                for (const auto &fullAddr : maybeAddrs.value()) {
                    auto [itemAddr, itemPath] = splitAddress(fullAddr);
                    StatusNotifierItem item(itemAddr, itemPath);
                    if (!item.connect())
                        continue;

                    bool matchFound = false;
                    if (!title.empty()) {
                        ifExpected(item.getTitle(), [&title, &matchFound](const std::string &ctitle) {
                            if (ctitle == title) {
                                matchFound = true;
                            }
                        });
                    } else if (!id.empty()) {
                        ifExpected(item.getId(), [&id, &matchFound](const std::string &cid) {
                            if (cid == id) {
                                matchFound = true;
                            }
                        });
                    }

                    if (matchFound) {
                        targetAddr = itemAddr;
                        targetPath = itemPath;
                        foundTarget = true;
                        break; // 找到匹配项后立即退出循环
                    }
                }
            }
        }

        // 处理找到的目标项
        if (foundTarget) {
            StatusNotifierItem item(targetAddr, targetPath);
            if (!item.connect()) {
                exitWithMsg(
                    "Could not connect to the StatusNotifierItem at " + targetAddr + ":" + targetPath, EXIT_ERROR_CODE
                );
            }

            if (activateMode) {
                // 执行激活操作 (tray-activate 功能)
                fmt::printf("Activation %s\n", item.activate(x, y) ? "succeeded" : "failed");
            } else if (contextMenuMode) {
                // 执行上下文菜单操作
                fmt::printf("Context menu %s\n", item.contextMenu(x, y) ? "succeeded" : "failed");
            } else {
                // 获取菜单路径并处理菜单项
                std::string menuPath;
                ifExpected(item.getMenu(), [&menuPath](const sdbus::ObjectPath &path) { menuPath = path; });

                if (menuPath.empty()) {
                    std::cerr << "No menu available for this item\n";
                } else {
                    DBusMenu dbusMenu(targetAddr, menuPath);
                    if (auto connRes = dbusMenu.connect()) {
                        ifExpected(
                            dbusMenu.getLayout(0, -1), [&options, listMode, &dbusMenu](const auto &layoutResult) {
                                const auto &[revision, rootItem] = layoutResult;

                                if (listMode) {
                                    // 列出菜单项
                                    fmt::printf("Menu revision: %d\n", revision);
                                    fmt::printf("Menu items:\n");
                                    printMenuItems(rootItem);
                                } else {
                                    // 点击菜单项
                                    std::optional<int32_t> menuId = options["menu-id"].as<int32_t>();

                                    // 查找菜单项
                                    int32_t foundId = MENU_ITEM_NOT_FOUND;
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
                            }
                        );
                    } else {
                        std::cerr << "Could not connect to the DBusMenu with error: " << connRes.error().show() << '\n';
                    }
                }
            }
        } else {
            std::cerr << "No matching system tray item found\n";
        }
    }

    return 0;
}