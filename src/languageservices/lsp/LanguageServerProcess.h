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
#ifndef LANGUAGESERVERPROCESS_H
#define LANGUAGESERVERPROCESS_H

#include "languageservices/lsp/JsonRpcTransport.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QTimer>

namespace Tw {
namespace LanguageServices {
namespace Lsp {

class LanguageServerProcess : public QObject
{
	Q_OBJECT

public:
	enum State {
		NotRunning,
		Starting,
		Running,
		Stopping,
		Failed
	};
	Q_ENUMS(State)

	explicit LanguageServerProcess(QObject * parent = nullptr);
	~LanguageServerProcess() override;

	State state() const { return m_state; }
	bool isRunning() const { return m_process.state() != QProcess::NotRunning; }
	JsonRpcTransport * transport() { return &m_transport; }
	const JsonRpcTransport * transport() const { return &m_transport; }

	bool start(const QString & program, const QStringList & arguments = {},
	           const QProcessEnvironment & environment = QProcessEnvironment::systemEnvironment(),
	           const QString & workingDirectory = {});
	void stop(int gracefulTimeoutMs = 1000, int terminateTimeoutMs = 1000);

signals:
	void stateChanged(Tw::LanguageServices::Lsp::LanguageServerProcess::State state);
	void standardErrorReceived(const QByteArray & data);
	void processError(QProcess::ProcessError error, const QString & description);
	void processFinished(int exitCode, QProcess::ExitStatus exitStatus, bool unexpected);
	void writeFailed(const QString & description);

private slots:
	void writeFrame(const QByteArray & frame);
	void handleProcessError(QProcess::ProcessError error);
	void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void handleProtocolError(const QString & reason);

private:
	void setState(State state);
	void forceTerminate();
	void forceKill();

	QProcess m_process;
	JsonRpcTransport m_transport;
	QTimer m_gracefulTimer;
	QTimer m_terminateTimer;
	State m_state{NotRunning};
	bool m_stopRequested{false};
};

} // namespace Lsp
} // namespace LanguageServices
} // namespace Tw

Q_DECLARE_METATYPE(Tw::LanguageServices::Lsp::LanguageServerProcess::State)

#endif // LANGUAGESERVERPROCESS_H
