/* Copyright (C) Sami SKHIRI, Inc - All Rights Reserved
 * Unauthorized using of this file for commercial purpose, via any medium is strictly prohibited
 * Written by Sami SKHIRI <skhiri.samy@gmail.com>, September 2014
 */

#include <QVBoxLayout>
#include <QDir>
#include <QTextCodec>
#include <QTcpServer>
#include <QTcpSocket>

#include "ui_serverwindow.h"
#include "serverwindow.h"
#include "datamanager.h"
#include "consts.h"

ServerWindow::ServerWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    m_nbConnectedClients = 0;
    m_nbTCPConnections = 0;
    m_messageSize = 0;
    DrawInterface();

    m_serveur = new QTcpServer(this);
    if (!m_serveur->listen(QHostAddress::Any, 50885))
    {
        ui->L_serverStatus->setText(QString::fromUtf8("Le serveur n'a pas pu être démarré. Raison :<br />") +
                                    m_serveur->errorString());
    }
    else
    {
        ui->L_serverStatus->setText(QString::fromUtf8("Le serveur a été démarré sur le port <strong>") +
                                    QString::number(m_serveur->serverPort()) +
                                    "</strong>.<br />Des clients peuvent maintenant se connecter.");

        connect(m_serveur, SIGNAL(newConnection()), this, SLOT(nouvelleConnexion()));
    }
}

ServerWindow::~ServerWindow()
{
    if (m_serveur)
    {
        m_serveur->close();
        delete m_serveur;
    }
    delete ui;
}

void ServerWindow::nouvelleConnexion()
{
    m_nbTCPConnections++;
    ui->L_tcpNumber->setText("Connexions TCP: " + QString::number(m_nbTCPConnections));
    QTcpSocket* newClient = m_serveur->nextPendingConnection();
    m_clients << newClient;
    connect(newClient, SIGNAL(readyRead()), this, SLOT(donneesRecues()));
    connect(newClient, SIGNAL(disconnected()), this, SLOT(deconnexionClient()));
    log("[new TcpConnection]: ");
}

void ServerWindow::donneesRecues()
{
    // 1 : on reçoit un paquet (ou un sous-paquet) d'un des clients

    // On détermine quel client envoie le message (recherche du QTcpSocket du client)
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket == 0)  // Si par hasard on n'a pas trouvé le client à l'origine du signal, on arrête la méthode
        return;

    // Si tout va bien, on continue : on récupère le message
    QDataStream in(socket);

    if (m_messageSize == 0)  // Si on ne connaît pas encore la taille du message, on essaie de la récupérer
    {
        if (socket->bytesAvailable() < (int)sizeof(quint16))  // On n'a pas reçu la taille du message en entier
            return;

        in >> m_messageSize;  // Si on a reçu la taille du message en entier, on la récupère
    }

    // Si on connaît la taille du message, on vérifie si on a reçu le message en entier
    if (socket->bytesAvailable() < m_messageSize)  // Si on n'a pas encore tout reçu, on arrête la méthode
        return;

    // Si ces lignes s'exécutent, c'est qu'on a reçu tout le message : on peut le récupérer !
    QString message;
    in >> message;
    log("[donneesRecues]: " + message);
    UserCommand userCMD = getCommand(message);

    //request handling
    switch (userCMD)
    {
        case UserCommand::newAccount:
            subscribe(message, socket);
            break;
        case UserCommand::connectRequest:
            connectUser(message, socket);
            break;
        case UserCommand::sendRequest:
            handleMessage(message, socket);
            break;
        case UserCommand::sentMessagesList:
        case UserCommand::receivedMessagesList:
            sendSavedMessages(message, socket, userCMD);
            break;
        case UserCommand::deleteMessage:
            deleteMessage(message, socket);
        default:
            break;
    }

    // 3 : remise de la taille du message à 0 pour permettre la réception des futurs messages
    m_messageSize = 0;
}

void ServerWindow::deconnexionClient()
{
    // On détermine quel client se déconnecte
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket == 0)
        return;

    log("[deconnexionClient]: " + m_users.value(socket));
    m_clients.removeOne(socket);
    if (m_users.contains(socket))
    {
        QString userName = m_users.value(socket);
        m_users.remove(socket);
        NotifyOthers(userName, false);
        if (m_nbConnectedClients > 0)
            m_nbConnectedClients--;
    }

    socket->deleteLater();
    m_nbTCPConnections--;
    ui->L_clientsNumber->setText("Nb clients :" + QString::number(m_nbConnectedClients));
    ui->L_tcpNumber->setText("Connexions TCP: " + QString::number(m_nbTCPConnections));
}

void ServerWindow::DumpConnectedClients()
{
    QList<QString> connectedUsers = m_users.values();
    ui->TE_log->clear();
    log(QString::number(connectedUsers.size()) + " connected Clients :");

    for (int i = 0; i < connectedUsers.size(); i++)
        log(connectedUsers[i]);
}

void ServerWindow::NotifyOthers(const QString& UserName, bool newConnection)
{
    QString message = _serverToken + _tcpSeparator;
    if (newConnection)
        message += _usersConnected;
    else
        message += _usersDIsconnected;

    message += _tcpSeparator + UserName;
    log("---NotifyOther---");
    sendToAll(message);
}

void ServerWindow::sendToAll(const QString& message) const
{
    QList<QTcpSocket*> connectedUsers = m_users.keys();
    for (auto it = connectedUsers.cbegin(); it != connectedUsers.cend(); ++it)
    {
        sendData(message, *it);
    }
}

/*
void ServerWindow::envoyerA(const QString &message,QString &dest)
{
    // Préparation du paquet
    QByteArray paquet;
    QDataStream out(&paquet, QIODevice::WriteOnly);

    out << (quint16) 0; // On écrit 0 au début du paquet pour réserver la place pour écrire la taille
    out << message; // On ajoute le message à la suite
    out.device()->seek(0); // On se replace au début du paquet
    out << (quint16) (paquet.size() - sizeof(quint16));
    m_users.key(dest)->write(paquet);
//    m_users.key(dest)->flush();
}
*/
void ServerWindow::sendUsersList(QTcpSocket* NewUser) const
{
    QString list = _serverToken + _tcpSeparator + _usersConnected + _tcpSeparator;
    QList<QString> usersList = m_users.values();
    log("SendUserList");
    int size = usersList.size();
    for (int i = 0; i < size - 1; i++)
    {
        list += usersList[i] + _tcpSeparator;
    }
    if (size > 0)
        list += usersList[size - 1];

    sendData(list, NewUser);
}

int ServerWindow::newMessagesCount(QString& userName)
{
    return DataManager::Instance()->MessagesCount(userName, NewMessage);
}

void ServerWindow::subscribe(const QString data, QTcpSocket* userSocket)
{
    log("[subscribe]: " + data);
    QString ServerResponse;
    QString login = data.split(_tcpSeparator).at(1);
    if (!findUser(login))
    {
        QString password = data.split(_tcpSeparator).at(2);
        QString newUser = login + _textSeparator + password + _newLine;
        QFile mdp("passwd.txt");
        if (appendToFile(mdp, newUser))
        {
            QDir currentDirectory(".");
            currentDirectory.mkdir(login);
            ServerResponse = _serverToken + _tcpSeparator + _newAccount + _tcpSeparator + _serverOk;
        }
        else
            ServerResponse =
                _serverToken + _tcpSeparator + _serverError + _tcpSeparator + "Could not insert a new user";
    }
    else
        ServerResponse = _serverToken + _tcpSeparator + _serverError + _tcpSeparator + "Login is already in use";

    sendData(ServerResponse, userSocket);
    userSocket->disconnectFromHost();
}

void ServerWindow::connectUser(const QString data, QTcpSocket* userSocket)
{
    log("[connection request]" + data);
    QStringList args = data.split(_tcpSeparator);

    // check if the client is already connected
    if (m_users.values().contains(args[1]))
    {
        sendData(_serverToken + _tcpSeparator + _serverError + _tcpSeparator + "User is already connected", userSocket);
        userSocket->disconnectFromHost();
    }
    else if (checkCredentials(args[1] + _textSeparator + args[2]))
    {
        sendData(_serverToken + _tcpSeparator + _connect + _tcpSeparator + _serverOk, userSocket);
        sendUsersList(userSocket);
        int count = newMessagesCount(args[1]);
        if (count > 0)
            sendData(_serverToken + _tcpSeparator + _newMessage + _tcpSeparator + QString::number(count), userSocket);

        NotifyOthers(args[1], true);
        m_users[userSocket] = args[1];
        m_nbConnectedClients++;
        ui->L_clientsNumber->setText("Nb clients :" + QString::number(m_nbConnectedClients));
        //        messagesnonrecu(a);
        log(args[1] + " is connected");
    }
    else
    {
        sendData(_serverToken + _tcpSeparator + _serverError + _tcpSeparator + "Incorrect Login or password",
                 userSocket);
        userSocket->disconnectFromHost();
    }
}

void ServerWindow::handleMessage(const QString data, QTcpSocket* userSocket)
{
    if (m_users.contains(userSocket))
    {
        QStringList args = data.split(_tcpSeparator);
        QString Tcpmessage, Logmessage;

        if (findUser(args[1]))  // si le client est inscrit
        {
            Tcpmessage = _serverToken + _tcpSeparator + _instantMessage + _tcpSeparator + m_users[userSocket] +
                         _tcpSeparator + args[2];

            Logmessage = QDateTime::currentDateTime().toString(Qt::ISODate) + _textSeparator;

            DataManager::Instance()->saveMessage(
                m_users[userSocket], Logmessage + args[1] + _textSeparator + args[2] + _newLine, SentMessage);
            if (m_users.key(args[1]) != nullptr)  // s'il est connecté
            {
                // envoyerA(Tcpmessage, args[1]);
                sendData(Tcpmessage, m_users.key(args[1]));
                DataManager::Instance()->saveMessage(
                    args[1], Logmessage + m_users[userSocket] + _textSeparator + args[2] + _newLine, ReceivedMessage);
            }
            else
            {
                DataManager::Instance()->saveMessage(args[1],
                                                     Logmessage + m_users[userSocket] + _textSeparator + args[2] +
                                                         _textSeparator + QString::number(1) + _newLine,
                                                     ReceivedMessage);
            }
        }
        else
        {
            Tcpmessage =
                _serverToken + _tcpSeparator + _serverError + _tcpSeparator + args[1] + " is not a member of A7kili";
            sendData(Tcpmessage, userSocket);
        }
    }
}

void ServerWindow::sendSavedMessages(const QString msg, QTcpSocket* userSocket, UserCommand userCMD)
{
    QStringList args = msg.split(_tcpSeparator);
    QString messageList = _serverToken + _tcpSeparator;
    // Check if the user was connected
    if (m_users.contains(userSocket))
    {
        QString userName = m_users[userSocket];
        QDateTime since = QDateTime::fromString(args[1], Qt::ISODate);
        int count = args[2].toInt();
        QStringList messages;
        MessageType type;
        if (userCMD == UserCommand::receivedMessagesList)
        {
            type = MessageType::ReceivedMessage;
            messageList += _ReceivedMessages + _tcpSeparator;
        }
        else if (userCMD == UserCommand::sentMessagesList)
        {
            type = MessageType::SentMessage;
            messageList += _SentMessages + _tcpSeparator;
        }
        DataManager::Instance()->getMessages(userName, messages, type, since, count);
        for (auto it = messages.cbegin(); it != messages.cend(); ++it)
        {
            // TODO: remove the \n from the data
            // and the last <tcpSeparator>
            messageList += (*it) + _tcpSeparator;
        }
        int pos = messageList.size() - _tcpSeparator.size();
        messageList.remove(pos, _tcpSeparator.size());
        sendData(messageList, userSocket);
    }
}

//TODO: finish DataManager::deleteMessage()
void ServerWindow::deleteMessage(const QString &msg, QTcpSocket* userSocket)
{
    QStringList args = msg.split(_tcpSeparator);
    if (args.size() < 4)
        return;

    MessageType type = ReceivedMessage;
    QString userName = m_users[userSocket];

    if (args[1] == "Sent")
        type = SentMessage;

    QString ServerResponse = _serverToken + _tcpSeparator + args[0] + _tcpSeparator;
    QString date = QDate::fromString(args[2], "dd/MM/yyyy").toString("yyyy-MM-dd");
    if (DataManager::Instance()->deleteMessage(userName, date+"T"+args[3] ,type))
        ServerResponse += _serverOk;
    else
        ServerResponse += _serverError;

    sendData(ServerResponse, userSocket);

}

//TODO: Move it to the Data manager
bool ServerWindow::checkCredentials(const QString text) const
{
    QFile file("passwd.txt");

    if (!file.exists())
        return false;

    bool found = false;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QByteArray data;
        while ((!file.atEnd()) && (found == false))
        {
            data = file.readLine();
            data.truncate(data.length() - 1);
            if (QString(data) == text)
                found = true;
        }
        file.close();
    }
    return found;
}

//TODO: Should be in the Data Manager
bool ServerWindow::findUser(const QString text) const
{
    QFile file("passwd.txt");

    if (!file.exists())
        return false;

    bool found = false;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QByteArray data;
        while ((!file.atEnd()) && (found == false))
        {
            data = file.readLine();
            QString ligne(data);
            ligne = ligne.split(_textSeparator).at(0);
            if (ligne == text)
                found = true;
        }
        file.close();
    }
    return found;
}

void ServerWindow::sendData(const QString& message, QTcpSocket* client) const
{
    QByteArray paquet;
    QDataStream out(&paquet, QIODevice::WriteOnly);
    // out.setVersion(QDataStream::Qt_4_0);
    // message.prepend("register,");
    out << (quint16)0;
    out << message;
    out.device()->seek(0);
    out << (quint16)(paquet.size() - sizeof(quint16));
    qint64 data = client->write(paquet);
    client->flush();
    client->waitForBytesWritten();
    log("sentData: Size =" + QString::number(data) + ", message = " + message + "\n");
}

bool ServerWindow::appendToFile(QFile& file, const QString data) const
{
    if (! file.open(QIODevice::Append | QIODevice::Text))
        return false;

    QTextStream flux(&file);
    flux << data;
    file.close();
    return true;
}

UserCommand ServerWindow::getCommand(const QString txt) const
{
    if (txt.isEmpty())
        return UserCommand::error;

    QString cmd = txt.split(_tcpSeparator).at(0);
    if (cmd.isEmpty())
        return UserCommand::error;
    else if (cmd == _newAccount)
        return UserCommand::newAccount;
    else if (cmd == _sendMessage)
        return UserCommand::sendRequest;
    else if (cmd == _connect)
        return UserCommand::connectRequest;
    else if (cmd == _ReceivedMessages)
        return UserCommand::receivedMessagesList;
    else if (cmd == _SentMessages)
        return UserCommand::sentMessagesList;
    else if (cmd == _deleteMessage)
        return UserCommand::deleteMessage;
    else
        return UserCommand::other;
}

void ServerWindow::DrawInterface()
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf8"));
    ui->setupUi(this);
    ui->centralwidget->adjustSize();
    move(600, 200);

    setWindowTitle("A7kili - Serveur");
    this->setWindowIcon(QIcon("tchat.jpg"));
    ui->L_clientsNumber->setText(QString::fromUtf8("Aucun client connecté"));
    ui->L_tcpNumber->setText(QString::fromLatin1("0 connexions Tcp"));

    connect(ui->PB_exit, SIGNAL(clicked()), qApp, SLOT(quit()));
    connect(ui->PB_Clear, SIGNAL(clicked()), ui->TE_log, SLOT(clear()));
    connect(ui->PB_Clients, SIGNAL(clicked()), this, SLOT(DumpConnectedClients()));
}

void ServerWindow::log(const QString text) const
{
    ui->TE_log->append(text);
}
