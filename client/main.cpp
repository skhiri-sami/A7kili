/* Copyright (C) Sami SKHIRI, Inc - All Rights Reserved
 * Unauthorized using of this file for commercial purpose, via any medium is strictly prohibited
 * Written by Sami SKHIRI <skhiri.samy@gmail.com>, September 2014
 */

#include <QApplication>
#include "connectionwindow.h"

//useful for detecting memLeaks

#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define DEBUG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

#if defined(Q_OS_IOS)
extern "C" int qtmn(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    QApplication app(argc, argv);

    ConnectionWindow cw;
    cw.show();

    return app.exec();
}
