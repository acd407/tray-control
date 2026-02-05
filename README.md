# tray-control

这是一个用于显示和操作系统托盘项的简单CLI工具集。

## 工具概述

该项目包含三个命令行工具：

- `tray-trigger`: 主要工具，用于显示、激活系统托盘项目或触发其菜单项
- `tray-navigate`: 交互式导航系统托盘项目的菜单
- `tray-show` 和 `tray-activate` 的功能现已整合到 `tray-trigger` 中

## 使用方法

### tray-trigger

统一的系统托盘操作工具，可以显示、激活系统托盘项目或触发其菜单项：

```shell
$ tray-trigger -h
Interact with system tray items (show, activate, or trigger menu items)
Usage:
  tray-trigger [OPTION...]

  -h, --help       Print help and exit
  -i, --id arg     Find items by id
  -t, --title arg  Find items by title
  -a, --addr arg   Directly specify the address of the item
  -p, --path arg   Directly specify the path of the item
  -m, --menu-id    Menu item ID to click
  -l, --list       List menu items instead of clicking
  -s, --show       Show all system tray items (equivalent to tray-show)
  --activate       Activate the system tray item (equivalent to tray-activate)
  -x arg           X coordinate for activation (default: 0)
  -y arg           Y coordinate for activation (default: 0)
  -v, --verbose    Show full info about each item (when using --show)
```

#### 显示所有系统托盘项目 (原tray-show功能)

使用 `--show` 或 `-s` 参数显示所有系统托盘项目：

```shell
$ tray-trigger --show -v
```

这相当于原来的 `tray-show -v` 命令。

#### 激活系统托盘项目 (原tray-activate功能)

使用 `--activate` 参数激活特定的系统托盘项目：

```shell
$ tray-trigger --activate --title "MyApp"
$ tray-trigger --activate --id "my-app-id" -x 100 -y 200
```

这相当于原来的 `tray-activate` 命令。

#### 触发菜单项 (原tray-trigger功能)

使用 `--list` 选项列出所有菜单项及其ID，然后使用 `--menu-id` 指定要点击的菜单项ID：

```shell
$ tray-trigger --title "MyApp" --list
Menu items:
ID: 1, label: Preferences, enabled: true
ID: 2, label: Settings, enabled: true
ID: 3, label: Exit, enabled: true

$ tray-trigger --title "MyApp" --menu-id 3
```

### tray-navigate

交互式导航系统托盘项目的菜单：

```shell
$ tray-navigate -h
Interactive menu navigation for system tray items
Usage:
  tray-navigate [OPTION...]

  -h, --help       Print help and exit
  -i, --id arg     Find items by id
  -t, --title arg  Find items by title
```

该工具提供了一个基于终端的交互式界面，允许您浏览和点击系统托盘项目的菜单项。使用方向键导航，Enter键选择，q键退出。

### tray-trigger

触发系统托盘项目的特定菜单项：

```shell
$ tray-trigger -h
Trigger a menu item of a system tray item
Usage:
  tray-trigger [OPTION...]

  -h, --help       Print help and exit
  -i, --id arg     Find items by id
  -t, --title arg  Find items by title
  -m, --menu-id    Menu item ID to click
  -l, --list       List menu items instead of clicking
```

使用`--list`选项可以列出所有菜单项及其ID，然后使用`--menu-id`指定要点击的菜单项ID。

## 技术细节

该项目使用以下技术：

- **DBus**: 通过StatusNotifierItem接口与系统托盘通信
- **C++23**: 使用现代C++特性
- **ftxui**: 为tray-navigate提供终端UI界面

## 依赖项

该工具依赖于以下库：

- `systemd-libs`
- `sdbus-c++`
- `fmt`
- `cxxopts` (作为子模块包含)
- `magic_enum` (作为子模块包含)
- `ftxui` (作为子模块包含，仅tray-navigate使用)

## 安装

### Arch Linux

检出仓库并使用[makepkg](https://wiki.archlinux.org/title/makepkg)：

```shell
git checkout https://github.com/acd407/tray-control.git
cd tray-control
makepkg -si
```

希望将来能将此工具添加到AUR。

### 其他 Linux 发行版

检出仓库并使用cmake + make构建：

```shell
git checkout https://github.com/acd407/tray-control.git
cd tray-control
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8
cmake --install .
```

这将（默认）安装四个文件：`/usr/local/bin/tray-show`、`/usr/local/bin/tray-activate`、`/usr/local/bin/tray-navigate`和`/usr/local/bin/tray-trigger`。

使用`CMAKE_INSTALL_PREFIX`可以更改安装文件夹。

## 许可证

该项目采用GNU General Public License v3.0许可证。详见[LICENSE](LICENSE)文件。

## 参考资料

有关StatusNotifierItem的更多信息，请参阅：[StatusNotifierItem](https://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/StatusNotifierItem/)。