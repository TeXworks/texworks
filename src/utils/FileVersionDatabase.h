#ifndef FileVersionDatabase_H
#define FileVersionDatabase_H

#include <QFileInfo>

namespace Tw {
namespace Utils {


class FileVersionDatabase
{
public:
	struct Record {
		QFileInfo filePath;
		QString version;
		QByteArray hash;
	};

	FileVersionDatabase() = default;
	virtual ~FileVersionDatabase() = default;

	static QByteArray hashForFile(const QString & path);

	static FileVersionDatabase load(const QString & path);
	bool save(const QString & path) const;

	void addFileRecord(const QFileInfo & file, const QByteArray & hash, const QString & version);
	bool hasFileRecord(const QFileInfo & file) const;
	Record getFileRecord(const QFileInfo & file) const;
	const QList<Record> & getFileRecords() const { return m_records; }
	QList<Record> & getFileRecords() { return m_records; }

private:
	QList<Record> m_records;
};

} // namespace Utils
} // namespace Tw

#endif // !defined(FileVersionDatabase_H)
