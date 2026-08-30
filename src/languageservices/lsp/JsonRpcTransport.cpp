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

#include "languageservices/lsp/JsonRpcTransport.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QtGlobal>

#include <cmath>

namespace Tw {
namespace LanguageServices {
namespace Lsp {

JsonRpcTransport::JsonRpcTransport(QObject * parent)
	: QObject(parent)
{
}

QByteArray JsonRpcTransport::frame(const QJsonObject & message)
{
	const QByteArray body = QJsonDocument(message).toJson(QJsonDocument::Compact);
	return QByteArrayLiteral("Content-Length: ") + QByteArray::number(body.size()) + QByteArrayLiteral("\r\n\r\n") + body;
}

bool JsonRpcTransport::emitMessage(const QJsonObject & message)
{
	const QByteArray body = QJsonDocument(message).toJson(QJsonDocument::Compact);
	if (body.size() > MaximumMessageSize) {
		fail(tr("JSON-RPC message exceeds the maximum size"));
		return false;
	}
	emit outgoingFrame(QByteArrayLiteral("Content-Length: ") + QByteArray::number(body.size()) + QByteArrayLiteral("\r\n\r\n") + body);
	return !m_failed;
}

qint64 JsonRpcTransport::sendRequest(const QString & method, const QJsonValue & params)
{
	if (m_failed || method.isEmpty() || m_nextRequestId > MaximumRequestId)
		return -1;

	const qint64 id = m_nextRequestId;
	QJsonObject message;
	message.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
	message.insert(QStringLiteral("id"), static_cast<double>(id));
	message.insert(QStringLiteral("method"), method);
	if (!params.isUndefined())
		message.insert(QStringLiteral("params"), params);
	const QByteArray body = QJsonDocument(message).toJson(QJsonDocument::Compact);
	if (body.size() > MaximumMessageSize) {
		fail(tr("JSON-RPC message exceeds the maximum size"));
		return -1;
	}
	++m_nextRequestId;
	m_pendingRequestIds.insert(id);
	emit outgoingFrame(QByteArrayLiteral("Content-Length: ") + QByteArray::number(body.size()) + QByteArrayLiteral("\r\n\r\n") + body);
	return m_failed ? -1 : id;
}

bool JsonRpcTransport::sendNotification(const QString & method, const QJsonValue & params)
{
	if (m_failed || method.isEmpty())
		return false;

	QJsonObject message;
	message.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
	message.insert(QStringLiteral("method"), method);
	if (!params.isUndefined())
		message.insert(QStringLiteral("params"), params);
	return emitMessage(message);
}

bool JsonRpcTransport::sendErrorResponse(const QJsonValue & id, int code, const QString & message)
{
	if (m_failed || message.isEmpty())
		return false;

	QJsonObject error;
	error.insert(QStringLiteral("code"), code);
	error.insert(QStringLiteral("message"), message);
	QJsonObject response;
	response.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
	response.insert(QStringLiteral("id"), id);
	response.insert(QStringLiteral("error"), error);
	return emitMessage(response);
}

void JsonRpcTransport::receiveData(const QByteArray & data)
{
	if (m_failed || data.isEmpty())
		return;

	decltype(data.size()) offset = 0;
	while (!m_failed && offset < data.size()) {
		const auto available = static_cast<decltype(data.size())>(MaximumReceiveBufferSize) - m_receiveBuffer.size();
		if (available <= 0) {
			fail(tr("JSON-RPC receive buffer exceeds the maximum size"));
			return;
		}
		const auto chunkSize = qMin(available, data.size() - offset);
		m_receiveBuffer.append(data.constData() + offset, chunkSize);
		offset += chunkSize;
		parseAvailableMessages();
	}
}

void JsonRpcTransport::parseAvailableMessages()
{
	while (!m_failed) {
		if (m_expectedBodySize < 0) {
			const auto headerEnd = m_receiveBuffer.indexOf("\r\n\r\n");
			if (headerEnd < 0) {
				if (m_receiveBuffer.size() > MaximumHeaderSize)
					fail(tr("JSON-RPC header exceeds the maximum size"));
				return;
			}
			if (headerEnd > MaximumHeaderSize) {
				fail(tr("JSON-RPC header exceeds the maximum size"));
				return;
			}
			if (!parseHeader(headerEnd, m_expectedBodySize))
				return;
			m_receiveBuffer.remove(0, headerEnd + 4);
		}

		if (m_receiveBuffer.size() < m_expectedBodySize)
			return;

		const QByteArray body = m_receiveBuffer.left(m_expectedBodySize);
		m_receiveBuffer.remove(0, m_expectedBodySize);
		m_expectedBodySize = -1;
		processMessage(body);
	}
}

bool JsonRpcTransport::parseHeader(decltype(QByteArray().size()) headerEnd, int & contentLength)
{
	contentLength = -1;
	const QList<QByteArray> lines = m_receiveBuffer.left(headerEnd).split('\n');
	for (QByteArray line : lines) {
		if (line.endsWith('\r'))
			line.chop(1);
		const auto colon = line.indexOf(':');
		if (colon <= 0) {
			fail(tr("Malformed JSON-RPC header"));
			return false;
		}
		const QByteArray name = line.left(colon).trimmed();
		const QByteArray value = line.mid(colon + 1).trimmed();
		if (name.toLower() != QByteArrayLiteral("content-length"))
			continue;
		if (contentLength >= 0) {
			fail(tr("Duplicate Content-Length header"));
			return false;
		}
		if (value.isEmpty()) {
			fail(tr("Invalid Content-Length header"));
			return false;
		}
		for (char character : value) {
			if (character < '0' || character > '9') {
				fail(tr("Invalid Content-Length header"));
				return false;
			}
		}
		bool ok = false;
		const qulonglong parsed = value.toULongLong(&ok, 10);
		if (!ok || parsed > static_cast<qulonglong>(MaximumMessageSize)) {
			fail(parsed > static_cast<qulonglong>(MaximumMessageSize)
			         ? tr("JSON-RPC message exceeds the maximum size")
			         : tr("Invalid Content-Length header"));
			return false;
		}
		contentLength = static_cast<int>(parsed);
	}

	if (contentLength < 0) {
		fail(tr("Missing Content-Length header"));
		return false;
	}
	return true;
}

void JsonRpcTransport::processMessage(const QByteArray & body)
{
	QJsonParseError parseError;
	const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
	if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
		fail(tr("Malformed JSON-RPC JSON body: %1").arg(parseError.errorString()));
		return;
	}

	const QJsonObject message = document.object();
	if (message.value(QStringLiteral("jsonrpc")) != QJsonValue(QStringLiteral("2.0"))) {
		fail(tr("Invalid or missing JSON-RPC version marker"));
		return;
	}

	const QJsonValue methodValue = message.value(QStringLiteral("method"));
	const bool hasMethod = methodValue.isString() && !methodValue.toString().isEmpty();
	const bool hasId = message.contains(QStringLiteral("id"));
	const bool hasResult = message.contains(QStringLiteral("result"));
	const bool hasError = message.contains(QStringLiteral("error"));

	if (hasMethod && !hasResult && !hasError) {
		const QJsonValue params = message.value(QStringLiteral("params"));
		if (hasId)
			emit requestReceived(message.value(QStringLiteral("id")), methodValue.toString(), params);
		else
			emit notificationReceived(methodValue.toString(), params);
		return;
	}

	if (!hasMethod && hasId && (hasResult != hasError)) {
		if (hasError && !message.value(QStringLiteral("error")).isObject()) {
			fail(tr("JSON-RPC error response has a non-object error"));
			return;
		}
		qint64 id = -1;
		if (!correlatedRequestId(message.value(QStringLiteral("id")), id) || !m_pendingRequestIds.remove(id)) {
			emit unknownResponseReceived(message.value(QStringLiteral("id")));
			return;
		}
		if (hasResult) {
			emit responseReceived(id, message.value(QStringLiteral("result")));
			return;
		}
		emit errorResponseReceived(id, message.value(QStringLiteral("error")).toObject());
		return;
	}

	fail(tr("Invalid JSON-RPC message shape"));
}

bool JsonRpcTransport::correlatedRequestId(const QJsonValue & value, qint64 & id) const
{
	if (!value.isDouble())
		return false;
	const double numericId = value.toDouble();
	if (!std::isfinite(numericId) || std::floor(numericId) != numericId || numericId < 0
	    || numericId > static_cast<double>(MaximumRequestId))
		return false;
	id = static_cast<qint64>(numericId);
	return true;
}

void JsonRpcTransport::fail(const QString & reason)
{
	if (m_failed)
		return;
	m_failed = true;
	m_receiveBuffer.clear();
	m_expectedBodySize = -1;
	invalidatePendingRequests(reason);
	emit protocolError(reason);
}

void JsonRpcTransport::close(const QString & reason)
{
	if (m_failed)
		return;
	m_failed = true;
	m_receiveBuffer.clear();
	m_expectedBodySize = -1;
	invalidatePendingRequests(reason);
}

void JsonRpcTransport::invalidatePendingRequests(const QString & reason)
{
	const QSet<qint64> pendingIds = m_pendingRequestIds;
	m_pendingRequestIds.clear();
	for (qint64 id : pendingIds)
		emit pendingRequestFailed(id, reason);
}

} // namespace Lsp
} // namespace LanguageServices
} // namespace Tw
