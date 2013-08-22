/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2007-2012  Jonathan Kew, Stefan Löffler, Charlie Sharpsteen

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	For links to further information, or to contact the authors,
	see <http://www.tug.org/texworks/>.
*/

#ifndef TWSystemCmd_H
#define TWSystemCmd_H

#include <QProcess>



class TWSystemCmd : public QProcess {
	Q_OBJECT
	
public:
	TWSystemCmd(QObject* parent, const bool isOutputWanted = true, const bool runInBackground = false)
		: QProcess(parent), wantOutput(isOutputWanted), deleteOnFinish(runInBackground)
	{
		connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(processOutput()));
		connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
		connect(this, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
		finishedSuccessfully = false;
		finished = false;
	}
	virtual ~TWSystemCmd() {}
	
	QString getResult() { return result; }
	
	// replacement of QProcess::waitForStarted()
	// unlike the QProcess version, this returns true if the process has already
	// finished when the function is called
	bool waitForStarted(int msecs = 30000) {
		if (finished)
			return finishedSuccessfully;
		else
			return QProcess::waitForStarted(msecs);
	}

	// replacement of QProcess::waitForFinished()
	// unlike the QProcess version, this returns true if the process has already
	// finished when the function is called
	bool waitForFinished(int msecs = 30000) {
		if (finished)
			return finishedSuccessfully;
		else
			return QProcess::waitForFinished(msecs);
	}
	
private slots:
	void processError(QProcess::ProcessError error) {
		finished = true;
		if (wantOutput)
			result = tr("ERROR: failure code %1").arg(error);
		if (deleteOnFinish)
			deleteLater();
	}
	void processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
		finished = true;
		finishedSuccessfully = (exitStatus == QProcess::NormalExit);
		if (wantOutput) {
			if (exitStatus == QProcess::NormalExit) {
				if (bytesAvailable() > 0) {
					QByteArray ba = readAllStandardOutput();
					result += QString::fromLocal8Bit(ba);
				}
			}
			else {
				result = tr("ERROR: exit code %1").arg(exitCode);
			}
		}
		if (deleteOnFinish)
			deleteLater();
	}
	void processOutput() {
		if (wantOutput && bytesAvailable() > 0) {
			QByteArray ba = readAllStandardOutput();
			result += QString::fromLocal8Bit(ba);
		}
	}

private:
	bool wantOutput;
	bool deleteOnFinish;
	bool finished;
	bool finishedSuccessfully;
	QString result;
};


#endif // !defined(TWSystemCmd_H)

