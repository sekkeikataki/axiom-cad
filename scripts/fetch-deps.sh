#!/usr/bin/env bash
# Vendor Dear ImGui 1.91.8 next to the tree so a clone can build offline.
# CMake also fetches this tarball automatically when imgui.cpp is missing.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
imgui_dir="$root/third_party/imgui"
glad_dir="$root/third_party/glad"

if [[ ! -f "$imgui_dir/imgui.cpp" ]]; then
  echo "fetching Dear ImGui v1.91.8"
  tmp="$(mktemp -d)"
  trap 'rm -rf "$tmp"' EXIT
  curl -fsSL -o "$tmp/imgui.tar.gz" \
    https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8.tar.gz
  echo "db3a2e02bfd6c269adf0968950573053d002f40bdfb9ef2e4a90bce804b0f286  $tmp/imgui.tar.gz" \
    | sha256sum -c -
  tar -xzf "$tmp/imgui.tar.gz" -C "$tmp"
  mkdir -p "$imgui_dir/backends"
  src="$tmp/imgui-1.91.8"
  cp -a "$src"/imgui{,_draw,_tables,_widgets}.cpp "$imgui_dir/"
  cp -a "$src"/imgui{,_internal}.h "$imgui_dir/"
  cp -a "$src"/imconfig.h "$imgui_dir/"
  cp -a "$src"/imstb_{rectpack,textedit,truetype}.h "$imgui_dir/"
  cp -a "$src"/LICENSE.txt "$imgui_dir/"
  cp -a "$src"/backends/imgui_impl_glfw.{h,cpp} "$imgui_dir/backends/"
  cp -a "$src"/backends/imgui_impl_opengl3.{h,cpp} "$imgui_dir/backends/"
  cp -a "$src"/backends/imgui_impl_opengl3_loader.h "$imgui_dir/backends/"
fi

if [[ ! -f "$glad_dir/src/gl.c" || ! -f "$glad_dir/include/glad/gl.h" ]]; then
  echo "error: GLAD OpenGL 3.3 loader is not in third_party/glad." >&2
  echo "Clone https://github.com/sekkeikataki/axiom-cad — those three files are part of this repo." >&2
  exit 1
fi

echo "deps ok"
