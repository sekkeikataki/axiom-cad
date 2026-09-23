# Dear ImGui (vendored or fetched)

Axiom targets **Dear ImGui 1.91.8**.

- A full vendor copy may live in this folder (`imgui.cpp` and backends).
- If `imgui.cpp` is absent, CMake downloads
  `https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8.tar.gz`
  (SHA-256 `db3a2e02bfd6c269adf0968950573053d002f40bdfb9ef2e4a90bce804b0f286`).
- `scripts/fetch-deps.sh` does the same into this directory for offline builds.

Do not bump the tag without rebuilding the desktop UI.
