#pragma once

#include <QString>
#include <QUrl>

namespace DocumentStorageKey {

QString id(const QUrl &documentUrl);
QString key(const QUrl &documentUrl, const QString &name);

} // namespace DocumentStorageKey
