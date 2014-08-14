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

/* Include Qt headers (requires qt4-dev-tools) */
#include <QApplication>
#include <QMessageBox>
#include <QPalette>

#include <freerds/fdsapi.h>

#include <winpr/crt.h>

#include "simple_greeter.h"

#define UI_TIMEOUT 30

SimpleGreeterWindow::SimpleGreeterWindow()
{
	setupUi(this);

#if 1
	// Set the background color.
	QPalette palette;
	palette.setColor(QPalette::Background, QColor(0, 133, 195));
	setAutoFillBackground(true);
	setPalette(palette);
#else
	// TODO: Setting background color from image does not work correctly.
	setAttribute(Qt::WA_PaintOnScreen, true);
	setStyleSheet("background-image: url(:/images/background.png)");
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
	setBackgroundRole(QPalette::Background);
#endif

	// Display the logo.
	QPixmap pix(":/images/logo.png");
	logoLabel->setPixmap(pix);
	logoLabel->setScaledContents(true);

	// Adjust contents of the credentials box.
	innerGrid->setContentsMargins(20, 20, 20, 20);

	palette.setColor(QPalette::Background, QColor(200, 200, 200));
	innerBox->setAutoFillBackground(true);
	innerBox->setPalette(palette);

	// Create a timer.
	mTimer = new QTimer;
	mTimer->setInterval(UI_TIMEOUT * 1000);
	mTimer->setSingleShot(true);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(timeout()));

	// Initialize widgets.
	const char *domain = getenv("FREERDS_DOMAIN");
	const char *username = getenv("FREERDS_USER");
	if (domain && username)
	{
		QString text = QString("%1\\%2").arg(domain, username);
		usernameEdit->setText(text);
	}
	else if (username)
	{
		usernameEdit->setText(username);
	}

	// Update the buttons.
	updateButtons();
}

SimpleGreeterWindow::~SimpleGreeterWindow()
{
}

void SimpleGreeterWindow::updateButtons()
{
	QString username = usernameEdit->text();

	if (username.isEmpty())
	{
		// Disable the login button.
		loginButton->setEnabled(false);
	}
	else
	{
		// Enable the login button.
		loginButton->setEnabled(true);
	}

	// Restart the timer.
	mTimer->start();
}

void SimpleGreeterWindow::on_usernameEdit_textChanged(const QString& text)
{
	updateButtons();
}

void SimpleGreeterWindow::on_usernameEdit_returnPressed()
{
	on_loginButton_clicked(true);
}

void SimpleGreeterWindow::on_passwordEdit_textChanged(const QString& text)
{
	updateButtons();
}

void SimpleGreeterWindow::on_passwordEdit_returnPressed()
{
	on_loginButton_clicked(true);
}

void SimpleGreeterWindow::on_loginButton_clicked(bool checked)
{
	QString username = usernameEdit->text();
	QString password = passwordEdit->text();
	QString domain;

	int index = username.indexOf('\\');
	if (index >= 0)
	{
		domain = username.left(index);
		username = username.mid(index + 1);
	}

	int status = FreeRDS_AuthenticateUser(
		WTS_CURRENT_SESSION,
		username.toLatin1().constData(),
		password.toLatin1().constData(),
		domain.toLatin1().constData());
	qDebug("FreeRDS_AuthenticateUser returned %d", status);
	if (status != 0)
	{
		QMessageBox::critical(NULL, "FreeRDS", "Login failure: bad username or password");
	}
}

void SimpleGreeterWindow::on_cancelButton_clicked(bool checked)
{
	this->close();
}

void SimpleGreeterWindow::timeout()
{
	this->close();
}

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	SimpleGreeterWindow window;
	window.showMaximized();
	return app.exec();
}
