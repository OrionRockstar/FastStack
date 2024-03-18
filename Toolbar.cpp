#include "pch.h"
#include "Toolbar.h"
#include "ImageWindow.h"

Toolbar::Toolbar(QWidget* parent) : QWidget(parent) {
	this->setGeometry(0, parent->size().height() - dy, parent->size().width(), dx);

	m_apply = new QPushButton(this);
	m_apply->setAutoDefault(false);
	m_apply->setToolTip("Apply");
	m_apply->move(0, 0);
	m_apply->resize(dx, dy);
	m_apply->setText("A");

	m_preview = new QPushButton(this);
	m_preview->setAutoDefault(false);
	m_preview->setToolTip("Show Preview");
	m_preview->move(dx, 0);
	m_preview->resize(dx, dy);
	m_preview->setText("P");

	m_reset = new QPushButton(this);
	m_reset->setAutoDefault(false);
	m_reset->setToolTip("Reset");
	m_reset->move(dx * 2, 0);
	m_reset->resize(dx, dy);
	m_reset->setText("R");

	this->setStyleSheet("QToolTip {border : 0px solid dark gray; background: solid dark gray; color: white}");

	this->setAutoFillBackground(true);
	QPalette pal = QPalette();
	pal.setColor(QPalette::Window, Qt::lightGray);
	this->setPalette(pal);

	connect(m_apply, &QPushButton::pressed, this, &Toolbar::sendApply);
	connect(m_preview, &QPushButton::pressed, this, &Toolbar::sendPreview);
	connect(m_reset, &QPushButton::pressed, this, &Toolbar::sendReset);
}


void ProcessDialog::closeEvent(QCloseEvent* close) {

	for (auto sw : m_workspace->subWindowList()) {
		auto iwptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (iwptr->rtpExists()) {
			QString str = iwptr->rtp->windowTitle();
			if (str.contains(Name())) {
				switch (iwptr->source.Bitdepth()) {
				case 8: {
					iwptr->ShowRTP();
					break;
				}
				case 16: {
					reinterpret_cast<ImageWindow16*>(iwptr)->ShowRTP();
					break;
				}
				case -32: {
					reinterpret_cast<ImageWindow32*>(iwptr)->ShowRTP();
					break;
				}
				}
				str.remove(Name());
				iwptr->rtp->setWindowTitle(str);
			}
		}
	}

	onClose();
	close->accept();
}

void ProcessDialog::showPreview() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());


	switch (iwptr->source.Bitdepth()) {
	case 8: {
		iwptr->ShowRTP();
		break;
	}
	case 16: {
		reinterpret_cast<ImageWindow16*>(iwptr)->ShowRTP();
		break;
	}
	case -32: {
		reinterpret_cast<ImageWindow32*>(iwptr)->ShowRTP();
		break;
	}
	}

	iwptr->rtp->setWindowTitle("Real-Time Preview: " + Name());
}


void DoubleValidator::fixup(QString& input) const {

	float val = input.toFloat();
	if (val > top())
		val = top();
	else if (val < bottom())
		val = bottom();

	input = QString::number(val, 'f', decimals());
}


DoubleLineEdit::DoubleLineEdit(DoubleValidator* validator, QWidget* parent) : QLineEdit(parent) {
	this->setFocusPolicy(Qt::ClickFocus);
	this->setValidator(validator);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::addTrailingZeros);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::removeEndDecimal);
}

DoubleLineEdit::DoubleLineEdit(const QString& contents, DoubleValidator* validator, QWidget* parent) : QLineEdit(contents, parent) {
	this->setFocusPolicy(Qt::ClickFocus);
	this->setValidator(validator);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::addTrailingZeros);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::removeEndDecimal);
}


void DoubleLineEdit::removeEndDecimal() {
	auto validator = reinterpret_cast<const DoubleValidator*>(this->validator());

	QString str = this->text();


	if (str[str.length() - 1] == '.')
		str.removeLast();

	this->setText(str);
}

void DoubleLineEdit::addTrailingZeros() {
	auto validator = reinterpret_cast<const DoubleValidator*>(this->validator());

	this->setText(QString::number(this->text().toDouble(), 'f', validator->decimals()));
}
