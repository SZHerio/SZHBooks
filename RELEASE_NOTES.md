# SZHBooks 1.0.0

Released July 22, 2026.

SZHBooks 1.0 is the first stable Windows release of the local-first desktop
reader.

## Included

- Library management backed by a user-selected local or OneDrive folder
- Recursive collections that mirror the folder hierarchy
- Reading support for TXT, FB2, EPUB, PDF, HTML, Markdown, and DOCX
- EPUB and DOCX loading through an isolated ZIP reader
- Reading progress, navigation history, bookmarks, highlights, and notes
- Per-book typography, continuous and paged text modes, and PDF zoom
- Metadata editing, custom covers, sorting, filtering, and full-text search
- Profile export/import and conflict-preserving folder synchronization
- White and black application themes
- Russian and English interface translations
- Automatic profile snapshots, migration backups, corruption recovery, and
  rotating diagnostic logs
- Windows installer and self-contained portable ZIP

## Upgrade Notes

Installing 1.0 over an earlier SZHBooks build keeps the profile in the user's
application data directory. The database is backed up before schema migration.
Portable builds keep a separate profile beside the executable and do not
automatically import an installed profile.

## Known Limitations

- Android packages are not part of 1.0.
- EPUB fixed-layout features and DRM-protected books are not supported.
- PDF annotations are stored in the SZHBooks profile and are not written back
  into the PDF file.
- OneDrive access depends on the desktop synchronization client and local file
  availability; SZHBooks does not call the Microsoft Graph API.
