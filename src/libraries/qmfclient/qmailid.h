/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAILID_H
#define QMAILID_H

#include "qmailglobal.h"
#include "qmailipc.h"
#include "qmailfolderfwd.h"
#include <QDebug>
#include <QScopedPointer>
#include <QString>
#include <QVariant>

class QMailIdPrivate;

class QMF_EXPORT QMailAccountId
{
    QScopedPointer<QMailIdPrivate> d;
public:
    QMailAccountId();
    explicit QMailAccountId(quint64 value);
    QMailAccountId(const QMailAccountId& other);
    virtual ~QMailAccountId();

    QMailAccountId& operator=(const QMailAccountId& other);

    bool isValid() const;
    quint64 toULongLong() const;

    operator QVariant() const;

    bool operator!=(const QMailAccountId& other) const;
    bool operator==(const QMailAccountId& other) const;
    bool operator<(const QMailAccountId& other) const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);
};


class QMF_EXPORT QMailFolderId
{
    QScopedPointer<QMailIdPrivate> d;
public:
    QMailFolderId();
    QMailFolderId(QMailFolderFwd::PredefinedFolderId id);
    explicit QMailFolderId(quint64 value);
    QMailFolderId(const QMailFolderId& other);
    virtual ~QMailFolderId();

    QMailFolderId& operator=(const QMailFolderId& other);

    bool isValid() const;
    quint64 toULongLong() const;

    operator QVariant() const;

    bool operator!=(const QMailFolderId& other) const;
    bool operator==(const QMailFolderId& other) const;
    bool operator<(const QMailFolderId& other) const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);
};


class QMF_EXPORT QMailMessageId
{
    QScopedPointer<QMailIdPrivate> d;
public:
    QMailMessageId();
    explicit QMailMessageId(quint64 value);
    QMailMessageId(const QMailMessageId& other);
    virtual ~QMailMessageId();

    QMailMessageId& operator=(const QMailMessageId& other);

    bool isValid() const;
    quint64 toULongLong() const;

    operator QVariant() const;

    bool operator!=(const QMailMessageId& other) const;
    bool operator==(const QMailMessageId& other) const;
    bool operator<(const QMailMessageId& other) const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);
};


typedef QList<QMailAccountId> QMailAccountIdList;
typedef QList<QMailFolderId> QMailFolderIdList;
typedef QList<QMailMessageId> QMailMessageIdList;

QMF_EXPORT QDebug& operator<< (QDebug& debug, const QMailAccountId &id);
QMF_EXPORT QDebug& operator<< (QDebug& debug, const QMailFolderId &id);
QMF_EXPORT QDebug& operator<< (QDebug& debug, const QMailMessageId &id);

QMF_EXPORT QTextStream& operator<< (QTextStream& s, const QMailAccountId &id);
QMF_EXPORT QTextStream& operator<< (QTextStream& s, const QMailFolderId &id);
QMF_EXPORT QTextStream& operator<< (QTextStream& s, const QMailMessageId &id);

Q_DECLARE_USER_METATYPE(QMailAccountId)
Q_DECLARE_USER_METATYPE(QMailFolderId)
Q_DECLARE_USER_METATYPE(QMailMessageId)

Q_DECLARE_METATYPE(QMailAccountIdList)
Q_DECLARE_METATYPE(QMailFolderIdList)
Q_DECLARE_METATYPE(QMailMessageIdList)

Q_DECLARE_USER_METATYPE_TYPEDEF(QMailAccountIdList, QMailAccountIdList)
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailFolderIdList, QMailFolderIdList)
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailMessageIdList, QMailMessageIdList)

uint QMF_EXPORT qHash(const QMailAccountId &id);
uint QMF_EXPORT qHash(const QMailFolderId &id);
uint QMF_EXPORT qHash(const QMailMessageId &id);

#endif
