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

#include "service.h"
#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
#include "settings.h"
#endif
#include <QtPlugin>
#include <QCoreApplication>

namespace { const QString serviceKey("qmfsettings"); }


class QmfConfigurator : public QMailMessageServiceConfigurator
{
public:
    QmfConfigurator();
    ~QmfConfigurator();

    virtual QString service() const;
    virtual QString displayName() const;

#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
    virtual QMailMessageServiceEditor *createEditor(QMailMessageServiceFactory::ServiceType type);
#endif
};

QmfConfigurator::QmfConfigurator()
{
}

QmfConfigurator::~QmfConfigurator()
{
}

QString QmfConfigurator::service() const
{
    return serviceKey;
}

QString QmfConfigurator::displayName() const
{
    return QCoreApplication::instance()->translate("QMailMessageService", "Mailfile");
}

#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
QMailMessageServiceEditor *QmfConfigurator::createEditor(QMailMessageServiceFactory::ServiceType type)
{
    if (type == QMailMessageServiceFactory::Storage)
        return new QmfSettings;

    return 0;
}
#endif

Q_EXPORT_PLUGIN2(qmfsettings,QmfServicePlugin)

QmfServicePlugin::QmfServicePlugin()
    : QMailMessageServicePlugin()
{
}

QString QmfServicePlugin::key() const
{
    return serviceKey;
}

bool QmfServicePlugin::supports(QMailMessageServiceFactory::ServiceType type) const
{
    return (type == QMailMessageServiceFactory::Storage);
}

bool QmfServicePlugin::supports(QMailMessage::MessageType) const
{
    return true;
}

QMailMessageService *QmfServicePlugin::createService(const QMailAccountId &)
{
    return 0;
}

QMailMessageServiceConfigurator *QmfServicePlugin::createServiceConfigurator()
{
    return new QmfConfigurator();
}


