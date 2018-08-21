/* Copyright (C) Sami SKHIRI, Inc - All Rights Reserved
 * Unauthorized using of this file for commercial purpose, via any medium is strictly prohibited
 * Written by Sami SKHIRI <skhiri.samy@gmail.com>, September 2014
 */

#ifndef PASSWORD_H
#define PASSWORD_H

#include <QWidget>

class QLineEdit;

class PasswordWindow : public QWidget
{
    Q_OBJECT
public:
    PasswordWindow(QWidget* parent = nullptr, const QString& userName = QString());

private slots:
//    void changePassword();

private:
    QLineEdit* m_oldPwd;
    QLineEdit* m_newPwd;
    QString m_name;

    void drawInterface(const QString& userName);
};

#endif  // PASSWORD_H
