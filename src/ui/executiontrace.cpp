#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QVariant>
#include <QModelIndex>
#include <QString>

#include <sstream>
#include <iomanip>
#include <cmath>

#include "executiontrace.h"

using std::stringstream;

ExecutionTrace::ExecutionTrace(QObject *parent)
    : QAbstractListModel(parent),
      ConsoleSubscriber()
{

}

void ExecutionTrace::onPause()
{
    m_instructions.clear();

    const auto & trace = m_console.get()->trace();

    beginResetModel();

    for (const auto & op : trace) {
        m_instructions.append(op);
    }
    endResetModel();
}

void ExecutionTrace::onResume()
{

}

QVariant ExecutionTrace::data(const QModelIndex & index, int role) const
{
    if ((index.row() < 0) || (index.row() >= m_instructions.size())) {
        return QVariant();
    }

    const auto & instruction = m_instructions.at(index.row());
    switch (role) {
    case IndexRole: {
        int w = std::ceil(std::log10(m_instructions.size())) + 1;

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

QHash<int, QByteArray> ExecutionTrace::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IndexRole]       = "index";
    roles[AddressRole]     = "address";
    roles[DescriptionRole] = "description";
    roles[StatusRole]      = "status";
    roles[RegistersRole]   = "registers";

    return roles;
}
