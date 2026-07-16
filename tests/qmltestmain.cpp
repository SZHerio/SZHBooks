#include <QGuiApplication>
#include <QQuickStyle>
#include <QtQuickTest/quicktest.h>

int main(int argc, char **argv)
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    return quick_test_main(argc,
                           argv,
                           "szhbooks_qmlui",
                           QUICK_TEST_SOURCE_DIR);
}
