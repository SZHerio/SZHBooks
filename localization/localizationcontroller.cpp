#include "localizationcontroller.h"

#include "storage/localstatestore.h"

#include <QCoreApplication>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QDebug>

namespace {

const QString englishLanguage = QStringLiteral("en");
const QString russianLanguage = QStringLiteral("ru");

} // namespace

LocalizationController::LocalizationController(LocalStateStore *stateStore,
                                               QQmlApplicationEngine *engine,
                                               QObject *parent)
    : QObject(parent)
    , m_stateStore(stateStore)
    , m_engine(engine)
{
    Q_ASSERT(m_stateStore);
    Q_ASSERT(m_engine);

    connect(m_stateStore,
            &LocalStateStore::languageChanged,
            this,
            [this]() {
                applyLanguage();
                emit languageChanged();
            });
    applyLanguage();
}

LocalizationController::~LocalizationController()
{
    QCoreApplication::removeTranslator(&m_translator);
}

QString LocalizationController::language() const
{
    return m_stateStore->language();
}

QString LocalizationController::effectiveLanguage() const
{
    return m_effectiveLanguage;
}

void LocalizationController::setLanguage(const QString &language)
{
    m_stateStore->setLanguage(language);
}

QString LocalizationController::resolvedLanguage() const
{
    if (m_stateStore->language() == russianLanguage) {
        return russianLanguage;
    }
    if (m_stateStore->language() == englishLanguage) {
        return englishLanguage;
    }
    return QLocale::system().language() == QLocale::Russian
               ? russianLanguage
               : englishLanguage;
}

void LocalizationController::applyLanguage()
{
    const QString nextLanguage = resolvedLanguage();
    QCoreApplication::removeTranslator(&m_translator);

    if (nextLanguage == russianLanguage) {
        if (m_translator.load(QStringLiteral(":/i18n/SZHBooks_ru.qm"))) {
            QCoreApplication::installTranslator(&m_translator);
        } else {
            qWarning() << "Could not load the Russian translation catalog.";
        }
        QLocale::setDefault(QLocale(QStringLiteral("ru_RU")));
    } else if (m_stateStore->language() == QStringLiteral("system")) {
        QLocale::setDefault(QLocale::system());
    } else {
        QLocale::setDefault(QLocale(QStringLiteral("en_US")));
    }

    const bool effectiveLanguageWasChanged = m_effectiveLanguage != nextLanguage;
    m_effectiveLanguage = nextLanguage;
    m_engine->retranslate();
    if (effectiveLanguageWasChanged) {
        emit effectiveLanguageChanged();
    }
}
