#pragma once

#include <qstring.h>

class Output
{
public:
    typedef void(*WriteCallback)(const QString& text);

    static void write(const QString& text);

    static WriteCallback writeCallback();
    static void setWriteCallback(WriteCallback callback);

private:
    static WriteCallback s_writeCallback;
};