#ifndef _EXECUTION_TRACE_H
#define _EXECUTION_TRACE_H

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <QVariant>
#include <QModelIndex>
#include <QVector>
#include <QQmlEngine>

#include "gbproc.h"
#include "consolesubscriber.h"

class ExecutionTrace : public QAbstractListModel, public ConsoleSubscriber {
    Q_OBJECT

public:
    enum ExecutionTraceRoles {
        IndexRole  = Qt::UserRole + 1,
        AddressRole,
        DescriptionRole,
        StatusRole,
        RegistersRole,
    };

    explicit ExecutionTrace(QObject *parent = nullptr);

    static void registerQML()
    {
        qmlRegisterType<ExecutionTrace>("GameBoy.Trace", 1, 0, "ExecutionTrace");
    }

    QVariant data(
        const QModelIndex & index,
        int role = Qt::DisplayRole) const override;

    inline int rowCount(const QModelIndex&) const override
        { return m_instructions.size(); }

public slots:
    void onPause();
    void onResume();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<GB::Command> m_instructions;
};

#endif
