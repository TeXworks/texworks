#include "utils/FileVersionDatabase.h"

#include <QCryptographicHash>
#include <QDir>
#include <QTextStream>

namespace Tw {
namespace Utils {

/*static*/
FileVersionDatabase FileVersionDatabase::load(const QString & path)
{
	QFile fin(path);
	FileVersionDatabase retVal;
	QDir rootDir(QFileInfo(path).absoluteDir());

	if (!fin.open(QIODevice::ReadOnly | QIODevice::Text))
		return retVal;

	QTextStream strm(&fin);

	while (!strm.atEnd()) {
		FileVersionDatabase::Record rec;
		QString line = strm.readLine().trimmed();

		// ignore comments
		if (line.startsWith(QChar::fromLatin1('#'))) continue;

		rec.version = line.section(QChar::fromLatin1(' '), 0, 0);
		rec.hash = QByteArray::fromHex(line.section(QChar::fromLatin1(' '), 1, 1).toLatin1());
		rec.filePath = line.section(QChar::fromLatin1(' '), 2).trimmed();
		rec.filePath = rootDir.absoluteFilePath(rec.filePath.filePath());
		retVal.m_records.append(rec);
	}

	fin.close();

	return retVal;
}

bool FileVersionDatabase::save(const QString & path) const
{
	QFile fout(path);
	QDir rootDir(QFileInfo(path).absoluteDir());

	if (!fout.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;

	QTextStream strm(&fout);

	foreach (FileVersionDatabase::Record rec, m_records) {
		QString filePath = rec.filePath.absoluteFilePath();
		strm << rec.version << " " << rec.hash.toHex() << " " << rootDir.relativeFilePath(filePath) << endl;
	}

	fout.close();
	return true;
}

void FileVersionDatabase::addFileRecord(const QFileInfo & file, const QByteArray & md5Hash, const QString & version)
{
	// remove all existing entries for this file
	QMutableListIterator<FileVersionDatabase::Record> it(m_records);

	while (it.hasNext()) {
		const FileVersionDatabase::Record rec = it.next();
		if (file.absoluteFilePath() == rec.filePath.absoluteFilePath()) {
			it.remove();
		}
	}

	// add the new data
	FileVersionDatabase::Record rec;
	rec.filePath = file;
	rec.version = version;
	rec.hash = md5Hash;
	m_records.append(rec);
}

bool FileVersionDatabase::hasFileRecord(const QFileInfo & file) const
{
	QListIterator<FileVersionDatabase::Record> it(m_records);

	while (it.hasNext()) {
		const FileVersionDatabase::Record rec = it.next();
		if (file.filePath() == rec.filePath.filePath())
			return true;
	}
	return false;
}

FileVersionDatabase::Record FileVersionDatabase::getFileRecord(const QFileInfo & file) const
{
	QListIterator<FileVersionDatabase::Record> it(m_records);

	while (it.hasNext()) {
		const FileVersionDatabase::Record rec = it.next();
		if (file == rec.filePath)
			return rec;
	}

	FileVersionDatabase::Record retVal;
	retVal.version = QString();
	retVal.hash = QByteArray::fromHex("d41d8cd98f00b204e9800998ecf8427e"); // hash for the zero-length string
	return retVal;
}

/*static*/
QByteArray FileVersionDatabase::hashForFile(const QString & path)
{
	QByteArray retVal = QByteArray::fromHex("d41d8cd98f00b204e9800998ecf8427e"); // hash for the zero-length string;
	QFile fin(path);

	if (!fin.open(QIODevice::ReadOnly))
		return retVal;

	retVal = QCryptographicHash::hash(fin.readAll(), QCryptographicHash::Md5);
	fin.close();
	return retVal;
}


} // namespace Utils
} // namespace Tw
