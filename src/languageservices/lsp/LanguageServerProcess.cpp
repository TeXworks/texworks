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

#include "languageservices/lsp/LanguageServerProcess.h"

namespace Tw {
namespace LanguageServices {
namespace Lsp {

LanguageServerProcess::LanguageServerProcess(QObject * parent)
	: QObject(parent)
{
	m_process.setProcessChannelMode(QProcess::SeparateChannels);
	m_gracefulTimer.setSingleShot(true);
	m_terminateTimer.setSingleShot(true);

	connect(&m_process, &QProcess::started, this, [this]() {
		if (!m_stopRequested)
			setState(Running);
	});
	connect(&m_process, &QProcess::readyReadStandardOutput, this, [this]() {
		m_transport.receiveData(m_process.readAllStandardOutput());
	});
	connect(&m_process, &QProcess::readyReadStandardError, this, [this]() {
		emit standardErrorReceived(m_process.readAllStandardError());
	});
#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
	connect(&m_process, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error), this, &LanguageServerProcess::handleProcessError);
#else
	connect(&m_process, &QProcess::errorOccurred, this, &LanguageServerProcess::handleProcessError);
#endif
	connect(&m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
	        this, &LanguageServerProcess::handleFinished);
	connect(&m_transport, &JsonRpcTransport::outgoingFrame, this, &LanguageServerProcess::writeFrame);
	connect(&m_transport, &JsonRpcTransport::protocolError, this, &LanguageServerProcess::handleProtocolError);
	connect(&m_gracefulTimer, &QTimer::timeout, this, &LanguageServerProcess::forceTerminate);
	connect(&m_terminateTimer, &QTimer::timeout, this, &LanguageServerProcess::forceKill);
}

LanguageServerProcess::~LanguageServerProcess()
{
	m_gracefulTimer.stop();
	m_terminateTimer.stop();
	if (m_process.state() != QProcess::NotRunning) {
		disconnect(&m_process, nullptr, this, nullptr);
		disconnect(&m_transport, nullptr, this, nullptr);
		m_process.kill();
		m_process.waitForFinished(1000);
	}
}

bool LanguageServerProcess::start(const QString & program, const QStringList & arguments,
	                              const QProcessEnvironment & environment, const QString & workingDirectory)
{
	if (m_state != NotRunning || m_transport.isFailed() || program.isEmpty())
		return false;

	m_stopRequested = false;
	m_process.setProcessEnvironment(environment);
	m_process.setWorkingDirectory(workingDirectory);
	m_process.setProcessChannelMode(QProcess::SeparateChannels);
	setState(Starting);
	m_process.start(program, arguments);
	return true;
}

void LanguageServerProcess::stop(int gracefulTimeoutMs, int terminateTimeoutMs)
{
	if (m_process.state() == QProcess::NotRunning) {
		if (m_state != Failed)
			setState(NotRunning);
		return;
	}

	m_stopRequested = true;
	setState(Stopping);
	m_process.closeWriteChannel();
	m_gracefulTimer.start(qMax(0, gracefulTimeoutMs));
	m_terminateTimer.setInterval(qMax(0, terminateTimeoutMs));
}

void LanguageServerProcess::writeFrame(const QByteArray & frame)
{
	if (m_state != Running || m_process.write(frame) < 0) {
		const QString description = tr("Could not write to the language server process");
		emit writeFailed(description);
		m_transport.fail(description);
	}
}

void LanguageServerProcess::handleProcessError(QProcess::ProcessError error)
{
	const QString description = m_process.errorString();
	emit processError(error, description);
	if (m_stopRequested && error == QProcess::Crashed)
		return;
	if (error == QProcess::FailedToStart || error == QProcess::Crashed || error == QProcess::WriteError || error == QProcess::ReadError) {
		m_gracefulTimer.stop();
		m_terminateTimer.stop();
		setState(Failed);
		m_transport.fail(description);
	}
}

void LanguageServerProcess::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	const bool unexpected = !m_stopRequested;
	m_gracefulTimer.stop();
	m_terminateTimer.stop();
	const QByteArray remainingOutput = m_process.readAllStandardOutput();
	if (!remainingOutput.isEmpty())
		m_transport.receiveData(remainingOutput);
	const QByteArray remainingError = m_process.readAllStandardError();
	if (!remainingError.isEmpty())
		emit standardErrorReceived(remainingError);
	emit processFinished(exitCode, exitStatus, unexpected);
	if (unexpected) {
		setState(Failed);
		m_transport.fail(tr("Language server process exited unexpectedly"));
	}
	else {
		m_transport.close(tr("Language server process stopped"));
		setState(NotRunning);
	}
}

void LanguageServerProcess::handleProtocolError(const QString &)
{
	if (m_process.state() != QProcess::NotRunning)
		m_process.kill();
	setState(Failed);
}

void LanguageServerProcess::setState(State state)
{
	if (m_state == state)
		return;
	m_state = state;
	emit stateChanged(m_state);
}

void LanguageServerProcess::forceTerminate()
{
	if (m_process.state() == QProcess::NotRunning)
		return;
	m_process.terminate();
	m_terminateTimer.start();
}

void LanguageServerProcess::forceKill()
{
	if (m_process.state() != QProcess::NotRunning)
		m_process.kill();
}

} // namespace Lsp
} // namespace LanguageServices
} // namespace Tw
