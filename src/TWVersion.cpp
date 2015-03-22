/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2015  Jonathan Kew, Stefan Löffler, Charlie Sharpsteen

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	For links to further information, or to contact the authors,
	see <http://www.tug.org/texworks/>.
*/

#include "TWVersion.h"
#include "GitRev.h"

#include <QDateTime>

bool isGitInfoAvailable()
{
	return (!QString::fromAscii(GIT_COMMIT_HASH).startsWith("$Format:") && !QString::fromAscii(GIT_COMMIT_DATE).startsWith("$Format:"));
}

QString gitCommitHash()
{
	if(QString::fromAscii(GIT_COMMIT_HASH).startsWith("$Format:"))
		return QString();
	return GIT_COMMIT_HASH;
}

QDateTime gitCommitDate()
{
	if (QString::fromAscii(GIT_COMMIT_DATE).startsWith("$Format:"))
		return QDateTime();
	return QDateTime::fromString(GIT_COMMIT_DATE, Qt::ISODate).toUTC();
}

