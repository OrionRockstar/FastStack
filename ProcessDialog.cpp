#include "pch.h"
#include "ProcessDialog.h"
#include "FastStack.h"
#include "ImageWindow.h"
#include "Interpolator.h"



Toolbar::Toolbar(QWidget* parent, bool preview, bool apply_dd, bool apply) : m_preview(preview), m_apply_dd(apply_dd), m_apply(apply), QWidget(parent) {

	this->setGeometry(0, parent->size().height() - m_button_size.height(), parent->size().width(), m_button_size.width());
	
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
}

void Toolbar::addApplyDDButton() {

	m_apply_dd_pb = new PushButton("A2", this);
	m_apply_dd_pb->setAutoDefault(false);
	m_apply_dd_pb->setToolTip("Apply to... (Drag and Drop)");
	m_apply_dd_pb->resize(m_button_size);
	m_apply_dd_pb->setVisible(m_apply_dd);
	m_apply_dd_pb->setEnabled(m_apply_dd);
	connect(m_apply_dd_pb, &QPushButton::pressed, [this]() { m_apply_dd_pb->setDown(false); });

}

void Toolbar::addApplyButton() {

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

void Toolbar::addPreviewButton() {

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



ProcessDialog::ProcessDialog(QString name, const QSize& size, QMdiArea* parent_workspace, bool preview, bool apply_dd, bool apply) : QDialog(parent_workspace), m_name(name) {

	m_workspace = parent_workspace;

	QIcon icon;
	icon.addFile("./Icons//fast_stack_icon_fs2.png");
	this->setWindowIcon(icon);
	this->setWindowTitle(m_name);
	

	this->setWindowOpacity(0.95);
	installEventFilter(this);

	this->resize(size);
	m_toolbar = new Toolbar(this, preview, apply_dd, apply);
	m_timer = new Timer;

	//this->set
	this->setFocus();
	this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	this->setAttribute(Qt::WA_DeleteOnClose);
}

void ProcessDialog::startTimer() {
	m_timer->start();
}

void ProcessDialog::CreateDragInstance() {
	QDrag* drag = new QDrag(this);
	QMimeData* mimeData = new QMimeData;
	QByteArray pointer(reinterpret_cast<char*>(this), 8);
	mimeData->setData("process", pointer);
	drag->setMimeData(mimeData);

	Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
}

void ProcessDialog::closeEvent(QCloseEvent* close) {

	m_preview = nullptr;
	
	for (auto sw : m_workspace->subWindowList()) {

		auto iwptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (iwptr->previewExists()) {
			QString str = iwptr->preview()->windowTitle();
			if (str.contains(name())) {
				str.remove(name());
				PreviewWindow8* pptr = previewRecast(iwptr->preview());
				pptr->setWindowTitle(str);
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

	onClose();
	close->accept();
}

void ProcessDialog::showPreview() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	for (auto sw : m_workspace->subWindowList()) {
		auto iw = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (iw->previewExists()) {
			auto pw = reinterpret_cast<PreviewWindow8*>(iw->preview());
			if (pw->processType() == name())
				return;
		}
	}

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->showPreview();
		auto iws = reinterpret_cast<PreviewWindow8*>(iwptr->preview())->windowSignals();
		connect(iws, &WindowSignals::windowClosed, [this]() { m_preview = nullptr; });
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->showPreview();
		auto iws = reinterpret_cast<PreviewWindow16*>(iw16->preview())->windowSignals();
		connect(iws, &WindowSignals::windowClosed, [this]() { m_preview = nullptr; });
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->showPreview();
		auto iws = reinterpret_cast<PreviewWindow32*>(iw32->preview())->windowSignals();
		connect(iws, &WindowSignals::windowClosed, [this]() { m_preview = nullptr; });
		break;
	}
	}

	m_preview = iwptr->preview();
	//connect(m_preview->window)
	iwptr->preview()->setWindowTitle(iwptr->name() + " Preview: " + name());
	reinterpret_cast<PreviewWindow8*>(iwptr->preview())->setProcessType(name());
}

bool ProcessDialog::isPreviewValid()const {

	if (m_workspace->subWindowList().size() == 0)
		return false;

	if (m_preview == nullptr)
		return false;

	return true;
}

void ProcessDialog::updateImageLabel(const QMdiSubWindow* window) {
	reinterpret_cast<FastStack*>(m_workspace->parentWidget())->toolbar()->imageInformationLabel()->displayText(window);
}




void DoubleValidator::fixup(QString& input) const {

	double val = input.toDouble();
	if (val > top())
		val = top();
	else if (val < bottom())
		val = bottom();

	input = QString::number(val, 'd', decimals());
}

void IntValidator::fixup(QString& input) const {

	int val = input.toInt();

	if (val > top())
		val = top();
	else if (val < bottom())
		val = bottom();

	input = QString::number(val);
}

int IntValidator::validate(int value)const {

	if (value > top())
		value = top();
	else if (value < bottom())
		value = bottom();

	return value;
}


void LineEdit::setLabelText(const QString& txt) {

	if (m_label == nullptr)
		return this->addLabel(new QLabel(txt, parentWidget()));

	m_label->setText(txt);

	int x = geometry().x();
	int y = geometry().y();
	int h = geometry().height();
	int fh = m_label->fontMetrics().height();
	int tw = m_label->fontMetrics().horizontalAdvance(m_label->text());

	m_label->move(x - tw, ((2 * y) + h - fh) / 2);
}

void LineEdit::addLabel(QLabel* label) {

	m_label = label;

	int x = geometry().x();
	int y = geometry().y();
	int h = geometry().height();
	int fh = label->fontMetrics().height();
	int tw = label->fontMetrics().horizontalAdvance(label->text());

	label->move(x - tw, ((2 * y) + h - fh) / 2);
}

void LineEdit::addSlider(QSlider* slider) {
	int x = geometry().topRight().x();
	int y = geometry().topRight().y();
	int h = geometry().height();
	int sh = slider->style()->pixelMetric(QStyle::PM_SliderThickness);

	slider->move(x + 15, ((2 * y) + h - sh) / 2);
}


IntLineEdit::IntLineEdit(int default_value, IntValidator* validator, QWidget* parent) : m_default(default_value), LineEdit(parent) {
	this->setValidator(validator);
	validator->setParent(this);
	this->setValue(default_value);
	this->resize(85, 30);
}

void IntLineEdit::setValue(int value) {

	value = intValidator()->validate(value);
	this->setText(QString::number(value));
}

void IntLineEdit::reset() {
	setValue(m_default);
}

//have line edit adjust width based on precision?

DoubleLineEdit::DoubleLineEdit(DoubleValidator* validator, QWidget* parent) : LineEdit(parent) {
	this->setFocusPolicy(Qt::ClickFocus);
	this->setValidator(validator);
	validator->setParent(this);
	this->resize(85, 30);

	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::addTrailingZeros);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::removeEndDecimal);
}

DoubleLineEdit::DoubleLineEdit(double default_value, DoubleValidator* validator, QWidget* parent) : m_default(default_value), LineEdit(parent) {
	this->setFocusPolicy(Qt::ClickFocus);
	this->setValidator(validator);
	validator->setParent(this);
	this->setValue(default_value);
	this->resize(85, 30);

	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::addTrailingZeros);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::removeEndDecimal);
}

DoubleLineEdit::DoubleLineEdit(double default_value, DoubleValidator* validator, int max_length, QWidget* parent) : m_default(default_value), LineEdit(parent){

	this->setFocusPolicy(Qt::ClickFocus);
	this->setValidator(validator);
	validator->setParent(this);
	this->setMaxLength(max_length);
	this->setValue(default_value);
	this->resize(85, 30);
	
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::addTrailingZeros);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::removeEndDecimal);
}

void DoubleLineEdit::setValue(float value) {
	QString str = QString::number(value, 'f', maxLength());

	this->doubleValidator()->fixup(str);

	if (str.length() > maxLength())
		str = str.sliced(0, maxLength());


	if (str[str.length() - 1] == '.')
		str.removeLast();

	this->setText(str);
}

void DoubleLineEdit::setValue(double value) {

	QString str = QString::number(value, 'f', 6);
	this->doubleValidator()->fixup(str);

	if (str.length() > maxLength())
		str = str.sliced(0, maxLength());


	if (str[str.length() - 1] == '.')
		str.removeLast();

	this->setText(str);
}

void DoubleLineEdit::removeEndDecimal() {
	QString str = this->text();

	if (str[str.length() - 1] == '.')
		str.removeLast();

	this->setText(str);
}

void DoubleLineEdit::addTrailingZeros() {
	auto validator = reinterpret_cast<const DoubleValidator*>(this->validator());

	this->setText(QString::number(this->text().toDouble(), 'f', validator->decimals()));
}


void ScrollBar::paintEvent(QPaintEvent* event) {

	QPainter p(this);
	QStyleOptionSlider opt;
	QPalette pal;

	initStyleOption(&opt);
	opt.subControls = QStyle::SC_ScrollBarSlider | QStyle::SC_ScrollBarAddLine | QStyle::SC_ScrollBarSubLine;

	auto groove = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarGroove);
	p.fillRect(groove, QColor(69, 69, 69));

	pal.setColor(QPalette::ColorRole::Button, QColor(169, 169, 169));
	pal.setColor(QPalette::ColorRole::WindowText, QColor(0, 0, 0));
	opt.palette = pal;

	style()->drawComplexControl(QStyle::CC_ScrollBar, &opt, &p, this);
}

void Slider::paintEvent(QPaintEvent* event) {
	QPainter p(this);
	QStyleOptionSlider opt;
	initStyleOption(&opt);
	opt.subControls = QStyle::SC_SliderGroove;

	QPalette pal;
	if (opt.state & QStyle::State_Enabled)
		pal.setBrush(QPalette::Highlight, QColor(123, 0, 216));
	else
		pal.setBrush(QPalette::Highlight, QColor(120, 120, 120));

	if (!(opt.state & QStyle::State_Active))
		pal.setBrush(QPalette::Highlight, QColor(39, 39, 39));

	opt.palette = pal;
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);


	opt.subControls = QStyle::SC_SliderHandle;
	pal.setBrush(QPalette::Button, QColor(129, 129, 129));

	opt.palette = pal;
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);
}


void RadioButton::paintEvent(QPaintEvent* event) {
	QPainter p(this);
	QStyleOptionButton opt;
	initStyleOption(&opt);
	//p.setBrush(QColor(0, 255, 0));
	QPalette pal;
	pal.setBrush(QPalette::ColorRole::Text, m_indicator_color);
	pal.setBrush(QPalette::ColorRole::Base, m_base_color);
	//pal.setColor(QPalette::ColorRole::WindowText, m_text_color);

	opt.palette = pal;
	style()->drawControl(QStyle::CE_RadioButton, &opt, &p, this);

	//style()->drawItemText(&p, rect(), 0, pal, false, "SSS");
}


ComboBox::ComboBox(QWidget* parent) : QComboBox(parent) {
	QPalette pal;
	pal.setColor(QPalette::Window, QColor(169, 169, 169));
	pal.setColor(QPalette::ButtonText, Qt::black);
	this->setPalette(pal);
	this->view()->setPalette(pal);
}

void ComboBox::addLabel(QLabel* label)const {

	int x = geometry().x();
	int y = geometry().y();
	int h = geometry().height();
	int fh = label->fontMetrics().height();
	int tw = label->fontMetrics().horizontalAdvance(label->text());

	label->move(x - tw, ((2 * y) + h - fh) / 2);
}


InterpolationComboBox::InterpolationComboBox(QWidget* parent) : ComboBox(parent) {

	addItems({ "Nearest Neighbor", "Bilinear", "Bicubic Spline", "Bicubic B-Spline",
								"Cubic B-Spline", "Catmull-Rom", "Lanczos-3" });

	setCurrentIndex(m_defualt_index);
}



ComponentPushButton::ComponentPushButton(const QString& text, QWidget* parent) : QPushButton(text, parent) {

	this->setAutoFillBackground(true);
	this->setBackgroundRole(QPalette::ColorRole::Dark);
	this->setFlat(true);

	this->setAutoDefault(false);
	this->setCheckable(true);
	this->setAutoExclusive(true);

	QFont font = this->font();
	font.setPointSize(8);
	this->setFont(font);
	//this->resize(this->fontMetrics().horizontalAdvance(text) + 10, this->size().height());
}

ComponentPushButton::ComponentPushButton(const QIcon& icon, const QString& text, QWidget* parent) : QPushButton(icon, text, parent) {

	this->setAutoFillBackground(true);
	this->setBackgroundRole(QPalette::ColorRole::Dark);
	this->setFlat(true);

	this->setAutoDefault(false);
	this->setCheckable(true);
	this->setAutoExclusive(true);

	QFont font = this->font();
	font.setPointSize(8);
	this->setFont(font);
}

void ComponentPushButton::paintEvent(QPaintEvent* event ) {

	QPushButton::paintEvent(event);

	QStyleOptionButton opt;
	initStyleOption(&opt);
	QPalette pal;

	if (opt.state & QStyle::State_Off) {
		pal.setBrush(QPalette::Button, palette().brush(QPalette::Button));
		pal.setBrush(QPalette::ButtonText, Qt::black);
	}

	if (opt.state & QStyle::State_On) {
		pal.setBrush(QPalette::Button, QColor(69, 69, 69));
		pal.setBrush(QPalette::ButtonText, Qt::white);
		pal.setBrush(QPalette::Dark, Qt::black);
	}

	this->setPalette(pal);
}


CheckablePushButton::CheckablePushButton(const QString& text, QWidget* parent) : QPushButton(text, parent) {

	this->setCheckable(true);
	this->setAutoDefault(false);
	connect(this, &QPushButton::clicked, this, &QPushButton::setChecked);
}

void CheckablePushButton::paintEvent(QPaintEvent* event) {

	QPushButton::paintEvent(event);

	QStyleOptionButton opt;
	initStyleOption(&opt);
	QPalette pal;

	if (opt.state & QStyle::State_Off) {
		pal.setBrush(QPalette::Button, palette().brush(QPalette::Button));
		pal.setBrush(QPalette::ButtonText, Qt::black);
	}

	if (opt.state & QStyle::State_On) {
		pal.setBrush(QPalette::Button, QColor(69, 69, 69));
		pal.setBrush(QPalette::ButtonText, Qt::white);
	}

	this->setPalette(pal);
}

SpinBox::SpinBox(QWidget* parent) : QSpinBox(parent) {
	QPalette pal;
	pal.setBrush(QPalette::WindowText, Qt::black);
	this->setPalette(pal);
}

SpinBox::SpinBox(int value, int min, int max, QWidget* parent) : SpinBox(parent) {

	this->setRange(min, max);
	this->setValue(value);
}

void SpinBox::addLabel(QLabel* label)const {

	int x = geometry().x();
	int y = geometry().y();
	int h = geometry().height();
	int fh = label->fontMetrics().height();
	int tw = label->fontMetrics().horizontalAdvance(label->text());

	label->move(x - tw, ((2 * y) + h - fh) / 2);
}

DoubleSpinBox::DoubleSpinBox(QWidget* parent) : QDoubleSpinBox(parent) {

	QPalette pal;
	pal.setBrush(QPalette::WindowText, Qt::black);
	this->setPalette(pal);
}

DoubleSpinBox::DoubleSpinBox(double value, double min, double max, int precision, QWidget* parent) : DoubleSpinBox(parent) {

	this->setRange(min, max);
	this->setValue(value);
	this->setDecimals(precision);
}

void DoubleSpinBox::addLabel(QLabel* label)const {

	int x = geometry().x();
	int y = geometry().y();
	int h = geometry().height();
	int fh = label->fontMetrics().height();
	int tw = label->fontMetrics().horizontalAdvance(label->text());

	label->move(x - tw, ((2 * y) + h - fh) / 2);
}


void GroupBox::paintEvent(QPaintEvent* event) {
	QStylePainter p(this);
	QStyleOptionGroupBox opt;
	initStyleOption(&opt);

	int text_width = QFontMetrics(font()).horizontalAdvance(title());
	int text_height = QFontMetrics(font()).height();
	int text_margin = QFontMetrics(font()).xHeight();
	int text_beginX = (opt.rect.width() / 2) - (text_width / 2);
	int text_endX = text_beginX + text_width;

	QPen pen;
	pen.setColor(QColor(95, 45, 145));
	pen.setWidthF(1.75);
	p.setRenderHint(QPainter::Antialiasing);
	p.setPen(pen);

	int radius = 5;

	int y_offset = (text_height / 2);
	const QRect draw_rect = { opt.rect.x() + 1, opt.rect.y() + 1 + y_offset, opt.rect.width() - 2, opt.rect.height() - 2 - y_offset};
	const QRect radius_rect = draw_rect.adjusted(radius, radius, -radius, -radius);

	if (title() != "") {
		QList<QPointF> points;
		double dtheta = (2 * std::_Pi * 15) / 360;

		QPoint point = draw_rect.topLeft();
		p.drawLine(QPoint(point.x() + radius + 1, point.y()), { text_beginX - text_margin, point.y() });

		point = draw_rect.topRight();
		p.drawLine(QPoint(point.x() - radius - 1, point.y()), { text_endX + text_margin, point.y() });

		point = radius_rect.topLeft();
		for (double theta = std::_Pi / 2; theta <= std::_Pi; theta += dtheta)
			points.push_back(QPointF(point.x() + radius * cos(theta), point.y() - radius * sin(theta)));

		point = radius_rect.bottomLeft();
		for (double theta = std::_Pi; theta <= 3 * std::_Pi / 2; theta += dtheta)
			points.push_back(QPointF(point.x() + radius * cos(theta), point.y() - radius * sin(theta)));

		point = radius_rect.bottomRight();
		for (double theta = 3 * std::_Pi / 2; theta <= 2 * std::_Pi; theta += dtheta)
			points.push_back(QPointF(point.x() + radius * cos(theta), point.y() - radius * sin(theta)));

		point = radius_rect.topRight();
		for (double theta = 0; theta <= std::_Pi / 2; theta += dtheta)
			points.push_back(QPointF(point.x() + radius * cos(theta), point.y() - radius * sin(theta)));

		p.drawPolyline(QPolygonF(points));
	}
	else
		p.drawRoundedRect(draw_rect, radius, radius);

	p.setPen(Qt::white);
	p.drawText((opt.rect.width() / 2) - (text_width / 2), opt.rect.y() + text_height / 2 + text_margin, title());
}


ListWidget::ListWidget(QWidget* parent) : QListWidget(parent) {

	QPalette p;
	p.setBrush(QPalette::ColorRole::AlternateBase, QColor(169, 69, 255));
	this->setPalette(p);

	this->setVerticalScrollBar(new ScrollBar(this));
	this->setHorizontalScrollBar(new ScrollBar(this));

	this->setAlternatingRowColors(true);
}


FileSelection::FileSelection(QWidget* parent) : QWidget(parent) {

	this->setAcceptDrops(true);

	m_file_list_view = new QListWidget(this);
	m_file_list_view->move(10, 10);
	m_file_list_view->resize(365, m_file_list_view->sizeHint().height());


	QPalette p;
	p.setBrush(QPalette::ColorRole::AlternateBase, QColor(127, 127, 255));


	//p.setBrush(QPalette::ColorRole::Base, QColor(0, 255, 0));
	m_file_list_view->setPalette(p);
	m_file_list_view->setAlternatingRowColors(true);

	m_add_files_pb = new QPushButton("Add File(s)", this);
	m_add_files_pb->move(380, 10);
	m_add_files_pb->setFixedWidth(100);


	m_remove_file_pb = new QPushButton("Remove File", this);
	m_remove_file_pb->move(380, 45);
	m_remove_file_pb->setFixedWidth(100);

	m_clear_list_pb = new QPushButton("Clear List", this);
	m_clear_list_pb->move(380, 80);
	m_clear_list_pb->setFixedWidth(100);

	connect(m_add_files_pb, &QPushButton::pressed, this, &FileSelection::onAddFiles);
	connect(m_remove_file_pb, &QPushButton::pressed, this, &FileSelection::onRemoveFile);
	connect(m_clear_list_pb, &QPushButton::pressed, this, &FileSelection::onClearList);
}


void FileSelection::dragEnterEvent(QDragEnterEvent* event) {
	if (event->mimeData()->hasUrls())
		event->acceptProposedAction();
}

void FileSelection::dropEvent(QDropEvent* event) {
	for (auto url : event->mimeData()->urls()) {
		QString file = url.toLocalFile();
		if (!isValidFileType(file))
			continue;
		m_file_list_view->addItem(QFileInfo(file).fileName());
		m_paths.push_back(file.toStdString());
	}
}

bool FileSelection::isValidFileType(const QString& path) {
	auto ext = QFileInfo(path).suffix();
	return m_typelist.contains(ext);
}

void FileSelection::onAddFiles() {
	QStringList file_paths = QFileDialog::getOpenFileNames(this, tr("Open Files"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist);

	for (auto file : file_paths)
		m_file_list_view->addItem(QFileInfo(file).fileName());

	for (int i = 0; i < file_paths.size(); ++i)
		m_paths.push_back(file_paths[i].toStdString());
}

void FileSelection::onRemoveFile() {

	if (m_file_list_view->count() == 0)
		return;

	int index = m_file_list_view->currentIndex().row();

	if (index == -1)
		index += m_paths.size();

	m_file_list_view->takeItem(index);
	m_paths.erase(m_paths.begin() + index);
}

void FileSelection::onClearList() {

	m_file_list_view->clear();
	m_paths.clear();
}
