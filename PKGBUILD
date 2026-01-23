pkgname=tray-control-git
pkgver=0.0.1
pkgrel=1
arch=('x86_64')

depends=('systemd-libs' 'fmt' 'sdbus-cpp')

makedepends=('git' 'cmake')

provides=('tray-control')

pkgdesc="Simple CLI tool to show items in systray and activate them. Build on top of DBus"

url="https://github.com/andrewerf/tray-control"

source=('git+https://github.com/acd407/tray-control.git')

md5sums=('SKIP')

license=('GPL3')

pkgname_no_git=${pkgname/-git/}

pkgver() {
    cd $pkgname_no_git
    printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
    cd $pkgname_no_git
    mkdir -p build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release ..
    make
}

package() {
    cd $pkgname_no_git/build
    DESTDIR="$pkgdir/" cmake --install .
}
