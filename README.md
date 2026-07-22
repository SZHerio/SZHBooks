# SZHBooks

SZHBooks is a desktop-first, local-first reader built with Qt 6, Qt Quick/QML,
C++, and CMake.

## Highlights

- Library folders mirrored from a user-selected OneDrive directory
- TXT, FB2, EPUB, PDF, HTML, Markdown, and DOCX reading
- Reading position, bookmarks, highlights, notes, metadata, and custom covers
- Full-text library search and collection management
- White and black themes with scalable reading typography
- Local profile backups, automatic recovery, and conflict-preserving sync
- Installed and portable Windows packages

No server account is required. OneDrive integration uses the local synchronized
folder selected by the reader.

## Build

Requirements:

- CMake 3.16 or newer
- Ninja or another CMake-supported generator
- Qt 6.8 or newer with Quick, Quick Controls, PDF, SQL, and XML
- A C++17 compiler

```powershell
cmake -S . -B build/release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.10.2/msvc2022_64
cmake --build build/release --parallel
ctest --test-dir build/release -C Release --output-on-failure
```

Build the Windows installer with Qt Installer Framework available:

```powershell
cpack --config build/release/CPackConfig.cmake -G IFW -C Release
```

Build a portable ZIP from a separate configuration:

```powershell
cmake -S . -B build/portable -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DSZHBOOKS_PORTABLE_BUILD=ON
cmake --build build/portable --parallel
cpack --config build/portable/CPackConfig.cmake -G ZIP -C Release
```

Portable mode is enabled by `portable.flag` beside `SZHBooks.exe` or by the
`--portable` command-line option. Its profile is stored in the adjacent `data`
directory.

## Release

See [RELEASE_NOTES.md](RELEASE_NOTES.md) for the current release and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for dependency notices.

Copyright 2026 SZHerio. All rights reserved.
