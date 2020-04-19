#ifndef _DISASSEMBLY_H
#define _DISASSEMBLY_H

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <QVariant>
#include <QModelIndex>
#include <QVector>
#include <QQmlEngine>

#include "gbproc.h"
#include "consolesubscriber.h"

class Disassembly : public QAbstractListModel, public ConsoleSubscriber {
    Q_OBJECT

public:
    enum DisassemblyRoles {
        AddressRole = Qt::UserRole + 1,
        DescriptionRole,
    };

    explicit Disassembly(QObject *parent = nullptr);

    static void registerQML()
    {
        qmlRegisterType<Disassembly>("GameBoy.Disassembly", 1, 0, "Disassembly");
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
    Disassembly(const Disassembly &) = delete;
    Disassembly(const Disassembly &&) = delete;
    Disassembly(Disassembly &&) = delete;
    Disassembly & operator=(const Disassembly &) = delete;

    QVector<GB::Command> m_instructions;
};

#endif
