/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef EMAILCLIENT_H
#define EMAILCLIENT_H

#include "messagelistview.h"
#include <qmailmessageserver.h>
#include <qmailserviceaction.h>
#include <qmailviewer.h>
#include <QList>
#include <QMainWindow>
#include <QStack>
#include <QTime>
#include <QTimer>
#include <QProcess>

class MessageFolder;
class MessageStore;
class EmailFolderModel;
class EmailFolderView;
class SearchView;
class UILocation;
class ReadMail;
class WriteMail;
class SearchProgressDialog;
class QAction;
class QMailAccount;
class QMailMessageSet;
class QMailRetrievalAction;
class QMailSearchAction;
class QMailTransmitAction;
class QMailStorageAction;
class QStackedWidget;
class QStringList;

class MessageUiBase : public QWidget
{
    Q_OBJECT

public:
    MessageUiBase(QWidget* parent);
    virtual ~MessageUiBase() {}

signals:
    void statusVisible(bool);
    void updateStatus(const QString&);
    void updateProgress(uint, uint);
    void clearStatus();

protected:
    void viewSearchResults(const QMailMessageKey& filter, const QString& title = QString());
    void viewComposer(const QString& title = QString());

protected:
    WriteMail* writeMailWidget() const;
    ReadMail* readMailWidget() const;
    EmailFolderView* folderView() const;
    MessageListView* messageListView() const;
    MessageStore* messageStore() const;
    EmailFolderModel* emailFolderModel() const;

    void suspendMailCounts();
    void resumeMailCounts();
    virtual void contextStatusUpdate();
    virtual void showFolderStatus(QMailMessageSet* item);
    virtual void setMarkingMode(bool set);
    virtual void clearStatusText();

    virtual WriteMail* createWriteMailWidget();
    virtual ReadMail* createReadMailWidget();
    virtual EmailFolderView* createFolderView();
    virtual MessageListView* createMessageListView();
    virtual MessageStore* createMessageStore();
    virtual EmailFolderModel* createEmailFolderModel();

protected slots:
    virtual void messageSelectionChanged();
    virtual void presentMessage(const QMailMessageId &, QMailViewerFactory::PresentationType);

    // Slots to be implemented by subclass
    virtual void folderSelected(QMailMessageSet*) = 0;
    virtual void composeActivated() = 0;
    virtual void emptyTrashFolder() = 0;
    virtual void messageActivated() = 0;
    virtual void allWindowsClosed() = 0;

protected:
    QString appTitle;
    bool suspendMailCount;
    bool markingMode;
    QMailMessageId selectedMessageId;
    int selectionCount;
    bool emailCountSuspended;
};


class EmailClient : public MessageUiBase
{
    Q_OBJECT

public:
    EmailClient(QWidget* parent);
    ~EmailClient();

    bool cleanExit(bool force);
    bool closeImmediately();

public slots:
    void sendMessageTo(const QMailAddress &address, QMailMessage::MessageType type);
    void quit();

protected:
    void setupUi();
    void showEvent(QShowEvent* e);

protected slots:
    void cancelOperation();

    void noSendAccount(QMailMessage::MessageType);
    void enqueueMail(const QMailMessage&);
    void saveAsDraft(const QMailMessage&);
    void discardMail();

    void sendAllQueuedMail(bool userRequest = false);
    void sendSingleMail(QMailMessageMetaData& message);

    void getSingleMail(const QMailMessageMetaData& message);
    void retrieveMessagePart(const QMailMessagePart::Location& partLocation);
    void messageActivated();
    void emptyTrashFolder();

    void moveSelectedMessages();
    void copySelectedMessages();
    void restoreSelectedMessages();
    void deleteSelectedMessages();

    void moveSelectedMessagesTo(MessageFolder* destination);
    void copySelectedMessagesTo(MessageFolder* destination);

    void selectAll();

    void connectivityChanged(QMailServiceAction::Connectivity connectivity);
    void activityChanged(QMailServiceAction::Activity activity);
    void statusChanged(const QMailServiceAction::Status &status);
    void progressChanged(uint progress, uint total);

    void transmitCompleted();
    void retrievalCompleted();
    void storageActionCompleted();
    void searchCompleted();

    void folderSelected(QMailMessageSet*);

    void composeActivated();

    void getAllNewMail();
    void getAccountMail();
    void getNewMail();

    void synchronizeFolder();

    void updateAccounts();

    void transferFailure(const QMailAccountId& accountId, const QString&, int);
    void storageActionFailure(const QMailAccountId& accountId, const QString&);

    void setStatusText(QString &);

    void search();
    void searchRequested();

    void automaticFetch();

    void externalEdit(const QString &);

    void resend(const QMailMessage& message, int);
    void modify(const QMailMessage& message);

    void retrieveMoreMessages();

    bool removeMessage(const QMailMessageId& id, bool userRequest);

    void readReplyRequested(const QMailMessageMetaData&);

    void settings();

    void accountsAdded(const QMailAccountIdList& ids);
    void accountsRemoved(const QMailAccountIdList& ids);
    void accountsUpdated(const QMailAccountIdList& ids);

    void messagesUpdated(const QMailMessageIdList& ids);

    void messageSelectionChanged();

    void allWindowsClosed();

private slots:
    void delayedInit();
    void openFiles();
    void initActions();
    void updateActions();
    void markMessages();
    void resumeInterruptedComposition();
    bool startMessageServer();
    bool waitForMessageServer();
    void messageServerProcessError(QProcess::ProcessError);

private:
    bool isMessageServerRunning() const;
    virtual EmailFolderView* createFolderView();
    virtual MessageListView* createMessageListView();

    void init();
    void userInvocation();

    void mailResponded();

    void closeAfterTransmissionsFinished();
    bool isTransmitting();
    bool isSending();
    bool isRetrieving();

    bool checkMailConflict(const QString& msg1, const QString& msg2);
    void setNewMessageCount(QMailMessage::MessageType type, uint);

    void readSettings();
    bool saveSettings();

    void displayCachedMail();

    void accessError(const MessageFolder &box);
    void copyError(const MessageFolder &dest);
    void moveError(const MessageFolder &dest);

    void getNextNewMail();
    bool verifyAccount(const QMailAccountId &accountId, bool outgoing);

    void setSendingInProgress(bool y);
    void setRetrievalInProgress(bool y);

    void transferStatusUpdate(int status);
    void setSuspendPermitted(bool y);
    void clearOutboxFolder();

    void updateGetMailButton();
    void updateGetAccountButton();

    QString mailType(QMailMessage::MessageType type);

    void setActionVisible(QAction*, bool);

    bool restoreMessages(const QMailMessageIdList& ids, MessageFolder*);
    bool deleteMessages(const QMailMessageIdList& ids, MessageFolder*);

    void contextStatusUpdate();

    void setMarkingMode(bool set);

    MessageFolder* containingFolder(const QMailMessageId& id);

    bool applyToSelectedFolder(void (EmailClient::*function)(MessageFolder*));

    void sendFailure(const QMailAccountId &accountId);
    void receiveFailure(const QMailAccountId &accountId);

    void closeApplication();

    QMailAccountIdList emailAccounts() const;

    SearchProgressDialog* searchProgressDialog() const;

    void clearNewMessageStatus(const QMailMessageKey& key);

private:
    // Whether the initial action for the app was to view incoming messages 
    enum InitialAction { None = 0, IncomingMessages, NewMessages, View, Compose, Cleanup };

    bool filesRead;
    QMailMessageId cachedDisplayMailId;

    int transferStatus;
    int primaryActivity;

    uint totalSize;

    QMailAccountId mailAccountId;

    QAction *getMailButton;
    QAction *getAccountButton;
    QAction *composeButton;
    QAction *searchButton;
    QAction *cancelButton;
    QAction *synchronizeAction;
    QAction *settingsAction;
    QAction *emptyTrashAction;
    QAction *deleteMailAction;
    QAction *markAction;
    bool enableMessageActions;

    QAction *moveAction;
    QAction *copyAction;
    QAction *restoreAction;
    QAction *selectAllAction;
    bool closeAfterTransmissions;
    bool closeAfterWrite;

    QTimer fetchTimer;
    bool autoGetMail;

    QMailMessageId repliedFromMailId;
    quint64 repliedFlags;

    QList<int> queuedAccountIds;

    InitialAction initialAction;

    QMap<QAction*, bool> actionVisibility;

    SearchView *searchView;
    int preSearchWidgetId;

    QSet<QMailFolderId> locationSet;

    QMailAccountIdList transmitAccountIds;
    QMailAccountIdList retrievalAccountIds;

    QMailMessageId lastDraftId;

    QMailTransmitAction *transmitAction;
    QMailRetrievalAction *retrievalAction;
    QMailStorageAction *storageAction;
    QMailSearchAction *searchAction;

    QProcess* m_messageServerProcess;

    bool retrievingFolders;
};

#endif