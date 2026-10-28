# Third-Party Notices

This repository (`P5D-PC`) is licensed under the **MIT License** — see [LICENSE](LICENSE).
The MIT license covers the original source code, generators, tools, scripts and research
documentation written by this project's contributors.

It does **not** cover the game itself, game data, or game-derived assets:

- *Persona 5: Dancing in Starlight* (P5D, PCSH10109) and all related characters, music, artwork,
  text and code are © ATLUS / SEGA. This repository contains **no game files, no keys, no firmware,
  no decrypted game modules, no SPIR-V/AOT caches derived from game data, and no game screenshots
  or other game assets**.
- To run the project, players must use their own legally owned copy of the game and extract the
  data themselves (see [docs/RELEASE_COMPLIANCE.md](docs/RELEASE_COMPLIANCE.md)).
- Caches generated locally from the player's own game data (AOT C++/object files, SPIR-V shaders,
  pipeline caches) are the player's local artifacts and are not distributed by this project.

Development/build dependencies (not vendored in this repository unless noted):

| Component | License | Use |
|---|---|---|
| [Capstone](https://www.capstone-engine.org/) disassembly engine | BSD-3-Clause | Offline ARM/Thumb disassembly in the AOT code generator (`engine/recompiler/*.py`, `tools/*.py`) |
| [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers) | Apache-2.0 | Build-time headers for the GXM→Vulkan backend (`engine/gxm/`) |
| MinGW-w64 / GCC runtime | GPL-3.0 with GCC Runtime Library Exception | Local build toolchain only; not distributed |
| Optional local research tools (ffmpeg, vgmstream, CriPakGUI) | their respective licenses | Referenced only by historical research notes; **not** part of this repository or its release artifacts |

If you believe a third-party component is missing from this notice, please open an issue.
