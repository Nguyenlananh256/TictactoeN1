# Caro Arena — C++ Desktop

A native C++17 / Qt Widgets Caro game interface for the AI course assignment. The application uses a notebook grid background, blue X marks, red O marks, and an English interface.

## Features

- 9×9 board; X moves first; at least five consecutive marks wins horizontally, vertically, or diagonally, regardless of blocked ends.
- Local two-player play, win/draw detection, undo, restart confirmation, move history, clock, and JSON export.
- Human vs AI / AI vs AI configuration with Random, MiniMax + Alpha-Beta, and MCTS + UCT selections.
- Search depth, candidate limit, exploration coefficient, and iteration settings.
- C++ AI integration on a worker thread with cancellation and real metrics.

**This repository provides the interface and board rules. The team's AI algorithms and batch experiments are not implemented yet.** Missing AI integrations are indicated in the interface.

## Visual Studio 2026

Install **Desktop development with C++**, a Windows SDK, CMake 4.2+, and Qt built for **MSVC x64**. Use the MSVC Qt package, not the MinGW package.

From the repository root in PowerShell:

```powershell
$env:CARO_QT_MSVC = 'C:/Qt/6.8.3/msvc2022_64'
cmake --preset vs2026-x64 -S desktop
cmake --build desktop/build-vs2026 --config Release
ctest --test-dir desktop/build-vs2026 -C Release --output-on-failure
```

Replace the Qt path with your installed location. Open the generated `desktop/build-vs2026/CaroArena.slnx` in Visual Studio, select **Release | x64**, and run **CaroArena** with Ctrl+F5. Older compatible CMake releases may generate a `.sln` solution instead. Qt runtime libraries are deployed next to the executable automatically.

You can also open `desktop/CMakeLists.txt` in Qt Creator with a matching Desktop Qt kit.

## Source structure

| Path | Purpose |
| --- | --- |
| `desktop/src/main.cpp` | Application entry point and AI registration |
| `desktop/src/window.cpp` | Settings, player cards, history, and game actions |
| `desktop/src/board_widget.cpp` | Board drawing and mouse/keyboard input |
| `desktop/src/game.h` | Board state, legal moves, win/draw rules, and undo |
| `desktop/src/ai.h` | Native AI request/response contract |
| `desktop/tests/` | Board rule and UI integration tests |
| `desktop/assets/` | Bundled handwriting font and its license |

See [the desktop guide](desktop/README.md) for detailed setup and the AI integration contract.

The former HTML/CSS/JavaScript version has been replaced by this C++ application; earlier versions remain in Git history. Build output, local Qt paths, SDK libraries, and executables are excluded from source control.

The Patrick Hand font is distributed under the SIL Open Font License in `desktop/assets/OFL.txt`. This project was developed with assistance from Codex; the team should acknowledge tools and understand the submitted code as required by the assignment.
