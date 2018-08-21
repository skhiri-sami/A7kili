﻿/* Copyright (C) Sami SKHIRI, Inc - All Rights Reserved
 * Unauthorized using of this file for commercial purpose, via any medium is strictly prohibited
 * Written by Sami SKHIRI <skhiri.samy@gmail.com>, September 2014
 */

#ifndef SUBSCRIPTIONWINDOW_H
#define SUBSCRIPTIONWINDOW_H

#include <QWidget>

class QPushButton;
class QLineEdit;

class SubscriptionWindow : public QWidget
{
    Q_OBJECT

public:
    SubscriptionWindow(QWidget* parent = nullptr);
    ~SubscriptionWindow() {}

private slots:
    void closeWindow();
    void newAccountRequest();

private:
    QPushButton* m_annuler_PB;
    QPushButton* m_valide_PB;
    QLineEdit* m_name_LE;
    QLineEdit* m_password_LE;

    void drawInterface();
    void sendData(const QString&) const;

public:
    void displayError(const QString&);
    void readData(const QString&);
};

#endif  // INSCRIT_H
