#ifndef MANAGER_H
#define MANAGER_H

#include <QtCore/qobject.h>
#include "shared_memory.h"
#include <string>
#include <vector>
#include <signal.h>
using std::string;
// #pragma once
QT_USE_NAMESPACE

class manager : public QObject
{
    Q_OBJECT

public:
    manager(shared_memory shmem);
    ~manager();
    void threadReadFromSM();
    void startThread();
    bool stopThread;
    

public slots:
    void writeOnSM(const QString &sender, const QString &message);

signals:
    void sendToClient(const QString &message);

private:
    shared_memory shmem;
    string buildXMLMessage(const std::vector<string>& types, const std::vector<string>& payloads);
    bool stopSend;
    string getMotivation();

};

#endif