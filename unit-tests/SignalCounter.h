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

#ifndef SignalCounter_H
#define SignalCounter_H

#include <QEventLoop>
#include <QObject>
#include <QTimerEvent>
#include <type_traits>

namespace UnitTest {

class SignalCounter : public QObject
{
	Q_OBJECT
	int _count{0};
	QMetaObject::Connection _connection;
	QEventLoop _eventLoop;
	int _timerId{-1};
public:
	template <typename Object, typename Func, typename std::enable_if<std::is_member_function_pointer<Func>::value, int>::type = 0>
	SignalCounter(Object * obj, Func signal) : _connection(connect(obj, signal, this, &SignalCounter::increment)) { }

	int count() const { return _count; }
	void clear() { _count = 0; }
	bool isValid() const { return static_cast<bool>(_connection); }
	bool wait(int timeout = 5000);
protected:
	void timerEvent(QTimerEvent * event) override;
private slots:
	void increment();
};

} // namespace UnitTest

#endif // !defined(SignalCounter_H)
