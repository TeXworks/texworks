/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef LANGUAGESERVICEMANAGER_H
#define LANGUAGESERVICEMANAGER_H

#include "languageservices/LanguageService.h"

#include <QObject>

namespace Tw {
namespace LanguageServices {

class LanguageServiceManager : public QObject
{
	Q_OBJECT

public:
	explicit LanguageServiceManager(QObject * parent = nullptr);
	~LanguageServiceManager() override;

	bool setService(LanguageService * service);
	bool replaceService(LanguageService * service, bool startService = true);
	LanguageService * service() const { return m_service; }
	LanguageService::State state() const;
	LanguageServiceCapabilities capabilities() const;

	bool start();
	void stop();

signals:
	void serviceChanged(Tw::LanguageServices::LanguageService * service);
	void stateChanged(Tw::LanguageServices::LanguageService::State state);
	void capabilitiesChanged(const Tw::LanguageServices::LanguageServiceCapabilities & capabilities);
	void failed(const QString & reason);

private:
	void installService(LanguageService * service);
	void finishServiceReplacement();

	LanguageService * m_service{nullptr};
	LanguageService * m_pendingService{nullptr};
	bool m_startPendingService{false};
	bool m_replacingService{false};
};

} // namespace LanguageServices
} // namespace Tw

#endif // LANGUAGESERVICEMANAGER_H
