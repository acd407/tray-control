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
    cxxopts::Options optionsDecl("tray-activate", "Activate items from the system tray");
    optionsDecl.add_options()
        ( "h,help", "Print help and exit", cxxopts::value<bool>()->default_value("false") )
        ( "i,id", "Find items by id", cxxopts::value<std::string>() )
        ( "t,title", "Find items by title", cxxopts::value<std::string>() )
        ( "a,addr", "Directly specify the address of the item", cxxopts::value<std::string>() )
        ( "p,path", "Directly specify the path of the item", cxxopts::value<std::string>() )
        ( "x", "X coord", cxxopts::value<int>()->default_value( "0" ) )
        ( "y", "Y coord", cxxopts::value<int>()->default_value( "0" ) );

    const auto options = optionsDecl.parse(argc, argv);
    if (options["help"].as<bool>()) {
        std::cout << optionsDecl.help();
        return 0;
    }

    std::string addr, path, id, title;

    // Count parameters
    auto [countId, countTitle, countAddr, countPath] =
        std::make_tuple(options.count("id"), options.count("title"), options.count("addr"), options.count("path"));

    bool found = false;

    // Check if direct addressing is used
    if (countAddr > 0 && countPath > 0) {
        // Direct addressing mode - both addr and path provided
        addr = options["addr"].as<std::string>();
        path = options["path"].as<std::string>();
        found = true;
    } else if (countAddr > 0 || countPath > 0) {
        // Only one of addr/path provided - this is an error
        exitWithMsg("Both addr and path must be provided together", 0);
    } else {
        // Traditional search mode - use id or title
        if (countId != 0 && countTitle != 0 || countId == 0 && countTitle == 0) {
            exitWithMsg("Please specify either id or title (and not both)", 0);
        } else if (countId != 0)
            id = options["id"].as<std::string>();
        else if (countTitle != 0)
            title = options["title"].as<std::string>();
    }

    const int x = options["x"].as<int>();
    const int y = options["y"].as<int>();

    StatusNotifierWatcher watcher;
    if (auto connRes = watcher.connect(); !connRes)
        exitWithMsg("Could not connect to the StatusNotifierWatcher with error: " + connRes.error().show(), -1);

    if (!found) {
        if (auto maybeAddrs = watcher.getRegisteredAddresses()) {
            for (const auto &fullAddr : maybeAddrs.value()) {
                auto [itemAddr, itemPath] = splitAddress(fullAddr);
                StatusNotifierItem item(itemAddr, itemPath);
                if (!item.connect())
                    continue;

                bool isMatch = false;
                if (countTitle) {
                    // Use ifExpected instead of the >> operator
                    ifExpected(item.getTitle(), [&title, &isMatch](const std::string &ctitle) {
                        isMatch = (ctitle == title);
                    });
                } else if (countId) {
                    // Use ifExpected instead of the >> operator
                    ifExpected(item.getId(), [&id, &isMatch](const std::string &cid) { isMatch = (cid == id); });
                }

                if (isMatch) {
                    addr = itemAddr;
                    path = itemPath;
                    found = true;
                    break;
                }
            }
        }
    }

    // Activate the target item if found
    if (found) {
        StatusNotifierItem item(addr, path);
        if (item.connect()) {
            fmt::printf("Activation %s\n", item.activate(x, y) ? "succeeded" : "failed");
        }
    }
}