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
#ifndef JSONRPCTRANSPORT_H
#define JSONRPCTRANSPORT_H

#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QSet>

namespace Tw {
namespace LanguageServices {
namespace Lsp {

class JsonRpcTransport : public QObject
{
	Q_OBJECT

public:
	static const int MaximumHeaderSize = 16 * 1024;
	static const int MaximumMessageSize = 8 * 1024 * 1024;
	static const int MaximumReceiveBufferSize = MaximumHeaderSize + MaximumMessageSize + 4;
	static const qint64 MaximumRequestId = 9007199254740991LL;

	explicit JsonRpcTransport(QObject * parent = nullptr);

	bool isFailed() const { return m_failed; }
	int pendingRequestCount() const { return static_cast<int>(m_pendingRequestIds.size()); }

	qint64 sendRequest(const QString & method, const QJsonValue & params = QJsonValue(QJsonValue::Undefined));
	bool sendNotification(const QString & method, const QJsonValue & params = QJsonValue(QJsonValue::Undefined));
	bool sendErrorResponse(const QJsonValue & id, int code, const QString & message);
	void receiveData(const QByteArray & data);
	void fail(const QString & reason);
	void close(const QString & reason);

	static QByteArray frame(const QJsonObject & message);

signals:
	void outgoingFrame(const QByteArray & frame);
	void responseReceived(qint64 id, const QJsonValue & result);
	void errorResponseReceived(qint64 id, const QJsonObject & error);
	void unknownResponseReceived(const QJsonValue & id);
	void notificationReceived(const QString & method, const QJsonValue & params);
	void requestReceived(const QJsonValue & id, const QString & method, const QJsonValue & params);
	void pendingRequestFailed(qint64 id, const QString & reason);
	void protocolError(const QString & reason);

private:
	bool emitMessage(const QJsonObject & message);
	void invalidatePendingRequests(const QString & reason);
	void parseAvailableMessages();
	bool parseHeader(decltype(QByteArray().size()) headerEnd, int & contentLength);
	void processMessage(const QByteArray & body);
	bool correlatedRequestId(const QJsonValue & value, qint64 & id) const;

	QByteArray m_receiveBuffer;
	QSet<qint64> m_pendingRequestIds;
	qint64 m_nextRequestId{1};
	int m_expectedBodySize{-1};
	bool m_failed{false};
};

} // namespace Lsp
} // namespace LanguageServices
} // namespace Tw

#endif // JSONRPCTRANSPORT_H
