/* Copyright (C) Sami SKHIRI, Inc - All Rights Reserved
 * Unauthorized using of this file for commercial purpose, via any medium is strictly prohibited
 * Written by Sami SKHIRI <skhiri.samy@gmail.com>, September 2014
 */

#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QLineEdit>

#include "consts.h"
#include "subscriptionwindow.h"
#include "connectionmanager.h"

//TODO: Create a string list of the different error messages
// And managing the user locale
SubscriptionWindow::SubscriptionWindow(QWidget* parent) : QWidget(parent)
{
    drawInterface();
}

void SubscriptionWindow::closeWindow()
{
    this->close();
}

void SubscriptionWindow::sendData(const QString& message) const
{
    // TODO: display an error message in case the message wasn't sent
    ConnectionManager::Instance()->sendData(message);
}

void SubscriptionWindow::readData(const QString& receivdMessage)
{
    qDebug() << "readData = " + receivdMessage;
    if (receivdMessage.isEmpty())
    {
        m_valide_PB->setEnabled(true);
        return;
    }

    QStringList result = receivdMessage.split(_tcpSeparator);
    if (result[0] == _serverToken)
    {
        if (result[1] == _newAccount && result[2] == _serverOk)
        {
            QMessageBox::information(this, "info", QString::fromUtf8("Utilisateur ajouté avec succès"));
            m_name_LE->clear();
            m_password_LE->clear();
            m_valide_PB->setEnabled(true);
            closeWindow();
        }
        else
        {
            QMessageBox::information(this, "Error", QString::fromLatin1("le login demandé est déjà utilisé"));
            m_valide_PB->setEnabled(true);
        }
    }
    qDebug() << "/readData";
}

void SubscriptionWindow::newAccountRequest()
{
    qDebug() << "newAccount";
    if (m_name_LE->text().isEmpty() || m_password_LE->text().isEmpty())
        QMessageBox::critical(this, "Erreur", "Login ou mot de passe incorrect");

    else
    {
        ConnectionManager::Instance()->connect(this, 0, "127.0.0.1", 50885);
        QString msg = _newAccount + _tcpSeparator + m_name_LE->text() + _tcpSeparator + m_password_LE->text();

        if (ConnectionManager::Instance()->sendData(msg))
            m_valide_PB->setEnabled(false);
    }
    qDebug() << "/newAccount";
}

void SubscriptionWindow::displayError(const QString& error)
{
    QMessageBox::information(this, "A7kili Client", error);
    m_valide_PB->setEnabled(true);
}

void SubscriptionWindow::drawInterface()
{
    setWindowTitle("inscription");
    this->setWindowIcon(QIcon(":/img/app_icon.png"));
    setFixedSize(258, 130);
    setGeometry(600, 200, 258, 130);
    QPalette palette;
    palette.setBrush(this->backgroundRole(), QBrush(QImage(":/img/registration_background.jpg")));
    this->setPalette(palette);

    QLabel* login_label = new QLabel("Nom d'utilisateur :", this);
    login_label->setGeometry(70, 20, 130, 19);
    m_name_LE = new QLineEdit(this);
    m_name_LE->setGeometry(70, 40, 130, 20);

    QLabel* password_label = new QLabel("Mot de passe:", this);
    password_label->setGeometry(70, 60, 100, 20);
    m_password_LE = new QLineEdit(this);
    m_password_LE->setEchoMode(QLineEdit::Password);
    m_password_LE->setGeometry(70, 80, 130, 20);

    m_valide_PB = new QPushButton("Valider", this);
    m_valide_PB->setGeometry(70, 100, 70, 23);

    m_annuler_PB = new QPushButton("Annuler", this);
    m_annuler_PB->setGeometry(160, 100, 70, 23);

    QObject::connect(m_annuler_PB, SIGNAL(clicked()), this, SLOT(closeWindow()));
    QObject::connect(m_valide_PB, SIGNAL(clicked()), this, SLOT(newAccountRequest()));
}
