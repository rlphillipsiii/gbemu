#ifndef _CONSOLE_SUBSCRIBER_H
#define _CONSOLE_SUBSCRIBER_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QList>

#include <memory>
#include <functional>
#include <cassert>

#include "gameboyinterface.h"
#include "console.h"

class ConsoleSubscriber {
public:
    ConsoleSubscriber() : m_console(GameBoyInterface::NULLOPT) { }
    virtual ~ConsoleSubscriber() = default;

    virtual inline void setConsole(
        std::unique_ptr<GameBoyInterface> & console) final
    {
        m_console = std::ref(console);
    }

    static void init(QQmlApplicationEngine & engine)
    {
        QList<QObject*> objects = engine.rootObjects();
        if (objects.empty()) { assert(0); return; }

        Console *console = objects[0]->findChild<Console*>("console");
        if (!console) { assert(0); return; }

        QList<QObject*> children = objects[0]->findChildren<QObject*>();
        for (int i = 0; i < children.size(); i++) {
            auto *child = dynamic_cast<ConsoleSubscriber*>(children[i]);
            if (child) {
                child->setConsole(console->get());

                QObject::connect(console, SIGNAL(paused()), children[i], SLOT(onPause()));
                QObject::connect(console, SIGNAL(resumed()), children[i], SLOT(onResume()));
            }
        }
    }

protected:
    GameBoyReference m_console;
};

#endif
