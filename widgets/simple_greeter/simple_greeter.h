/**
 * FreeRDS: Simple Greeter (Logon UI)
 *
 * Copyright 2014 Dell Software <Mike.McDonald@software.dell.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef __FREERDS_SIMPLE_GREETER_H__
#define __FREERDS_SIMPLE_GREETER_H__

#include <QTimer>

#include "ui_simple_greeter.h"

class SimpleGreeterWindow : public QMainWindow, private Ui_SimpleGreeterWindow
{
	Q_OBJECT
public:
	SimpleGreeterWindow();
	virtual ~SimpleGreeterWindow();

public slots:
	void updateButtons();

	// UI Signals
	void on_usernameEdit_textChanged(const QString& text);
	void on_usernameEdit_returnPressed();
	void on_passwordEdit_textChanged(const QString& text);
	void on_passwordEdit_returnPressed();

	void on_loginButton_clicked(bool checked);
	void on_cancelButton_clicked(bool checked);

	void timeout();

protected:
	QTimer *mTimer;
};

#endif //__FREERDS_SIMPLE_GREETER_H__
