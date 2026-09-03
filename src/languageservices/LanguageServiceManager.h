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

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QTimer>

namespace Tw { namespace Document { class TeXDocument; } }

namespace Tw {
namespace LanguageServices {

class LanguageServiceDocumentBinding;

class LanguageServiceManager : public QObject
{
	Q_OBJECT

public:
	explicit LanguageServiceManager(QObject * parent = nullptr);
	~LanguageServiceManager() override;

	bool setService(LanguageService * service, const QStringList & supportedLanguages = QStringList{});
	bool replaceService(LanguageService * service, const QStringList & supportedLanguages = QStringList{},
	                    bool startService = true);
	LanguageService * service() const { return m_service; }
	LanguageService::State state() const;
	LanguageServiceCapabilities capabilities() const;

	bool start();
	void stop();
	void registerDocument(Document::TeXDocument * document, const QString & sourceLanguage = QString{});
	void unregisterDocument(Document::TeXDocument * document);
	void updateDocumentIdentity(Document::TeXDocument * document);
	void updateDocumentLanguageHint(Document::TeXDocument * document, const QString & sourceLanguage);
	LanguageServiceDocumentBinding * bindingForDocument(Document::TeXDocument * document) const;
	bool canRequestCompletion(Document::TeXDocument * document) const;
	bool requestCompletion(Document::TeXDocument * document, const LanguagePosition & position);
	void cancelCompletionRequest(Document::TeXDocument * document);
	void beginCompletionEdit(Document::TeXDocument * document);
	void endCompletionEdit(Document::TeXDocument * document);
	bool canRequestDefinition(Document::TeXDocument * document) const;
	bool requestDefinition(Document::TeXDocument * document, const LanguagePosition & position);
	void cancelDefinitionRequest(Document::TeXDocument * document);

	static bool isSourceEligible(const Document::TeXDocument * document);
	static QString identifyLanguageId(const Document::TeXDocument * document, const QString & sourceLanguage = QString{});

signals:
	void serviceChanged(Tw::LanguageServices::LanguageService * service);
	void stateChanged(Tw::LanguageServices::LanguageService::State state);
	void capabilitiesChanged(const Tw::LanguageServices::LanguageServiceCapabilities & capabilities);
	void failed(const QString & reason);
	void completionAvailabilityChanged(Tw::Document::TeXDocument * document, bool available);
	void completionReady(Tw::Document::TeXDocument * document,
	                     const QList<Tw::LanguageServices::CompletionItem> & items);
	void definitionAvailabilityChanged(Tw::Document::TeXDocument * document, bool available);
	void definitionReady(Tw::Document::TeXDocument * document,
	                     const QList<Tw::LanguageServices::LanguageLocation> & locations);

private slots:
	void refreshPendingDocuments();

private:
	void installService(LanguageService * service, const QStringList & supportedLanguages);
	void finishServiceReplacement();
	void createBinding(Document::TeXDocument * document);
	void scheduleDocumentRefresh(Document::TeXDocument * document);
	void refreshBinding(Document::TeXDocument * document);

	LanguageService * m_service{nullptr};
	LanguageService * m_pendingService{nullptr};
	QStringList m_pendingSupportedLanguages;
	bool m_startPendingService{false};
	bool m_replacingService{false};
	QStringList m_supportedLanguages;
	QHash<Document::TeXDocument *, QString> m_documents;
	QHash<Document::TeXDocument *, LanguageServiceDocumentBinding *> m_bindings;
	QTimer m_deferredRefreshTimer;
	QList<QPointer<Document::TeXDocument>> m_pendingDocumentRefreshes;
};

} // namespace LanguageServices
} // namespace Tw

#endif // LANGUAGESERVICEMANAGER_H
