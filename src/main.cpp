#include <QCoreApplication>
#include <QCommandLineParser>

#include <QtNetwork/QtNetwork>

#include "it100mqtt.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // default
    QString settingsPath = "settings.cfg";

    QCommandLineParser parser;
    parser.addPositionalArgument("config","config");
    parser.process(a);

    const QStringList args = parser.positionalArguments();
    if (args.length()) settingsPath = args.at(0);

    It100Mqtt app(settingsPath);
    
    if (!app.failed()) return a.exec();
}
