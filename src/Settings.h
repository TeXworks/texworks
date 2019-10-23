#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

namespace Tw {

class Settings : public QSettings
{
	Q_OBJECT
public:
	Settings();

	using QSettings::defaultFormat;
	using QSettings::setDefaultFormat;
};

} // namespace Tw

#endif // SETTINGS_H
