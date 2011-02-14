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

#include "imapservice.h"
#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
#include "imapsettings.h"
#endif
#include "imapconfiguration.h"
#include "imapstrategy.h"
#include <QtPlugin>
#include <QTimer>
#include <qmaillog.h>
#include <qmailmessage.h>
#include <qmaildisconnected.h>
#include <QCoreApplication>

namespace { 

const QString serviceKey("imap4");

QMailFolderId mailboxId(const QMailAccountId &accountId, const QString &path)
{
    QMailFolderIdList folderIds = QMailStore::instance()->queryFolders(QMailFolderKey::parentAccountId(accountId) & QMailFolderKey::path(path));
    if (folderIds.count() == 1)
        return folderIds.first();

    return QMailFolderId();
}

QMailFolderIdList statusFolders(const QMailAccountId &accountId, quint64 mask)
{
    return QMailStore::instance()->queryFolders(QMailFolderKey::parentAccountId(accountId) & QMailFolderKey::status(mask));
}
 
}

class ImapService::Source : public QMailMessageSource
{
    Q_OBJECT

public:
    Source(ImapService *service)
        : QMailMessageSource(service),
          _service(service),
          _flagsCheckQueued(false),
          _queuedMailCheckInProgress(false),
          _mailCheckPhase(RetrieveFolders),
          _unavailable(false),
          _synchronizing(false),
          _setMask(0),
          _unsetMask(0)
    {
        connect(&_service->_client, SIGNAL(allMessagesReceived()), this, SIGNAL(newMessagesAvailable()));
        connect(&_service->_client, SIGNAL(messageCopyCompleted(QMailMessage&, QMailMessage)), this, SLOT(messageCopyCompleted(QMailMessage&, QMailMessage)));
        connect(&_service->_client, SIGNAL(messageActionCompleted(QString)), this, SLOT(messageActionCompleted(QString)));
        connect(&_service->_client, SIGNAL(retrievalCompleted()), this, SLOT(retrievalCompleted()));
        connect(&_service->_client, SIGNAL(idleNewMailNotification(QMailFolderId)), this, SLOT(queueMailCheck(QMailFolderId)));
        connect(&_service->_client, SIGNAL(idleFlagsChangedNotification(QMailFolderId)), this, SLOT(queueFlagsChangedCheck()));
        connect(&_service->_client, SIGNAL(matchingMessageIds(QMailMessageIdList)), this, SIGNAL(matchingMessageIds(QMailMessageIdList)));
        connect(&_intervalTimer, SIGNAL(timeout()), this, SLOT(intervalCheck()));
    }
    
    void setIntervalTimer(int interval)
    {
        _intervalTimer.stop();
        if (interval > 0)
            _intervalTimer.start(interval*1000*60); // interval minutes
    }

    virtual QMailStore::MessageRemovalOption messageRemovalOption() const { return QMailStore::CreateRemovalRecord; }

signals:
    void messageActionCompleted(const QMailMessageIdList &ids);

public slots:
    virtual bool retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    virtual bool retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);

    virtual bool retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    virtual bool retrieveMessagePart(const QMailMessagePart::Location &partLocation);

    virtual bool retrieveMessageRange(const QMailMessageId &messageId, uint minimum);
    virtual bool retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum);

    virtual bool retrieveAll(const QMailAccountId &accountId);
    virtual bool exportUpdates(const QMailAccountId &accountId);

    virtual bool synchronize(const QMailAccountId &accountId);

    virtual bool deleteMessages(const QMailMessageIdList &ids);

    virtual bool copyMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId);
    virtual bool moveMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId);
    virtual bool flagMessages(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask);

    virtual bool createFolder(const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId);
    virtual bool deleteFolder(const QMailFolderId &folderId);
    virtual bool renameFolder(const QMailFolderId &folderId, const QString &name);

    virtual bool searchMessages(const QMailMessageKey &searchCriteria, const QString &bodyText, const QMailMessageSortKey &sort);
    virtual bool cancelSearch();

    virtual bool prepareMessages(const QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > &ids);

    void messageCopyCompleted(QMailMessage &message, const QMailMessage &original);
    void messageActionCompleted(const QString &uid);
    void retrievalCompleted();
    void retrievalTerminated();
    void intervalCheck();
    void queueMailCheck(QMailFolderId folderId);
    void queueFlagsChangedCheck();

private:
    bool doDelete(const QMailMessageIdList & ids);

    virtual bool setStrategy(ImapStrategy *strategy, const char *signal = 0);

    virtual void appendStrategy(ImapStrategy *strategy, const char *signal = 0);
    virtual bool initiateStrategy();
    void queueDisconnectedOperations(const QMailAccountId &accountId);

    enum MailCheckPhase { RetrieveFolders = 0, RetrieveMessages, CheckFlags };

    ImapService *_service;
    bool _flagsCheckQueued;
    bool _queuedMailCheckInProgress;
    MailCheckPhase _mailCheckPhase;
    QMailFolderId _mailCheckFolderId;
    bool _unavailable;
    bool _synchronizing;
    QTimer _intervalTimer;
    QList<QMailFolderId> _queuedFolders;
    quint64 _setMask;
    quint64 _unsetMask;
    QList<QPair<ImapStrategy*, QLatin1String> > _pendingStrategies;
};

bool ImapService::Source::retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending)
{
    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    _service->_client.strategyContext()->foldersOnlyStrategy.clearSelection();
    _service->_client.strategyContext()->foldersOnlyStrategy.setBase(folderId);
    _service->_client.strategyContext()->foldersOnlyStrategy.setQuickList(!folderId.isValid());
    _service->_client.strategyContext()->foldersOnlyStrategy.setDescending(descending);
    appendStrategy(&_service->_client.strategyContext()->foldersOnlyStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort)
{
    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    if (!sort.isEmpty()) {
        qWarning() << "IMAP Search sorting not yet implemented!";
    }
    
    QMailFolderIdList folderIds;
    uint adjustedMinimum = minimum ? minimum : INT_MAX; // zero means retrieve all mail
    _service->_client.strategyContext()->retrieveMessageListStrategy.clearSelection();
    _service->_client.strategyContext()->retrieveMessageListStrategy.setMinimum(adjustedMinimum);
    if (folderId.isValid()) {
        folderIds.append(folderId);
        _service->_client.strategyContext()->retrieveMessageListStrategy.setAccountCheck(false);
    } else {
        // Retrieve messages for all folders in the account that have undiscovered messages
        QMailFolderKey accountKey(QMailFolderKey::parentAccountId(accountId));
        QMailFolderKey canSelectKey(QMailFolderKey::status(QMailFolder::MessagesPermitted));
        QMailFolderKey filterKey(accountKey & canSelectKey);
        folderIds = QMailStore::instance()->queryFolders(filterKey, QMailFolderSortKey::id(Qt::AscendingOrder));
        _service->_client.strategyContext()->retrieveMessageListStrategy.setAccountCheck(true);
    }

    _service->_client.strategyContext()->retrieveMessageListStrategy.setOperation(_service->_client.strategyContext(), QMailRetrievalAction::Auto);
    _service->_client.strategyContext()->retrieveMessageListStrategy.selectedFoldersAppend(folderIds);
    appendStrategy(&_service->_client.strategyContext()->retrieveMessageListStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec)
{
    if (messageIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No messages to retrieve"));
        return false;
    }

    if (spec == QMailRetrievalAction::Flags) {
        _service->_client.strategyContext()->updateMessagesFlagsStrategy.clearSelection();
        _service->_client.strategyContext()->updateMessagesFlagsStrategy.selectedMailsAppend(messageIds);
        appendStrategy(&_service->_client.strategyContext()->updateMessagesFlagsStrategy);
        if(!_unavailable)
            return initiateStrategy();
        return true;
    }

    _service->_client.strategyContext()->selectedStrategy.clearSelection();

    // Select the parts to be downloaded according to "spec".
    _service->_client.strategyContext()->selectedStrategy.setOperation(
            _service->_client.strategyContext(), spec);
    QMailMessageIdList completionList;
    QList<QPair<QMailMessagePart::Location, uint> > completionSectionList;
    foreach (const QMailMessageId &id, messageIds) {
        QMailMessage message(id);
        _service->_client.strategyContext()->selectedStrategy.prepareCompletionList(
                _service->_client.strategyContext(), message,
                completionList, completionSectionList);
    }
    _service->_client.strategyContext()->selectedStrategy.selectedMailsAppend(completionList);
    typedef QPair<QMailMessagePart::Location, uint > SectionDescription;
    foreach (const SectionDescription &section, completionSectionList) {
        _service->_client.strategyContext()->selectedStrategy.selectedSectionsAppend(section.first, section.second);
    }

    appendStrategy(&_service->_client.strategyContext()->selectedStrategy);

    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::retrieveMessagePart(const QMailMessagePart::Location &partLocation)
{
    if (!partLocation.containingMessageId().isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No message to retrieve"));
        return false;
    }
    if (!partLocation.isValid(false)) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No part specified"));
        return false;
    }
    if (!QMailMessage(partLocation.containingMessageId()).id().isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Invalid message specified"));
        return false;
    }

    QMailMessage msg(partLocation.containingMessageId());
    if (!msg.contains(partLocation) || msg.partAt(partLocation).contentAvailable()) {
        // Already retrieved (or invalid)
        if (!_unavailable)
            QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
        return true;
    }
    
    _service->_client.strategyContext()->selectedStrategy.clearSelection();
    _service->_client.strategyContext()->selectedStrategy.setOperation(_service->_client.strategyContext(), QMailRetrievalAction::Content);
    _service->_client.strategyContext()->selectedStrategy.selectedSectionsAppend(partLocation);
    appendStrategy(&_service->_client.strategyContext()->selectedStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::retrieveMessageRange(const QMailMessageId &messageId, uint minimum)
{
    if (!messageId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No message to retrieve"));
        return false;
    }
    if (!QMailMessage(messageId).id().isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Invalid message specified"));
        return false;
    }
    // Not tested yet
    if (!minimum) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No minimum specified"));
        return false;
    }
    
    QMailMessage msg(messageId);
    if (msg.contentAvailable()) {
        // Already retrieved
        if (!_unavailable)
            QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
        return true;
    }
    
    QMailMessagePart::Location location;
    location.setContainingMessageId(messageId);

    _service->_client.strategyContext()->selectedStrategy.clearSelection();
    _service->_client.strategyContext()->selectedStrategy.setOperation(_service->_client.strategyContext(), QMailRetrievalAction::Content);
    _service->_client.strategyContext()->selectedStrategy.selectedSectionsAppend(location, minimum);
    appendStrategy(&_service->_client.strategyContext()->selectedStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum)
{
    if (!partLocation.containingMessageId().isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No message to retrieve"));
        return false;
    }
    if (!partLocation.isValid(false)) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No part specified"));
        return false;
    }
    if (!QMailMessage(partLocation.containingMessageId()).id().isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Invalid message specified"));
        return false;
    }
    if (!minimum) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No minimum specified"));
        return false;
    }

    QMailMessage msg(partLocation.containingMessageId());
    if (!msg.contains(partLocation) || msg.partAt(partLocation).contentAvailable()) {
        // Already retrieved (or invalid)
        if (!_unavailable)
            QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
        return true;
    }
    
    _service->_client.strategyContext()->selectedStrategy.clearSelection();
    _service->_client.strategyContext()->selectedStrategy.setOperation(_service->_client.strategyContext(), QMailRetrievalAction::Content);
    _service->_client.strategyContext()->selectedStrategy.selectedSectionsAppend(partLocation, minimum);
    appendStrategy(&_service->_client.strategyContext()->selectedStrategy);

    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::retrieveAll(const QMailAccountId &accountId)
{
    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    _service->_client.strategyContext()->retrieveAllStrategy.clearSelection();
    _service->_client.strategyContext()->retrieveAllStrategy.setBase(QMailFolderId());
    _service->_client.strategyContext()->retrieveAllStrategy.setQuickList(false);
    _service->_client.strategyContext()->retrieveAllStrategy.setDescending(true);
    _service->_client.strategyContext()->retrieveAllStrategy.setOperation(_service->_client.strategyContext(), QMailRetrievalAction::MetaData);
    appendStrategy(&_service->_client.strategyContext()->retrieveAllStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

void ImapService::Source::queueDisconnectedOperations(const QMailAccountId &accountId)
{
    //sync disconnected move and copy operations for account

    QMailAccount account(accountId);
    QMailFolderIdList folderList = QMailStore::instance()->queryFolders(QMailFolderKey::parentAccountId(accountId));
    bool pendingDisconnectedOperations = false;
    _service->_client.strategyContext()->moveMessagesStrategy.clearSelection();

    foreach(const QMailFolderId& folderId, folderList) {
        
        if (!folderId.isValid())
            continue;

        QMailMessageKey movedIntoFolderKey(QMailDisconnected::destinationKey(folderId));
        QMailMessageIdList movedMessages = QMailStore::instance()->queryMessages(movedIntoFolderKey);

        if (movedMessages.isEmpty())
            continue;

        pendingDisconnectedOperations = true;
        _service->_client.strategyContext()->moveMessagesStrategy.appendMessageSet(movedMessages, folderId);
    }

    if (pendingDisconnectedOperations)
        appendStrategy(&_service->_client.strategyContext()->moveMessagesStrategy, SIGNAL(messagesMoved(QMailMessageIdList)));
}

bool ImapService::Source::exportUpdates(const QMailAccountId &accountId)
{
    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }
    
    queueDisconnectedOperations(accountId);

    _service->_client.strategyContext()->exportUpdatesStrategy.clearSelection();
    appendStrategy(&_service->_client.strategyContext()->exportUpdatesStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::synchronize(const QMailAccountId &accountId)
{
    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    queueDisconnectedOperations(accountId);

    _service->_client.strategyContext()->synchronizeAccountStrategy.clearSelection();
    _service->_client.strategyContext()->synchronizeAccountStrategy.setBase(QMailFolderId());
    _service->_client.strategyContext()->synchronizeAccountStrategy.setQuickList(false);
    _service->_client.strategyContext()->synchronizeAccountStrategy.setDescending(true);
    _service->_client.strategyContext()->synchronizeAccountStrategy.setOperation(_service->_client.strategyContext(), QMailRetrievalAction::Auto);
    appendStrategy(&_service->_client.strategyContext()->synchronizeAccountStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::deleteMessages(const QMailMessageIdList &allIds)
{
    // If a server crash has occurred duplicate messages may exist in the store.
    // A duplicate message is one that refers to the same serverUid as another message in the same account & folder.
    // Ensure that when a duplicate message is deleted no message is deleted from the server.

    QMailMessageKey::Properties props(QMailMessageKey::ServerUid | QMailMessageKey::Id);
    QStringList serverUids;
    QMailMessageIdList ids;
    QMailMessageIdList localIds;

    foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(QMailMessageKey::id(allIds), props)) {
        if (!metaData.serverUid().isEmpty()) {
            serverUids.push_back(metaData.serverUid());
            ids.push_back(metaData.id());
        } else {
            localIds.append(metaData.id());
        }
    }
    if (!localIds.isEmpty()) {
        if (!QMailMessageSource::deleteMessages(localIds)) {
            _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Could not delete messages"));
            return false;
        }
        if (ids.isEmpty())
            return true;
    }

    Q_ASSERT(serverUids.size() == ids.size());
    QMailMessageKey accountKey(QMailMessageKey::parentAccountId(_service->accountId()));

    int matching(QMailStore::instance()->countMessages(QMailMessageKey::serverUid(serverUids, QMailDataComparator::Includes) & accountKey));
    Q_ASSERT(matching >= ids.size());

    if (matching == ids.size()) { // no dupes, lets go
        return doDelete(ids);
    } else {
        QMailMessageIdList duplicateIds;
        QMailMessageIdList singularIds;

        for (int i(0) ; i < ids.size() ; ++i) {
            if (QMailStore::instance()->countMessages(QMailMessageKey::serverUid(serverUids[i]) & accountKey) > 1) {
                duplicateIds.push_back(ids[i]);
            } else {
                singularIds.push_back(ids[i]);
            }
        }
        Q_ASSERT(!duplicateIds.empty());

        if (!QMailMessageSource::deleteMessages(duplicateIds)) {
            _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Could not delete messages"));
            return false;
        }

        return doDelete(singularIds);
    }
}

bool ImapService::Source::doDelete(const QMailMessageIdList &ids)
{
    QMailAccountConfiguration accountCfg(_service->accountId());
    ImapConfiguration imapCfg(accountCfg);
    if (imapCfg.canDeleteMail()) {
        // Delete the messages from the server
        _service->_client.strategyContext()->deleteMessagesStrategy.clearSelection();
        _service->_client.strategyContext()->deleteMessagesStrategy.setLocalMessageRemoval(true);
        _service->_client.strategyContext()->deleteMessagesStrategy.selectedMailsAppend(ids);
        appendStrategy(&_service->_client.strategyContext()->deleteMessagesStrategy, SIGNAL(messagesDeleted(QMailMessageIdList)));
        if(!_unavailable)
            return initiateStrategy();
        return true;
    }

    // Just delete the local copies
    return QMailMessageSource::deleteMessages(ids);
}

bool ImapService::Source::copyMessages(const QMailMessageIdList &messageIds, const QMailFolderId &destinationId)
{
    if (messageIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No messages to copy"));
        return false;
    }
    if (!destinationId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Invalid destination folder"));
        return false;
    }

    QMailFolder destination(destinationId);
    if (destination.parentAccountId() == _service->accountId()) {
        _service->_client.strategyContext()->copyMessagesStrategy.clearSelection();
        _service->_client.strategyContext()->copyMessagesStrategy.appendMessageSet(messageIds, destinationId);
        appendStrategy(&_service->_client.strategyContext()->copyMessagesStrategy, SIGNAL(messagesCopied(QMailMessageIdList)));
        if(!_unavailable)
            return initiateStrategy();
        return true;
    }

    // Otherwise create local copies
    return QMailMessageSource::copyMessages(messageIds, destinationId);
}

bool ImapService::Source::moveMessages(const QMailMessageIdList &messageIds, const QMailFolderId &destinationId)
{
    if (messageIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No messages to move"));
        return false;
    }
    if (!destinationId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Invalid destination folder"));
        return false;
    }

    QMailFolder destination(destinationId);
    if (destination.parentAccountId() == _service->accountId()) {
        _service->_client.strategyContext()->moveMessagesStrategy.clearSelection();
        _service->_client.strategyContext()->moveMessagesStrategy.appendMessageSet(messageIds, destinationId);
        appendStrategy(&_service->_client.strategyContext()->moveMessagesStrategy, SIGNAL(messagesMoved(QMailMessageIdList)));
        if(!_unavailable)
            return initiateStrategy();
        return true;
    }

    // Otherwise - if any of these messages are in folders on the server, we should remove them
    QMailMessageIdList serverMessages;

    // Do we need to remove these messages from the server?
    QMailAccountConfiguration accountCfg(_service->accountId());
    ImapConfiguration imapCfg(accountCfg);
    if (imapCfg.canDeleteMail()) {
        serverMessages = QMailStore::instance()->queryMessages(QMailMessageKey::id(messageIds) & QMailMessageKey::parentAccountId(_service->accountId()));
        if (!serverMessages.isEmpty()) {
            // Delete the messages from the server
            _service->_client.strategyContext()->deleteMessagesStrategy.clearSelection();
            _service->_client.strategyContext()->deleteMessagesStrategy.setLocalMessageRemoval(false);
            _service->_client.strategyContext()->deleteMessagesStrategy.selectedMailsAppend(serverMessages);
            appendStrategy(&_service->_client.strategyContext()->deleteMessagesStrategy);
            if(!_unavailable)
                initiateStrategy();
        }
    }

    // Move the local copies
    QMailMessageMetaData metaData;
    metaData.setParentFolderId(destinationId);

    // Clear the server UID, because it won't refer to anything useful...
    metaData.setServerUid(QString());

    QMailMessageKey idsKey(QMailMessageKey::id(messageIds));
    if (!QMailStore::instance()->updateMessagesMetaData(idsKey, QMailMessageKey::ParentFolderId | QMailMessageKey::ServerUid, metaData)) {
        qWarning() << "Unable to update message metadata for move to folder:" << destinationId;
    } else {
        emit messagesMoved(messageIds);
    }

    if (serverMessages.isEmpty()) {
        QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
    }
    return true;
}

bool ImapService::Source::flagMessages(const QMailMessageIdList &messageIds, quint64 setMask, quint64 unsetMask)
{
    if (messageIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No messages to flag"));
        return false;
    }
    if (!setMask && !unsetMask) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No flags to be applied"));
        return false;
    }

    // Update the local copy status immediately
    QMailMessageSource::modifyMessageFlags(messageIds, setMask, unsetMask);


    // See if there are any further actions to be taken
    QMailAccountConfiguration accountCfg(_service->accountId());
    ImapConfiguration imapCfg(accountCfg);

    // Note: we can't do everything all at once - just perform the first change that we
    // identify, as a client can always perform the changes incrementally.

    if ((setMask & QMailMessage::Trash) || (unsetMask & QMailMessage::Trash)) {

        QMailFolderId trashId(QMailAccount(_service->accountId()).standardFolder(QMailFolder::TrashFolder));

        if (trashId.isValid()) {

            _setMask = setMask;
            _unsetMask = unsetMask;

            if (setMask & QMailMessage::Trash) {
                _service->_client.strategyContext()->moveMessagesStrategy.clearSelection();
                _service->_client.strategyContext()->moveMessagesStrategy.appendMessageSet(messageIds, trashId);

                appendStrategy(&_service->_client.strategyContext()->moveMessagesStrategy, SIGNAL(messagesFlagged(QMailMessageIdList)));

                if(!_unavailable)
                    return initiateStrategy();
                return true;

            } else if (_unsetMask & QMailMessage::Trash) {

                QMap<QMailFolderId, QMailMessageIdList> destinationList;
                // These messages need to be restored to their previous locations
                destinationList = QMailDisconnected::restoreMap(messageIds);

                _service->_client.strategyContext()->moveMessagesStrategy.clearSelection();
                QMap<QMailFolderId, QMailMessageIdList>::const_iterator it = destinationList.begin(), end = destinationList.end();
                for ( ; it != end; ++it) {
                    _service->_client.strategyContext()->moveMessagesStrategy.appendMessageSet(it.value(), it.key());
                }

                appendStrategy(&_service->_client.strategyContext()->moveMessagesStrategy, SIGNAL(messagesFlagged(QMailMessageIdList)));
                if(!_unavailable)
                    return initiateStrategy();
                return true;
            }
        }
    }

    if (setMask & QMailMessage::Sent) {
        QMailFolderId sentId(QMailAccount(_service->accountId()).standardFolder(QMailFolder::SentFolder));
        if (sentId.isValid()) {
            _setMask = setMask;
            _unsetMask = unsetMask;

            QMailMessageIdList moveIds;
            QMailMessageIdList flagIds;

            QMailMessageKey key(QMailMessageKey::id(messageIds));
            QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::ParentFolderId);

            foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(key, props)) {
                // If the message is already in the correct location just update the flags to remove \Draft
                if (metaData.parentFolderId() == sentId) {
                    flagIds.append(metaData.id());
                } else {
                    moveIds.append(metaData.id());
                }
            }

            if (!flagIds.isEmpty()) {
                _service->_client.strategyContext()->flagMessagesStrategy.clearSelection();
                _service->_client.strategyContext()->flagMessagesStrategy.setMessageFlags(MFlag_Draft, false);
                _service->_client.strategyContext()->flagMessagesStrategy.selectedMailsAppend(flagIds);
                appendStrategy(&_service->_client.strategyContext()->flagMessagesStrategy, SIGNAL(messagesFlagged(QMailMessageIdList)));
            }
            if (!moveIds.isEmpty()) {
                _service->_client.strategyContext()->moveMessagesStrategy.clearSelection();
                _service->_client.strategyContext()->moveMessagesStrategy.appendMessageSet(moveIds, sentId);
                appendStrategy(&_service->_client.strategyContext()->moveMessagesStrategy, SIGNAL(messagesFlagged(QMailMessageIdList)));
            }
            if(!_unavailable)
                return initiateStrategy();
            else return true;
        }
    }

    if (setMask & QMailMessage::Draft) {
        QMailFolderId draftId(QMailAccount(_service->accountId()).standardFolder(QMailFolder::DraftsFolder));
        if (draftId.isValid()) {
            _setMask = setMask;
            _unsetMask = unsetMask;

            // Move these messages to the predefined location - if they're already in the drafts
            // folder, we still want to overwrite them with the current content in case it has been updated
            _service->_client.strategyContext()->moveMessagesStrategy.clearSelection();
            _service->_client.strategyContext()->moveMessagesStrategy.appendMessageSet(messageIds, draftId);
            appendStrategy(&_service->_client.strategyContext()->moveMessagesStrategy, SIGNAL(messagesFlagged(QMailMessageIdList)));
            if(!_unavailable)
                return initiateStrategy();
            return true;
        }
    }

    quint64 updatableFlags(QMailMessage::Replied | QMailMessage::RepliedAll | QMailMessage::Forwarded);
    if ((setMask & updatableFlags) || (unsetMask & updatableFlags)) {
        // We could hold on to these changes until exportUpdates instead...
        MessageFlags setFlags(0);
        MessageFlags unsetFlags(0);

        if (setMask & (QMailMessage::Replied | QMailMessage::RepliedAll)) {
            setFlags |= MFlag_Answered;
        }
        if (unsetMask & (QMailMessage::Replied | QMailMessage::RepliedAll)) {
            unsetFlags |= MFlag_Answered;
        }

        if ((setMask | unsetMask) & QMailMessage::Forwarded) {
            // We can only modify this flag if the folders support $Forwarded
            bool supportsForwarded(true);

            QMailMessageKey key(QMailMessageKey::id(messageIds));
            QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::ParentFolderId);

            foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(key, props, QMailStore::ReturnDistinct)) {
                QMailFolder folder(metaData.parentFolderId());
                if (folder.customField("qmf-supports-forwarded").isEmpty()) {
                    supportsForwarded = false;
                    break;
                }
            }

            if (supportsForwarded) {
                if (setMask & QMailMessage::Forwarded) {
                    setFlags |= MFlag_Forwarded;
                }
                if (unsetMask & QMailMessage::Forwarded) {
                    unsetFlags |= MFlag_Forwarded;
                }
            }
        }

        if (setFlags || unsetFlags) {
            _service->_client.strategyContext()->flagMessagesStrategy.clearSelection();
            if (setFlags) {
                _service->_client.strategyContext()->flagMessagesStrategy.setMessageFlags(setFlags, true);
            }
            if (unsetFlags) {
                _service->_client.strategyContext()->flagMessagesStrategy.setMessageFlags(unsetFlags, true);
            }
            _service->_client.strategyContext()->flagMessagesStrategy.selectedMailsAppend(messageIds);
            appendStrategy(&_service->_client.strategyContext()->flagMessagesStrategy, SIGNAL(messagesFlagged(QMailMessageIdList)));
            if(!_unavailable)
                return initiateStrategy();
            return true;
        }
    }

    //ensure retrievalCompleted gets called when a strategy has not been used (i.e. local read flag change)
    //otherwise actionCompleted does not get signaled to messageserver and service becomes permanently unavailable

    if(!_unavailable)
        QTimer::singleShot(0, this, SLOT(retrievalCompleted()));

    return true;
}

bool ImapService::Source::createFolder(const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId)
{
    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }
    //here we'll create a QMailFolder and give it to the strategy
    //if it is successful, we'll let it register it as a real folder in the QMailStore
    if(name.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Cannot create empty named folder"));
        return false;
    }

    _service->_client.strategyContext()->createFolderStrategy.createFolder(parentId, name);

    appendStrategy(&_service->_client.strategyContext()->createFolderStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::deleteFolder(const QMailFolderId &folderId)
{
    if(!folderId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Deleting invalid folder"));
        return false;
    }

    // Don't delete messages that the user has moved out of the folder
    QMailFolder folder(folderId);
    queueDisconnectedOperations(folder.parentAccountId());
    
    //remove remote copy
    _service->_client.strategyContext()->deleteFolderStrategy.deleteFolder(folderId);
    appendStrategy(&_service->_client.strategyContext()->deleteFolderStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::renameFolder(const QMailFolderId &folderId, const QString &name)
{
    if(name.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Cannot rename to an empty folder"));
        return false;
    }
    if(!folderId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Cannot rename an invalid folder"));
        return false;
    }

    _service->_client.strategyContext()->renameFolderStrategy.renameFolder(folderId, name);

    appendStrategy(&_service->_client.strategyContext()->renameFolderStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::searchMessages(const QMailMessageKey &searchCriteria, const QString &bodyText, const QMailMessageSortKey &sort)
{
    if(searchCriteria.isEmpty() && bodyText.isEmpty()) {
        //we're not going to do an empty search (which returns all emails..)
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Empty search provided"));
        return false;
    }

    _service->_client.strategyContext()->searchMessageStrategy.searchArguments(searchCriteria, bodyText, sort);
    appendStrategy(&_service->_client.strategyContext()->searchMessageStrategy);
    if(!_unavailable)
        initiateStrategy();
    return true;
}

bool ImapService::Source::cancelSearch()
{
    _service->_client.strategyContext()->searchMessageStrategy.cancelSearch();
    appendStrategy(&_service->_client.strategyContext()->searchMessageStrategy);
    if(!_unavailable)
        initiateStrategy();
   return true;
}

bool ImapService::Source::prepareMessages(const QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > &messageIds)
{
    if (messageIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No messages to prepare"));
        return false;
    }

    QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > unresolved;
    QSet<QMailMessageId> referringIds;
    QMailMessageIdList externaliseIds;

    QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> >::const_iterator it = messageIds.begin(), end = messageIds.end();
    for ( ; it != end; ++it) {
        if (!(*it).second.isValid()) {
            // This message just needs to be externalised
            externaliseIds.append((*it).first.containingMessageId());
        } else {
            // This message needs to be made available for another message's reference
            unresolved.append(*it);
            referringIds.insert((*it).second.containingMessageId());
        }
    }

    if (!unresolved.isEmpty()) {
        bool external(false);

        // Are these messages being resolved for internal or external references?
        QMailMessageKey key(QMailMessageKey::id(referringIds.toList()));
        QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::ParentAccountId | QMailMessageKey::Status);

        foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(key, props)) {
            if ((metaData.parentAccountId() != _service->accountId()) ||
                !(metaData.status() & QMailMessage::TransmitFromExternal)) {
                // This message won't be transmitted by reference from the IMAP server - supply an external reference
                external = true;
                break;
            }
        }

        _service->_client.strategyContext()->prepareMessagesStrategy.setUnresolved(unresolved, external);
        appendStrategy(&_service->_client.strategyContext()->prepareMessagesStrategy, SIGNAL(messagesPrepared(QMailMessageIdList)));
    }

    if (!externaliseIds.isEmpty()) {
        QMailAccountConfiguration accountCfg(_service->accountId());
        ImapConfiguration imapCfg(accountCfg);

        QMailFolderId sentId(QMailAccount(_service->accountId()).standardFolder(QMailFolder::SentFolder));
        // Prepare these messages by copying to the sent folder
        _service->_client.strategyContext()->externalizeMessagesStrategy.clearSelection();
        _service->_client.strategyContext()->externalizeMessagesStrategy.appendMessageSet(externaliseIds, sentId);
        appendStrategy(&_service->_client.strategyContext()->externalizeMessagesStrategy, SIGNAL(messagesPrepared(QMailMessageIdList)));
    }
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::setStrategy(ImapStrategy *strategy, const char *signal)
{
    disconnect(this, SIGNAL(messageActionCompleted(QMailMessageIdList)));
    if (signal) {
        connect(this, SIGNAL(messageActionCompleted(QMailMessageIdList)), this, signal);
    }

    _unavailable = true;
    _service->_client.setStrategy(strategy);
    _service->_client.newConnection();
    return true;
}

void ImapService::Source::appendStrategy(ImapStrategy *strategy, const char *signal)
{
    _pendingStrategies.append(qMakePair(strategy, QLatin1String(signal)));
}

bool ImapService::Source::initiateStrategy()
{
    if (_pendingStrategies.isEmpty())
        return false;

    QPair<ImapStrategy*, QLatin1String> data(_pendingStrategies.takeFirst());
    return setStrategy(data.first, data.second.latin1());
}

// Copy or Move Completed
void ImapService::Source::messageCopyCompleted(QMailMessage &message, const QMailMessage &original)
{
    if (_service->_client.strategy()->error()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Destination message failed to match source message"));
        return;
    }
    if (_setMask || _unsetMask) {
        if (_setMask) {
            message.setStatus(_setMask, true);
        }
        if (_unsetMask) {
            message.setStatus(_unsetMask, false);
        }
    }
    Q_UNUSED(original);
}

void ImapService::Source::messageActionCompleted(const QString &uid)
{
    if (uid.startsWith("id:")) {
        emit messageActionCompleted(QMailMessageIdList() << QMailMessageId(uid.mid(3).toULongLong()));
    } else if (!uid.isEmpty()) {
        QMailMessageMetaData metaData(uid, _service->accountId());
        if (metaData.id().isValid()) {
            emit messageActionCompleted(QMailMessageIdList() << metaData.id());
        }
    }
}

void ImapService::Source::retrievalCompleted()
{
    _unavailable = false;
    _setMask = 0;
    _unsetMask = 0;

    // See if there are any other actions pending
    if (initiateStrategy()) {
        return;
    }

    if (_queuedMailCheckInProgress) {
        if (_mailCheckPhase == RetrieveFolders) {
            _mailCheckPhase = RetrieveMessages;
            retrieveMessageList(_service->accountId(), _mailCheckFolderId, 1, QMailMessageSortKey());
            return;
        } else {
            // Push email must be established
            _service->_establishingPushEmail = false;
            _service->_pushRetry = ThirtySeconds;
            
            _queuedMailCheckInProgress = false;
            emit _service->availabilityChanged(true);
        }
    } else {
        emit _service->actionCompleted(true);
    }

    if (_synchronizing) {
        _synchronizing = false;

        // Mark this account as synchronized
        QMailAccount account(_service->accountId());
        if (!(account.status() & QMailAccount::Synchronized)) {
            account.setStatus(QMailAccount::Synchronized, true);
            QMailStore::instance()->updateAccount(&account);
        }
    }

    if (!_queuedFolders.isEmpty()) {
        queueMailCheck(_queuedFolders.first());
    }
    if (_flagsCheckQueued) {
        queueFlagsChangedCheck();
    }
}

// Interval mail checking timer has expired, perform mail check on all folders
void ImapService::Source::intervalCheck()
{
    _flagsCheckQueued = true; // Convenient for user to check for flag changes on server also
    exportUpdates(_service->accountId()); // Convenient for user to export pending changes also
    queueMailCheck(QMailFolderId());
}

void ImapService::Source::queueMailCheck(QMailFolderId folderId)
{
    if (_unavailable) {
        if (!_queuedFolders.contains(folderId)) {
            _queuedFolders.append(folderId);
        }
        return;
    }

    _queuedFolders.removeAll(folderId);
    _queuedMailCheckInProgress = true;
    _mailCheckPhase = RetrieveFolders;
    _mailCheckFolderId = folderId;

    emit _service->availabilityChanged(false);
    if (folderId.isValid()) {
        retrievalCompleted(); // move onto retrieveMessageList stage
    } else {
        retrieveFolderList(_service->accountId(), folderId, true);
    }
}

void ImapService::Source::queueFlagsChangedCheck()
{
    if (_unavailable) {
        _flagsCheckQueued = true;
        return;
    }
    
    _flagsCheckQueued = false;
    _queuedMailCheckInProgress = true;
    _mailCheckPhase = CheckFlags;

    emit _service->availabilityChanged(false);
    
    // Check same messages as last time
    appendStrategy(&_service->_client.strategyContext()->updateMessagesFlagsStrategy);
    if(!_unavailable)
        initiateStrategy();
}

void ImapService::Source::retrievalTerminated()
{
    _unavailable = false;
    _synchronizing = false;
    if (_queuedMailCheckInProgress) {
        _queuedMailCheckInProgress = false;
        emit _service->availabilityChanged(true);
    }
    
    // Just give up if an error occurs
    _queuedFolders.clear();
    _flagsCheckQueued = false;
}


ImapService::ImapService(const QMailAccountId &accountId)
    : QMailMessageService(),
      _client(this),
      _source(new Source(this)),
      _establishingPushEmail(false),
      _pushRetry(ThirtySeconds)
{
    connect(&_client, SIGNAL(progressChanged(uint, uint)), this, SIGNAL(progressChanged(uint, uint)));
    connect(&_client, SIGNAL(errorOccurred(int, QString)), this, SLOT(errorOccurred(int, QString)));
    connect(&_client, SIGNAL(errorOccurred(QMailServiceAction::Status::ErrorCode, QString)), this, SLOT(errorOccurred(QMailServiceAction::Status::ErrorCode, QString)));
    connect(&_client, SIGNAL(updateStatus(QString)), this, SLOT(updateStatus(QString)));
    connect(&_client, SIGNAL(restartPushEmail()), this, SLOT(restartPushEmail()));

    _client.setAccount(accountId);
    QMailAccountConfiguration accountCfg(accountId);
    ImapConfiguration imapCfg(accountCfg);
    if (imapCfg.pushEnabled()) {
        initiatePushEmail();
    }
    _source->setIntervalTimer(imapCfg.checkInterval());
}

ImapService::~ImapService()
{
    delete _source;
}

QString ImapService::service() const
{
    return serviceKey;
}

QMailAccountId ImapService::accountId() const
{
    return _client.account();
}

bool ImapService::hasSource() const
{
    return true;
}

QMailMessageSource &ImapService::source() const
{
    return *_source;
}

bool ImapService::available() const
{
    return true;
}

bool ImapService::cancelOperation(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    _client.cancelTransfer(code, text);
    _client.closeConnection();
    _source->retrievalTerminated();
    return true;
}

void ImapService::restartPushEmail()
{
    cancelOperation(QMailServiceAction::Status::ErrInternalStateReset, tr("Initiating push email"));
    initiatePushEmail();
}
    
void ImapService::initiatePushEmail()
{    
    QMailFolderIdList ids(_client.configurationIdleFolderIds());
    if (ids.count()) {
        _establishingPushEmail = true;
        foreach(QMailFolderId id, ids) {
            _source->queueMailCheck(id);
        }
    }
}

bool ImapService::pushEmailEstablished()
{
    if (!_establishingPushEmail)
        return true;
    if (_client.idlesEstablished())
        return true;

    const int oneHour = 60*60;
    qMailLog(Messaging) << "Push email connection could not be established. Reattempting to establish in" << _pushRetry << "seconds";
    QTimer::singleShot(_pushRetry * 1000, this, SLOT(restartPushEmail()));
    _pushRetry = qMin(oneHour, _pushRetry * 2);
    return false;
}

void ImapService::errorOccurred(int code, const QString &text)
{
    if (!pushEmailEstablished())
        return;
    _source->retrievalTerminated();
    updateStatus(code, text, _client.account());
    emit actionCompleted(false);
}

void ImapService::errorOccurred(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    if (!pushEmailEstablished())
        return;
    _source->retrievalTerminated();
    updateStatus(code, text, _client.account());
    emit actionCompleted(false);
}

void ImapService::updateStatus(const QString &text)
{
    updateStatus(QMailServiceAction::Status::ErrNoError, text, _client.account());
}

class ImapConfigurator : public QMailMessageServiceConfigurator
{
public:
    ImapConfigurator();
    ~ImapConfigurator();

    virtual QString service() const;
    virtual QString displayName() const;

#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
    virtual QMailMessageServiceEditor *createEditor(QMailMessageServiceFactory::ServiceType type);
#endif
};

ImapConfigurator::ImapConfigurator()
{
}

ImapConfigurator::~ImapConfigurator()
{
}

QString ImapConfigurator::service() const
{
    return serviceKey;
}

QString ImapConfigurator::displayName() const
{
    return QCoreApplication::instance()->translate("QMailMessageService", "IMAP");
}

#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
QMailMessageServiceEditor *ImapConfigurator::createEditor(QMailMessageServiceFactory::ServiceType type)
{
    if (type == QMailMessageServiceFactory::Source)
        return new ImapSettings;

    return 0;
}
#endif

Q_EXPORT_PLUGIN2(imap,ImapServicePlugin)

ImapServicePlugin::ImapServicePlugin()
    : QMailMessageServicePlugin()
{
}

QString ImapServicePlugin::key() const
{
    return serviceKey;
}

bool ImapServicePlugin::supports(QMailMessageServiceFactory::ServiceType type) const
{
    return (type == QMailMessageServiceFactory::Source);
}

bool ImapServicePlugin::supports(QMailMessage::MessageType type) const
{
    return (type == QMailMessage::Email);
}

QMailMessageService *ImapServicePlugin::createService(const QMailAccountId &id)
{
    return new ImapService(id);
}

QMailMessageServiceConfigurator *ImapServicePlugin::createServiceConfigurator()
{
    return new ImapConfigurator();
}


#include "imapservice.moc"

