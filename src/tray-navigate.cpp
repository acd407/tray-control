//
// Created by tray-control on 2023/11/24.
//
#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <optional>

#include "ftxui/screen/screen.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"

#include "StatusNotifierWatcher.h"
#include "StatusNotifierItem.h"
#include "DBusMenu.h"
#include "Utils.h"

void exitWithMsg(std::string_view msg, int code = -1) {
    std::cerr << msg << std::endl;
    exit(code);
}

// 菜单项信息
struct MenuItemInfo {
    int32_t id;
    std::string label;
    bool enabled;
    bool isSeparator;
    std::vector<MenuItemInfo> children;
    int depth;       // 菜单项深度
    int parentIndex; // 在父菜单中的索引
};

// 递归构建菜单项信息
void buildMenuItemInfo(
    const MenuLayoutItem &item, std::vector<MenuItemInfo> &menuItems, int depth = 0, int parentIndex = -1
) {
    MenuItemInfo info;
    info.id = item.id;
    info.depth = depth;
    info.parentIndex = parentIndex;

    // 获取标签
    auto labelIt = item.properties.find("label");
    if (labelIt != item.properties.end() && std::holds_alternative<std::string>(labelIt->second)) {
        info.label = std::get<std::string>(labelIt->second);
    } else {
        info.label = "(无标签)";
    }

    // 获取启用状态
    auto enabledIt = item.properties.find("enabled");
    if (enabledIt != item.properties.end() && std::holds_alternative<bool>(enabledIt->second)) {
        info.enabled = std::get<bool>(enabledIt->second);
    } else {
        info.enabled = true;
    }

    // 检查是否为分隔符
    auto typeIt = item.properties.find("type");
    if (typeIt != item.properties.end() && std::holds_alternative<std::string>(typeIt->second)) {
        info.isSeparator = (std::get<std::string>(typeIt->second) == "separator");
    } else {
        info.isSeparator = false;
    }

    // 添加到菜单项列表
    menuItems.push_back(info);
    int currentIndex = menuItems.size() - 1;

    // 递归处理子菜单项
    for (const auto &child : item.children) {
        buildMenuItemInfo(child, menuItems, depth + 1, currentIndex);
    }
}

int main(int argc, char **argv) {
    cxxopts::Options optionsDecl("tray-navigate", "Interactive menu navigation for system tray items");
    optionsDecl.add_options()("h,help", "Print help and exit", cxxopts::value<bool>()->default_value("false"))("i,id", "Find items by id", cxxopts::value<std::string>())("t,title", "Find items by title", cxxopts::value<std::string>())("a,addr", "Directly specify the address of the item", cxxopts::value<std::string>())(
        "p,path", "Directly specify the path of the item", cxxopts::value<std::string>()
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

    StatusNotifierWatcher watcher;
    if (auto connRes = watcher.connect(); !connRes)
        exitWithMsg("Could not connect to the StatusNotifierWatcher with error: " + connRes.error().show(), -1);

    std::vector<MenuItemInfo> menuItems;
    std::string service;
    std::string menuPath;
    bool foundTarget = false;

    // 直接指定模式：直接使用指定的地址
    if (!addr.empty() && !path.empty()) {
        service = addr;
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
            if (!title.empty())
                ifExpected(item.getTitle(), [&title, &found](const std::string &ctitle) { found = ctitle == title; });
            else if (!id.empty())
                ifExpected(item.getId(), [&id, &found](const std::string &cid) { found = cid == id; });

            if (found) {
                service = itemAddr;
                foundTarget = true;
                break; // 找到匹配项后立即退出循环
            }
        }
    }

    // 统一处理找到的目标项
    if (foundTarget) {
        StatusNotifierItem item(service, !path.empty() ? path : "");
        if (!path.empty() && !item.connect()) {
            exitWithMsg("Could not connect to the StatusNotifierItem at " + service + ":" + path, -1);
        }

        // 获取菜单路径
        ifExpected(item.getMenu(), [&menuPath](const sdbus::ObjectPath &path) { menuPath = path; });

        if (!menuPath.empty()) {
            // 创建DBusMenu对象
            DBusMenu dbusMenu(service, menuPath);
            if (auto connRes = dbusMenu.connect()) {
                // 获取菜单布局
                ifExpected(dbusMenu.getLayout(0, -1), [&menuItems](const auto &layoutResult) {
                    const auto &[revision, rootItem] = layoutResult;
                    buildMenuItemInfo(rootItem, menuItems);
                });
            } else {
                std::cerr << "Could not connect to the DBusMenu with error: " << connRes.error().show() << '\n';
                return 1;
            }
        } else {
            std::cerr << "No menu available for this item\n";
            return 1;
        }
    } else {
        std::cerr << "No matching system tray item found\n";
        return 1;
    }

    if (menuItems.empty()) {
        std::cerr << "No menu items found\n";
        return 1;
    }

    // 使用 ftxui 创建交互式菜单
    using namespace ftxui;

    // 创建屏幕
    auto screen = ScreenInteractive::TerminalOutput();

    // 创建菜单项
    std::vector<std::string> menuEntries;
    std::vector<int32_t> menuIds;
    std::vector<bool> menuEnabled;

    for (const auto &item : menuItems) {
        std::string entry;
        for (int i = 0; i < item.depth; ++i) {
            entry += "  ";
        }

        if (item.isSeparator) {
            entry += "------------------------";
        } else {
            if (!item.enabled) {
                entry += "(" + item.label + ")"; // 禁用项用括号表示
            } else {
                entry += item.label;
            }
        }

        menuEntries.push_back(entry);
        menuIds.push_back(item.id);
        menuEnabled.push_back(item.enabled && !item.isSeparator);
    }

    int selected = 0;
    std::string statusMessage = "使用方向键导航，Enter选择，q退出";

    // 创建菜单组件
    auto menu = Menu(&menuEntries, &selected);

    // 创建主容器
    auto mainRenderer = Renderer(menu, [&] {
        return vbox(
                   {text("=== 系统托盘菜单导航 ===") | bold | center, separator(), menu->Render() | flex, separator(),
                    text(statusMessage) | dim}
               ) |
               border;
    });

    // 添加事件处理
    auto component = CatchEvent(mainRenderer, [&](Event event) {
        if (event == Event::Character('q') || event == Event::Character('Q')) {
            screen.ExitLoopClosure()();
            return true;
        }

        if (event == Event::Return) {
            // 检查选中的菜单项是否可用
            if (selected >= 0 && selected < static_cast<int>(menuItems.size())) {
                if (menuEnabled[selected]) {
                    // 创建DBusMenu对象并发送事件
                    DBusMenu dbusMenu(service, menuPath);
                    if (auto connRes = dbusMenu.connect()) {
                        std::variant<bool, int32_t, std::string> data = static_cast<int32_t>(0);
                        if (auto clickRes = dbusMenu.sendEvent(menuIds[selected], "clicked", data, 0)) {
                            statusMessage = "已点击菜单项: " + menuItems[selected].label;
                        } else {
                            statusMessage = "点击菜单项失败: " + clickRes.error().show();
                        }
                    } else {
                        statusMessage = "无法连接到DBusMenu: " + connRes.error().show();
                    }
                } else {
                    statusMessage = "菜单项已禁用，无法点击";
                }
            }
            return true;
        }

        return false; // 让菜单组件自己处理方向键
    });

    // 运行界面
    screen.Loop(component);

    return 0;
}