#include "Output.h"

Output::WriteCallback Output::s_writeCallback = nullptr;

void Output::write(const QString& text)
{
    if (s_writeCallback)
    {
        s_writeCallback(text);
    }
}

Output::WriteCallback Output::writeCallback()
{
    return s_writeCallback;
}

void Output::setWriteCallback(WriteCallback callback)
{
    s_writeCallback = callback;
}
