#pragma once
#include "CustomWidgets.h"
#include "SFML/Audio.hpp"
#include "SFML/Config.hpp"
#include "Maths.h"
#include "taglib/fileref.h"

class ToolTip : public QLabel {

	QTimer m_timer;
	QPropertyAnimation* m_pa = nullptr;
public:
	ToolTip(const QString& txt, QWidget* parent = nullptr) : QLabel(txt, parent, Qt::ToolTip) {
		this->setContentsMargins(6, 4, 6, 4);
		this->setStyleSheet("QLabel {border : 1px solid dark gray; background: rgb(169,169,169); color: black}");
		m_timer.setSingleShot(true);
		m_timer.setInterval(4'000);
		//connect(&m_timer, &QTimer::timeout, this, &QLabel::hide);

		m_pa = new QPropertyAnimation(this, "windowOpacity");
		m_pa->setStartValue(1.0);
		m_pa->setEndValue(0.0);
		m_pa->setDuration(500);
		connect(&m_timer, &QTimer::timeout, this, [this]() { m_pa->start(); });
		connect(m_pa, &QPropertyAnimation::finished, this, [this]() { QLabel::hide(); setWindowOpacity(1.0); });
	}

	void setDuration(int msec) {
		m_timer.setInterval(msec);
	}

	void setText(const QString& txt) {
		QLabel::setText(txt);
		this->adjustSize();
		m_timer.start();
	}

	void show() {
		if (isHidden())
			QLabel::show();
		this->move(QCursor::pos() + QPoint(10, 20));
		m_timer.start();
	}

	void hide() {
		m_pa->start();
	}
};





QString timePointFromSeconds(int secs);




class MediaSlider : public QSlider {
	Q_OBJECT

private:
	ToolTip* m_tooltip = nullptr;
	int m_handle_width = 16;
	QWidget* m_time_point = nullptr;
	QPen m_pen = QPen( QColor(123, 0, 216), 3);
	QBrush m_brush = QBrush({ 39,39,39 });
public:
	MediaSlider(QWidget* parent = nullptr);

signals:
	void timePoint(const QString& time_point);

public:
	void setValue(int value);

private:
	int handleWidth()const { return m_handle_width; }

	QString timePointFromMouse(const QPoint& p)const;


	void leaveEvent(QEvent* e);

	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* e);
};





class MediaButton : public QPushButton {

	QIcon m_active;
	QIcon m_inactive;

	MediaButton(QWidget* parent = nullptr);

public:
	static MediaButton* skipPrevious(QWidget* parent = nullptr);

	static MediaButton* skipNext(QWidget* parent = nullptr);

private:
	void paintEvent(QPaintEvent* e);
};





class PlayPauseButton : public QPushButton {

	QIcon m_pause = style()->standardPixmap(QStyle::SP_MediaPause);
	QIcon m_play = style()->standardPixmap(QStyle::SP_MediaPlay);

	QIcon m_pause_inactive = QIcon("./Icons\\MediaIcons\\pause_white.svg");
	QIcon m_pause_active = QIcon("./Icons\\MediaIcons\\pause_purple.svg");

	QIcon m_play_inactive = QIcon("./Icons\\MediaIcons\\play_arrow_white.svg");
	QIcon m_play_active = QIcon("./Icons\\MediaIcons\\play_arrow_purple.svg");

public:
	PlayPauseButton(QWidget* parent = nullptr);

	void reset();
private:
	void paintEvent(QPaintEvent* e);
};





class Song {
	
	std::filesystem::path m_path;

	QString m_track_num;
	QString m_title;
	QString m_duration;
	QString m_album;
	QString m_artist;

public:
	Song(const std::filesystem::path& file_path) {

		TagLib::FileRef file(file_path.c_str());
		auto tag = file.tag();

		if (tag == nullptr || tag->isEmpty())
			return;

		m_path = file_path;
		m_track_num = QString::number(tag->track());
		m_title = tag->title().toCString(true);
		m_album = tag->album().toCString(true);
		m_artist = tag->artist().toCString(true);
		m_duration = timePointFromSeconds(file.audioProperties()->lengthInSeconds());
	}

	Song(const QString& file_path) {
		
		TagLib::FileRef file(file_path.toStdString().c_str());
		auto tag = file.tag();

		if (tag == nullptr || tag->isEmpty())
			return;

		m_path = file_path.toStdString();
		m_track_num = QString::number(tag->track());
		m_title = tag->title().toCString(true);
		m_album = tag->album().toCString(true);
		m_artist = tag->artist().toCString(true);
		m_duration = timePointFromSeconds(file.audioProperties()->lengthInSeconds());
	}

	Song() = default;

	bool isValid()const { return m_path.empty(); }

	std::filesystem::path path()const { return m_path; }

	QString trackNumber()const { return m_track_num; }

	QString songTitle()const { return m_title; }

	QString duration()const { return m_duration; }

	QString album()const { return m_album; }

	QString artist()const { return m_artist; }

	bool exists()const { return (!m_path.empty() && std::filesystem::exists(m_path)); }
};





class SongLabel : public QTableWidgetItem {

	Song m_song;

public:
	SongLabel(const Song& song) : m_song(song), QTableWidgetItem(song.songTitle()) {}

	std::filesystem::path songPath()const { return m_song.path(); }

	Song* song() { return &m_song; }
};





class CustomDropIndicatorStyle : public QProxyStyle {
public:
	CustomDropIndicatorStyle(QStyle* style = nullptr) : QProxyStyle(style) {}

private:
	void drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* opt, QPainter* p, const QWidget* widget) const override {
		
		if (element == QStyle::PE_IndicatorItemViewItemDrop) {
			p->setPen(QPen(QColor(0,255,255), 1));
			if (!opt->rect.isNull()) 
				p->drawRect(opt->rect);		
		}
		else 
			QProxyStyle::drawPrimitive(element, opt, p, widget);
	}
};




class SongList : public QTableWidget {

	Q_OBJECT
private:
	int m_current_row = -1;
	int m_old_row = 0;

	QIcon m_sound_icon = QIcon("./Icons\\MediaIcons\\volume_down.svg");
	QIcon m_no_sound_icon = QIcon("./Icons\\MediaIcons\\volume_mute.svg");

	const QString m_format = "application/x-qabstractitemmodeldatalist";

	QAction* m_remove_act = nullptr;

public:
	SongList(QWidget* parent = nullptr);

signals:
	void songSelected(Song* song);

	//void songSelected(Song* song, bool play = true);

	void rowDoubleClicked(int row);

public:
	static bool isAudioFile(const QString& file) {

		auto ext = QFileInfo(file).suffix();
		if (ext == "mp3" || ext == "wav" || ext == "flac")
			return true;

		return false;
	}

	static bool isAudioFile(const std::filesystem::path& file) {

		auto ext = file.extension();
		if (ext == ".mp3" || ext == ".wav" || ext == ".flac")
			return true;

		return false;
	}

	bool empty()const { return (this->rowCount() == 0); }

	Song* songAt(int row);

	void addSong(const Song& song);

	void insertSong(const Song& song, int row);

	void removeSong(int row);

	bool previousSong() {
		if (m_current_row - 1 < 0) {
			if (!empty())
				item(m_current_row, 0)->setIcon(QIcon());
			return false;
		}
		emit rowDoubleClicked(m_current_row - 1);
		return true;
	}

	bool nextSong() {
		if (m_current_row + 1 >= this->rowCount()) {
			if (!empty())
				item(m_current_row, 0)->setIcon(QIcon());
			return false;
		}
		emit rowDoubleClicked(m_current_row + 1);
		return true;
	}

	void onPlay() {
		if (item(m_current_row, 0))
			item(m_current_row, 0)->setIcon(m_sound_icon);
	}

	void onPause() {
		if (item(m_current_row, 0))
			item(m_current_row, 0)->setIcon(m_no_sound_icon);
	}

private:
	static bool mimeHasAudio(const QMimeData* mime);

	void moveRow(int row, int new_row);

	void showContextMenu(const QPoint& pos);

	void dragEnterEvent(QDragEnterEvent* e);

	void dragMoveEvent(QDragMoveEvent* e);

	void dropEvent(QDropEvent* e);

	bool eventFilter(QObject* obj, QEvent* e);

	void mousePressEvent(QMouseEvent* e);

	void mouseDoubleClickEvent(QMouseEvent* e);

	//QStringList mimeTypes() const { return { m_format, "application/x-qt-windows-mime" }; }
};





class VolumeControl : public QWidget {

	QImage m_volume = QImage("./Icons\\MediaIcons\\volume_up.svg");

	QLabel* m_label = nullptr;
	Slider* m_slider = nullptr;
	ToolTip* m_tooltip = nullptr;

public:
	VolumeControl(QWidget* parent = nullptr);

	Slider* slider()const { return m_slider; }

private:
	bool eventFilter(QObject* obj, QEvent* e);
};





class MediaPlayer : public sf::Music {

	QTimer m_timer;
	float m_volume = 100.0;
	float m_delta = 0.0;
	QMetaObject::Connection m_connection;

public:
	void setVolume(float volume) {
		sf::Music::setVolume(m_volume = volume);
	}

	void playFadeIn(uint32_t duration_ms = 2'000);

	void pauseFadeOut(uint32_t duration_ms = 2'000);

private:
	void fadeIn();

	void fadeOut();
};






class MediaPlayerDialog : public Dialog {
	Q_OBJECT

private:
	MediaSlider* m_progress_slider = nullptr;
	PlayPauseButton* m_play_pause_pb = nullptr;
	MediaButton* m_previous_pb = nullptr;
	MediaButton* m_next_pb = nullptr;
	bool m_slider_pressed = false;
	bool m_resume_playing = false;

	VolumeControl* m_volume_control = nullptr;
	QLabel* m_time_point = nullptr;

	SongList* m_song_list = nullptr;
	Song* m_current_song = nullptr;
	QLabel* m_current_song_label = nullptr;

	MediaPlayer m_music_player;
public:
	MediaPlayerDialog(QWidget* widget = nullptr);

private:
	void songSelected(Song* song);

	void addMediaSlider();

	void addMediaButtons();

	void onPlay();

	void onPause();
};

