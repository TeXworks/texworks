/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2007-2010  Jonathan Kew

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

int main(int argc, char *argv[])
{
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
				for (int i = 1; i < argc; ++i) {
					QFileInfo fi(QString::fromLocal8Bit(argv[i]));
					if (!fi.exists())
						continue;
					QByteArray ba = fi.absoluteFilePath().toLocal8Bit();
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

	TWApp app(argc, argv);

#ifdef Q_WS_X11
	if (QDBusConnection::sessionBus().registerService(TW_SERVICE_NAME) == false) {
		QDBusInterface	interface(TW_SERVICE_NAME, TW_APP_PATH, TW_INTERFACE_NAME);
		if (interface.isValid()) {
			interface.call("bringToFront");
			for (int i = 1; i < argc; ++i) {
				QFileInfo fi(QString::fromLocal8Bit(argv[i]));
				if (!fi.exists())
					continue;
				interface.call("openFile", fi.absoluteFilePath());
			}
		}
		return 0;
	}

	new TWAdaptor(&app);
	if (QDBusConnection::sessionBus().registerObject(TW_APP_PATH, &app) == false) {
		// failed to register the application object, so unregister our service
		// and continue as a multiple-instance app instead
		(void)QDBusConnection::sessionBus().unregisterService(TW_SERVICE_NAME);
	}
#endif

	// first argument is the executable name, so we skip that
	for (int i = 1; i < argc; ++i)
		app.openFile(QTextCodec::codecForLocale()->toUnicode(argv[i]));

	QTimer::singleShot(1, &app, SLOT(launchAction()));

	int rval = app.exec();

#ifdef Q_WS_WIN
	CloseHandle(hMutex);
#endif

	return rval;
}
