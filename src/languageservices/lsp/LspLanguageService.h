/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef LSPLANGUAGESERVICE_H
#define LSPLANGUAGESERVICE_H

#include "languageservices/LanguageService.h"
#include "languageservices/LanguageServiceConfiguration.h"
#include "languageservices/lsp/LanguageServerProcess.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QTimer>

namespace Tw {
namespace LanguageServices {
namespace Lsp {

class LspLanguageService : public LanguageService
{
	Q_OBJECT

public:
	explicit LspLanguageService(const LanguageServiceConfiguration & configuration,
	                            QObject * parent = nullptr,
	                            int initializeTimeoutMs = 5000,
	                            int shutdownTimeoutMs = 1000,
	                            int terminateTimeoutMs = 1000);
	~LspLanguageService() override;

	bool start() override;
	void stop() override;

private:
	void beginInitialize();
	void handleResponse(qint64 id, const QJsonValue & result);
	void handleErrorResponse(qint64 id, const QJsonObject & error);
	void handlePendingFailure(qint64 id, const QString & reason);
	void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus, bool unexpected);
	bool mapCapabilities(const QJsonObject & capabilities, LanguageServiceCapabilities & mapped, QString & error) const;
	void initializationFailed(const QString & reason);
	void beginProcessStop();
	void sendExitAndStop();

	LanguageServiceConfiguration m_configuration;
	LanguageServerProcess m_process;
	QTimer m_initializeTimer;
	QTimer m_shutdownTimer;
	qint64 m_initializeRequestId{-1};
	qint64 m_shutdownRequestId{-1};
	int m_initializeTimeoutMs;
	int m_shutdownTimeoutMs;
	int m_terminateTimeoutMs;
	bool m_teardownStarted{false};
};

} // namespace Lsp
} // namespace LanguageServices
} // namespace Tw

#endif // LSPLANGUAGESERVICE_H
