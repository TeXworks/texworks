/* This file is based on textedit.cpp from the Qt examples;
   the original copyright notice appears below.
   Changes for TeXworks are (c) 2007 Jonathan Kew. */

/****************************************************************************
**
** Copyright (C) 2006-2007 Trolltech ASA. All rights reserved.
**
** This file is part of the example classes of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.0, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** In addition, as a special exception, Trolltech, as the sole copyright
** holder for Qt Designer, grants users of the Qt/Eclipse Integration
** plug-in the right for the Qt/Eclipse Integration to link to
** functionality provided by Qt Designer and its related libraries.
**
** Trolltech reserves all rights not expressly granted herein.
** 
** Trolltech ASA (c) 2007
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#define POPUP 1

#include "CompletingEdit.h"

#include <QCompleter>
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QtDebug>
#include <QApplication>
#include <QModelIndex>
#include <QAbstractItemModel>
#include <QScrollBar>
#include <QTextCursor>

CompletingEdit::CompletingEdit(QWidget *parent)
	: QTextEdit(parent), c(NULL)
{
}

CompletingEdit::~CompletingEdit()
{
}

void CompletingEdit::setCompleter(QCompleter *completer)
{
    if (c)
        QObject::disconnect(c, 0, this, 0);

    c = completer;

    if (!c)
        return;

    c->setWidget(this);
#if POPUP
    c->setCompletionMode(QCompleter::PopupCompletion);
#else
    c->setCompletionMode(QCompleter::InlineCompletion);	// doesn't yet work
#endif
    c->setCaseSensitivity(Qt::CaseInsensitive);
#if POPUP
    QObject::connect(c, SIGNAL(activated(const QString&)),
                     this, SLOT(insertCompletion(const QString&)));
#else
    QObject::connect(c, SIGNAL(highlighted(const QString&)),
                     this, SLOT(showCurrentCompletion(const QString&)));
#endif
}

QCompleter *CompletingEdit::completer() const
{
    return c;
}

void CompletingEdit::insertCompletion(const QString& completion)
{
    if (c->widget() != this)
        return;
    QTextCursor tc = textCursor();
//    int extra = completion.length() - c->completionPrefix().length();
//    tc.movePosition(QTextCursor::Left);
//    tc.movePosition(QTextCursor::EndOfWord);
//    tc.insertText(completion.right(extra));
	tc.select(QTextCursor::WordUnderCursor);
	tc.insertText(completion);
    setTextCursor(tc);
}

void CompletingEdit::showCurrentCompletion(const QString& completion)
{
    if (c->widget() != this)
        return;
    QTextCursor tc = textCursor();
    int extra = completion.length() - c->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
	tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, extra);
	tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, extra);
    setTextCursor(tc);
}

QString CompletingEdit::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
	return tc.selectedText();
}

void CompletingEdit::focusInEvent(QFocusEvent *e)
{
    if (c)
        c->setWidget(this);
    QTextEdit::focusInEvent(e);
}

void CompletingEdit::keyPressEvent(QKeyEvent *e)
{
    if (c && c->popup() != NULL && c->popup()->isVisible()) {
        // The following keys are forwarded by the completer to the widget
       switch (e->key()) {
       case Qt::Key_Enter:
       case Qt::Key_Return:
       case Qt::Key_Escape:
       case Qt::Key_Tab:
       case Qt::Key_Backtab:
            e->ignore(); 
            return; // let the completer do default behavior
       default:
           break;
       }
    }

    bool isShortcut = (e->key() == Qt::Key_Escape);
#if POPUP
    if (!c || !isShortcut) // don't process the shortcut when we have a completer
        QTextEdit::keyPressEvent(e);
#else
    if (!isShortcut) // don't process the shortcut when we have a completer
        QTextEdit::keyPressEvent(e);
#endif

    const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    if (!c || (ctrlOrShift && e->text().isEmpty()))
        return;

    static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); // end of word
    bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
    QString completionPrefix = textUnderCursor();

    if (!isShortcut && (hasModifier || e->text().isEmpty() || completionPrefix.length() < 3 
                      || eow.contains(e->text().right(1)))) {
        if (c->popup() != NULL)
			c->popup()->hide();
        return;
    }

    if (completionPrefix != c->completionPrefix()) {
        c->setCompletionPrefix(completionPrefix);
        if (c->popup() != NULL)
			c->popup()->setCurrentIndex(c->completionModel()->index(0, 0));
    }
#if !POPUP
	else if (isShortcut) {
		fprintf(stderr,"next\n");
	}
#endif
	QRect cr = cursorRect();
#if POPUP
	if (c->popup() != NULL) {
		cr.setWidth(c->popup()->sizeHintForColumn(0)
					+ c->popup()->verticalScrollBar()->sizeHint().width());
	}
#endif
    c->complete(cr);
}

void CompletingEdit::mousePressEvent(QMouseEvent *e)
{
	if (e->modifiers() == Qt::ControlModifier)
		e->accept();
	else
		QTextEdit::mousePressEvent(e);
}

void CompletingEdit::mouseReleaseEvent(QMouseEvent *e)
{
	if (e->modifiers() == Qt::ControlModifier) {
		e->accept();
		QTextCursor cursor = cursorForPosition(e->pos());
		emit syncClick(cursor.blockNumber() + 1);
	}
	else
		QTextEdit::mouseReleaseEvent(e);
}
