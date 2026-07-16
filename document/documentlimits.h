#pragma once

#include <QtGlobal>

namespace DocumentLimits {

inline constexpr qint64 maximumTextSourceBytes = 128LL * 1024LL * 1024LL;
inline constexpr qint64 maximumArchiveContainerBytes = 2LL * 1024LL * 1024LL * 1024LL;

} // namespace DocumentLimits
