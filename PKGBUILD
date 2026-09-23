# Maintainer: Axiom contributors
pkgname=axiom-cad
pkgver=0.1.0
pkgrel=1
pkgdesc="Parametric solid CAD with assemblies and material analysis"
arch=('x86_64')
url="https://github.com/sekkeikataki/axiom-cad"
license=('MIT')
depends=('glfw' 'mesa' 'libglvnd')
makedepends=('cmake' 'gcc' 'pkgconf')
source=()
sha256sums=()

build() {
  cmake -S "$startdir" -B "$srcdir/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
  cmake --build "$srcdir/build" -j"$(nproc)"
}

package() {
  DESTDIR="$pkgdir" cmake --install "$srcdir/build"
}
