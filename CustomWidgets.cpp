#include "pch.h"
#include "CustomWidgets.h"
#include "Maths.h"
#include "ImageWindow.h"

DigitalClock::DigitalClock(QWidget* parent) : QLCDNumber(parent) {

	QTimer* timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &DigitalClock::displayTime);

	timer->start(500);
	displayTime();

	this->setAutoFillBackground(true);

	QPalette p(this->palette());
	p.setColor(QPalette::Button, Qt::black);
	p.setColor(QPalette::ButtonText, QColor(0,199,0));
	p.setColor(QPalette::Light, QColor(0, 99, 0));
	p.setColor(QPalette::Dark, QColor(0, 99, 0));

	this->setPalette(p);
}

void DigitalClock::displayTime() {

	auto time = QTime::currentTime().toString("hh:mm");

	this->display(time);
}


Frame::Frame(const QRect& rect, int frame_width) : m_frame_width(frame_width) {

	this->resize(rect);
}

bool Frame::isOnFrame(const QPoint& p)const {

	if (m_inner_region.contains(p))
		return false;

	if (m_left.contains(p))
		return true;
	else if (m_right.contains(p))
		return true;
	else if (m_top.contains(p))
		return true;
	else if (m_bottom.contains(p))
		return true;

	return false;
}

RectBorder Frame::rectBorder(const QPoint& p)const {

	bool l = m_left.contains(p);
	bool r = m_right.contains(p);
	bool t = m_top.contains(p);
	bool b = m_bottom.contains(p);

	if (l && t)
		return RectBorder::TopLeftCorner;

	if (l && b)
		return RectBorder::BottomLeftCorner;

	if (r && t)
		return RectBorder::TopRightCorner;

	if (r && b)
		return RectBorder::BottomRightCorner;

	if (t)
		return RectBorder::TopEdge;

	if (l)
		return RectBorder::LeftEdge;

	if (r)
		return RectBorder::RightEdge;

	if (b)
		return RectBorder::BottomEdge;

	return RectBorder::None;
}

void Frame::resize(const QRect& rect) {

	m_left = QRect(rect.topLeft(), QSize(frameWidth(), rect.height()));
	m_right = QRect(QPoint(rect.right() - frameWidth() + 1, rect.y()), QSize(frameWidth(), rect.height()));
	m_top = QRect(rect.topLeft(), QSize(rect.width(), frameWidth()));
	m_bottom = QRect(QPoint(rect.x(), rect.bottom() - frameWidth() + 1), QSize(rect.width(), frameWidth()));
	m_inner_region = QRect(QPoint(m_left.right(), m_top.bottom()), QPoint(m_right.left(), m_bottom.top()));
}

QCursor Frame::cursorAt(const QPoint& p, QCursor default_return_cursor) {

	if (m_inner_region.contains(p))
		return default_return_cursor;

	bool l = m_left.contains(p);
	bool r = m_right.contains(p);
	bool t = m_top.contains(p);
	bool b = m_bottom.contains(p);

	if ((l && t) || (r && b))
		return Qt::SizeFDiagCursor;

	else if ((l && b) || (r && t))
		return Qt::SizeBDiagCursor;

	else if (l || r)
		return Qt::SizeHorCursor;

	else if (t || b)
		return Qt::SizeVerCursor;

	else
		return default_return_cursor;
}





PushButton::PushButton(const QString& text, QWidget* parent) : QPushButton(text, parent) {

	this->setAutoDefault(false);
	this->setStyleSheet("QToolTip {border : 0px solid dark gray; background: solid dark gray; color: white}");
}

PushButton::PushButton(const QIcon& icon, const QString& text, QWidget* parent) : PushButton(text, parent) {

	this->setIcon(icon);
}

void PushButton::mousePressEvent(QMouseEvent* e) {

	setDown(true);
	emit pressed();
}

void PushButton::mouseMoveEvent(QMouseEvent* e) {

	if (hitButton(e->pos()))
		setDown(true);
	else
		setDown(false);
}

void PushButton::mouseReleaseEvent(QMouseEvent* e) {

	setDown(false);
	if (hitButton(e->pos())) {
		if (isCheckable())
			click();
		emit released();
	}
	e->accept();
}

void PushButton::paintEvent(QPaintEvent* e) {

	QPainter p(this);
	QStyleOptionButton opt;
	this->initStyleOption(&opt);
	p.setOpacity(m_opacity);

	if (hitButton(mapFromGlobal(QCursor::pos())) && isEnabled())
		opt.palette.setBrush(QPalette::Button, m_hover_color);

	//style()->drawPrimitive(QStyle::PE_PanelButtonCommand, &opt, &p);
	//style()->drawControl(QStyle::CE_PushButtonLabel, &opt, &p);
	style()->drawControl(QStyle::CE_PushButton, &opt, &p, this);
}



FlatPushButton::FlatPushButton(const QString& text, QWidget* parent) : PushButton(text, parent) {

	this->setFlat(true);
}

FlatPushButton::FlatPushButton(const QIcon& icon, const QString& text, QWidget* parent) : FlatPushButton(text, parent) {

	this->setIcon(icon);
}

void FlatPushButton::enterEvent(QEnterEvent* e) {

	this->setFlat(false);
}

void FlatPushButton::leaveEvent(QEvent* e) {

	this->setFlat(true);
}




DialogTitleBar::DialogTitleBar(QWidget* parent) : QWidget(parent) {

	this->setFixedHeight(m_titlebar_height);
	int w = (parent) ? parentWidget()->width() : 300;
	this->setGeometry(0, 0, w, m_titlebar_height);

	QPalette pal(palette());
	pal.setColor(QPalette::Window, QColor(69, 0, 128));
	pal.setColor(QPalette::Inactive,QPalette::Window, QColor(128, 128, 128));

	pal.setColor(QPalette::Button, QColor(39, 39, 39));
	pal.setColor(QPalette::Inactive, QPalette::Button, QColor(128, 128, 128));

	pal.setColor(QPalette::Accent, QColor(0, 0, 69));
	pal.setColor(QPalette::Inactive, QPalette::Accent, QColor(128, 128, 128));
	this->setPalette(pal);

	m_bg = new QButtonGroup(this);
	m_bg->setExclusive(true);

	m_bg->addButton(new PushButton(QIcon("./Icons//close-button-32.png"), "", this), 1);
	connect(m_bg->button(1), &QPushButton::released, parent, &QWidget::close);

	m_bg->addButton(new PushButton(m_shade_icon, "", this), 2);
	auto onShade = [this]() {
		if (m_shaded == false) {
			m_shaded = true;
			m_bg->button(2)->setIcon(m_unshade_icon);
			emit shadeWindow();
		}
		else {
			m_shaded = false;
			m_bg->button(2)->setIcon(m_shade_icon);
			emit unshadeWindow();
		}
	};
	connect(m_bg->button(2), &QPushButton::released, this, onShade);

	for (auto button : m_bg->buttons()) {
		button->setPalette(palette());
		button->resize(m_button_dim, m_button_dim);
		button->setIconSize({ 18, 18 });
		button->move(width() - 4 - (m_bg->id(button) * m_button_dim), 4);
		button->setCursor(Qt::CursorShape::ArrowCursor);
	}

	this->setMouseTracking(true);
	this->show();
}

void DialogTitleBar::resize(int width) {
	QWidget::resize(width, m_titlebar_height);

	for (auto button : m_bg->buttons())
		button->move(width - 4 - (m_bg->id(button) * m_button_dim), 4);
}

void DialogTitleBar::setOpacity(float opacity) {

	m_opacity = opacity;
	for (auto b : m_bg->buttons())
		reinterpret_cast<PushButton*>(b)->setOpacity(opacity);
	update();
}

void DialogTitleBar::mousePressEvent(QMouseEvent* e) {

	if (pm.rect().contains(e->pos())) {
		emit iconPressed(e->buttons());
		e->accept();
	}
	else
		e->ignore();
}

void DialogTitleBar::mouseMoveEvent(QMouseEvent* e) {
	e->ignore();
}

void DialogTitleBar::paintEvent(QPaintEvent* e) {

	QPainter p(this);
	p.setOpacity(m_opacity);

	QPen pen;
	pen.setWidth(2);
	pen.setColor(palette().color(palette().currentColorGroup(), QPalette::Accent));
	p.setPen(pen);
	auto r = this->rect();
	p.drawRect(r);
	r.adjust(1, 1, -1, -1);
	p.fillRect(r, palette().color(QPalette::Window));

	p.drawPixmap(3, 3, pm);

	pen.setColor(Qt::white);
	p.setPen(pen);
	p.drawText(QRect(m_label_offset, 3, m_bg->button(2)->x() - m_label_offset - 5, 20), QTextOption::WordWrap,title());
}





Dialog::Dialog(QWidget* parent, bool resizable) : m_resizable(resizable), QDialog(parent) {

	this->setMinimumSize(50, DialogTitleBar::titleBarHeight());

	m_titlebar = new DialogTitleBar(this);
	m_titlebar->setGeometry(0, 0, width(), DialogTitleBar::titleBarHeight());

	auto shade = [this]() {
		if (m_sg) 
			m_sg->setHidden(true); 

		m_pre_shade_size = size();
		m_min_size = minimumSize(); 
		//QSize s = { math::min(m_shaded_width, width()),DialogTitleBar::titleBarHeight() };
		QSize s = { width(),DialogTitleBar::titleBarHeight() };
		this->setMinimumSize(s);
		this->resize(s);
	};

	auto unshade = [this]() {
		if (m_sg)
			m_sg->setHidden(false);
		this->setMinimumSize(m_min_size);
		this->resize(m_pre_shade_size);
	};

	connect(m_titlebar, &DialogTitleBar::shadeWindow, this, shade);
	connect(m_titlebar, &DialogTitleBar::unshadeWindow, this, unshade);

	m_draw_area = new QWidget(this);
	m_draw_area->setCursor(Qt::ArrowCursor);
	drawArea()->setGeometry(m_border_width, DialogTitleBar::titleBarHeight(), width() - m_border_width, height() - DialogTitleBar::titleBarHeight());

	this->setAttribute(Qt::WA_DeleteOnClose);
	this->setAttribute(Qt::WA_TranslucentBackground);
	this->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

	if (resizable) {
		m_sg = new QSizeGrip(this);
		m_sg->setFixedSize(12, 12);
		m_sg->move(rect().bottomRight() - QPoint(m_sg->width(), m_sg->height()));
		m_sg->setToolTip("Right-click to restore to default size.");
		m_sg->installEventFilter(this);
	}

	m_timer = new QTimer(this);
	m_timer->setSingleShot(true);
	connect(m_timer, &QTimer::timeout, this, [this]() { this->setOpacity(m_opacity = m_event_opacity); });

	emit windowCreated();
}

void Dialog::setDefaultSize(const QSize& size) { 

	m_default_size = size + QSize(2 * m_border_width, DialogTitleBar::titleBarHeight() + m_border_width);
	this->resize(m_default_size);
}

void Dialog::resizeDialog(int w, int h) {
	this->resize(w + 2 * m_border_width, h + DialogTitleBar::titleBarHeight() + m_border_width);
}

void Dialog::resizeEvent(QResizeEvent* e) {

	QDialog::resizeEvent(e);

	m_titlebar->resize(width());
	drawArea()->setGeometry(m_border_width, DialogTitleBar::titleBarHeight(), width() - 2 * m_border_width, height() - DialogTitleBar::titleBarHeight() - m_border_width);

	if (m_sg)
		m_sg->move(rect().bottomRight() - QPoint(m_sg->width() + m_border_width, m_sg->height() + m_border_width));

	e->accept();
}

void Dialog::closeEvent(QCloseEvent* e) {

	QWidget::closeEvent(e);
	emit windowClosed();
	e->accept();
}

void Dialog::mousePressEvent(QMouseEvent* e) {

	if (e->buttons() == Qt::LeftButton && childAt(e->pos()) == m_titlebar) {
		m_timer->start(500);
		m_start_pos = e->pos();
		m_moving = true;
	}
}

void Dialog::mouseMoveEvent(QMouseEvent* e) {

	if (e->buttons() == Qt::LeftButton && m_moving) {

		if (m_timer->id() != Qt::TimerId::Invalid) {
			m_timer->stop();
			this->setOpacity(m_opacity = m_event_opacity);
		}
		this->move(geometry().topLeft() + (e->pos() - m_start_pos));		
	}
}

void Dialog::mouseReleaseEvent(QMouseEvent* e) {

	if (e->button() == Qt::RightButton && childAt(e->pos()) == m_sg) {
		m_resizing = true;
		this->resize(m_default_size);
		m_resizing = false;
	}

	if (e->button() == Qt::LeftButton) {

		if (m_timer->id() != Qt::TimerId::Invalid)
			m_timer->stop();

		if (m_moving)
			this->setOpacity(m_opacity = m_default_opacity);

		m_moving = false;
		m_resizing = false;
	}

}

void Dialog::paintEvent(QPaintEvent* e) {

	QPainter p(this);
	p.setOpacity(m_opacity);

	QPen pen;

	pen.setColor(QColor(69, 69, 69));
	pen.setWidth(2 * m_border_width);
	p.setPen(pen);
	p.drawRect(this->rect());

	if (palette().currentColorGroup() == QPalette::Active)
		pen.setColor(QColor(69, 0, 128, 128));
	else
		pen.setColor(QColor(128, 128, 128));

	pen.setWidth(2);
	p.setPen(pen);
	p.drawRect(this->rect());
	p.fillRect(drawArea()->geometry(), palette().color(QPalette::Window));
}

bool Dialog::eventFilter(QObject* obj, QEvent* e) {

	if (obj == m_sg) {
		if (e->type() == QEvent::MouseButtonPress)
			if (reinterpret_cast<QMouseEvent*>(e)->buttons() == Qt::LeftButton) {
				this->setWindowOpacity(m_event_opacity);
				m_resizing = true;
			}
		if (e->type() == QEvent::MouseButtonRelease)
			if (reinterpret_cast<QMouseEvent*>(e)->button() == Qt::LeftButton) {
				this->setWindowOpacity(m_default_opacity);
				m_resizing = false;
			}

		if (e->type() == QEvent::Leave) {
			this->setWindowOpacity(m_default_opacity);
			m_resizing = false;
		}
	}

	return false;
}





CheckablePushButton::CheckablePushButton(const QString& text, QWidget* parent) : PushButton(text, parent) {

	this->setCheckable(true);
	this->setAutoDefault(false);
	QPalette pal;
	//pal.setBrush(QPalette::Active, QPalette::Button, Qt::red);
}

CheckablePushButton::CheckablePushButton(const QIcon& icon, const QString& text, QWidget* parent) : CheckablePushButton(text, parent) {

	this->setIcon(icon);
}

void CheckablePushButton::paintEvent(QPaintEvent* e) {

	QPainter p(this);
	p.setOpacity(m_opacity);
	QStyleOptionButton opt;
	initStyleOption(&opt);
	QPalette pal(this->palette());

	if (isChecked()) {
		pal.setBrush(QPalette::Button, QColor(69, 69, 69));
		pal.setBrush(QPalette::ButtonText, Qt::white);
	}
	else {
		pal.setBrush(QPalette::Button, palette().brush(QPalette::Button));
		pal.setBrush(QPalette::ButtonText, palette().brush(QPalette::ButtonText));
	}

	//if (isDown())
		//pal.setBrush(QPalette::Button, palette().brush(QPalette::Button));
		//std::cout<< this->backgroundRole() << '\n';
	/*if (opt.state & QStyle::State_Off) {
		//std::cout << "SSS\n";
		pal.setBrush(QPalette::Button, palette().brush(QPalette::Button));
		pal.setBrush(QPalette::ButtonText, Qt::red);
	}

	if (opt.state & QStyle::State_On) {
		pal.setBrush(QPalette::Button, QColor(69, 69, 69));
		pal.setBrush(QPalette::ButtonText, Qt::white);
	}*/


	//if (opt.state & QStyle::State_MouseOver)
	if (hitButton(mapFromGlobal(QCursor::pos())) && isEnabled()) {
		pal.setBrush(QPalette::Button, m_hover_color);
	}
	opt.palette = pal;

	//QStyle::PE_PanelButtonCommand
	style()->drawControl(QStyle::CE_PushButton, &opt, &p);
	//style()->drawControl(QStyle::CE_PushButtonLabel, &opt, &p);
	//style()->drawControl(QStyle::CE_PushButton, &opt, &p, this);
}


FlatCheckablePushButton::FlatCheckablePushButton(const QString& text, QWidget* parent) : CheckablePushButton(text, parent) {

	this->setFlat(true);
}

FlatCheckablePushButton::FlatCheckablePushButton(const QIcon& icon, const QString& text, QWidget* parent) : FlatCheckablePushButton(text, parent) {

	this->setIcon(icon);
}

void FlatCheckablePushButton::enterEvent(QEnterEvent* e) {

	this->setFlat(false);
}

void FlatCheckablePushButton::leaveEvent(QEvent* e) {

	this->setFlat(true);
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



void LineEdit::addLabel(QLabel* label) {

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
	this->setAlignment(Qt::AlignHCenter);
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
	this->setAlignment(Qt::AlignHCenter);

	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::addTrailingZeros);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::removeEndDecimal);
}

DoubleLineEdit::DoubleLineEdit(double default_value, DoubleValidator* validator, QWidget* parent) : m_default(default_value), LineEdit(parent) {

	this->setFocusPolicy(Qt::ClickFocus);
	this->setValidator(validator);
	validator->setParent(this);
	this->setValue(default_value);
	this->resize(85, 30);
	this->setAlignment(Qt::AlignHCenter);

	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::addTrailingZeros);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::removeEndDecimal);
}

DoubleLineEdit::DoubleLineEdit(double default_value, DoubleValidator* validator, int max_length, QWidget* parent) : DoubleLineEdit(default_value, validator, parent) {

	this->setMaxLength(max_length);
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

void DoubleLineEdit::reset() {
	this->setValue(m_default);
}




ScrollBar::ScrollBar(QWidget* parent) : QScrollBar(parent) {
	this->setAttribute(Qt::WA_NoMousePropagation);

	QPalette pal;
	pal.setBrush(QPalette::Active, QPalette::Button, QColor(169, 169, 169));
	pal.setBrush(QPalette::Inactive, QPalette::Button, QColor(169, 169, 169));
	pal.setBrush(QPalette::Disabled, QPalette::Button, QColor(69, 69, 69));
	pal.setBrush(QPalette::Window, QColor(39, 39, 39));
	pal.setBrush(QPalette::WindowText, QColor(0, 0, 0));
	this->setPalette(pal);
}

ScrollBar::ScrollBar(Qt::Orientation orientation, QWidget* parent) : ScrollBar(parent) {
	this->setOrientation(orientation);
}

void ScrollBar::mousePressEvent(QMouseEvent* e) {

	if (e->buttons() == Qt::LeftButton) {
		QStyleOptionSlider opt;
		initStyleOption(&opt);
		m_current_sc = style()->hitTestComplexControl(QStyle::CC_ScrollBar, &opt, e->pos(), this);
	}

	QScrollBar::mousePressEvent(e);
}

void ScrollBar::mouseReleaseEvent(QMouseEvent* e) {
	
	m_current_sc = QStyle::SC_None;
	QScrollBar::mouseReleaseEvent(e);
}

void ScrollBar::paintEvent(QPaintEvent* event) {

	QPainter p(this);
	p.setOpacity(m_opacity);

	QStyleOptionSlider opt;
	initStyleOption(&opt);

	auto groove = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarGroove);
	p.fillRect(groove, QColor(69, 69, 69));

	const auto subcontrol = style()->hitTestComplexControl(QStyle::CC_ScrollBar, &opt, mapFromGlobal(QCursor::pos()));
	bool no_button = (QApplication::mouseButtons() == Qt::NoButton);


	//add-line
	opt.subControls = sc_sb_addline;

	if (subcontrol == sc_sb_addline && no_button) {
		opt.palette.setBrush(QPalette::Active, QPalette::Button, hover_color);
		opt.palette.setBrush(QPalette::Inactive, QPalette::Button, hover_color);
	}

	else if (currentSC() == sc_sb_addline) 
		opt.palette.setBrush(QPalette::Button, action_color);
	
	style()->drawComplexControl(QStyle::CC_ScrollBar, &opt, &p, this);
	//

	//sub-line
	opt.palette = this->palette();
	opt.subControls = sc_sb_subline;

	if (subcontrol == sc_sb_subline && no_button) {
		opt.palette.setBrush(QPalette::Active, QPalette::Button, hover_color);
		opt.palette.setBrush(QPalette::Inactive, QPalette::Button, hover_color);
	}

	else if (currentSC() == sc_sb_subline)
		opt.palette.setBrush(QPalette::Button, action_color);

	style()->drawComplexControl(QStyle::CC_ScrollBar, &opt, &p, this);
	//

	//handle
	opt.palette = this->palette();
	opt.subControls = sc_sb_slider;
	
	if (subcontrol == sc_sb_slider && no_button) {
		opt.palette.setBrush(QPalette::Active, QPalette::Button, hover_color);
		opt.palette.setBrush(QPalette::Inactive, QPalette::Button, hover_color);
	}

	else if (currentSC() == sc_sb_slider)
		opt.palette.setBrush(QPalette::Button, action_color);

	style()->drawComplexControl(QStyle::CC_ScrollBar, &opt, &p, this);
}




Slider::Slider(QWidget* parent, Qt::Orientation orientation) : QSlider(orientation, parent) {

	QPalette pal;
	pal.setBrush(QPalette::Button, QColor(129, 129, 129));
	pal.setBrush(QPalette::Disabled, QPalette::Button, QColor(96, 96, 96));
	pal.setBrush(QPalette::Active, QPalette::Highlight, QColor(123, 0, 216));
	pal.setBrush(QPalette::Inactive, QPalette::Highlight, QColor(39, 39, 39));
	pal.setBrush(QPalette::Disabled, QPalette::Highlight, QColor(96, 96, 96));
	this->setPalette(pal);
}

bool Slider::isMouseOverHandle() {

	QStyleOptionSlider opt;
	initStyleOption(&opt);
	return style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle).contains(mapFromGlobal(QCursor::pos()));
}

void Slider::drawHandle(QPainter& p, QStyleOptionSlider& opt) {
	auto rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	auto pt = rect.center().toPointF();

	pt.ry() += m_pen.widthF() - 0.5;

	pt.rx() += 0.5;
	qreal r = (rect.height() - (2 * m_pen.widthF())) / 2;

	p.setPen(m_pen);
	p.setBrush(m_brush);

	if ((opt.state & QStyle::State_MouseOver || opt.state & QStyle::State_Sunken) && opt.state & QStyle::State_Enabled)
		p.setBrush(QColor(69, 0, 128));
	
	p.drawEllipse(pt, r, r);
}

bool Slider::event(QEvent* e) {

	if (e->type() == QEvent::ToolTip) {

		if (!toolTip().isEmpty()) {
			QStyleOptionSlider opt;
			initStyleOption(&opt);
			if (style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle).contains(mapFromGlobal(QCursor::pos())))
				QToolTip::showText(QCursor::pos(), toolTip(), this);
		}
		return true;
	}

	if (e->type() == QEvent::MouseButtonPress) {
		if (reinterpret_cast<QMouseEvent*>(e)->buttons() == Qt::LeftButton)
			if (isMouseOverHandle())
				m_sunken = true;
	}

	if (e->type() == QEvent::MouseButtonRelease)
		m_sunken = false;

	return QSlider::event(e);
}

void Slider::paintEvent(QPaintEvent* event) {

	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	QStyleOptionSlider opt;
	initStyleOption(&opt);

	opt.subControls = QStyle::SC_SliderGroove;
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);

	opt.subControls = QStyle::SC_SliderHandle;

	if (!isMouseOverHandle()) {
		opt.state &= ~QStyle::State_MouseOver;
	}

	(m_sunken) ? opt.state |= QStyle::State_Sunken : opt.state &= ~QStyle::State_Sunken;

	drawHandle(p, opt);
	/*auto rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	auto pt = rect.center().toPointF();

	pt.ry() += m_pen.widthF() - 0.5;
	
	pt.rx() += 0.5;
	qreal r = (rect.height() - (2 * m_pen.widthF())) / 2;

	p.setPen(m_pen);
	p.setBrush(m_brush);
	p.drawEllipse(pt, r, r);*/

	/*if ((opt.state & QStyle::State_MouseOver && subcontrol == QStyle::SC_SliderHandle) || opt.activeSubControls & QStyle::SC_SliderHandle) {
		opt.palette.setBrush(QPalette::Active, QPalette::Button, Qt::blue);
		opt.palette.setBrush(QPalette::Inactive, QPalette::Button, Qt::blue);
	}*/
}




InputBase::InputBase(const QString& label_txt, QWidget* parent, float slider_multiplier) : m_slider_mult(slider_multiplier), QObject(parent) {

	m_label = new QLabel(label_txt, parent);
	m_slider = new Slider(parent);

	connect(m_slider, &QSlider::actionTriggered, this, &InputBase::actionTriggered);
	//connect(m_line_edit, &QLineEdit::editingFinished, this, &InputBase::editingFinished);
}

void InputBase::resizeLineEdit(int w, int h) {

	int dw = w - m_line_edit->width();
	int dh = (h - m_line_edit->height()) / 2;

	m_line_edit->resize(w, h);
	m_slider->move(m_slider->pos() + QPoint(dw, dh));
	m_label->move(m_label->pos() + QPoint(0, dh));
}

void InputBase::setLineEditWidth(int w) {

	int dw = w - m_line_edit->width();

	m_line_edit->resize(w, m_line_edit->height());
	m_slider->move(m_slider->pos() + QPoint(dw, 0));
}

void InputBase::setSliderAttributes(int min, int max, int single_step, int page_step) {

	m_slider->setRange(min, max);
	m_slider->setSingleStep(single_step);
	m_slider->setPageStep(page_step);
}

void InputBase::move(int ax, int ay) {

	QPoint pos(ax, ay);
	QPoint dp = pos - m_line_edit->pos();

	m_label->move(m_label->pos() + dp);
	m_line_edit->move(pos);
	m_slider->move(m_slider->pos() + dp);
}

void InputBase::setText(const QString& txt) {

	m_label->setText(txt);
	int x = m_line_edit->geometry().x();
	int y = m_line_edit->geometry().y();
	int h = m_line_edit->geometry().height();
	int fh = m_label->fontMetrics().height();
	int tw = m_label->fontMetrics().horizontalAdvance(txt);

	m_label->move(x - tw, ((2 * y) + h - fh) / 2);
}




IntegerInput::IntegerInput(const QString& label_txt, int default_value, IntValidator* validator, QWidget* parent, float slider_multiplier) : InputBase(label_txt, parent, slider_multiplier) {

	m_line_edit = m_ile = new IntLineEdit(default_value, validator, parent);
	connect(m_line_edit, &QLineEdit::editingFinished, this, &InputBase::editingFinished);

	m_line_edit->addLabel(m_label);
	m_line_edit->addSlider(m_slider);

	if (validator)
		m_slider->setRange(validator->bottom() * slider_multiplier, validator->top() * slider_multiplier);

	m_slider->setValue(default_value * slider_multiplier);

	onEdited();
	onAction();
}

void IntegerInput::onAction(std::function<void(int)> func) {

	disconnect(m_slider_conn);
	if (func)
		m_slider_conn = connect(this, &InputBase::actionTriggered, this, func);
	else
		m_slider_conn = connect(this, &InputBase::actionTriggered, this, &IntegerInput::sliderAction);
}

void IntegerInput::onEdited(std::function<void()> func) {

	disconnect(m_le_conn);
	if (func) {
		m_edited = func;
		m_le_conn = connect(this, &InputBase::editingFinished, this, func);
	}
	else {
		m_edited = std::bind(&IntegerInput::lineEdited, this);
		m_le_conn = connect(this, &InputBase::editingFinished, this, &IntegerInput::lineEdited);
	}
}


DoubleInput::DoubleInput(const QString& label_txt, double default_value, DoubleValidator* validator, QWidget* parent, float slider_multiplier) : InputBase(label_txt, parent, slider_multiplier) {
	
	m_line_edit = m_dle = new DoubleLineEdit(default_value, validator, parent);
	connect(m_line_edit, &QLineEdit::editingFinished, this, &InputBase::editingFinished);

	m_line_edit->addLabel(m_label);
	m_line_edit->addSlider(m_slider);

	if (validator)
		m_slider->setRange(validator->bottom() * slider_multiplier, validator->top() * slider_multiplier);

	m_slider->setValue(default_value * slider_multiplier);

	onEdited();
	onAction();
}

void DoubleInput::onAction(std::function<void(int)> func) {

	disconnect(m_slider_conn);
	if (func)
		m_slider_conn = connect(this, &InputBase::actionTriggered, this, func);
	else 
		m_slider_conn = connect(this, &InputBase::actionTriggered, this, &DoubleInput::sliderAction);
}

void DoubleInput::onEdited(std::function<void()> func) {

	disconnect(m_le_conn);
	if (func) {
		m_edited = func;
		m_le_conn = connect(this, &InputBase::editingFinished, this, func);
	}
	else {
		m_edited = std::bind(&DoubleInput::lineEdited, this);
		m_le_conn = connect(this, &InputBase::editingFinished, this, &DoubleInput::lineEdited);
	}
}




void RadioButton::paintEvent(QPaintEvent* event) {

	QPainter p(this);
	QStyleOptionButton opt;
	initStyleOption(&opt);

	QPalette pal;
	pal.setBrush(QPalette::ColorRole::Text, m_indicator_color);
	pal.setBrush(QPalette::ColorRole::Base, m_base_color);

	opt.palette = pal;
	style()->drawControl(QStyle::CE_RadioButton, &opt, &p, this);
}



ComboBox::ComboBox(QWidget* parent) : QComboBox(parent) {
	QPalette pal;
	pal.setColor(QPalette::Window, QColor(169, 169, 169));
	pal.setColor(QPalette::ButtonText, Qt::black);
	pal.setColor(QPalette::Disabled, QPalette::Button, QColor(129, 129, 129));
	pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(69,69,69));

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


void ImageComboBox::addImage(const ImageWindow<>* iw) {
	this->addItem(iw->name(), QVariant::fromValue(&iw->source()));
}

void ImageComboBox::addImage(const QString& name, const Image8* img) {
	this->addItem(name, QVariant::fromValue(img));
}

const Image8* ImageComboBox::currentImage() {
	return currentData().value<const Image8*>();
}

const Image8* ImageComboBox::imageAt(int index) {
	return itemData(index).value<const Image8*>();
}

int ImageComboBox::findImage(const Image8* img) {

	for (int i = 0; i < count(); ++i)
		if (img == imageAt(i))
			return i;

	return -1;
}




InterpolationComboBox::InterpolationComboBox(QWidget* parent) : ComboBox(parent) {

	addItems({ "Nearest Neighbor", "Bilinear", "Bicubic Spline", "Bicubic B-Spline",
								"Cubic B-Spline", "Catmull-Rom", "Lanczos-3" });

	setCurrentIndex(m_defualt_index);
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




ComponentPushButton::ComponentPushButton(const QString& text, QWidget* parent) : FlatCheckablePushButton(text, parent) {

	this->setAutoFillBackground(true);
	this->setBackgroundRole(QPalette::ColorRole::Dark);

	//this->setFlat(true);
	//this->setAutoDefault(false);
	//this->setCheckable(true);
	//this->setAutoExclusive(true);

	QPalette pal(palette());
	pal.setColor(QPalette::ButtonText, Qt::white);
	this->setPalette(pal);

	QFont font = this->font();
	font.setPointSize(8);
	this->setFont(font);
	//this->resize(this->fontMetrics().horizontalAdvance(text) + 10, this->size().height());
}

ComponentPushButton::ComponentPushButton(const QIcon& icon, const QString& text, QWidget* parent) : ComponentPushButton(text, parent) {

	this->setIcon(icon);
	//this->setAutoFillBackground(true);
	//this->setBackgroundRole(QPalette::ColorRole::Dark);

	//this->setFlat(true);
	//this->setAutoDefault(false);
	//this->setCheckable(true);
	//this->setAutoExclusive(true);

	//QFont font = this->font();
	//font.setPointSize(8);
	//this->setFont(font);
}

/*void ComponentPushButton::paintEvent(QPaintEvent* event) {

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
}*/





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
	const QRect draw_rect = { opt.rect.x() + 1, opt.rect.y() + 1 + y_offset, opt.rect.width() - 2, opt.rect.height() - 2 - y_offset };
	const QRect radius_rect = draw_rect.adjusted(radius, radius, -radius, -radius);

	if (title() != "") {
		QList<QPointF> points;
		double dtheta = (2 * std::numbers::pi * 15) / 360;

		QPoint point = draw_rect.topLeft();
		p.drawLine(QPoint(point.x() + radius + 1, point.y()), { text_beginX - text_margin, point.y() });

		point = draw_rect.topRight();
		p.drawLine(QPoint(point.x() - radius - 1, point.y()), { text_endX + text_margin, point.y() });

		point = radius_rect.topLeft();
		for (double theta = std::numbers::pi / 2; theta <= std::numbers::pi; theta += dtheta)
			points.push_back(QPointF(point.x() + radius * cos(theta), point.y() - radius * sin(theta)));

		point = radius_rect.bottomLeft();
		for (double theta = std::numbers::pi; theta <= 3 * std::numbers::pi / 2; theta += dtheta)
			points.push_back(QPointF(point.x() + radius * cos(theta), point.y() - radius * sin(theta)));

		point = radius_rect.bottomRight();
		for (double theta = 3 * std::numbers::pi / 2; theta <= 2 * std::numbers::pi; theta += dtheta)
			points.push_back(QPointF(point.x() + radius * cos(theta), point.y() - radius * sin(theta)));

		point = radius_rect.topRight();
		for (double theta = 0; theta <= std::numbers::pi / 2; theta += dtheta)
			points.push_back(QPointF(point.x() + radius * cos(theta), point.y() - radius * sin(theta)));

		p.drawPolyline(QPolygonF(points));
	}
	else
		p.drawRoundedRect(draw_rect, radius, radius);

	p.setPen(Qt::white);
	p.drawText((opt.rect.width() / 2) - (text_width / 2), opt.rect.y() + text_height / 2 + text_margin, title());
}




ListWidget::ListWidget(QWidget* parent) : QListWidget(parent) {

	this->setDragDropMode(QAbstractItemView::DragDropMode::InternalMove);

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