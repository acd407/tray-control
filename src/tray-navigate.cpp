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
    int depth;  // 菜单项深度
    int parentIndex;  // 在父菜单中的索引
};

// 递归构建菜单项信息
void buildMenuItemInfo(const MenuLayoutItem &item, std::vector<MenuItemInfo> &menuItems, int depth = 0, int parentIndex = -1) {
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
    optionsDecl.add_options()("h,help", "Print help and exit", cxxopts::value<bool>()->default_value("false"))(
        "i,id", "Find items by id", cxxopts::value<std::string>()
    )(
        "t,title", "Find items by title", cxxopts::value<std::string>()
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

    StatusNotifierWatcher watcher;
    if (auto connRes = watcher.connect(); !connRes)
        exitWithMsg("Could not connect to the StatusNotifierWatcher with error: " + connRes.error().show(), -1);

    std::vector<MenuItemInfo> menuItems;
    std::string serviceName;
    std::string menuPath;

    // 查找托盘项并获取菜单
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
                ifExpected(item.getMenu(), [&menuPath](const sdbus::ObjectPath &path) {
                    menuPath = path;
                });

                if (!menuPath.empty()) {
                    serviceName = addr;  // 地址已经是服务名称，不需要再处理

                    // 创建DBusMenu对象
                    DBusMenu dbusMenu(serviceName, menuPath);
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
                
                // 找到了匹配项，退出循环
                break;
            }
        }
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
                entry += "(" + item.label + ")";  // 禁用项用括号表示
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
        return vbox({
            text("=== 系统托盘菜单导航 ===") | bold | center,
            separator(),
            menu->Render() | flex,
            separator(),
            text(statusMessage) | dim
        }) | border;
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
                    DBusMenu dbusMenu(serviceName, menuPath);
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
        
        return false;  // 让菜单组件自己处理方向键
    });
    
    // 运行界面
    screen.Loop(component);
    
    return 0;
}