#include <QCommandLineParser>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStyleHints>
#include <QVariant>
#include <QVariantMap>
#include <QWindow>
#include <QtMath>

#include <memory>

#include "library/bookcoverprovider.h"
#include "diagnostics/diagnosticservice.h"
#include "library/bookmetadataservice.h"
#include "library/librarymodel.h"
#include "library/libraryrepository.h"
#include "library/libraryscanservice.h"
#include "localization/localizationcontroller.h"
#include "notes/notescentermodel.h"
#include "platform/applicationinfoservice.h"
#include "platform/applicationpaths.h"
#include "platform/desktopintegration.h"
#include "reader/readingsearchcontroller.h"
#include "reader/readingdocumentformatter.h"
#include "readercontroller.h"
#include "search/librarysearchindex.h"
#include "search/librarysearchmodel.h"
#include "storage/localstatestore.h"
#include "storage/profilearchiveservice.h"
#include "storage/readingannotationstore.h"
#include "sync/onedrivelibraryservice.h"

namespace {

constexpr int normalWheelScrollLines = 6;

int wheelScrollLinesForSpeed(int scrollSpeed)
{
    return qMax(1, qRound(normalWheelScrollLines * scrollSpeed / 100.0));
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QCoreApplication::setOrganizationName(QStringLiteral("SZHBooks"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("szhbooks.local"));
    QCoreApplication::setApplicationName(QStringLiteral("SZHBooks"));
    QCoreApplication::setApplicationVersion(QStringLiteral(SZHBOOKS_VERSION));

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(
        ":/qt/qml/SZHBooks/assets/branding/szhbooks-icon.png")));

    QCommandLineParser commandLine;
    commandLine.setApplicationDescription(
        QCoreApplication::translate("main", "SZHBooks desktop reader"));
    commandLine.addHelpOption();
    commandLine.addVersionOption();
    const QCommandLineOption portableOption(
        QStringLiteral("portable"),
        QCoreApplication::translate(
            "main",
            "Keep profile data beside the application executable."));
    commandLine.addOption(portableOption);
    commandLine.addPositionalArgument(
        QStringLiteral("books"),
        QCoreApplication::translate("main", "Books to open."),
        QStringLiteral("[books...]"));
    commandLine.process(app);

    const ApplicationPaths applicationPaths(app.applicationDirPath(),
                                            commandLine.isSet(portableOption));
    DiagnosticService diagnosticService(applicationPaths.diagnosticLogDirectory());
    DesktopIntegration desktopIntegration(
        BookMetadataService::supportedSuffixes(),
        applicationPaths.desktopSettingsFilePath());
    const QString localSettingsFilePath = applicationPaths.portableMode()
                                              ? applicationPaths.settingsFilePath()
                                              : LocalStateStore::defaultSettingsFilePath();
    LocalStateStore localState(localSettingsFilePath);
    ApplicationInfoService applicationInfo(
        applicationPaths,
        QFileInfo(localState.settingsFilePath()).absolutePath());
    qInfo().noquote() << "Storage mode:"
                      << (applicationPaths.portableMode() ? "portable" : "installed")
                      << "Profile:" << localState.databaseFilePath();
    if (localState.profileRecoveryState() != QLatin1String("healthy")) {
        qWarning().noquote() << localState.profileRecoveryMessage();
    }
    ProfileArchiveService profileArchiveService(&localState);
    const auto applyScrollSpeed = [&app, &localState]() {
        // This hint is shared by every Qt Quick scroll view, including the PDF table view.
        app.styleHints()->setWheelScrollLines(wheelScrollLinesForSpeed(localState.scrollSpeed()));
    };
    applyScrollSpeed();
    QObject::connect(&localState,
                     &LocalStateStore::scrollSpeedChanged,
                     &app,
                     applyScrollSpeed);
    BookCoverProvider coverProvider(applicationPaths.coverCacheDirectory());
    BookMetadataService metadataService(&coverProvider);
    LibraryRepository libraryRepository(&localState,
                                        &metadataService,
                                        applicationPaths.customCoverDirectory());
    LibraryScanService libraryScanService(&metadataService);
    LibraryModel libraryModel(&libraryRepository);
    libraryModel.setScanService(&libraryScanService);
    ReaderController reader;
    ReadingDocumentFormatter documentFormatter;
    ReadingSearchController searchController;
    ReadingAnnotationStore annotationStore(localState.profileDatabase());
    NotesCenterModel notesCenterModel(localState.profileDatabase());
    LibrarySearchModel librarySearchModel(
        &libraryRepository,
        LibrarySearchIndex::databasePathForProfile(localState.databaseFilePath()));
    OneDriveLibraryService oneDriveLibraryService(
        &localState,
        &libraryRepository,
        applicationPaths.syncSettingsFilePath());

    QObject::connect(&localState,
                     &LocalStateStore::profileReplaced,
                     &annotationStore,
                     &ReadingAnnotationStore::reload);
    QObject::connect(&localState,
                     &LocalStateStore::profileReplaced,
                     &notesCenterModel,
                     &NotesCenterModel::refresh);
    QObject::connect(&libraryRepository,
                     &LibraryRepository::changed,
                     &notesCenterModel,
                     &NotesCenterModel::refresh);
    QObject::connect(&annotationStore,
                     &ReadingAnnotationStore::annotationsChanged,
                     &notesCenterModel,
                     &NotesCenterModel::refresh);
    QObject::connect(&notesCenterModel,
                     &NotesCenterModel::annotationChanged,
                     &annotationStore,
                     [&annotationStore](const QUrl &sourceUrl) {
                         if (annotationStore.documentUrl() == sourceUrl) {
                             annotationStore.reload();
                         }
                     });
    QObject::connect(&annotationStore,
                     &ReadingAnnotationStore::profileChanged,
                     &oneDriveLibraryService,
                     &OneDriveLibraryService::scheduleSynchronization);
    QObject::connect(&notesCenterModel,
                     &NotesCenterModel::profileChanged,
                     &oneDriveLibraryService,
                     &OneDriveLibraryService::scheduleSynchronization);

    QObject::connect(&reader,
                     &ReaderController::documentOpened,
                     &localState,
                     [&localState, &libraryRepository, &reader](const QUrl &sourceUrl) {
                         if (!sourceUrl.isEmpty()) {
                             localState.setLastBookUrl(sourceUrl);
                             libraryRepository.recordBookOpened(sourceUrl,
                                                                reader.title(),
                                                                reader.author(),
                                                                reader.formatName());
                         }
                     });
    QObject::connect(&app,
                     &QCoreApplication::aboutToQuit,
                     &app,
                     [&annotationStore,
                      &desktopIntegration,
                      &localState,
                      &oneDriveLibraryService]() {
                         annotationStore.sync();
                         desktopIntegration.saveWindowState();
                         localState.sync();
                         if (oneDriveLibraryService.configured()) {
                             oneDriveLibraryService.synchronizeNow();
                         }
                     });

    auto engine = std::make_unique<QQmlApplicationEngine>();
    LocalizationController localizationController(&localState, engine.get());
    engine->setInitialProperties({
        {QStringLiteral("readerController"),
         QVariant::fromValue(static_cast<QObject *>(&reader))},
        {QStringLiteral("localStateStore"),
         QVariant::fromValue(static_cast<QObject *>(&localState))},
        {QStringLiteral("profileArchiveService"),
         QVariant::fromValue(static_cast<QObject *>(&profileArchiveService))},
        {QStringLiteral("localizationController"),
         QVariant::fromValue(static_cast<QObject *>(&localizationController))},
        {QStringLiteral("libraryModel"),
         QVariant::fromValue(static_cast<QObject *>(&libraryModel))},
        {QStringLiteral("readingDocumentFormatter"),
         QVariant::fromValue(static_cast<QObject *>(&documentFormatter))},
        {QStringLiteral("readingSearchController"),
         QVariant::fromValue(static_cast<QObject *>(&searchController))},
        {QStringLiteral("readingAnnotationStore"),
         QVariant::fromValue(static_cast<QObject *>(&annotationStore))},
        {QStringLiteral("notesCenterModel"),
         QVariant::fromValue(static_cast<QObject *>(&notesCenterModel))},
        {QStringLiteral("librarySearchModel"),
         QVariant::fromValue(static_cast<QObject *>(&librarySearchModel))},
        {QStringLiteral("oneDriveLibraryService"),
         QVariant::fromValue(static_cast<QObject *>(&oneDriveLibraryService))},
        {QStringLiteral("desktopIntegration"),
         QVariant::fromValue(static_cast<QObject *>(&desktopIntegration))},
        {QStringLiteral("diagnosticService"),
         QVariant::fromValue(static_cast<QObject *>(&diagnosticService))},
        {QStringLiteral("applicationInfo"),
         QVariant::fromValue(static_cast<QObject *>(&applicationInfo))}
    });

    QObject::connect(
        engine.get(),
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine->loadFromModule("SZHBooks", "Main");

    if (!engine->rootObjects().isEmpty()) {
        desktopIntegration.attachWindow(
            qobject_cast<QWindow *>(engine->rootObjects().constFirst()));
        desktopIntegration.setReady();
    }

    const int exitCode = app.exec();
    engine.reset();
    return exitCode;
}
