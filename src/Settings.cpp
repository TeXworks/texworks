/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2019-2024  Stefan Löffler

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.

	For links to further information, or to contact the authors,
	see <https://tug.org/texworks/>.
*/
#include "Settings.h"

namespace Tw {

bool Settings::contains(KeyType key) const
{
	return m_s.contains(key);
}

void Settings::remove(KeyType key)
{
	m_s.remove(key);
}

void Settings::setValue(KeyType key, const QVariant &value)
{
	m_s.setValue(key, value);
}

QVariant Settings::value(KeyType key, const QVariant &defaultValue) const
{
	return m_s.value(key, defaultValue);
}

QString Settings::fileName() const
{
	return m_s.fileName();
}

LanguageServiceSettings Settings::languageServiceSettings() const
{
	LanguageServiceSettings settings;
	settings.enabled = m_s.value(QStringLiteral("LanguageServices/enabled"), false).toBool();
	settings.executable = m_s.value(QStringLiteral("LanguageServices/executable")).toString();
	settings.arguments = m_s.value(QStringLiteral("LanguageServices/arguments")).toStringList();
	return settings;
}

void Settings::setLanguageServiceSettings(const LanguageServiceSettings & settings)
{
	m_s.setValue(QStringLiteral("LanguageServices/enabled"), settings.enabled);
	m_s.setValue(QStringLiteral("LanguageServices/executable"), settings.executable);
	m_s.setValue(QStringLiteral("LanguageServices/arguments"), settings.arguments);
}

void Settings::setPortableIniPath(const QString &iniPath)
{
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, iniPath);
}

#if defined(Q_OS_WIN)
bool Settings::isStoredInRegistry()
{
	if (m_s.format() == QSettings::NativeFormat) {
		return true;
	}
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
	if (m_s.format() == QSettings::Registry32Format || m_s.format() == QSettings::Registry64Format) {
		return true;
	}
#endif
	return false;
}
#endif // defined(Q_OS_WIN)

} // namespace Tw
