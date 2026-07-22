#include "cloudfileavailability.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace CloudFileAvailability {

bool isOnlineOnly(const QString &filePath)
{
#ifdef Q_OS_WIN
    constexpr DWORD recallOnOpen = 0x00040000;
    constexpr DWORD recallOnDataAccess = 0x00400000;
    const DWORD attributes = GetFileAttributesW(
        reinterpret_cast<LPCWSTR>(filePath.utf16()));
    return attributes != INVALID_FILE_ATTRIBUTES
           && (attributes & (FILE_ATTRIBUTE_OFFLINE
                             | recallOnOpen
                             | recallOnDataAccess)) != 0;
#else
    Q_UNUSED(filePath)
    return false;
#endif
}

} // namespace CloudFileAvailability
