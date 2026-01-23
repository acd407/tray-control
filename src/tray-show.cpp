//
// Created by andrew on 11/17/22.
//
#include <cxxopts.hpp>
#include <iostream>
#include <fmt/printf.h>

#include "StatusNotifierWatcher.h"
#include "StatusNotifierItem.h"
#include "Utils.h"

void exitWithMsg(std::string_view msg, int code = -1) {
    std::cerr << msg << std::endl;
    exit(code);
}

int main(int argc, char **argv) {
    cxxopts::Options optionsDecl("tray-show", "Show items in the system tray");
    optionsDecl.add_options()("h,help", "Print help and exit", cxxopts::value<bool>()->default_value("false"))(
        "v,verbose", "Show full info about each item", cxxopts::value<bool>()->default_value("false")
    );

    const auto options = optionsDecl.parse(argc, argv);
    if (options["help"].as<bool>()) {
        std::cout << optionsDecl.help();
        return 0;
    }
    const bool verboseOutput = options["verbose"].as<bool>();

    StatusNotifierWatcher watcher;
    if (auto connRes = watcher.connect(); !connRes)
        exitWithMsg("Could not connect to the StatusNotifierWatcher with error: " + connRes.error().show(), -1);

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

                    ifExpected(item.getWindowId(), [](uint32_t windowId) { fmt::printf("WindowId: %d\n", windowId); });

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
}
