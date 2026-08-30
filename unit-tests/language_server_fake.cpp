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

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPair>
#include <QStringList>
#include <QThread>

#include <cstdlib>
#include <initializer_list>

namespace {

QJsonObject jsonObject(const std::initializer_list<QPair<QString, QJsonValue>> & values)
{
	QJsonObject object;
	for (const QPair<QString, QJsonValue> & value : values)
		object.insert(value.first, value.second);
	return object;
}

QJsonArray jsonArray(const std::initializer_list<QJsonValue> & values)
{
	QJsonArray array;
	for (const QJsonValue & value : values)
		array.append(value);
	return array;
}

QByteArray frame(const QJsonObject & message)
{
	const QByteArray body = QJsonDocument(message).toJson(QJsonDocument::Compact);
	return QByteArrayLiteral("Content-Length: ") + QByteArray::number(body.size()) + QByteArrayLiteral("\r\n\r\n") + body;
}

bool writeAll(QFile & output, const QByteArray & data)
{
	qint64 offset = 0;
	while (offset < data.size()) {
		const qint64 written = output.write(data.constData() + offset, data.size() - offset);
		if (written <= 0)
			return false;
		offset += written;
	}
	return output.flush();
}

bool readMessage(QFile & input, QJsonObject & message)
{
	qint64 contentLength = -1;
	while (true) {
		const QByteArray line = input.readLine();
		if (line.isEmpty())
			return false;
		if (line == QByteArrayLiteral("\r\n"))
			break;
		const auto colon = line.indexOf(':');
		if (colon > 0 && line.left(colon).trimmed().toLower() == QByteArrayLiteral("content-length"))
			contentLength = line.mid(colon + 1).trimmed().toLongLong();
	}
	if (contentLength < 0)
		return false;

	QByteArray body;
	while (body.size() < contentLength) {
		const QByteArray chunk = input.read(contentLength - body.size());
		if (chunk.isEmpty())
			return false;
		body.append(chunk);
	}
	const QJsonDocument document = QJsonDocument::fromJson(body);
	if (!document.isObject())
		return false;
	message = document.object();
	return true;
}

QJsonObject initializeCapabilities(const QString & profile)
{
	QJsonObject capabilities;
	if (profile == QStringLiteral("digestif") || profile == QStringLiteral("unknown")) {
		capabilities = jsonObject({
			{QStringLiteral("positionEncoding"), QStringLiteral("utf-16")},
			{QStringLiteral("textDocumentSync"), jsonObject({{QStringLiteral("openClose"), true}, {QStringLiteral("change"), 2}})},
			{QStringLiteral("completionProvider"), QJsonObject{}},
			{QStringLiteral("signatureHelpProvider"), QJsonObject{}},
			{QStringLiteral("hoverProvider"), true},
			{QStringLiteral("definitionProvider"), true},
			{QStringLiteral("referencesProvider"), true},
			{QStringLiteral("documentSymbolProvider"), true},
			{QStringLiteral("workspaceSymbolProvider"), true}
		});
		if (profile == QStringLiteral("unknown"))
			capabilities.insert(QStringLiteral("experimentalProviderThing"), jsonObject({{QStringLiteral("future"), true}}));
	}
	else if (profile == QStringLiteral("numeric-incremental")) {
		capabilities.insert(QStringLiteral("textDocumentSync"), 2);
	}
	else if (profile == QStringLiteral("full")) {
		capabilities.insert(QStringLiteral("textDocumentSync"), 1);
	}
	else if (profile == QStringLiteral("none")) {
		capabilities.insert(QStringLiteral("textDocumentSync"), 0);
	}
	else if (profile == QStringLiteral("open-close-false")) {
		capabilities.insert(QStringLiteral("textDocumentSync"), jsonObject({{QStringLiteral("openClose"), false}, {QStringLiteral("change"), 2}}));
	}
	else if (profile == QStringLiteral("open-close-absent")) {
		capabilities.insert(QStringLiteral("textDocumentSync"), jsonObject({{QStringLiteral("change"), 2}}));
	}
	else if (profile == QStringLiteral("features-false")) {
		capabilities = jsonObject({
			{QStringLiteral("completionProvider"), false},
			{QStringLiteral("signatureHelpProvider"), false},
			{QStringLiteral("hoverProvider"), false},
			{QStringLiteral("definitionProvider"), false},
			{QStringLiteral("referencesProvider"), false},
			{QStringLiteral("documentSymbolProvider"), false},
			{QStringLiteral("workspaceSymbolProvider"), false}
		});
	}
	else if (profile == QStringLiteral("invalid-sync")) {
		capabilities.insert(QStringLiteral("textDocumentSync"), QStringLiteral("incremental"));
	}
	if (profile == QStringLiteral("utf8"))
		capabilities.insert(QStringLiteral("positionEncoding"), QStringLiteral("utf-8"));
	else if (profile == QStringLiteral("utf32"))
		capabilities.insert(QStringLiteral("positionEncoding"), QStringLiteral("utf-32"));
	return capabilities;
}

bool validInitializeRequest(const QJsonObject & request)
{
	const QJsonObject params = request.value(QStringLiteral("params")).toObject();
	const QJsonArray encodings = params.value(QStringLiteral("capabilities")).toObject()
	                               .value(QStringLiteral("general")).toObject()
	                               .value(QStringLiteral("positionEncodings")).toArray();
	return params.value(QStringLiteral("processId")).isDouble()
	       && params.contains(QStringLiteral("rootUri")) && params.value(QStringLiteral("rootUri")).isNull()
	       && encodings == jsonArray({QStringLiteral("utf-16")})
	       && !params.value(QStringLiteral("capabilities")).toObject().contains(QStringLiteral("textDocument"));
}

bool isUnsupportedRequestRejection(const QJsonObject & response)
{
	const QJsonObject error = response.value(QStringLiteral("error")).toObject();
	return response.value(QStringLiteral("jsonrpc")) == QJsonValue(QStringLiteral("2.0"))
	       && response.value(QStringLiteral("id")) == QJsonValue(QStringLiteral("server-request-id"))
	       && response.value(QStringLiteral("error")).isObject()
	       && error.value(QStringLiteral("code")) == QJsonValue(-32601)
	       && error.value(QStringLiteral("message")).isString()
	       && !error.value(QStringLiteral("message")).toString().isEmpty()
	       && !response.contains(QStringLiteral("result"))
	       && !response.contains(QStringLiteral("method"))
	       && !response.contains(QStringLiteral("params"));
}

} // namespace

int main(int argc, char * argv[])
{
	QCoreApplication app(argc, argv);
	const QStringList arguments = app.arguments();
	const auto profileIndex = arguments.indexOf(QStringLiteral("--lsp-profile"));
	const QString lspProfile = profileIndex >= 0 && profileIndex + 1 < arguments.size()
	                         ? arguments.at(profileIndex + 1) : QString{};
	const auto confirmationLogIndex = arguments.indexOf(QStringLiteral("--confirmation-log"));
	const QString confirmationLogPath = confirmationLogIndex >= 0 && confirmationLogIndex + 1 < arguments.size()
	                                  ? arguments.at(confirmationLogIndex + 1) : QString{};
	if (arguments.contains(QStringLiteral("--exit-normal")))
		return 0;
	if (arguments.contains(QStringLiteral("--crash")))
		std::abort();
	if (arguments.contains(QStringLiteral("--idle"))) {
		while (true)
			QThread::msleep(100);
	}

	QFile input;
	QFile output;
	QFile error;
	if (!input.open(stdin, QIODevice::ReadOnly) || !output.open(stdout, QIODevice::WriteOnly)
	    || !error.open(stderr, QIODevice::WriteOnly))
		return 2;
	QFile confirmationLog(confirmationLogPath);
	if (!confirmationLogPath.isEmpty() && !confirmationLog.open(QIODevice::WriteOnly | QIODevice::Append))
		return 2;

	QJsonObject request;
	bool initializedObserved = false;
	bool unsupportedRequestPending = false;
	bool unsupportedRequestRejected = false;
	while (readMessage(input, request)) {
		if (lspProfile == QStringLiteral("unsupported-request") && unsupportedRequestPending
		    && request.value(QStringLiteral("id")) == QJsonValue(QStringLiteral("server-request-id"))) {
			if (!isUnsupportedRequestRejection(request))
				return 8;
			unsupportedRequestPending = false;
			unsupportedRequestRejected = true;
			if (!writeAll(confirmationLog, QByteArrayLiteral("unsupported request rejected\n")))
				return 3;
			continue;
		}
		const QString method = request.value(QStringLiteral("method")).toString();
		if (!lspProfile.isEmpty() && method == QStringLiteral("initialize")) {
			if (lspProfile == QStringLiteral("initialize-exit"))
				return 0;
			if (lspProfile == QStringLiteral("delayed-initialize"))
				QThread::msleep(300);
			if (!validInitializeRequest(request)) {
				const QJsonObject errorResponse = jsonObject({
					{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
					{QStringLiteral("id"), request.value(QStringLiteral("id"))},
					{QStringLiteral("error"), jsonObject({{QStringLiteral("code"), -32602}, {QStringLiteral("message"), QStringLiteral("invalid initialize shape")}})}
				});
				if (!writeAll(output, frame(errorResponse)))
					return 3;
				continue;
			}
			if (lspProfile == QStringLiteral("initialize-error")) {
				const QJsonObject errorResponse = jsonObject({
					{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
					{QStringLiteral("id"), request.value(QStringLiteral("id"))},
					{QStringLiteral("error"), jsonObject({{QStringLiteral("code"), -32002}, {QStringLiteral("message"), QStringLiteral("scripted initialize failure")}})}
				});
				if (!writeAll(output, frame(errorResponse)))
					return 3;
				continue;
			}
			const QJsonValue result = lspProfile == QStringLiteral("malformed-initialize")
			                        ? QJsonValue(QJsonArray{})
			                        : QJsonValue(jsonObject({{QStringLiteral("capabilities"), initializeCapabilities(lspProfile)}}));
			const QJsonObject response = jsonObject({
				{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
				{QStringLiteral("id"), request.value(QStringLiteral("id"))},
				{QStringLiteral("result"), result}
			});
			if (!writeAll(output, frame(response)))
				return 3;
			continue;
		}
		if (!lspProfile.isEmpty() && method == QStringLiteral("initialized")) {
			initializedObserved = true;
			if (lspProfile == QStringLiteral("exit-after-initialized"))
				return 0;
			const QJsonObject observed = jsonObject({
				{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
				{QStringLiteral("method"), QStringLiteral("fake/initializedObserved")}
			});
			if (!writeAll(output, frame(observed)))
				return 3;
			if (lspProfile == QStringLiteral("unsupported-request")) {
				const QJsonObject unsupportedRequest = jsonObject({
					{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
					{QStringLiteral("id"), QStringLiteral("server-request-id")},
					{QStringLiteral("method"), QStringLiteral("fake/unsupportedRequest")}
				});
				if (!writeAll(output, frame(unsupportedRequest)))
					return 3;
				unsupportedRequestPending = true;
			}
			continue;
		}
		if (!lspProfile.isEmpty() && method == QStringLiteral("shutdown")) {
			if (!initializedObserved || (lspProfile == QStringLiteral("unsupported-request") && !unsupportedRequestRejected))
				return 4;
			if (lspProfile == QStringLiteral("shutdown-timeout"))
				continue;
			const QJsonObject response = jsonObject({
				{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
				{QStringLiteral("id"), request.value(QStringLiteral("id"))},
				{QStringLiteral("result"), QJsonValue(QJsonValue::Null)}
			});
			if (!writeAll(output, frame(response)))
				return 3;
			continue;
		}
		if (!lspProfile.isEmpty() && method == QStringLiteral("exit")) {
			error.write("fake-server exit received\n");
			error.flush();
			return 0;
		}
		if (method == QStringLiteral("noResponse"))
			continue;
		if (method == QStringLiteral("exitNormal"))
			return 0;
		if (method == QStringLiteral("crash"))
			std::abort();
		if (method == QStringLiteral("malformed")) {
			if (!writeAll(output, QByteArrayLiteral("Content-Length: invalid\r\n\r\n")))
				return 3;
			continue;
		}
		if (method == QStringLiteral("delay"))
			QThread::msleep(100);
		if (method == QStringLiteral("stderr")) {
			error.write("fake-server stderr\n");
			error.flush();
		}

		QJsonObject response = jsonObject({
			{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
			{QStringLiteral("id"), request.value(QStringLiteral("id"))},
			{QStringLiteral("result"), request.value(QStringLiteral("params"))}
		});
		QByteArray outputBytes = frame(response);
		if (method == QStringLiteral("several")) {
			const QJsonObject notification = jsonObject({
				{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
				{QStringLiteral("method"), QStringLiteral("fake/notification")},
				{QStringLiteral("params"), jsonObject({{QStringLiteral("source"), QStringLiteral("fake")}})}
			});
			outputBytes.prepend(frame(notification));
		}

		if (method == QStringLiteral("fragmentHeader")) {
			if (!writeAll(output, outputBytes.left(8)))
				return 3;
			QThread::msleep(20);
			if (!writeAll(output, outputBytes.mid(8)))
				return 3;
		}
		else if (method == QStringLiteral("fragmentBody")) {
			const auto split = outputBytes.indexOf("\r\n\r\n") + 6;
			if (!writeAll(output, outputBytes.left(split)))
				return 3;
			QThread::msleep(20);
			if (!writeAll(output, outputBytes.mid(split)))
				return 3;
		}
		else if (!writeAll(output, outputBytes)) {
			return 3;
		}
	}
	return 0;
}
