#pragma once

#include <QString>
#include <QVariantMap>

class ProfileStorage
{
public:
    virtual ~ProfileStorage() = default;

    virtual QVariantMap profileValues() const = 0;
    virtual bool replaceProfileValues(const QVariantMap &values,
                                      QString *errorMessage = nullptr) = 0;
};
