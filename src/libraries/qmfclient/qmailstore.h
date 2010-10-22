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

#ifndef QMAILSTORE_H
#define QMAILSTORE_H

#include "qmailmessage.h"
#include "qmailmessagekey.h"
#include "qmailmessagesortkey.h"
#include "qmailfolder.h"
#include "qmailfolderkey.h"
#include "qmailfoldersortkey.h"
#include "qmailaccount.h"
#include "qmailaccountkey.h"
#include "qmailaccountsortkey.h"
#include "qmailaccountconfiguration.h"
#include "qmailmessageremovalrecord.h"
#include "qmailglobal.h"

class QMailStore;
class QMailStoreImplementation;

#ifdef QMAILSTOREINSTANCE_DEFINED_HERE
static QMailStore* QMailStoreInstance();
#endif

class QMF_EXPORT QMailStore : public QObject
{
    Q_OBJECT

public:
    enum InitializationState
    {
        Uninitialized = 0,
        InitializationFailed,
        Initialized
    };

    enum ReturnOption
    {
        ReturnAll = 0,
        ReturnDistinct
    };

    enum MessageRemovalOption
    {
        NoRemovalRecord = 1,
        CreateRemovalRecord
    };

    enum ChangeType
    {
        Added = 1,
        Removed,
        Updated,
        ContentsModified
    };

    enum ErrorCode
    {
        NoError = 0,
        InvalidId, 
        ConstraintFailure,
        ContentInaccessible,
        NotYetImplemented,
        ContentNotRemoved,
        FrameworkFault,
        StorageInaccessible
    };

public:
    virtual ~QMailStore();

    static InitializationState initializationState();

    QMailStore::ErrorCode lastError() const;

    bool addAccount(QMailAccount* account, QMailAccountConfiguration* config);
    bool addFolder(QMailFolder* f);
    bool addMessage(QMailMessage* m);
    bool addMessage(QMailMessageMetaData* m);
    bool addMessages(const QList<QMailMessage*>& m);
    bool addMessages(const QList<QMailMessageMetaData*>& m);

    bool removeAccount(const QMailAccountId& id);
    bool removeAccounts(const QMailAccountKey& key);

    bool removeFolder(const QMailFolderId& id, MessageRemovalOption option = NoRemovalRecord);
    bool removeFolders(const QMailFolderKey& key, MessageRemovalOption option = NoRemovalRecord);

    bool removeMessage(const QMailMessageId& id, MessageRemovalOption option = NoRemovalRecord);
    bool removeMessages(const QMailMessageKey& key, MessageRemovalOption option = NoRemovalRecord);

    bool updateAccount(QMailAccount* account, QMailAccountConfiguration* config = 0);
    bool updateAccountConfiguration(QMailAccountConfiguration* config);
    bool updateFolder(QMailFolder* f);
    bool updateMessage(QMailMessage* m);
    bool updateMessage(QMailMessageMetaData* m);
    bool updateMessages(const QList<QMailMessage*>& m);
    bool updateMessages(const QList<QMailMessageMetaData*>& m);
    bool updateMessagesMetaData(const QMailMessageKey& key, const QMailMessageKey::Properties& properties, const QMailMessageMetaData& data);
    bool updateMessagesMetaData(const QMailMessageKey& key, quint64 messageStatus, bool set);

    int countAccounts(const QMailAccountKey& key = QMailAccountKey()) const;
    int countFolders(const QMailFolderKey& key = QMailFolderKey()) const;
    int countMessages(const QMailMessageKey& key = QMailMessageKey()) const;

    int sizeOfMessages(const QMailMessageKey& key = QMailMessageKey()) const;

    const QMailAccountIdList queryAccounts(const QMailAccountKey& key = QMailAccountKey(), const QMailAccountSortKey& sortKey = QMailAccountSortKey(), uint limit = 0, uint offset = 0) const;
    const QMailFolderIdList queryFolders(const QMailFolderKey& key = QMailFolderKey(), const QMailFolderSortKey& sortKey = QMailFolderSortKey(), uint limit = 0, uint offset = 0) const;
    const QMailMessageIdList queryMessages(const QMailMessageKey& key = QMailMessageKey(), const QMailMessageSortKey& sortKey = QMailMessageSortKey(), uint limit = 0, uint offset = 0) const;

    QMailAccount account(const QMailAccountId& id) const;
    QMailAccountConfiguration accountConfiguration(const QMailAccountId& id) const;

    QMailFolder folder(const QMailFolderId& id) const;

    QMailMessage message(const QMailMessageId& id) const;
    QMailMessage message(const QString& uid, const QMailAccountId& accountId) const;

    QMailMessageMetaData messageMetaData(const QMailMessageId& id) const;
    QMailMessageMetaData messageMetaData(const QString& uid, const QMailAccountId& accountId) const;
    const QMailMessageMetaDataList messagesMetaData(const QMailMessageKey& key, const QMailMessageKey::Properties& properties, ReturnOption option = ReturnAll) const;

    const QMailMessageRemovalRecordList messageRemovalRecords(const QMailAccountId& parentAccountId, const QMailFolderId& parentFolderId = QMailFolderId()) const;

    void lock();
    void unlock();

    bool purgeMessageRemovalRecords(const QMailAccountId& parentAccountId, const QStringList& serverUid = QStringList());

    bool registerAccountStatusFlag(const QString& name);
    quint64 accountStatusMask(const QString& name) const;

    bool registerFolderStatusFlag(const QString& name);
    quint64 folderStatusMask(const QString& name) const;

    bool registerMessageStatusFlag(const QString& name);
    quint64 messageStatusMask(const QString& name) const;

    void setRetrievalInProgress(const QMailAccountIdList &ids);
    void setTransmissionInProgress(const QMailAccountIdList &ids);

    bool asynchronousEmission() const;

    void flushIpcNotifications();

    static QMailStore* instance();
#ifdef QMAILSTOREINSTANCE_DEFINED_HERE
    friend QMailStore* QMailStoreInstance();
#endif

signals:
    void errorOccurred(QMailStore::ErrorCode code);

    void accountsAdded(const QMailAccountIdList& ids);
    void accountsRemoved(const QMailAccountIdList& ids);
    void accountsUpdated(const QMailAccountIdList& ids);
    void accountContentsModified(const QMailAccountIdList& ids);

    void messagesAdded(const QMailMessageIdList& ids);
    void messagesRemoved(const QMailMessageIdList& ids);
    void messagesUpdated(const QMailMessageIdList& ids);
    void messageContentsModified(const QMailMessageIdList& ids);

    void messageDataAdded(const QMailMessageMetaDataList &data);
    void messageDataUpdated(const QMailMessageMetaDataList &data);
    void messagePropertyUpdated(const QMailMessageIdList& ids,  const QMailMessageKey::Properties& properties,
                            const QMailMessageMetaData& data);
    void messageStatusUpdated(const QMailMessageIdList& ids, quint64 status, bool set);

    void foldersAdded(const QMailFolderIdList& ids);
    void foldersRemoved(const QMailFolderIdList& ids);
    void foldersUpdated(const QMailFolderIdList& ids);
    void folderContentsModified(const QMailFolderIdList& ids);

    void messageRemovalRecordsAdded(const QMailAccountIdList& ids);
    void messageRemovalRecordsRemoved(const QMailAccountIdList& ids);

    void retrievalInProgress(const QMailAccountIdList &ids);
    void transmissionInProgress(const QMailAccountIdList &ids);

private:
    friend class QMailStoreImplementationBase;
    friend class QMailStorePrivate;
    friend class QMailMessageMetaDataPrivate; // for ensureCustomFields
    friend class tst_QMailStore;
    friend class tst_QMailStoreKeys;

    QMailStore();

    bool updateMessages(const QList<QPair<QMailMessageMetaData*, QMailMessage*> >&);

    QMap<QString, QString> messageCustomFields(const QMailMessageId &id);
    void clearContent();

    void emitErrorNotification(QMailStore::ErrorCode code);
    void emitAccountNotification(ChangeType type, const QMailAccountIdList &ids);
    void emitFolderNotification(ChangeType type, const QMailFolderIdList &ids);
    void emitMessageNotification(ChangeType type, const QMailMessageIdList &ids);
    void emitMessageDataNotification(ChangeType type, const QMailMessageMetaDataList &data);
    void emitMessageDataNotification(const QMailMessageIdList& ids,  const QMailMessageKey::Properties& properties,
                                     const QMailMessageMetaData& data);
    void emitMessageDataNotification(const QMailMessageIdList& ids, quint64 status, bool set);
    void emitRemovalRecordNotification(ChangeType type, const QMailAccountIdList &ids);
    void emitRetrievalInProgress(const QMailAccountIdList &ids);
    void emitTransmissionInProgress(const QMailAccountIdList &ids);

    QMailMessageMetaData dataToTransfer(const QMailMessageMetaData* message);
    QMailMessageMetaDataList dataList(const QList<QMailMessage*>& messages, const QMailMessageIdList& ids);
    QMailMessageMetaDataList dataList(const QList<QMailMessageMetaData*>& messages, const QMailMessageIdList& ids);

    QMailStoreImplementation* d;
};

Q_DECLARE_USER_METATYPE_ENUM(QMailStore::MessageRemovalOption)

#endif