/* Copyright (C) Sami SKHIRI, Inc - All Rights Reserved
 * Unauthorized using of this file for commercial purpose, via any medium is strictly prohibited
 * Written by Sami SKHIRI <skhiri.samy@gmail.com>, September 2014
 */

#include <QApplication>
#include "serverwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // TODO: Seperate the graphical interface from the data
    // Server must Be a singleton
    ServerWindow fenetre;
    fenetre.show();

    return app.exec();
}
