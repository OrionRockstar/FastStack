#include "pch.h"
#include "MediaPlayerDialog.h"
#include "Maths.h"


QString timePointFromSeconds(int secs) {

	int m = secs / 60;
	int s = secs % 60;

	auto minutes = ((m < 10) ? "" : "") + QString::number(m);
	auto seconds = ((s < 10) ? "0" : "") + QString::number(s);

	return minutes + ":" + seconds;
}


MediaSlider::MediaSlider(QWidget* parent) : QSlider(Qt::Horizontal, parent) {

	QPalette pal;
	pal.setColor(QPalette::Button, QColor(129, 129, 129));
	pal.setColor(QPalette::Disabled, QPalette::Button, QColor(96, 96, 96));
	pal.setColor(QPalette::Active, QPalette::Highlight, QColor(123, 0, 216));
	pal.setColor(QPalette::Inactive, QPalette::Highlight, QColor(39, 39, 39));
	pal.setColor(QPalette::Disabled, QPalette::Highlight, QColor(96, 96, 96));
	this->setPalette(pal);	

	m_tooltip = new ToolTip("00:00", this);
	m_tooltip->hide();

	setPageStep(0);
	setSingleStep(0);
	setRange(0, 0);
	this->setMouseTracking(true);
	//this->setFixedHeight(height() + 2);

	QStyleOptionSlider opt;
	initStyleOption(&opt);
	m_handle_width = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle).width();
}

void MediaSlider::setValue(int val) {

	if (val != value()) {
		QSlider::setValue(val);
		emit timePoint(timePointFromSeconds(val));
	}
}

QString MediaSlider::timePointFromMouse(const QPoint& p)const {

	int hw = m_handle_width;
	int value = QStyle::sliderValueFromPosition(minimum(), maximum(), p.x() - hw / 2, width() - hw);

	return timePointFromSeconds(value);
}

void MediaSlider::leaveEvent(QEvent* e) {
	m_tooltip->hide();
}

void MediaSlider::mousePressEvent(QMouseEvent* e) {

	emit sliderPressed();

	if (style()->hitTestComplexControl(QStyle::CC_Slider, nullptr, e->pos()) != QStyle::SC_SliderHandle) {
		setValue(QStyle::sliderValueFromPosition(minimum(), maximum(), e->x() - handleWidth() / 2, width() - handleWidth()));
		emit sliderMoved(value());
	}
}

void MediaSlider::mouseMoveEvent(QMouseEvent* e) {

	if (e->buttons() == Qt::LeftButton) {
		setValue(QStyle::sliderValueFromPosition(minimum(), maximum(), e->x() - handleWidth() / 2, width() - handleWidth()));
		emit sliderMoved(value());
	}

	if (maximum() != 0) {
		m_tooltip->setText(timePointFromMouse(e->pos()));
		m_tooltip->show();
	}
}

void MediaSlider::mouseReleaseEvent(QMouseEvent* e) {
	emit sliderReleased();
}

void MediaSlider::paintEvent(QPaintEvent* e) {

	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	QStyleOptionSlider opt;
	initStyleOption(&opt);

	opt.subControls = QStyle::SC_SliderGroove;
	opt.rect.adjust(3, 0, -3, 0);
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);

	opt.subControls = QStyle::SC_SliderHandle;
	//auto pt = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle).center();
	//pt.ry() += 2;
	//pt.rx() += 1;

	auto rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	auto pt = rect.center().toPointF();
	pt.ry() += m_pen.widthF() / 2;
	int r = (rect.height() - (2 * m_pen.widthF())) / 2;

	p.setPen(m_pen);
	p.setBrush(m_brush);
	p.drawEllipse(pt, r + 3, r + 3);
}





MediaButton::MediaButton(QWidget* parent) : QPushButton(parent) {

	this->setAutoDefault(false);
	this->setFlat(true);

	this->resize(48, 48);
	this->setIcon(m_inactive);
	this->setIconSize(QSize(48, 48));
}

MediaButton* MediaButton::skipPrevious(QWidget* parent) {

	MediaButton* mb = new MediaButton(parent);
	mb->m_inactive = QIcon("./Icons\\MediaIcons\\skip_previous_white.svg");
	mb->m_active = QIcon("./Icons\\MediaIcons\\skip_previous_purple.svg");
	return mb;
}

MediaButton* MediaButton::skipNext(QWidget* parent) {

	MediaButton* mb = new MediaButton(parent);
	mb->m_inactive = QIcon("./Icons\\MediaIcons\\skip_next_white.svg");
	mb->m_active = QIcon("./Icons\\MediaIcons\\skip_next_purple.svg");
	return mb;
}

void MediaButton::paintEvent(QPaintEvent* e) {

	QPainter p(this);
	QStyleOptionButton opt;
	initStyleOption(&opt);

	if (opt.state & QStyle::State_MouseOver)
		opt.icon = m_active;
	else 
		opt.icon = m_inactive;
	
	if (opt.state & QStyle::State_Sunken)
		opt.state &= ~QStyle::State_Sunken;

	if (opt.state & QStyle::State_On) {
		opt.state &= ~QStyle::State_On;
		opt.state |= QStyle::State_Off;
	}

	style()->drawControl(QStyle::CE_PushButton, &opt, &p, this);
}





PlayPauseButton::PlayPauseButton(QWidget* parent) : QPushButton("", parent) {

	this->setCheckable(true);
	this->setAutoDefault(false);
	this->setFlat(true);
	this->setStyleSheet("QToolTip {border : 0px solid dark gray; background: solid dark gray; color: white}");

	this->resize(48, 48);
	this->setIcon(m_play_inactive);
	this->setIconSize(QSize(48,48));
	
	connect(this, &QPushButton::toggled, this, [this]() { update(); });
}

void PlayPauseButton::reset() {

	this->blockSignals(true);
	this->setIcon(m_play);
	this->setChecked(false);
	this->blockSignals(false);
}

void PlayPauseButton::paintEvent(QPaintEvent* e) {

	QPainter p(this);
	QStyleOptionButton opt;
	initStyleOption(&opt);
	
	if (opt.state & QStyle::State_MouseOver) {
		if (opt.state & QStyle::State_On)
			opt.icon = m_pause_active;
		else
			opt.icon = m_play_active;
	}
	else {
		if (opt.state & QStyle::State_On)
			opt.icon = m_pause_inactive;
		else
			opt.icon = m_play_inactive;
	}


	if (opt.state & QStyle::State_Sunken)
		opt.state &= ~QStyle::State_Sunken;

	if (opt.state & QStyle::State_On) {
		opt.state &= ~QStyle::State_On;
		opt.state |= QStyle::State_Off;
	}

	style()->drawControl(QStyle::CE_PushButton, &opt, &p, this);
}




SongList::SongList(QWidget* parent) : QTableWidget(parent) {

	QPalette palette = this->palette();
	palette.setBrush(QPalette::Base, QColor(69, 69, 69));
	palette.setBrush(QPalette::AlternateBase, QColor(128, 128, 128));
	palette.setBrush(QPalette::Highlight, QColor(69,0,128,69));
	palette.setBrush(QPalette::Text, Qt::white);
	palette.setBrush(QPalette::HighlightedText, Qt::white);
	this->setPalette(palette);
	
	this->setShowGrid(false);
	this->setSelectionBehavior(QAbstractItemView::SelectRows);
	this->setSelectionMode(QAbstractItemView::SingleSelection);
	this->setAlternatingRowColors(true);
	this->setFocusPolicy(Qt::NoFocus);
	this->setEditTriggers(QAbstractItemView::NoEditTriggers);
	this->grabKeyboard();
	

	this->setDragEnabled(true);
	this->setAcceptDrops(true);
	this->setDragDropMode(QAbstractItemView::DragDrop);
	this->setDropIndicatorShown(true);

	viewport()->installEventFilter(this);
	this->setStyle(new CustomDropIndicatorStyle(style()));

	for (int i = 0; i < 5; ++i)
		this->insertColumn(i);

	this->setColumnWidth(0, 20);
	this->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Fixed);
	this->setColumnWidth(1, 20);
	this->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Fixed);
	this->setColumnWidth(2, 270);
	this->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeMode::Fixed);
	this->setColumnWidth(3, 200);
	this->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeMode::Fixed);
	this->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeMode::Stretch);


	this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	this->verticalHeader()->hide();
	this->horizontalHeader()->setMinimumSectionSize(25);
	this->setHorizontalHeaderLabels({ "","#","Title","Artist","Time","" });
	this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	this->horizontalHeader()->setFont(QFont("Calibri"));
	this->horizontalHeader()->setHighlightSections(false);
	this->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

	m_remove_act = this->addAction("Remove", Qt::Key_Delete, this, [this]() {	
		auto items = selectedItems();
		if (!items.isEmpty())
			removeSong(items[0]->row()); });

	connect(this, &QTableWidget::cellPressed, this, [this](int r, int c) { m_old_row = r; });

	connect(this, &SongList::rowDoubleClicked, this, [this](int row) {
		item(m_current_row, 0)->setIcon(QIcon());
		m_current_row = row;
		onPlay();
		emit songSelected(songAt(m_current_row)); });
}

Song* SongList::songAt(int row) {

	if (0 <= row && row < rowCount())
		return dynamic_cast<SongLabel*>(item(row, 2))->song();

	return nullptr;
}

void SongList::addSong(const Song& song) {

	if (m_current_row == -1)
		m_current_row = 0;

	int r = this->rowCount();
	this->insertRow(r);
	this->setItem(r, 0, new QTableWidgetItem());
	this->setItem(r, 1, new QTableWidgetItem(QString::number(r + 1)));
	this->setItem(r, 2, new SongLabel(song));
	this->setItem(r, 3, new QTableWidgetItem(song.artist()));
	this->setItem(r, 4, new QTableWidgetItem(song.duration()));
}

void SongList::insertSong(const Song& song, int row) {

	if (m_current_row == -1)
		m_current_row = 0;

	row = math::min(row, rowCount());

	this->insertRow(row);
	this->setItem(row, 0, new QTableWidgetItem());
	this->setItem(row, 1, new QTableWidgetItem(QString::number(row + 1)));
	this->setItem(row, 2, new SongLabel(song));
	this->setItem(row, 3, new QTableWidgetItem(song.artist()));
	this->setItem(row, 4, new QTableWidgetItem(song.duration()));

	for (int r = row; r < rowCount(); ++r)
		this->item(r, 1)->setText(QString::number(r + 1));
}

//alter
void SongList::removeSong(int row) {

	if (row == -1)
		return;

	row = math::min(row, rowCount());
	removeRow(row);

	if (row <= m_current_row) {
		//only select song if playing
		if (row == m_current_row)
			emit songSelected(songAt(m_current_row));
		else
			m_current_row--;
	}
}

void SongList::moveRow(int row, int new_row) {

	std::vector<QTableWidgetItem*> items;
	for (int c = 0; c < this->columnCount(); ++c)
		items.push_back(takeItem(row, c));

	this->insertRow(new_row);

	for (int c = 0; c < this->columnCount(); ++c)
		this->setItem(new_row, c, items[c]);

	if (new_row <= row)
		this->removeRow(row + 1);
	else
		this->removeRow(row);

	for (int r = math::min(row, new_row); r < rowCount(); ++r)
		this->item(r, 1)->setText(QString::number(r + 1));

	if (m_current_row == row) {
		if (new_row <= row)
			m_current_row = new_row;
		else
			m_current_row = new_row - 1;
	}
	else if (new_row <= m_current_row && m_current_row < row)
		m_current_row++;
	else if (row <m_current_row && m_current_row < new_row)
		m_current_row--;
}

void SongList::showContextMenu(const QPoint& pos) {

	int row = rowAt(pos.y());
	if (row == -1)
		return;

	QMenu menu;
	//causes bug where slider released event is not called when slider pressed after this action
	//menu.addAction(tr("Play Now"), this, [this, row]() { emit rowDoubleClicked(row); });
	
	menu.addAction(tr("Move Up"), this, [this, row]() {
		if (row > 0)
			moveRow(row,row - 1);
		});
	
	menu.addAction(tr("Move Down"), this, [this, row]() {
		if (row < rowCount() - 1)
			moveRow(row, row + 2);
		});

	menu.addAction(m_remove_act);

	menu.exec(mapToGlobal(pos));
}

void SongList::dragEnterEvent(QDragEnterEvent* e) {

	QTableWidget::dragEnterEvent(e);
	
	if (e->mimeData()->hasUrls())
		e->acceptProposedAction();
}

void SongList::dragMoveEvent(QDragMoveEvent* e) {

	QTableWidget::dragMoveEvent(e);
	e->acceptProposedAction();
}

void SongList::dropEvent(QDropEvent* e) {

	if (e->mimeData()->hasFormat(m_format)) {

		int new_row = this->rowAt(e->pos().y());

		if (new_row == -1)
			new_row = this->rowCount();
		else if (new_row > m_old_row)
			new_row++;
		else if (new_row < m_old_row)
			new_row = math::max(new_row--, 0);

		moveRow(m_old_row, new_row);
	}

	if (e->mimeData()->hasUrls()) {
		for (auto url : e->mimeData()->urls()) {
			std::filesystem::path path = url.toLocalFile().toStdString();
			if (path.has_extension()) {
				if (isAudioFile(path))
					this->addSong(Song(path));
			}
			else
				for (auto fp : std::filesystem::directory_iterator(path))
					if (isAudioFile(fp))
						this->addSong(Song(fp));
		}
	}
	e->acceptProposedAction();
}

bool SongList::eventFilter(QObject* obj, QEvent* e) {

	if (obj == viewport() && e->type() == QEvent::MouseButtonPress)
		if (rowAt(reinterpret_cast<QMouseEvent*>(e)->pos().y()) == -1)
			clearSelection();

	return false;
}

void SongList::mousePressEvent(QMouseEvent* e) {
	
	if (e->buttons() == Qt::RightButton) {
		selectRow(rowAt(e->pos().y()));
		showContextMenu(e->pos());
	}
	else
		QTableWidget::mousePressEvent(e);
}

void SongList::mouseDoubleClickEvent(QMouseEvent* e) {

	if (e->buttons() == Qt::LeftButton) {
		int row = rowAt(e->pos().y());
		if (row != -1) 
			emit rowDoubleClicked(row);
	}
}





QPixmap adjustWaveOpacity(const QImage& img, float opacity) {

	int x_start = img.width() / 2;
	int y_mid = img.height() / 2;
	float r = (img.width() - x_start) * opacity;

	QImage other(img);

	for (int y = 0; y < img.height(); ++y) {
		for (int x = x_start; x < img.width(); ++x) {
			float d = math::distancef(x_start, y_mid, x, y);
			if (r < d) {
				auto c = img.pixelColor(x, y);
				c.setAlpha(c.alpha() * pow(r / d,4));
				other.setPixelColor(x, y, c);
			}
		}
	}

	return QPixmap::fromImage(other);
}

VolumeControl::VolumeControl(QWidget* parent) : QWidget(parent) {

	this->setFixedSize(155, 38);
	m_label = new QLabel(this);
	m_label->setScaledContents(true);
	m_volume = m_volume.convertedTo(QImage::Format_ARGB32);
	m_label->setPixmap(QPixmap::fromImage(m_volume));
	
	m_tooltip = new ToolTip("100%");
	m_tooltip->hide();
	
	m_slider = new Slider(this);
	m_slider->installEventFilter(this);
	m_slider->move(50, 10);
	m_slider->setFixedWidth(100);
	m_slider->setRange(0, 100);
	m_slider->setValue(100);

	connect(m_slider, &QSlider::actionTriggered, this, [this](int) {
		int v = m_slider->sliderPosition();
		m_tooltip->setText(QString::number(v)+"%");
		m_label->setPixmap(adjustWaveOpacity(m_volume, v / 100.0)); });
}

bool VolumeControl::eventFilter(QObject* obj, QEvent* e) {

	if (obj == m_slider) {
		if (m_slider->isMouseOverHandle()) {			
			if (e->type() == QEvent::ToolTip || e->type() == QEvent::MouseMove)
				m_tooltip->show();
		}
		else 
			m_tooltip->hide();
	}
	return false;
}



MediaPlayerDialog::MediaPlayerDialog(QWidget* parent) : Dialog(parent) {

	this->resizeDialog(630, 630);
	this->setTitle("Playlist Player");

	addMediaSlider();
	addMediaButtons();

	m_volume_control = new VolumeControl(drawArea());
	m_volume_control->move(365, 580);
	connect(m_volume_control->slider(), &QSlider::actionTriggered, this, [this](int) { m_music_player.setVolume(m_volume_control->slider()->sliderPosition()); });

	m_current_song_label = new QLabel( "", drawArea());
	m_current_song_label->move(15, 525);
	
	QFont font;
	font.setFamily("Calibri");
	font.setPointSizeF(12);
	font.setBold(true);
	m_current_song_label->setFont(font);

	m_song_list = new SongList(drawArea());
	m_song_list->move(15, 15);
	m_song_list->resize(600, 500);
	connect(m_song_list, &SongList::songSelected, this, &MediaPlayerDialog::songSelected);

	this->show();
}

void MediaPlayerDialog::songSelected(Song* song) {

	//if (m_music_player.getStatus() == sf::Music::Status::Playing)
		//play = true;
	m_music_player.pause();
	m_current_song = song;

	auto setSongLabelText = [this](const QString& txt) {
		m_current_song_label->setText(txt);
		m_current_song_label->adjustSize();
	};

	if (m_current_song != nullptr && m_music_player.openFromFile(m_current_song->path())) {
		setSongLabelText(m_current_song->artist() + "-" + m_current_song->songTitle());
		m_progress_slider->timePoint("0:00");
		m_progress_slider->setRange(0, m_music_player.getDuration().asSeconds());
		if (!m_play_pause_pb->isChecked())
			m_play_pause_pb->setChecked(true);
		else
			onPlay_Pause(true);
	}
	else {
		m_progress_slider->setRange(0, 0);
		m_play_pause_pb->reset();
		m_progress_slider->timePoint("0:00");
		setSongLabelText("");
	}
}

void MediaPlayerDialog::addMediaSlider() {

	m_progress_slider = new MediaSlider(drawArea());
	m_progress_slider->move(15, 550);
	m_progress_slider->resize(600,m_progress_slider->height());

	m_time_point = new QLabel(drawArea());
	m_time_point->move(605, 528);
	m_time_point->adjustSize();

	connect(m_progress_slider, &MediaSlider::sliderPressed, this, [this]() { 
		m_slider_pressed = true;
		if (m_music_player.getStatus() == sf::Music::Status::Playing) {
			m_resume_playing = true;
			m_play_pause_pb->setChecked(false);} });

	connect(m_progress_slider, &MediaSlider::sliderMoved, this, [this](int pos) {
		m_music_player.setPlayingOffset(sf::seconds(pos)); });

	connect(m_progress_slider, &MediaSlider::sliderReleased, this, [this]() {
		m_slider_pressed = false;
		if (m_resume_playing) 
			m_play_pause_pb->setChecked(true);
		m_resume_playing = false; });

	connect(m_progress_slider, &MediaSlider::timePoint, this, [this](const QString& txt) {
		auto duration = (m_current_song) ? m_current_song->duration() : "0:00";
		m_time_point->setText(txt + "/ " + duration); 
		int old_width = m_time_point->width();
		m_time_point->adjustSize();
		m_time_point->move(m_time_point->x() + old_width - m_time_point->width(), m_time_point->y());});

	m_progress_slider->timePoint("0:00");
}

void MediaPlayerDialog::addMediaButtons() {

	m_play_pause_pb = new PlayPauseButton(drawArea());
	m_play_pause_pb->move(165, 577);
	connect(m_play_pause_pb, &QPushButton::toggled, this, &MediaPlayerDialog::onPlay_Pause);

	m_previous_pb = MediaButton::skipPrevious(drawArea());
	m_previous_pb->move(115, 577);
	connect(m_previous_pb, &PushButton::released, this, [this]() {
		if (!m_song_list->previousSong()) {
			m_music_player.pause();
			m_music_player.setPlayingOffset(sf::seconds(0));
			m_play_pause_pb->reset();
			m_progress_slider->setValue(0);
		} });

	m_next_pb = MediaButton::skipNext(drawArea());
	m_next_pb->move(215, 577);
	connect(m_next_pb, &PushButton::released, this, [this]() { m_music_player.stop(); });
}

void MediaPlayerDialog::onPlay_Pause(bool play) {

	if (play) {

		if (m_song_list->rowCount() == 0)
			return m_play_pause_pb->reset();

		if (m_current_song == nullptr)
			return m_song_list->rowDoubleClicked(0);

		m_music_player.play();
		m_song_list->onPlay();

		//be mindful when running QThreads::runThread as it has its own event loop
		Threads::runThread([this]() {
			while (m_music_player.getStatus() == sf::Music::Status::Playing)
				m_progress_slider->setValue(m_music_player.getPlayingOffset().asSeconds());

			if (m_music_player.getStatus() == sf::Music::Status::Stopped) 
				if (!m_song_list->nextSong()) {
					m_play_pause_pb->reset();
					m_progress_slider->setValue(0);
				}		
			});
	}
	else {
		m_music_player.pause();
		m_song_list->onPause();
	}
}