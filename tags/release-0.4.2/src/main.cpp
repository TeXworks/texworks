/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2007-2011  Jonathan Kew, Stefan Löffler

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

	For links to further information, or to contact the author,
	see <http://texworks.org/>.
*/

#include "TWApp.h"
#include "TWVersion.h"
#include "CommandlineParser.h"
#include "SvnRev.h"

#include <QTimer>
#include <QTextCodec>

#ifdef Q_WS_WIN
BOOL CALLBACK enumThreadWindowProc(HWND hWnd, LPARAM /*lParam*/)
{
	if (IsWindowVisible(hWnd))
		SetForegroundWindow(hWnd);
	return true;
}
#endif

struct fileToOpenStruct{
	QString filename;
	int position;
};

int main(int argc, char *argv[])
{
	TWApp app(argc, argv);

	CommandlineParser clp;
	QList<fileToOpenStruct> filesToOpen;
	fileToOpenStruct fileToOpen;
	
	clp.registerSwitch("help", "Display this message", "?");
	clp.registerOption("position", "Open the following file at the given position (line or page)", "p");
	clp.registerSwitch("version", "Display version information", "v");

	int pos;
	int numArgs = 0;
	bool launchApp = true;
	if (clp.parse()) {
		int i, j;
		while ((i = clp.getNextArgument()) >= 0) {
			++numArgs;
			pos = -1;
			if ((j = clp.getPrevOption("position", i)) >= 0) {
				pos = clp.at(j).value.toInt();
				clp.at(j).processed = true;
			}
			CommandlineParser::CommandlineItem & item = clp.at(i);
			item.processed = true;

			fileToOpen.filename = item.value.toString();
			fileToOpen.position = pos;
			filesToOpen << fileToOpen;
		}
		if ((i = clp.getNextSwitch("version")) >= 0) {
			if (numArgs == 0)
				launchApp = false;
			clp.at(i).processed = true;
			QTextStream strm(stdout);
			strm << QString("TeXworks %1r%2 (%3)\n\n").arg(TEXWORKS_VERSION).arg(SVN_REVISION_STR).arg(TW_BUILD_ID_STR);
			strm << QString::fromUtf8("\
Copyright (C) 2007-2011  Jonathan Kew, Stefan Löffler\n\
License GPLv2+: GNU GPL (version 2 or later) <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\n");
			strm.flush();
		}
		if ((i = clp.getNextSwitch("help")) >= 0) {
			if (numArgs == 0)
				launchApp = false;
			clp.at(i).processed = true;
			QTextStream strm(stdout);
			clp.printUsage(strm);
		}
	}

#ifdef Q_WS_WIN // single-instance code for Windows
#define TW_MUTEX_NAME		"org.tug.texworks-" TEXWORKS_VERSION
	HANDLE hMutex = CreateMutexA(NULL, FALSE, TW_MUTEX_NAME);
	if (hMutex == NULL)
		return 0;	// failure
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// this is a second instance: bring the original instance to the top
		for (int retry = 0; retry < 100; ++retry) {
			HWND hWnd = FindWindowExA(HWND_MESSAGE, NULL, TW_HIDDEN_WINDOW_CLASS, NULL);
			if (hWnd) {
				// pull the app's (visible) windows to the foreground
				DWORD thread = GetWindowThreadProcessId(hWnd, NULL);
				(void)EnumThreadWindows(thread, &enumThreadWindowProc, 0);
				// send each cmd-line arg as a WM_COPYDATA message to load a file
				foreach(fileToOpen, filesToOpen) {
					QFileInfo fi(fileToOpen.filename);
					if (!fi.exists())
						continue;
					QByteArray ba = fi.absoluteFilePath().toUtf8() + "\n" + QByteArray::number(fileToOpen.position);
					COPYDATASTRUCT cds;
					cds.dwData = TW_OPEN_FILE_MSG;
					cds.cbData = ba.length();
					cds.lpData = ba.data();
					SendMessageA(hWnd, WM_COPYDATA, 0, (LPARAM)&cds);
				}
				break;
			}
			// couldn't find the other instance; not ready yet?
			// sleep for 50ms and then retry
			Sleep(50);
		}
		CloseHandle(hMutex);	// close our handle to the mutex
		return 0;
	}
#endif

#ifdef QT_DBUS_LIB
	if (QDBusConnection::sessionBus().registerService(TW_SERVICE_NAME) == false) {
		QDBusInterface interface(TW_SERVICE_NAME, TW_APP_PATH, TW_INTERFACE_NAME);
		if (interface.isValid()) {
			interface.call("bringToFront");
			foreach(fileToOpen, filesToOpen) {
				QFileInfo fi(fileToOpen.filename);
				if (!fi.exists())
					continue;
				interface.call("openFile", fi.absoluteFilePath(), fileToOpen.position);
			}
			return 0;
		}
		else {
			// We could not register the service, but couldn't connect to an
			// already registered one, either. This can mean that something is
			// seriously wrong, we've met some race condition, or the dbus
			// service is not running. Let's assume the best (dbus not running)
			// and continue as a multiple-instance app instead
		}
	}

	new TWAdaptor(&app);
	if (QDBusConnection::sessionBus().registerObject(TW_APP_PATH, &app) == false) {
		// failed to register the application object, so unregister our service
		// and continue as a multiple-instance app instead
		(void)QDBusConnection::sessionBus().unregisterService(TW_SERVICE_NAME);
	}
#endif // defined(QT_DBUS_LIB)

	int rval = 0;
	if (launchApp) {
		foreach (fileToOpen, filesToOpen) {
			app.openFile(fileToOpen.filename, fileToOpen.position);
		}

		QTimer::singleShot(1, &app, SLOT(launchAction()));
		rval = app.exec();
	}

#ifdef Q_WS_WIN
	CloseHandle(hMutex);
#endif

	return rval;
}
