// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include <Pothos/Config.hpp>
#include <QTableWidget>
#include <QFutureWatcher>
#include <Poco/Timestamp.h>
#include <QString>
#include <vector>
#include <map>

class HostUriQLineEdit;
class QToolButton;
class QSignalMapper;
class QTimer;

//! information stored about a node
struct NodeInfo
{
    NodeInfo(void):
        isOnline(false),
        lastAccess(Poco::Timestamp::fromEpochTime(0))
    {}
    QString uri;
    bool isOnline;
    Poco::Timestamp lastAccess;
    QString nodeName;

    void update(void);

    bool neverAccessed(void) const
    {
        return this->lastAccess == Poco::Timestamp::fromEpochTime(0);
    }
};

//! widget to selection and edit available hosts
class HostSelectionTable : public QTableWidget
{
    Q_OBJECT
public:
    HostSelectionTable(QWidget *parent);

    //! Get a list of available host uris
    QStringList hostUriList(void) const;

signals:

    //! Emitted when the list of host uris changes
    void hostUriListChanged(void);

    //! A row for the following host was clicked
    void hostInfoRequest(const std::string &);

private slots:

    void handleCellClicked(const int row, const int col);

    void handleRemove(const QString &uri);

    void handleAdd(const QString &uri);

    void handleNodeQueryComplete(void);

    void handleUpdateStatus(void);

private:

    static std::vector<NodeInfo> peformNodeComms(std::vector<NodeInfo> nodes);

    void reloadRows(const std::vector<NodeInfo> &nodes);
    void reloadTable(void);
    void showErrorMessage(const QString &errMsg);
    HostUriQLineEdit *_lineEdit;
    QToolButton *_addButton;
    QSignalMapper *_removeMapper;
    QTimer *_timer;
    QFutureWatcher<std::vector<NodeInfo>> *_watcher;
    std::map<QString, size_t> _uriToRow;
    std::map<QString, NodeInfo> _uriToInfo;
    static const size_t nCols = 4;
};
