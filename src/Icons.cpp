#include "Icons.h"

#include <QtAwesome.h>
#include <qapplication.h>

QtAwesome* s_awesome = nullptr;

void Icons::init()
{
	Q_ASSERT(!s_awesome);

	s_awesome = new QtAwesome(qApp);
	s_awesome->initFontAwesome();
}

QIcon Icons::saveIcon()
{
	return s_awesome->icon(fa::save);
}

QIcon Icons::settingsIcon()
{
	return s_awesome->icon(fa::gear);
}
