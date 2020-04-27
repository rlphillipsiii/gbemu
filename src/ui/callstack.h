#ifndef _CALLSTACK_H
#define _CALLSTACK_H

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <QVariant>
#include <QModelIndex>
#include <QVector>
#include <QQmlEngine>

#include "gbproc.h"
#include "consolesubscriber.h"

class CallStack : public QAbstractListModel, public ConsoleSubscriber {
    Q_OBJECT

public:
    enum CallStackTraceRoles {
        IndexRole  = Qt::UserRole + 1,
        AddressRole,
        DescriptionRole,
        StatusRole,
        RegistersRole,
    };

    explicit CallStack(QObject *parent = nullptr);

    static void registerQML()
    {
        qmlRegisterType<CallStack>("GameBoy.Trace", 1, 0, "CallStack");
    }

    QVariant data(
        const QModelIndex & index,
        int role = Qt::DisplayRole) const override;

    inline int rowCount(const QModelIndex&) const override
        { return m_callstack.size(); }

public slots:
    void onPause();
    void onResume();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<GB::Command> m_callstack;
};

#endif
