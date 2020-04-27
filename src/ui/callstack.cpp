#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QVariant>
#include <QModelIndex>
#include <QString>

#include <sstream>
#include <iomanip>
#include <cmath>

#include "callstack.h"

using std::stringstream;

CallStack::CallStack(QObject *parent)
    : QAbstractListModel(parent),
      ConsoleSubscriber()
{

}

void CallStack::onPause()
{
    m_callstack.clear();

    auto trace = m_console.get()->callstack();

    beginResetModel();

    while (!trace.empty()) {
        m_callstack.append(trace.top());
        trace.pop();
    }
    endResetModel();
}

void CallStack::onResume()
{

}

QVariant CallStack::data(const QModelIndex & index, int role) const
{
    if ((index.row() < 0) || (index.row() >= m_callstack.size())) {
        return QVariant();
    }

    const auto & instruction = m_callstack.at(index.row());
    switch (role) {
    case IndexRole: {
        int w = std::ceil(std::log10(m_callstack.size())) + 1;

        stringstream stream;
        stream << std::setfill('0') << std::setw(w) << int(index.row() + 1);

        return QString::fromStdString(stream.str());
    }

    case AddressRole:{
        stringstream stream;
        stream << "0x" << std::setfill('0') << std::setw(4) << std::hex << int(instruction.pc);

        return QString::fromStdString(stream.str());
    }

    case DescriptionRole:{
        QString name = QString::fromStdString(instruction.operation->name);

        if ((instruction.operation->length - 1) > 0) {
            name += " ";

            stringstream stream;
            stream << "0x" << std::setfill('0') << std::setw(2) << std::hex;

            for (int i = instruction.operation->length - 2; i >= 0; --i) {
                stream << int(instruction.operands[i]);
            }
            name += QString::fromStdString(stream.str());
        }

        return name;
    }

    case StatusRole: {
        return QString::fromStdString(instruction.status());
    }

    case RegistersRole: {
        return QString::fromStdString(instruction.registers());
    }

    default: break;
    }

    return QVariant();
}

QHash<int, QByteArray> CallStack::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IndexRole]       = "index";
    roles[AddressRole]     = "address";
    roles[DescriptionRole] = "description";
    roles[StatusRole]      = "status";
    roles[RegistersRole]   = "registers";

    return roles;
}
