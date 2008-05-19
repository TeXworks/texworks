/*
    This is part of TeXworks, an environment for working with TeX documents
    Copyright (C) 2007-08  Jonathan Kew

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef PDFDOCKS_H
#define PDFDOCKS_H

#include <QDockWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QScrollArea>

class PDFDocument;
class QListWidget;
class QTableWidget;
class QTreeWidgetItem;

class PDFDock : public QDockWidget
{
	Q_OBJECT

public:
    PDFDock(const QString& title, PDFDocument *doc = 0);
    virtual ~PDFDock();

    virtual void documentLoaded();
    virtual void documentClosed();
    virtual void pageChanged(int page);

    void setPage(int page);
    void reloadPage();

protected:
    virtual void fillInfo() = 0;

    PDFDocument *document;

private slots:
    void myVisibilityChanged(bool visible);

private:
    bool filled;
};


class PDFOutlineDock : public PDFDock
{
	Q_OBJECT

public:
	PDFOutlineDock(PDFDocument *doc = 0);
	virtual ~PDFOutlineDock();

    virtual void documentClosed();

protected:
	virtual void fillInfo();

private slots:
	void followTocSelection();

private:
    QTreeWidget *tree;
};

class PDFDockTreeWidget : public QTreeWidget
{
	Q_OBJECT

public:
	PDFDockTreeWidget(QWidget* parent);
	virtual ~PDFDockTreeWidget();

	virtual QSize sizeHint() const;
};


class PDFInfoDock : public PDFDock
{
    Q_OBJECT

public:
    PDFInfoDock(PDFDocument *doc = 0);
    ~PDFInfoDock();

    virtual void documentClosed();

protected:
    virtual void fillInfo();

private:
    QListWidget *list;
};

class PDFDockListWidget : public QListWidget
{
	Q_OBJECT

public:
	PDFDockListWidget(QWidget* parent);
	virtual ~PDFDockListWidget();

	virtual QSize sizeHint() const;
};


class PDFFontsDock : public PDFDock
{
    Q_OBJECT

public:
    PDFFontsDock(PDFDocument *doc = 0);
    ~PDFFontsDock();

    virtual void documentClosed();

protected:
    virtual void fillInfo();

private:
    QTableWidget *table;
};


class PDFScrollArea : public QScrollArea
{
	Q_OBJECT

public:
	PDFScrollArea(QWidget *parent = NULL);
	virtual ~PDFScrollArea();

protected:
	virtual void resizeEvent(QResizeEvent *event);

signals:
	void resized();
};

#endif
