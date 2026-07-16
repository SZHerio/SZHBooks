#pragma once

#include <QObject>
#include <QString>
#include <QTranslator>

class LocalStateStore;
class QQmlApplicationEngine;

class LocalizationController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString effectiveLanguage READ effectiveLanguage NOTIFY effectiveLanguageChanged)

public:
    explicit LocalizationController(LocalStateStore *stateStore,
                                    QQmlApplicationEngine *engine,
                                    QObject *parent = nullptr);
    ~LocalizationController() override;

    QString language() const;
    QString effectiveLanguage() const;

    void setLanguage(const QString &language);

signals:
    void languageChanged();
    void effectiveLanguageChanged();

private:
    QString resolvedLanguage() const;
    void applyLanguage();

    LocalStateStore *m_stateStore = nullptr;
    QQmlApplicationEngine *m_engine = nullptr;
    QTranslator m_translator;
    QString m_effectiveLanguage;
};
