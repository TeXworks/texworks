/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2019-2023  Stefan Löffler, Jérôme Laurens

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
#include "TwxUITest.h"

#include "ui/ClickableLabel.h"
#include "ui/ColorButton.h"
#include "ui/ConsoleWidget.h"
#include "ui/ClosableTabWidget.h"
#include "ui/LineNumberWidget.h"
#include "ui/ScreenCalibrationWidget.h"

#include <QDoubleSpinBox>
#include <QMouseEvent>
#include <QTabBar>
#include <QDebug>

#include <qtestcase.h>

// See https://stackoverflow.com/questions/46491576/qcompare-of-two-floating-point-infinity-values
#define TWX_COMPARE_DBL(actual, expected, epsilon) \
do {\
	if (!QTest::compare_helper( \
			(qAbs(actual - expected) <= (epsilon) * (1+qAbs(actual)+qAbs(expected))), \
			QString{"Compared values are not the same ± %1"} \
					.arg(epsilon).toLocal8Bit().constData(), \
			QTest::toString(actual), \
			QTest::toString(expected), \
			#actual, #expected, __FILE__, __LINE__)) \
		return;\
} while (false)

namespace UnitTest {

class MyScreenCalibrationWidget : public Tw::UI::ScreenCalibrationWidget
{
public:
	QRect rulerRect() const { return _rulerRect; }
	bool isDragging() const { return _isDragging; }
	QDoubleSpinBox * spinBox() { return _sbDPI; }
	int unit() const { return _curUnit; }
	QMenu & contextMenu() { return _contextMenu; }
};

class ClosableTabWidget : public Tw::UI::ClosableTabWidget
{
public:
	QToolButton * closeButton() { return _closeButton; }
};

void TestUI::initTestCase()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	qRegisterMetaType<QMouseEvent*>("QMouseEvent*");
#endif
}

void TestUI::LineNumberWidget_bgColor()
{
	Tw::UI::LineNumberWidget w(nullptr);
	QColor color(21, 42, 84, 168);

	QCOMPARE(w.bgColor(), w.palette().color(QPalette::Mid));
	w.setBgColor(color);
	QCOMPARE(w.bgColor(), color);
}

void TestUI::LineNumberWidget_sizeHint()
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

void TestUI::LineNumberWidget_paint()
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

void TestUI::LineNumberWidget_setParent()
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

void TestUI::ScreenCalibrationWidget_dpi()
{
	Tw::UI::ScreenCalibrationWidget w;
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QSignalSpy spy(&w, SIGNAL(dpiChanged(double)));
#else
	QSignalSpy spy(&w, &Tw::UI::ScreenCalibrationWidget::dpiChanged);
#endif
	constexpr double MagicDPI = 42;

	QVERIFY(spy.isValid());

	QCOMPARE(w.dpi(), static_cast<double>(w.physicalDpiX()));
	QCOMPARE(spy.count(), 0);
	w.setDpi(MagicDPI);
	QCOMPARE(spy.count(), 1);
	QCOMPARE(w.dpi(), MagicDPI);
	QCOMPARE(spy[0][0].toDouble(), MagicDPI);
}

void TestUI::ScreenCalibrationWidget_drag()
{
	MyScreenCalibrationWidget w;
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QSignalSpy spy(&w, SIGNAL(dpiChanged(double)));
#else
	QSignalSpy spy(&w, &MyScreenCalibrationWidget::dpiChanged);
#endif

	w.resize(400, 40);
	w.show();
	QCoreApplication::processEvents();

	double dpi = w.dpi();
	double newDpi = dpi * (w.rulerRect().bottomRight().x() - w.rulerRect().left()) / (w.rulerRect().center().x() - w.rulerRect().left());
	// DPI values are passed through a QDoubleSpinbox, which rounds them
	newDpi = QString::number(newDpi, 'f', w.spinBox()->decimals()).toDouble();

	// Simulate dragging to the right

	QVERIFY(w.isDragging() == false);
	QCOMPARE(spy.count(), 0);

	QTest::mousePress(&w, Qt::LeftButton, {}, w.rulerRect().center());
	QVERIFY(w.isDragging() == false);
	QCOMPARE(spy.count(), 0);

	// QTest::mouseMove and QTest::mouseEvent don't allow to simulate dragging,
	// i.e., holding down a mouse button while moving. Hence we have to send our
	// own QMouseEvent.
	{
		const QPoint local = w.rulerRect().bottomRight();
		const QPoint global = w.mapToGlobal(local);
		QMouseEvent me(QEvent::MouseMove, local, global, Qt::LeftButton, Qt::LeftButton, {});
		QCoreApplication::instance()->notify(&w, &me);
	}
	QVERIFY(w.isDragging());
	QCOMPARE(w.dpi(), newDpi);
	QCOMPARE(spy.count(), 1);
	QCOMPARE(spy[0][0].toDouble(), newDpi);

	QTest::mouseRelease(&w, Qt::LeftButton, {}, w.rect().bottomRight());
	QCOMPARE(spy.count(), 1);
	QCOMPARE(w.dpi(), newDpi);
	QVERIFY(w.isDragging() == false);

	// Simulate dragging to the left
	spy.clear();

	QPoint downPos = w.rulerRect().topRight() + QPoint(-1, 1);
	QPoint upPos = w.rulerRect().center();
	dpi = newDpi;
	newDpi = dpi * (upPos.x() - w.rulerRect().left()) / (downPos.x() - w.rulerRect().left());
	newDpi = QString::number(newDpi, 'f', w.spinBox()->decimals()).toDouble();

	QVERIFY(w.isDragging() == false);
	QCOMPARE(spy.count(), 0);

	QTest::mousePress(&w, Qt::LeftButton, {}, downPos);
	QVERIFY(w.isDragging() == false);
	QCOMPARE(spy.count(), 0);

	{
		const QPointF global = w.mapToGlobal(upPos);
		QMouseEvent me(QEvent::MouseMove, upPos, global, Qt::LeftButton, Qt::LeftButton, {});
		QCoreApplication::instance()->notify(&w, &me);
	}
	QVERIFY(w.isDragging());
	TWX_COMPARE_DBL(w.dpi(), newDpi, 0.0001);
	QCOMPARE(spy.count(), 1);
	TWX_COMPARE_DBL(spy[0][0].toDouble(), newDpi, 0.0001);

	QTest::mouseRelease(&w, Qt::LeftButton, {}, upPos);
	QCOMPARE(spy.count(), 1);
	TWX_COMPARE_DBL(w.dpi(), newDpi, 0.0001);
	QVERIFY(w.isDragging() == false);
}

void TestUI::ScreenCalibrationWidget_unit()
{
	QLocale::setDefault(QLocale(QLocale::German));
	QCOMPARE(MyScreenCalibrationWidget().unit(), 0);
	QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
	MyScreenCalibrationWidget w;
	QCOMPARE(w.unit(), 1);
	w.setUnit(0);
	QCOMPARE(w.unit(), 0);
	w.setUnit(2);
	QCOMPARE(w.unit(), 0);

	QLocale::setDefault(QLocale::system());
}

void TestUI::ScreenCalibrationWidget_paint()
{
	MyScreenCalibrationWidget w;
	w.setGeometry(0, 0, 300, 20);
	w.grab();
	// TODO: actually test the result
}

void TestUI::ScreenCalibrationWidget_changeEvent()
{
	Tw::UI::ScreenCalibrationWidget w;
	QFont f(w.font());

	w.setGeometry(0, 0, 300, 20);
	QImage before = w.grab().toImage();

	f.setPointSize(2 * f.pointSize());
	w.setFont(f);

	QVERIFY(w.grab().toImage() != before);
}

void TestUI::ScreenCalibrationWidget_contextMenu()
{
	MyScreenCalibrationWidget w;

	w.setGeometry(0, 0, 300, 20);
	w.show();
	QCoreApplication::processEvents();

	// Click inside the rulerRect
	QVERIFY(w.contextMenu().isVisible() == false);
	{
		const QPoint local = w.rulerRect().center();
		const QPoint global = w.mapToGlobal(local);
		QContextMenuEvent cme(QContextMenuEvent::Mouse, local, global);
		QCoreApplication::instance()->notify(&w, &cme);
	}
	QVERIFY(w.contextMenu().isVisible());
	w.contextMenu().close();

	// Click outside the rulerRect
	QVERIFY(w.contextMenu().isVisible() == false);
	{
		const QPoint local = w.rulerRect().topLeft() - QPoint(1, 1);
		const QPoint global = w.mapToGlobal(local);
		QContextMenuEvent cme(QContextMenuEvent::Mouse, local, global);
		QCoreApplication::instance()->notify(&w, &cme);
	}
	QVERIFY(w.contextMenu().isVisible() == false);

	// Keypress ("outside the rulerRect")
	QVERIFY(w.contextMenu().isVisible() == false);
	{
		const QPoint local = w.rulerRect().center() - QPoint(1, 1);
		const QPoint global = w.mapToGlobal(local);
		QContextMenuEvent cme(QContextMenuEvent::Keyboard, local, global);
		QCoreApplication::instance()->notify(&w, &cme);
	}
	QVERIFY(w.contextMenu().isVisible());
	w.contextMenu().close();
}

void TestUI::ClickableLable_ctor()
{
	const QString s{QStringLiteral("test")};

	Tw::UI::ClickableLabel cl(s);
	QCOMPARE(cl.text(), s);
}

void TestUI::ClickableLabel_click()
{
	Tw::UI::ClickableLabel cl;
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QSignalSpy leftCounter(&cl, SIGNAL(mouseLeftClick(QMouseEvent *)));
	QSignalSpy middleCounter(&cl, SIGNAL(mouseMiddleClick(QMouseEvent *)));
	QSignalSpy rightCounter(&cl, SIGNAL(mouseRightClick(QMouseEvent *)));
#else
	QSignalSpy leftCounter(&cl, &Tw::UI::ClickableLabel::mouseLeftClick);
	QSignalSpy middleCounter(&cl, &Tw::UI::ClickableLabel::mouseMiddleClick);
	QSignalSpy rightCounter(&cl, &Tw::UI::ClickableLabel::mouseRightClick);
#endif

	// Clicking emits appropriate signals
	QCOMPARE(leftCounter.count(), 0);
	QCOMPARE(middleCounter.count(), 0);
	QCOMPARE(rightCounter.count(), 0);
	QTest::mouseClick(&cl, Qt::LeftButton);
	QCOMPARE(leftCounter.count(), 1);
	QCOMPARE(middleCounter.count(), 0);
	QCOMPARE(rightCounter.count(), 0);
	QTest::mouseClick(&cl, Qt::MiddleButton);
	QCOMPARE(leftCounter.count(), 1);
	QCOMPARE(middleCounter.count(), 1);
	QCOMPARE(rightCounter.count(), 0);
	QTest::mouseClick(&cl, Qt::RightButton);
	QCOMPARE(leftCounter.count(), 1);
	QCOMPARE(middleCounter.count(), 1);
	QCOMPARE(rightCounter.count(), 1);
	QTest::mouseClick(&cl, Qt::ExtraButton1);
	QCOMPARE(leftCounter.count(), 1);
	QCOMPARE(middleCounter.count(), 1);
	QCOMPARE(rightCounter.count(), 1);

	// Dragging does not emit signals
	QTest::mousePress(&cl, Qt::LeftButton, {}, QPoint(0, 0));
	QTest::mouseRelease(&cl, Qt::LeftButton, {}, QPoint(QApplication::startDragDistance(), 0));
	QCOMPARE(leftCounter.count(), 1);
	QCOMPARE(middleCounter.count(), 1);
	QCOMPARE(rightCounter.count(), 1);
	QTest::mousePress(&cl, Qt::MiddleButton, {}, QPoint(0, 0));
	QTest::mouseRelease(&cl, Qt::MiddleButton, {}, QPoint(QApplication::startDragDistance(), 0));
	QCOMPARE(leftCounter.count(), 1);
	QCOMPARE(middleCounter.count(), 1);
	QCOMPARE(rightCounter.count(), 1);
	QTest::mousePress(&cl, Qt::RightButton, {}, QPoint(0, 0));
	QTest::mouseRelease(&cl, Qt::RightButton, {}, QPoint(QApplication::startDragDistance(), 0));
	QCOMPARE(leftCounter.count(), 1);
	QCOMPARE(middleCounter.count(), 1);
	QCOMPARE(rightCounter.count(), 1);
	QTest::mousePress(&cl, Qt::ExtraButton1, {}, QPoint(0, 0));
	QTest::mouseRelease(&cl, Qt::ExtraButton1, {}, QPoint(QApplication::startDragDistance(), 0));
	QCOMPARE(leftCounter.count(), 1);
	QCOMPARE(middleCounter.count(), 1);
	QCOMPARE(rightCounter.count(), 1);
}

void TestUI::ClickableLabel_doubleClick()
{
	Tw::UI::ClickableLabel cl;

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QSignalSpy spy(&cl, SIGNAL(mouseDoubleClick(QMouseEvent *)));
#else
	QSignalSpy spy(&cl, &Tw::UI::ClickableLabel::mouseDoubleClick);
#endif

	QVERIFY(spy.isValid());
	QCOMPARE(spy.count(), 0);

	QTest::mouseDClick(&cl, Qt::LeftButton);
	QCOMPARE(spy.count(), 1);
}

void TestUI::ClosableTabWidget_signals()
{
	ClosableTabWidget w;
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QSignalSpy spy(&w, SIGNAL(requestClose()));
#else
	QSignalSpy spy(&w, &ClosableTabWidget::requestClose);
#endif

	QVERIFY(spy.isValid());
	QCOMPARE(spy.count(), 0);
	QTest::mouseClick(w.closeButton(), Qt::LeftButton);
	QCOMPARE(spy.count(), 1);
}

void TestUI::ClosableTabWidget_resizeEvent()
{
	ClosableTabWidget w;
	w.show();

	Q_ASSERT(w.closeButton() != nullptr);
	Q_ASSERT(w.tabBar() != nullptr);

	int buttonLeft = w.rect().right() - w.closeButton()->sizeHint().width();
	QCOMPARE(w.closeButton()->geometry().left(), buttonLeft);
	QCOMPARE(w.tabBar()->maximumWidth(), buttonLeft);
}

void TestUI::ConsoleWidget_setProcess()
{
	Tw::UI::ConsoleWidget console;
	QProcess process;
	QProcess * ptrNull{nullptr};
	const QString empty{};
	const QString dummyText{QStringLiteral("Dummy text")};

	QCOMPARE(console.process(), ptrNull);
	QCOMPARE(console.toPlainText(), empty);

	console.setPlainText(dummyText);
	console.setProcess(&process, false);
	QCOMPARE(console.process(), &process);
	QCOMPARE(console.toPlainText(), dummyText);

	console.setProcess(nullptr, false);
	QCOMPARE(console.process(), ptrNull);
	QCOMPARE(console.toPlainText(), dummyText);

	console.setProcess(&process, true);
	QCOMPARE(console.process(), &process);
	QCOMPARE(console.toPlainText(), empty);
}

void TestUI::ConsoleWidget_output()
{
	const QString testString{QStringLiteral(u"Hello \u00e4\u20ac\U0001d11e")};
	const QByteArray utf8Bytes = testString.toUtf8();
	Tw::UI::ConsoleWidget console;

	const QString cmd{QDir().filePath(QStringLiteral("byte_echo_TwxUI"))};

	using size_type = decltype(utf8Bytes.size());
	for (size_type flushPos = 1; flushPos < utf8Bytes.size(); ++flushPos) {
		QProcess process;
		console.setProcess(&process);
		QString arg;
		for (size_type pos = 0; pos < flushPos; ++pos) {
			arg += QStringLiteral("\\x%0").arg(static_cast<unsigned char>(utf8Bytes[pos]), 2, 16, QChar{'0'});
		}
		process.start(cmd, {arg});
		process.waitForStarted();
		process.waitForFinished();

		arg.clear();
		for (size_type pos = flushPos; pos < utf8Bytes.size(); ++pos) {
			arg += QStringLiteral("\\x%0").arg(static_cast<unsigned char>(utf8Bytes[pos]), 2, 16, QChar{'0'});
		}
		process.start(cmd, {arg});
		process.waitForStarted();
		process.waitForFinished();

		QCOMPARE(console.toPlainText(), testString);
	}
}

void TestUI::ColorButton_color()
{
	const QColor white{Qt::white}, col{qRgb(210, 42, 123)};
	Tw::UI::ColorButton btn;
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QSignalSpy spy{&btn, SIGNAL(colorChanged(QColor))};
#else
	QSignalSpy spy{&btn, &Tw::UI::ColorButton::colorChanged};
#endif

	QCOMPARE(btn.color(), white);
	QCOMPARE(spy.count(), 0);
	btn.setColor(col);
	QCOMPARE(spy.count(), 1);
	QCOMPARE(btn.color(), col);

	// Setting an invalid color does nothing
	btn.setColor(QColor());
	QCOMPARE(spy.count(), 1);
	QCOMPARE(btn.color(), col);
}


} // namespace UnitTest

#if defined(STATIC_QT5) && defined(Q_OS_WIN)
  Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

QTEST_MAIN(UnitTest::TestUI)
