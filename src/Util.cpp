#include "Util.h"

namespace Util
{
    QString hexToString(quint32 x)
    {
        return "0x" + QString("%1").arg(x, 0, 16).toUpper();
    }
}
