/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2019-2020  Stefan Löffler

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

#include "SignalCounter.h"

namespace UnitTest {

bool SignalCounter::wait(int timeout)
{
	const int origCount = count();
	_timerId = startTimer(timeout);
	_eventLoop.exec();
	return count() > origCount;
}

void SignalCounter::timerEvent(QTimerEvent * event)
{
	if (event->timerId() != _timerId)
		return;
	killTimer(_timerId);
	_timerId = -1;
	_eventLoop.exit();
}

void SignalCounter::increment()
{
	++_count;
	if (_eventLoop.isRunning())
		_eventLoop.exit();
}

} // namespace UnitTest
