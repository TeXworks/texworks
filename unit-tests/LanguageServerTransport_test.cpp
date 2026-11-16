/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

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

#include "LanguageServerTransport_test.h"

#include "languageservices/lsp/JsonRpcTransport.h"
#include "languageservices/lsp/LanguageServerProcess.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPair>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>

#include <initializer_list>

using Tw::LanguageServices::Lsp::JsonRpcTransport;
using Tw::LanguageServices::Lsp::LanguageServerProcess;

namespace {

QJsonObject jsonObject(std::initializer_list<QPair<QString, QJsonValue>> values)
{
	QJsonObject object;
	for (const QPair<QString, QJsonValue> & value : values)
		object.insert(value.first, value.second);
	return object;
}

QJsonArray jsonArray(std::initializer_list<QJsonValue> values)
{
	QJsonArray array;
	for (const QJsonValue & value : values)
		array.append(value);
	return array;
}

QJsonObject notification(const QString & method, const QJsonValue & params = QJsonValue(QJsonValue::Undefined))
{
	QJsonObject message = jsonObject({
		{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
		{QStringLiteral("method"), method}
	});
	if (!params.isUndefined())
		message.insert(QStringLiteral("params"), params);
	return message;
}

QByteArray framedBody(const QByteArray & body, const QByteArray & headerName = QByteArrayLiteral("Content-Length"),
	                  const QByteArray & additionalHeader = {})
{
	QByteArray header = headerName + QByteArrayLiteral(": ") + QByteArray::number(body.size()) + QByteArrayLiteral("\r\n");
	if (!additionalHeader.isEmpty())
		header += additionalHeader + QByteArrayLiteral("\r\n");
	return header + QByteArrayLiteral("\r\n") + body;
}

QJsonObject framedObject(const QByteArray & bytes)
{
	const auto bodyStart = bytes.indexOf("\r\n\r\n");
	if (bodyStart < 0)
		return {};
	return QJsonDocument::fromJson(bytes.mid(bodyStart + 4)).object();
}

bool hasByteCorrectContentLength(const QByteArray & bytes)
{
	const auto bodyStart = bytes.indexOf("\r\n\r\n");
	if (bodyStart < 0)
		return false;
	bool found = false;
	qlonglong contentLength = -1;
	for (QByteArray line : bytes.left(bodyStart).split('\n')) {
		if (line.endsWith('\r'))
			line.chop(1);
		const auto colon = line.indexOf(':');
		if (colon <= 0 || line.left(colon).trimmed().toLower() != QByteArrayLiteral("content-length"))
			continue;
		if (found)
			return false;
		bool ok = false;
		contentLength = line.mid(colon + 1).trimmed().toLongLong(&ok, 10);
		if (!ok)
			return false;
		found = true;
	}
	return found && contentLength == bytes.size() - bodyStart - 4;
}

QString fakeServerPath()
{
	QString name = QStringLiteral("language_server_fake");
#ifdef Q_OS_WIN
	name += QStringLiteral(".exe");
#endif
	return QDir(QCoreApplication::applicationDirPath()).filePath(name);
}

void waitForRunning(LanguageServerProcess & process)
{
	QTRY_COMPARE(process.state(), LanguageServerProcess::Running);
}

} // namespace

void LanguageServerTransportTest::initTestCase()
{
	qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
	qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
	qRegisterMetaType<LanguageServerProcess::State>("Tw::LanguageServices::Lsp::LanguageServerProcess::State");
	QVERIFY2(QFileInfo::exists(fakeServerPath()), qPrintable(fakeServerPath()));
}

void LanguageServerTransportTest::markTimerFired()
{
	m_timerFired = true;
}

void LanguageServerTransportTest::completeFrame()
{
	JsonRpcTransport transport;
	QSignalSpy notificationSpy(&transport, SIGNAL(notificationReceived(const QString &, const QJsonValue &)));
	transport.receiveData(JsonRpcTransport::frame(notification(QStringLiteral("one"))));
	QCOMPARE(notificationSpy.count(), 1);
	QCOMPARE(notificationSpy.takeFirst().at(0).toString(), QStringLiteral("one"));
}

void LanguageServerTransportTest::fragmentedHeader()
{
	JsonRpcTransport transport;
	QSignalSpy spy(&transport, SIGNAL(notificationReceived(const QString &, const QJsonValue &)));
	const QByteArray bytes = JsonRpcTransport::frame(notification(QStringLiteral("header")));
	for (char byte : bytes.left(18))
		transport.receiveData(QByteArray(1, byte));
	QCOMPARE(spy.count(), 0);
	transport.receiveData(bytes.mid(18));
	QCOMPARE(spy.count(), 1);
}

void LanguageServerTransportTest::fragmentedBody()
{
	JsonRpcTransport transport;
	QSignalSpy spy(&transport, SIGNAL(notificationReceived(const QString &, const QJsonValue &)));
	const QByteArray bytes = JsonRpcTransport::frame(notification(QStringLiteral("body")));
	const auto split = bytes.indexOf("\r\n\r\n") + 6;
	transport.receiveData(bytes.left(split));
	QCOMPARE(spy.count(), 0);
	transport.receiveData(bytes.mid(split));
	QCOMPARE(spy.count(), 1);
}

void LanguageServerTransportTest::severalFramesAndTrailingPartial()
{
	JsonRpcTransport transport;
	QSignalSpy spy(&transport, SIGNAL(notificationReceived(const QString &, const QJsonValue &)));
	const QByteArray first = JsonRpcTransport::frame(notification(QStringLiteral("first")));
	const QByteArray second = JsonRpcTransport::frame(notification(QStringLiteral("second")));
	const QByteArray third = JsonRpcTransport::frame(notification(QStringLiteral("third")));
	transport.receiveData(first + second + third.left(9));
	QCOMPARE(spy.count(), 2);
	transport.receiveData(third.mid(9));
	QCOMPARE(spy.count(), 3);
	QCOMPARE(spy.at(2).at(0).toString(), QStringLiteral("third"));
}

void LanguageServerTransportTest::caseInsensitiveAndAdditionalHeader()
{
	JsonRpcTransport transport;
	QSignalSpy spy(&transport, SIGNAL(notificationReceived(const QString &, const QJsonValue &)));
	const QByteArray body = QJsonDocument(notification(QStringLiteral("headers"))).toJson(QJsonDocument::Compact);
	transport.receiveData(framedBody(body, QByteArrayLiteral("cOnTeNt-LeNgTh"), QByteArrayLiteral("Content-Type: application/vscode-jsonrpc; charset=utf-8")));
	QCOMPARE(spy.count(), 1);
}

void LanguageServerTransportTest::malformedLengths_data()
{
	QTest::addColumn<QByteArray>("input");
	QTest::addColumn<QString>("errorFragment");
	QTest::newRow("missing") << QByteArrayLiteral("Content-Type: application/json\r\n\r\n{}") << QStringLiteral("Missing");
	QTest::newRow("non-numeric") << QByteArrayLiteral("Content-Length: twelve\r\n\r\n") << QStringLiteral("Invalid");
	QTest::newRow("negative") << QByteArrayLiteral("Content-Length: -1\r\n\r\n") << QStringLiteral("Invalid");
	QTest::newRow("oversized") << QByteArrayLiteral("Content-Length: 8388609\r\n\r\n") << QStringLiteral("exceeds");
	QTest::newRow("duplicate") << QByteArrayLiteral("Content-Length: 2\r\nContent-Length: 2\r\n\r\n{}") << QStringLiteral("Duplicate");
	QTest::newRow("malformed-header") << QByteArrayLiteral("Content-Length 2\r\n\r\n{}") << QStringLiteral("Malformed");
	QTest::newRow("oversized-header") << (QByteArrayLiteral("X-Test: ") + QByteArray(JsonRpcTransport::MaximumHeaderSize, 'x')) << QStringLiteral("header exceeds");
}

void LanguageServerTransportTest::malformedLengths()
{
	QFETCH(QByteArray, input);
	QFETCH(QString, errorFragment);
	JsonRpcTransport transport;
	QSignalSpy errorSpy(&transport, SIGNAL(protocolError(const QString &)));
	transport.receiveData(input);
	QCOMPARE(errorSpy.count(), 1);
	QVERIFY(errorSpy.takeFirst().at(0).toString().contains(errorFragment));
	QVERIFY(transport.isFailed());
}

void LanguageServerTransportTest::malformedAndZeroLengthJson()
{
	for (const QByteArray & bytes : {QByteArrayLiteral("Content-Length: 1\r\n\r\n{"), QByteArrayLiteral("Content-Length: 0\r\n\r\n")}) {
		JsonRpcTransport transport;
		QSignalSpy errorSpy(&transport, SIGNAL(protocolError(const QString &)));
		transport.receiveData(bytes);
		QCOMPARE(errorSpy.count(), 1);
		QVERIFY(transport.isFailed());
	}
}

void LanguageServerTransportTest::utf8Json()
{
	JsonRpcTransport transport;
	QSignalSpy spy(&transport, SIGNAL(notificationReceived(const QString &, const QJsonValue &)));
	const uint supplementaryCodePoint = 0x1F642; // U+1F642
	const QChar highSurrogate(QChar::highSurrogate(supplementaryCodePoint));
	const QChar lowSurrogate(QChar::lowSurrogate(supplementaryCodePoint));
	const QString supplementaryCharacter = QString(highSurrogate) + lowSurrogate;
	const QByteArray body = QByteArrayLiteral("{\"jsonrpc\":\"2.0\",\"method\":\"unicode\",\"params\":{\"text\":\"\xF0\x9F\x99\x82\"}}");
	const QByteArray frame = framedBody(body);
	QVERIFY(frame.contains(QByteArrayLiteral("\xF0\x9F\x99\x82")));
	transport.receiveData(frame);
	QCOMPARE(spy.count(), 1);
	QCOMPARE(spy.takeFirst().at(1).toJsonValue().toObject().value(QStringLiteral("text")).toString(), supplementaryCharacter);
}

void LanguageServerTransportTest::failedTransportDiscardsBufferedDataAndPendingRequests()
{
	JsonRpcTransport transport;
	QSignalSpy notificationSpy(&transport, SIGNAL(notificationReceived(const QString &, const QJsonValue &)));
	QSignalSpy pendingSpy(&transport, SIGNAL(pendingRequestFailed(qint64, const QString &)));
	transport.sendRequest(QStringLiteral("pending"));
	transport.receiveData(QByteArrayLiteral("Content-Length: 100\r\n\r\n{"));
	transport.fail(QStringLiteral("test failure"));
	QCOMPARE(pendingSpy.count(), 1);
	transport.receiveData(JsonRpcTransport::frame(notification(QStringLiteral("must/not/arrive"))));
	QCOMPARE(notificationSpy.count(), 0);
	QCOMPARE(transport.pendingRequestCount(), 0);
}

void LanguageServerTransportTest::notificationSerialization()
{
	JsonRpcTransport transport;
	QSignalSpy outputSpy(&transport, SIGNAL(outgoingFrame(const QByteArray &)));
	QVERIFY(transport.sendNotification(QStringLiteral("test/note"), jsonObject({{QStringLiteral("value"), 7}})));
	const QJsonObject object = framedObject(outputSpy.takeFirst().at(0).toByteArray());
	QCOMPARE(object.value(QStringLiteral("jsonrpc")).toString(), QStringLiteral("2.0"));
	QCOMPARE(object.value(QStringLiteral("method")).toString(), QStringLiteral("test/note"));
	QVERIFY(!object.contains(QStringLiteral("id")));
	QCOMPARE(object.value(QStringLiteral("params")).toObject().value(QStringLiteral("value")).toInt(), 7);
}

void LanguageServerTransportTest::requestSerializationAndIds()
{
	JsonRpcTransport transport;
	QSignalSpy outputSpy(&transport, SIGNAL(outgoingFrame(const QByteArray &)));
	const qint64 first = transport.sendRequest(QStringLiteral("first"));
	const qint64 second = transport.sendRequest(QStringLiteral("second"), jsonArray({1, 2}));
	QCOMPARE(second, first + 1);
	QCOMPARE(framedObject(outputSpy.at(0).at(0).toByteArray()).value(QStringLiteral("id")).toDouble(), static_cast<double>(first));
	QCOMPARE(framedObject(outputSpy.at(1).at(0).toByteArray()).value(QStringLiteral("id")).toDouble(), static_cast<double>(second));
	QVERIFY(!framedObject(outputSpy.at(0).at(0).toByteArray()).contains(QStringLiteral("params")));
}

void LanguageServerTransportTest::errorResponseSerializationNumericId()
{
	JsonRpcTransport transport;
	QSignalSpy outputSpy(&transport, SIGNAL(outgoingFrame(const QByteArray &)));
	const QJsonValue id(73);
	QVERIFY(transport.sendErrorResponse(id, -32601, QStringLiteral("Method not found")));
	QCOMPARE(outputSpy.count(), 1);
	const QByteArray frame = outputSpy.takeFirst().at(0).toByteArray();
	const QJsonObject object = framedObject(frame);
	QVERIFY(hasByteCorrectContentLength(frame));
	QCOMPARE(object.value(QStringLiteral("jsonrpc")).toString(), QStringLiteral("2.0"));
	QVERIFY(object.value(QStringLiteral("id")) == id);
	QCOMPARE(object.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt(), -32601);
	QVERIFY(!object.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString().isEmpty());
	QVERIFY(!object.contains(QStringLiteral("result")));
	QVERIFY(!object.contains(QStringLiteral("method")));
	QVERIFY(!object.contains(QStringLiteral("params")));
}

void LanguageServerTransportTest::errorResponseSerializationStringId()
{
	JsonRpcTransport transport;
	QSignalSpy outputSpy(&transport, SIGNAL(outgoingFrame(const QByteArray &)));
	const QJsonValue id(QStringLiteral("server-request-id"));
	QVERIFY(transport.sendErrorResponse(id, -32601, QStringLiteral("Method not found")));
	QCOMPARE(outputSpy.count(), 1);
	const QByteArray frame = outputSpy.takeFirst().at(0).toByteArray();
	const QJsonObject object = framedObject(frame);
	QVERIFY(hasByteCorrectContentLength(frame));
	QCOMPARE(object.value(QStringLiteral("jsonrpc")).toString(), QStringLiteral("2.0"));
	QVERIFY(object.value(QStringLiteral("id")) == id);
	QCOMPARE(object.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt(), -32601);
	QVERIFY(!object.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString().isEmpty());
	QVERIFY(!object.contains(QStringLiteral("result")));
	QVERIFY(!object.contains(QStringLiteral("method")));
	QVERIFY(!object.contains(QStringLiteral("params")));
}

void LanguageServerTransportTest::outgoingMessageSizeBound()
{
	JsonRpcTransport transport;
	QSignalSpy outputSpy(&transport, SIGNAL(outgoingFrame(const QByteArray &)));
	QSignalSpy errorSpy(&transport, SIGNAL(protocolError(const QString &)));
	const QString oversized(JsonRpcTransport::MaximumMessageSize, QLatin1Char('x'));
	QCOMPARE(transport.sendRequest(QStringLiteral("oversized"), oversized), static_cast<qint64>(-1));
	QCOMPARE(outputSpy.count(), 0);
	QCOMPARE(errorSpy.count(), 1);
	QCOMPARE(transport.pendingRequestCount(), 0);
}

void LanguageServerTransportTest::successfulAndErrorCorrelation()
{
	JsonRpcTransport transport;
	QSignalSpy resultSpy(&transport, SIGNAL(responseReceived(qint64, const QJsonValue &)));
	QSignalSpy errorSpy(&transport, SIGNAL(errorResponseReceived(qint64, const QJsonObject &)));
	const qint64 resultId = transport.sendRequest(QStringLiteral("result"));
	const qint64 errorId = transport.sendRequest(QStringLiteral("error"));
	transport.receiveData(JsonRpcTransport::frame(jsonObject({{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), resultId}, {QStringLiteral("result"), 42}})));
	transport.receiveData(JsonRpcTransport::frame(jsonObject({{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), errorId}, {QStringLiteral("error"), jsonObject({{QStringLiteral("code"), -32601}, {QStringLiteral("message"), QStringLiteral("missing")}})}})));
	QCOMPARE(resultSpy.count(), 1);
	QCOMPARE(resultSpy.takeFirst().at(0).toLongLong(), resultId);
	QCOMPARE(errorSpy.count(), 1);
	QCOMPARE(errorSpy.takeFirst().at(0).toLongLong(), errorId);
	QCOMPARE(transport.pendingRequestCount(), 0);
}

void LanguageServerTransportTest::unknownAndDuplicateResponse()
{
	JsonRpcTransport transport;
	QSignalSpy unknownSpy(&transport, SIGNAL(unknownResponseReceived(const QJsonValue &)));
	const qint64 id = transport.sendRequest(QStringLiteral("once"));
	const QByteArray response = JsonRpcTransport::frame(jsonObject({{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), id}, {QStringLiteral("result"), true}}));
	transport.receiveData(response);
	transport.receiveData(response);
	transport.receiveData(JsonRpcTransport::frame(jsonObject({{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), QStringLiteral("foreign")}, {QStringLiteral("result"), true}})));
	QCOMPARE(unknownSpy.count(), 2);
}

void LanguageServerTransportTest::incomingNotificationAndRequest()
{
	JsonRpcTransport transport;
	QSignalSpy notificationSpy(&transport, SIGNAL(notificationReceived(const QString &, const QJsonValue &)));
	QSignalSpy requestSpy(&transport, SIGNAL(requestReceived(const QJsonValue &, const QString &, const QJsonValue &)));
	transport.receiveData(JsonRpcTransport::frame(notification(QStringLiteral("server/note"), jsonArray({1}))));
	transport.receiveData(JsonRpcTransport::frame(jsonObject({{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), QStringLiteral("server-id")}, {QStringLiteral("method"), QStringLiteral("server/request")}})));
	QCOMPARE(notificationSpy.count(), 1);
	QCOMPARE(requestSpy.count(), 1);
	QCOMPARE(requestSpy.takeFirst().at(0).toJsonValue().toString(), QStringLiteral("server-id"));
}

void LanguageServerTransportTest::invalidJsonRpcMarker()
{
	for (const QJsonObject & message : {
		jsonObject({{QStringLiteral("method"), QStringLiteral("missing")}}),
		jsonObject({{QStringLiteral("jsonrpc"), QStringLiteral("1.0")}, {QStringLiteral("method"), QStringLiteral("old")}})
	}) {
		JsonRpcTransport transport;
		QSignalSpy errorSpy(&transport, SIGNAL(protocolError(const QString &)));
		transport.receiveData(JsonRpcTransport::frame(message));
		QCOMPARE(errorSpy.count(), 1);
	}
}

void LanguageServerTransportTest::processRoundTripAndFragmentation()
{
	LanguageServerProcess process;
	QSignalSpy responseSpy(process.transport(), SIGNAL(responseReceived(qint64, const QJsonValue &)));
	QVERIFY(process.start(fakeServerPath()));
	waitForRunning(process);
	process.transport()->sendRequest(QStringLiteral("echo"), jsonObject({{QStringLiteral("text"), QStringLiteral("stdin reached server")}}));
	QTRY_COMPARE(responseSpy.count(), 1);
	process.transport()->sendRequest(QStringLiteral("fragmentHeader"), jsonObject({{QStringLiteral("part"), QStringLiteral("header")}}));
	QTRY_COMPARE(responseSpy.count(), 2);
	process.transport()->sendRequest(QStringLiteral("fragmentBody"), jsonObject({{QStringLiteral("part"), QStringLiteral("body")}}));
	QTRY_COMPARE(responseSpy.count(), 3);
	process.stop(200, 200);
	QTRY_COMPARE(process.state(), LanguageServerProcess::NotRunning);
}

void LanguageServerTransportTest::processCoalescedFramesAndStderrIsolation()
{
	LanguageServerProcess process;
	QSignalSpy responseSpy(process.transport(), SIGNAL(responseReceived(qint64, const QJsonValue &)));
	QSignalSpy notificationSpy(process.transport(), SIGNAL(notificationReceived(const QString &, const QJsonValue &)));
	QSignalSpy stderrSpy(&process, SIGNAL(standardErrorReceived(const QByteArray &)));
	QSignalSpy protocolErrorSpy(process.transport(), SIGNAL(protocolError(const QString &)));
	QVERIFY(process.start(fakeServerPath()));
	waitForRunning(process);
	process.transport()->sendRequest(QStringLiteral("several"), jsonObject({{QStringLiteral("test"), true}}));
	QTRY_COMPARE(responseSpy.count(), 1);
	QCOMPARE(notificationSpy.count(), 1);
	process.transport()->sendRequest(QStringLiteral("stderr"), jsonObject({{QStringLiteral("test"), true}}));
	QTRY_COMPARE(responseSpy.count(), 2);
	QTRY_VERIFY(!stderrSpy.isEmpty());
	QVERIFY(stderrSpy.at(0).at(0).toByteArray().contains("fake-server stderr"));
	QCOMPARE(protocolErrorSpy.count(), 0);
	process.stop(200, 200);
	QTRY_COMPARE(process.state(), LanguageServerProcess::NotRunning);
}

void LanguageServerTransportTest::processDelayKeepsEventLoopResponsive()
{
	LanguageServerProcess process;
	QSignalSpy responseSpy(process.transport(), SIGNAL(responseReceived(qint64, const QJsonValue &)));
	QVERIFY(process.start(fakeServerPath()));
	waitForRunning(process);
	m_timerFired = false;
	process.transport()->sendRequest(QStringLiteral("delay"), jsonObject({{QStringLiteral("delayed"), true}}));
	QTimer::singleShot(0, this, SLOT(markTimerFired()));
	QTRY_VERIFY(m_timerFired);
	QCOMPARE(responseSpy.count(), 0);
	QTRY_COMPARE(responseSpy.count(), 1);
	process.stop(200, 200);
	QTRY_COMPARE(process.state(), LanguageServerProcess::NotRunning);
}

void LanguageServerTransportTest::processProtocolFailureStopsProcess()
{
	LanguageServerProcess process;
	QSignalSpy protocolErrorSpy(process.transport(), SIGNAL(protocolError(const QString &)));
	QSignalSpy pendingSpy(process.transport(), SIGNAL(pendingRequestFailed(qint64, const QString &)));
	QSignalSpy finishedSpy(&process, SIGNAL(processFinished(int, QProcess::ExitStatus, bool)));
	QVERIFY(process.start(fakeServerPath()));
	waitForRunning(process);
	process.transport()->sendRequest(QStringLiteral("malformed"), jsonObject({{QStringLiteral("test"), true}}));
	QTRY_COMPARE(protocolErrorSpy.count(), 1);
	QCOMPARE(pendingSpy.count(), 1);
	QTRY_COMPARE(finishedSpy.count(), 1);
	QCOMPARE(process.state(), LanguageServerProcess::Failed);
}

void LanguageServerTransportTest::processNormalAndUnexpectedExit()
{
	{
		LanguageServerProcess process;
		QSignalSpy finishedSpy(&process, SIGNAL(processFinished(int, QProcess::ExitStatus, bool)));
		QVERIFY(process.start(fakeServerPath()));
		waitForRunning(process);
		process.stop(200, 200);
	QTRY_COMPARE(finishedSpy.count(), 1);
		QCOMPARE(finishedSpy.at(0).at(1).toInt(), static_cast<int>(QProcess::NormalExit));
		QCOMPARE(finishedSpy.at(0).at(2).toBool(), false);
	}
	{
		LanguageServerProcess process;
		QSignalSpy finishedSpy(&process, SIGNAL(processFinished(int, QProcess::ExitStatus, bool)));
		QSignalSpy pendingSpy(process.transport(), SIGNAL(pendingRequestFailed(qint64, const QString &)));
		QVERIFY(process.start(fakeServerPath()));
		waitForRunning(process);
		process.transport()->sendRequest(QStringLiteral("exitNormal"), jsonObject({{QStringLiteral("pending"), true}}));
		QTRY_COMPARE(finishedSpy.count(), 1);
		QCOMPARE(finishedSpy.at(0).at(1).toInt(), static_cast<int>(QProcess::NormalExit));
		QCOMPARE(finishedSpy.at(0).at(2).toBool(), true);
		QCOMPARE(pendingSpy.count(), 1);
		QCOMPARE(process.state(), LanguageServerProcess::Failed);
	}
	{
		LanguageServerProcess process;
		QSignalSpy finishedSpy(&process, SIGNAL(processFinished(int, QProcess::ExitStatus, bool)));
		QVERIFY(process.start(fakeServerPath(), {QStringLiteral("--crash")}));
		QTRY_COMPARE(finishedSpy.count(), 1);
		QCOMPARE(finishedSpy.at(0).at(1).toInt(), static_cast<int>(QProcess::CrashExit));
		QCOMPARE(finishedSpy.at(0).at(2).toBool(), true);
	}
}

void LanguageServerTransportTest::processStartFailure()
{
	LanguageServerProcess process;
	QSignalSpy errorSpy(&process, SIGNAL(processError(QProcess::ProcessError, const QString &)));
	QVERIFY(process.start(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("missing-language-server-executable"))));
	QTRY_COMPARE(errorSpy.count(), 1);
	QCOMPARE(errorSpy.at(0).at(0).toInt(), static_cast<int>(QProcess::FailedToStart));
	QCOMPARE(process.state(), LanguageServerProcess::Failed);
}

void LanguageServerTransportTest::processStopIsBounded()
{
	LanguageServerProcess process;
	QSignalSpy finishedSpy(&process, SIGNAL(processFinished(int, QProcess::ExitStatus, bool)));
	QVERIFY(process.start(fakeServerPath(), {QStringLiteral("--idle")}));
	waitForRunning(process);
	QElapsedTimer elapsed;
	elapsed.start();
	process.stop(20, 20);
	QTRY_COMPARE(finishedSpy.count(), 1);
	QVERIFY(elapsed.elapsed() < 1000);
	QCOMPARE(finishedSpy.at(0).at(2).toBool(), false);
	QCOMPARE(process.state(), LanguageServerProcess::NotRunning);
}

void LanguageServerTransportTest::processDestructionDoesNotHang()
{
	LanguageServerProcess * process = new LanguageServerProcess;
	QVERIFY(process->start(fakeServerPath(), {QStringLiteral("--idle")}));
	waitForRunning(*process);
	QElapsedTimer elapsed;
	elapsed.start();
	delete process;
	QVERIFY(elapsed.elapsed() < 1000);
}

QTEST_GUILESS_MAIN(LanguageServerTransportTest)
