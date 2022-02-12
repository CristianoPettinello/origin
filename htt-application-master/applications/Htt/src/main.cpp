#include "MainWindow.h"
#include <QApplication>

using namespace Nidek::Solution::HTT::UserApplication;

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
