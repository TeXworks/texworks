/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2019  Stefan Löffler

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
#include "LineNumberWidget_test.h"
#include "ui/LineNumberWidget.h"

namespace UnitTest {

void TestLineNumberWidget::bgColor()
{
	Tw::UI::LineNumberWidget w(nullptr);
	QColor color(21, 42, 84, 168);

	QCOMPARE(w.bgColor(), w.palette().color(QPalette::Mid));
	w.setBgColor(color);
	QCOMPARE(w.bgColor(), color);
}

void TestLineNumberWidget::sizeHint()
{
	{
		Tw::UI::LineNumberWidget w(nullptr);
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
		QCOMPARE(w.sizeHint(), QSize(3 + w.fontMetrics().width(QChar::fromLatin1('9')), 0));
#else
		QCOMPARE(w.sizeHint(), QSize(3 + w.fontMetrics().horizontalAdvance(QChar::fromLatin1('9')), 0));
#endif
	}
	{
		QTextEdit e;
		Tw::UI::LineNumberWidget w(&e);
		int digits = 1;
		for (int lines = 1; lines <= 100; lines++, e.insertPlainText(QStringLiteral("\n"))) {
			if (lines == 10) ++digits;
			else if (lines == 100) ++digits;
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
			QCOMPARE(w.sizeHint(), QSize(3 + w.fontMetrics().width(QChar::fromLatin1('9')) * digits, 0));
#else
			QCOMPARE(w.sizeHint(), QSize(3 + w.fontMetrics().horizontalAdvance(QChar::fromLatin1('9')) * digits, 0));
#endif
		}
	}
}

void TestLineNumberWidget::paint()
{
	{
		Tw::UI::LineNumberWidget w(nullptr);
		w.setGeometry(0, 0, 100, 100);
		w.grab();
	}
	{
		QTextEdit e;
		Tw::UI::LineNumberWidget w(&e);
		w.setGeometry(0, 0, 100, 100);
		w.grab();

		e.insertPlainText(QStringLiteral("Hello World\n"));
		w.grab();
	}
}

void TestLineNumberWidget::setParent()
{
	QTextEdit e;
	Tw::UI::LineNumberWidget w(nullptr);

	e.insertPlainText(QStringLiteral("\n\n\n\n\n\n\n\n\n\n"));

#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
	QCOMPARE(w.sizeHint(), QSize(3 + w.fontMetrics().width(QChar::fromLatin1('9')) * 1, 0));
#else
	QCOMPARE(w.sizeHint(), QSize(3 + w.fontMetrics().horizontalAdvance(QChar::fromLatin1('9')) * 1, 0));
#endif
	w.setParent(&e);
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
	QCOMPARE(w.sizeHint(), QSize(3 + w.fontMetrics().width(QChar::fromLatin1('9')) * 2, 0));
#else
	QCOMPARE(w.sizeHint(), QSize(3 + w.fontMetrics().horizontalAdvance(QChar::fromLatin1('9')) * 2, 0));
#endif
}


} // namespace UnitTest

#if defined(STATIC_QT5) && defined(Q_OS_WIN)
  Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

QTEST_MAIN(UnitTest::TestLineNumberWidget)
