# SZHBooks

[Русский](#русский) | [English](#english)

## Русский

SZHBooks 1.0.0 — локальная настольная читалка на Qt 6, Qt Quick/QML и C++.
Приложение работает без собственного сервера и сохраняет книги и данные
чтения под контролем пользователя.

### Возможности

- TXT, FB2, EPUB, PDF, HTML, Markdown и DOCX
- крупная обложечная библиотека, коллекции и полнотекстовый поиск
- непрерывное и постраничное чтение, масштабирование текста и PDF
- закладки, выделения, заметки и восстановление позиции
- редактирование метаданных и пользовательские обложки
- белая и черная темы, русский и английский интерфейс
- локальные резервные копии и восстановление профиля
- работа с выбранной локальной папкой OneDrive
- установщик Windows и переносимая ZIP-версия

### Почему книги остаются после переустановки

Установщик заменяет файлы программы, но намеренно не удаляет пользовательский
профиль и выбранную папку библиотеки. Поэтому обновление или повторная установка
SZHBooks 1.0.0 сохраняет список книг, позиции, заметки и настройки. Сами книги
не встраиваются в установщик: они остаются в исходной или выбранной OneDrive
папке. Путь к профилю можно открыть из окна «О программе».

Portable-версия использует отдельный каталог `data` рядом с `SZHBooks.exe` и не
импортирует установленный профиль автоматически.

### Что такое `.sha256`

Файл `SZHBooks-1.0.0-win64.exe.sha256` содержит контрольную SHA-256 сумму
установщика. Это не дополнительная программа. Контрольная сумма позволяет
убедиться, что скачанный `.exe` не поврежден и совпадает с опубликованным
файлом.

Проверка в PowerShell:

```powershell
Get-FileHash .\SZHBooks-1.0.0-win64.exe -Algorithm SHA256
```

Полученное значение должно совпасть со строкой в `.sha256`.

### Сборка

Требования: CMake 3.16+, Ninja или другой генератор CMake, Qt 6.8+ с модулями
Quick, Quick Controls, PDF, SQL и XML, компилятор C++17.

```powershell
cmake -S . -B build/release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.10.2/msvc2022_64
cmake --build build/release --parallel
ctest --test-dir build/release -C Release --output-on-failure
```

Установщик Windows:

```powershell
cpack --config build/release/CPackConfig.cmake -G IFW -C Release
```

Portable ZIP собирается из отдельной конфигурации:

```powershell
cmake -S . -B build/portable -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DSZHBOOKS_PORTABLE_BUILD=ON
cmake --build build/portable --parallel
cpack --config build/portable/CPackConfig.cmake -G ZIP -C Release
```

План Android-версии находится в [docs/ANDROID_PLAN.md](docs/ANDROID_PLAN.md),
направление интерфейса и анимации — в
[docs/DESIGN_RESEARCH.md](docs/DESIGN_RESEARCH.md). Описание релиза находится в
[RELEASE_NOTES.md](RELEASE_NOTES.md), лицензии зависимостей — в
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## English

SZHBooks 1.0.0 is a local-first desktop reader built with Qt 6, Qt Quick/QML,
C++, and CMake. It has no application server and keeps books and reading data
under the user's control.

### Features

- TXT, FB2, EPUB, PDF, HTML, Markdown, and DOCX
- cover-focused library, collections, and full-text search
- continuous and paged reading, text scaling, and PDF zoom
- bookmarks, highlights, notes, and reading-position recovery
- metadata editing and custom covers
- white and black themes with Russian and English interfaces
- local profile backups and recovery
- a user-selected local OneDrive folder
- installed and portable Windows packages

### Why books survive reinstallation

The installer replaces application files but intentionally keeps the user
profile and selected library folder. Updating or reinstalling SZHBooks 1.0.0
therefore preserves books, positions, notes, and preferences. Book files are
not embedded in the installer; they remain in their original or selected
OneDrive folder. The profile location can be opened from the About dialog.

Portable builds use a separate `data` directory beside `SZHBooks.exe` and do
not automatically import the installed profile.

### What is a `.sha256` file?

`SZHBooks-1.0.0-win64.exe.sha256` contains the installer's SHA-256 checksum. It
is not another application. The checksum verifies that the downloaded `.exe`
is intact and identical to the published file.

```powershell
Get-FileHash .\SZHBooks-1.0.0-win64.exe -Algorithm SHA256
```

The resulting value must match the line in the `.sha256` file.

### Build

Requirements: CMake 3.16+, Ninja or another CMake generator, Qt 6.8+ with
Quick, Quick Controls, PDF, SQL, and XML, and a C++17 compiler.

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

Build the portable ZIP from a separate configuration:

```powershell
cmake -S . -B build/portable -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DSZHBOOKS_PORTABLE_BUILD=ON
cmake --build build/portable --parallel
cpack --config build/portable/CPackConfig.cmake -G ZIP -C Release
```

See [docs/ANDROID_PLAN.md](docs/ANDROID_PLAN.md) for the Android roadmap,
[docs/DESIGN_RESEARCH.md](docs/DESIGN_RESEARCH.md) for the interface and motion
direction, [RELEASE_NOTES.md](RELEASE_NOTES.md) for release details, and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for dependency notices.

Copyright 2026 SZHerio. All rights reserved.
