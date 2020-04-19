#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QVariant>
#include <QModelIndex>
#include <QString>

#include <sstream>
#include <iomanip>

#include "disassembly.h"

using std::stringstream;

Disassembly::Disassembly(QObject *parent)
    : QAbstractListModel(parent),
      ConsoleSubscriber()
{

}

void Disassembly::onPause()
{

}

void Disassembly::onResume()
{

}

QVariant Disassembly::data(const QModelIndex & index, int role) const
{
    if ((index.row() < 0) || (index.row() >= m_instructions.size())) {
        return QVariant();
    }

    const auto & instruction = m_instructions.at(index.row());
    switch (role) {
    case AddressRole:{
        stringstream stream;
        stream << "0x" << std::setfill('0') << std::setw(4) << std::hex << int(instruction.pc);

        return QString::fromStdString(stream.str());
    }

    case DescriptionRole:{
        return QString::fromStdString(instruction.operation->name);
    }

    default: break;
    }

    return QVariant();
}

QHash<int, QByteArray> Disassembly::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[AddressRole]     = "address";
    roles[DescriptionRole] = "description";

    return roles;
}
