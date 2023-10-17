#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>

#include "dev_scanner.h"

int main(int argc, char *argv[]) {
    QCoreApplication app{argc, argv};
    QCommandLineParser parser;
    QCommandLineOption num_of_monitors{{"nm", "Sets expected number of monitors in system"}, "num_of_monitors", "1"};
    parser.addOption(num_of_monitors);
    parser.process(app);
    int nm = 1;
    if(parser.isSet("nm")) nm = parser.value("nm").toInt();
    qDebug() << "Number of expected screens = " << nm;
    DevScanner scanner{nm};
    ConsoleLogObserver client{};
    client.subscribe(scanner);
    scanner.listen();
    return 0;
}
