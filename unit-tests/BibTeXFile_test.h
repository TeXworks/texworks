#include <QtTest/QtTest>

class TestBibTeXFile : public QObject
{
  Q_OBJECT
private slots:
  void load();
  void numEntries();
  void entry_type();
  void entry_typeString();
  void entry_key();
  void entry_hasField();
  void entry_value();
  void entry_title();
  void entry_author();
  void entry_year();
  void entry_howPublished();
};
