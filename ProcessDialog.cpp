#include "pch.h"
#include "ProcessDialog.h"
#include "FastStack.h"
#include "ImageWindow.h"
//#include "Interpolator.h"
#include "HistogramTransformation.h"



DialogToolbar::DialogToolbar(QWidget* parent, bool preview, bool apply_dd, bool apply) : m_preview(preview), m_apply_dd(apply_dd), m_apply(apply), QWidget(parent) {

	//this->setGeometry(0, parent->size().height() - m_button_size.height(), parent->size().width(), m_button_size.width());
	
	addApplyDDButton();
	addApplyButton();
	addPreviewButton();

	m_reset_pb = new PushButton("R", this);
	m_reset_pb->setAutoDefault(false);
	m_reset_pb->setToolTip("Reset");
	m_reset_pb->move(size().width() - m_button_size.width(), 0);
	m_reset_pb->resize(m_button_size);

	
	this->setAutoFillBackground(true);
	QPalette pal = QPalette();
	pal.setColor(QPalette::Window, Qt::lightGray);
	this->setPalette(pal);

	this->move(0, parentWidget()->height() - m_toolbarHeight);
}

void DialogToolbar::resize(int width) { QWidget::resize(width, m_toolbarHeight); }

void DialogToolbar::setGeometry(int x, int y, int w) { QWidget::setGeometry(x, y, w, m_toolbarHeight); }

void DialogToolbar::addApplyDDButton() {

	m_apply_dd_pb = new PushButton("A2", this);
	m_apply_dd_pb->setAutoDefault(false);
	m_apply_dd_pb->setToolTip("Apply to... (Drag and Drop)");
	m_apply_dd_pb->resize(m_button_size);
	m_apply_dd_pb->setVisible(m_apply_dd);
	m_apply_dd_pb->setEnabled(m_apply_dd);
	connect(m_apply_dd_pb, &QPushButton::pressed, [this]() { m_apply_dd_pb->setDown(false); });

}

void DialogToolbar::addApplyButton() {

	m_apply_pb = new PushButton("A", this);
	m_apply_pb->setAutoDefault(false);

	if (m_apply_dd) {
		m_apply_pb->setToolTip("Apply to current window");
		m_apply_pb->move(m_apply_dd_pb->width(), 0);
	}
	else 
		m_apply_pb->setToolTip("Apply process");	
	
	m_apply_pb->resize(m_button_size);
	m_apply_pb->setVisible(m_apply);
	m_apply_pb->setEnabled(m_apply);
}

void DialogToolbar::addPreviewButton() {

	m_preview_pb = new PushButton("P", this);
	m_preview_pb->setAutoDefault(false);
	m_preview_pb->setToolTip("Show Preview");

	int sx = 0;
	if (m_apply_dd)
		sx += m_apply_dd_pb->width();
	if (m_apply)
		sx += m_apply_pb->width();

	m_preview_pb->move(sx, 0);
	m_preview_pb->resize(m_button_size);
	
	m_preview_pb->setVisible(m_preview);
	m_preview_pb->setEnabled(m_preview);
}

void DialogToolbar::resizeEvent(QResizeEvent* e) {

	m_reset_pb->move(width() - m_button_size.width(), 0);

	if (m_apply && m_apply_dd)
		m_apply_pb->move(m_apply_dd_pb->width(), 0);

	if (m_preview_pb) {
		int sx = 0;
		if (m_apply_dd)
			sx += m_apply_dd_pb->width();
		if (m_apply)
			sx += m_apply_pb->width();

		m_preview_pb->move(sx, 0);
	}
}







ProgressDialog::ProgressDialog(QWidget* parent) : QProgressDialog(parent) {

	this->setRange(0, 100);
	this->setModal(true);
	this->setCancelButton(nullptr);

	this->setMinimumDuration(0);
	this->show();
}

ProgressDialog::ProgressDialog(const ProgressSignal& signal_object, QWidget* parent) : QProgressDialog(parent) {

	connect(&signal_object, &ProgressSignal::emitProgress, this, &QProgressDialog::setValue);
	connect(&signal_object, &ProgressSignal::emitText, this, &QProgressDialog::setLabelText);

	this->setRange(0, 100);
	this->setModal(true);
	this->setCancelButton(nullptr);

	this->setMinimumDuration(0);
	this->show();
}



TextDisplay::TextDisplay(const QString& title, QWidget* parent) : QDialog(parent) {

	this->setMinimumSize(525, 300);
	this->setWindowTitle(title);

	m_pte = new QPlainTextEdit(this);
	m_pte->resize(size());
	m_pte->ensureCursorVisible();
	m_pte->setReadOnly(true);
	QFont f("Monospace");
	f.setStyleHint(QFont::Monospace);
	m_pte->setFont(f);

	QPalette p;
	p.setColor(QPalette::ColorRole::Base, QColor(69, 69, 69));
	p.setColor(QPalette::ColorRole::Text, Qt::white);
	this->setPalette(p);

	this->setAttribute(Qt::WA_DeleteOnClose);
	this->show();

}

void TextDisplay::displayMatrix(const Matrix& m) {

	m_pte->insertPlainText("Transformation Matrix:\n");
	for (int j = 0; j < m.rows(); ++j) {
		for (int i = 0; i < m.cols(); ++i) {
			m_pte->insertPlainText(QString::number(m(j, i), 'f', 6));
			m_pte->insertPlainText((i < m.cols() - 1) ? " " : "\n");
		}
		m_pte->ensureCursorVisible();
	}
	m_pte->insertPlainText("\n\n");
}

void TextDisplay::displayText(const QString& txt) {
	m_pte->insertPlainText(txt + "\n");
	m_pte->ensureCursorVisible();
}

void TextDisplay::displayProgress(int progress) {

	QTextCursor cursor(m_pte->textCursor());

	cursor.movePosition(QTextCursor::StartOfLine);
	cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
	cursor.select(QTextCursor::LineUnderCursor);
	cursor.removeSelectedText();
	cursor.deleteChar();

	m_pte->setTextCursor(cursor);

	m_pte->insertPlainText("Progress: " + QString::number(progress) + "%");

	if (progress == 100)
		m_pte->insertPlainText("\n\n");

	m_pte->ensureCursorVisible();
}

void TextDisplay::setTextLine(const QString& txt) {
	m_pte->moveCursor(QTextCursor::StartOfLine);
	m_pte->insertPlainText(txt);
	m_pte->ensureCursorVisible();
}

void TextDisplay::displayPSFData(uint16_t size, const PSF& psf) {

	m_pte->insertPlainText("Stars Detected:   " + QString::number(size) + "\n");

	m_pte->insertPlainText("B:        " + QString::number(psf.B, 'f', 6) + "\n");
	m_pte->insertPlainText("A:        " + QString::number(psf.A, 'f', 6) + "\n");
	m_pte->insertPlainText("Std X:    " + QString::number(psf.sx, 'f', 6) + "\n");
	m_pte->insertPlainText("Std Y:    " + QString::number(psf.sy, 'f', 6) + "\n");
	m_pte->insertPlainText("FWHM X:   " + QString::number(psf.fwhmx, 'f', 6) + "\n");
	m_pte->insertPlainText("FWHM Y:   " + QString::number(psf.fwhmy, 'f', 6) + "\n");
	m_pte->insertPlainText("r:        " + QString::number(psf.roundness, 'f', 6) + "\n\n");
	m_pte->ensureCursorVisible();
}








ProcessDialog::ProcessDialog(const QString& name, const QSize& size, QMdiArea* parent_workspace, bool preview, bool apply_dd, bool apply) : m_name(name), Dialog(parent_workspace) {

	m_workspace = parent_workspace;

	this->resize(size.width() + 2 * m_border_width, size.height() + DialogTitleBar::titleBarHeight() + DialogToolbar::toolbarHeight() + m_border_width);
	this->setTitle(name);

	drawArea()->setGeometry(QRect({ m_border_width, DialogTitleBar::titleBarHeight() }, size));

	m_toolbar = new DialogToolbar(this, preview, apply_dd, apply);
	m_toolbar->setGeometry(m_border_width, drawArea()->geometry().bottom(), size.width());
	m_timer = std::make_unique<Timer>(100);

	this->setWindowOpacity(m_default_opacity);

	//this->setWindowFlags(Qt::WindowStaysOnTopHint);
	//this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
	this->setAttribute(Qt::WA_DeleteOnClose);
}

void ProcessDialog::enableSiblings(bool enable) {
	for (auto child : parentWidget()->children())
		if (child->inherits("ProcessDialog"))
			((QWidget*)child)->setEnabled(enable);
	QApplication::processEvents();
}

void ProcessDialog::resizeDialog(const QSize& size) {

	auto s = size + QSize(2 * m_border_width, DialogTitleBar::titleBarHeight() + DialogToolbar::toolbarHeight() + m_border_width);
	QDialog::resize(s);
	drawArea()->setGeometry(QRect({ m_border_width, DialogTitleBar::titleBarHeight() }, size));
	m_toolbar->setGeometry(m_border_width, drawArea()->geometry().bottom(), size.width());
}

bool ProcessDialog::isPreviewValid()const {

	if (m_workspace->subWindowList().size() == 0)
		return false;

	if (m_preview == nullptr)
		return false;

	return true;
}

void ProcessDialog::createDragInstance() {

	QDrag* drag = new QDrag(this);
	QMimeData* mimeData = new QMimeData;
	QByteArray pointer(reinterpret_cast<char*>(this), 8);
	mimeData->setData("process", pointer);
	drag->setMimeData(mimeData);

	Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
}

void ProcessDialog::showPreview(bool ignore_zoomwindow) {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = imageRecast<>(m_workspace->currentSubWindow()->widget());

	for (auto sw : m_workspace->subWindowList()) {
		auto iw = imageRecast<>(sw->widget());
		if (iw->previewExists())
			if (iw->preview()->processType() == name() || iw != iwptr)
				return;
	}

	if (iwptr->previewExists()) {
		auto type = previewRecast<>(iwptr->preview())->processType();
		if (type == m_crop_image_str || type == m_abe_str)
			return;
	}


	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		if (ignore_zoomwindow)
			iwptr->showPreview(new PreviewWindow8(iwptr, true));
		else
			iwptr->showPreview();
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		if (ignore_zoomwindow)
			iw16->showPreview(new PreviewWindow16(iw16, true));
		else
			iw16->showPreview();
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		if (ignore_zoomwindow)
			iw32->showPreview(new PreviewWindow32(iw32, true));
		else
			iw32->showPreview();
		break;
	}
	}

	connect(iwptr->preview()->windowSignals(), &WindowSignals::windowClosed, this, [this]() { m_preview = nullptr; });

	//transfers preview to current process
	for (auto child : m_workspace->children()) {
		auto ptr = dynamic_cast<ProcessDialog*>(child);
		if (ptr != nullptr) {
			if (ptr->m_preview == iwptr->preview()) {
				ptr->m_preview = nullptr;
				break;
			}
		}
	}
	m_preview = iwptr->preview();

	iwptr->preview()->setTitle(iwptr->name() + " Preview: " + name());
	iwptr->preview()->setProcessType(name());

	iwptr->connectPreview(m_obj, m_func);
	if (!ignore_zoomwindow)
		connectZoomWindow();
}

void ProcessDialog::closeEvent(QCloseEvent* close) {

	m_preview = nullptr;

	for (auto sw : m_workspace->subWindowList()) {

		auto iwptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (iwptr->previewExists()) {
			QString str = iwptr->preview()->title();
			if (str.contains(name())) {
				if (name() == m_abe_str) {
					iwptr->closePreview();
					break;
				}
				str.remove(name());
				PreviewWindow8* pptr = previewRecast(iwptr->preview());
				pptr->setTitle(str);
				pptr->setProcessType();
				switch (pptr->type()) {
				case ImageType::UBYTE:
					pptr->updatePreview();
					break;
				case ImageType::USHORT:
					previewRecast<uint16_t>(pptr)->updatePreview();
					break;
				case ImageType::FLOAT:
					previewRecast<float>(pptr)->updatePreview();
					break;
				}
			}
		}
	}

	emit windowClosed();
	close->accept();
}

void ProcessDialog::czw()const {
	auto ptr = imageRecast<>(m_workspace->currentSubWindow()->widget());
	ptr->zoomWindow()->connectZoomWindow(m_obj, m_func);
}

void ProcessDialog::connectZoomWindow() {

	auto ws = dynamic_cast<Workspace*>(m_workspace);

	auto ptr = imageRecast<>(m_workspace->currentSubWindow()->widget());
	if (ptr->zoomWindow())
		ptr->zoomWindow()->connectZoomWindow(m_obj, m_func);

	connect(ws, &Workspace::zoomWindowCreated, this, &ProcessDialog::czw);
	connect(ws, &Workspace::zoomWindowClosed, m_obj, m_func);
}

const Image8* ProcessDialog::findImage(const QString& name) {

	for (auto sw : m_workspace->subWindowList()) {
		auto iw = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (name == iw->name())
			return &iw->source();
	}
	return nullptr;
}