# tray-control

这是一个用于显示和操作系统托盘项的简单CLI工具集。

## 工具概述

该项目包含四个命令行工具：

- `tray-show`: 显示系统托盘中的所有项目
- `tray-activate`: 激活系统托盘中的项目
- `tray-navigate`: 交互式导航系统托盘项目的菜单
- `tray-trigger`: 触发系统托盘项目的特定菜单项

## 使用方法

### tray-show

显示系统托盘中的所有项目：

```shell
$ tray-show -h
Show items in the system tray
Usage:
  tray-show [OPTION...]

  -h, --help     Print help and exit
  -v, --verbose  Show full info about each item
```

示例输出：
```
$ tray-show -v
Category: ApplicationStatus
Title: TelegramDesktop
Id: TelegramDesktop
Status: Active
IconName: 

Category: ApplicationStatus
Title: Fleep
Id: Fleep
Status: Active
IconName: 
```

### tray-activate

激活系统托盘中的项目：

```shell
$ tray-activate -h
Activate items from the system tray
Usage:
  tray-activate [OPTION...]

  -h, --help       Print help and exit
  -i, --id arg     Find items by id
  -t, --title arg  Find items by title
  -x arg           X coord (default: 0)
  -y arg           Y coord (default: 0)
```

注意：必须指定id或title中的一个（但不能同时指定）。`-x`和`-y`参数是传递给激活调用的坐标，系统托盘项目应该考虑这些坐标来显示子菜单或其他内容（但通常应用程序会忽略它们）。

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