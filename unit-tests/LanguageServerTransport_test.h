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
#ifndef LANGUAGESERVERTRANSPORT_TEST_H
#define LANGUAGESERVERTRANSPORT_TEST_H

#include <QObject>

class LanguageServerTransportTest : public QObject
{
	Q_OBJECT

public slots:
	void markTimerFired();

private slots:
	void initTestCase();

	void completeFrame();
	void fragmentedHeader();
	void fragmentedBody();
	void severalFramesAndTrailingPartial();
	void caseInsensitiveAndAdditionalHeader();
	void malformedLengths_data();
	void malformedLengths();
	void malformedAndZeroLengthJson();
	void utf8Json();
	void failedTransportDiscardsBufferedDataAndPendingRequests();

	void notificationSerialization();
	void requestSerializationAndIds();
	void errorResponseSerializationNumericId();
	void errorResponseSerializationStringId();
	void outgoingMessageSizeBound();
	void successfulAndErrorCorrelation();
	void unknownAndDuplicateResponse();
	void incomingNotificationAndRequest();
	void invalidJsonRpcMarker();

	void processRoundTripAndFragmentation();
	void processCoalescedFramesAndStderrIsolation();
	void processDelayKeepsEventLoopResponsive();
	void processProtocolFailureStopsProcess();
	void processNormalAndUnexpectedExit();
	void processStartFailure();
	void processStopIsBounded();
	void processDestructionDoesNotHang();

private:
	bool m_timerFired{false};
};

#endif // LANGUAGESERVERTRANSPORT_TEST_H
