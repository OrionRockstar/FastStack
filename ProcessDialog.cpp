#include "pch.h"
#include "ProcessDialog.h"
#include "FastStack.h"
#include "ImageWindow.h"


Toolbar::Toolbar(QWidget* parent, bool preview) : QWidget(parent) {
	this->setGeometry(0, parent->size().height() - dy, parent->size().width(), dx);

	m_apply_to = new QPushButton(this);
	m_apply_to->setAutoDefault(false);
	m_apply_to->setToolTip("Apply to... (Drag and Drop)");
	m_apply_to->move(0, 0);
	m_apply_to->resize(dx, dy);
	//m_apply_to->setFlat(true);
	m_apply_to->setText("A2");

	m_apply_current = new QPushButton(this);
	m_apply_current->setAutoDefault(false);
	m_apply_current->setToolTip("Apply to current window");
	m_apply_current->move(dx, 0);
	m_apply_current->resize(dx, dy);
	//m_apply_current->setFlat(true);
	m_apply_current->setText("A");

	m_preview = new QPushButton(this);
	m_preview->setAutoDefault(false);
	m_preview->setToolTip("Show Preview");
	m_preview->move(2*dx, 0);
	m_preview->resize(dx, dy);
	m_preview->setText("P");
	m_preview->setVisible(preview);
	

	m_reset = new QPushButton(this);
	m_reset->setAutoDefault(false);
	m_reset->setToolTip("Reset");
	m_reset->move(size().width() - dx, 0);
	m_reset->resize(dx, dy);
	m_reset->setText("R");

	this->setStyleSheet("QToolTip {border : 0px solid dark gray; background: solid dark gray; color: white}");

	this->setAutoFillBackground(true);
	QPalette pal = QPalette();
	pal.setColor(QPalette::Window, Qt::lightGray);
	this->setPalette(pal);

	connect(m_apply_to, &QPushButton::pressed, this, &Toolbar::setDownFalse);
}





ProcessDialog::ProcessDialog(QString name, const QSize& size, QWidget* parent, bool preview): QDialog(parent), m_name(name) {
	m_workspace = reinterpret_cast<FastStack*>(parent)->m_workspace;

	this->setWindowOpacity(0.95);
	installEventFilter(this);

	this->resize(size);
	m_tb = new Toolbar(this, preview);
	this->setFocus();

	this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	this->setAttribute(Qt::WA_DeleteOnClose);
}

ProcessDialog::ProcessDialog(QString name, const QSize& size, QMdiArea& workspace, QWidget* parent, bool preview) : QDialog(parent), m_workspace(&workspace), m_name(name) {

	this->setWindowTitle(m_name);

	this->setWindowOpacity(0.95);
	installEventFilter(this);
	this->resize(size);
	m_tb = new Toolbar(this, preview);

	//this->setFocusPolicy(Qt::StrongFocus);

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

	//delete m_progress_bar;

	for (auto sw : m_workspace->subWindowList()) {
		auto iwptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (iwptr->previewExists()) {
			QString str = iwptr->Preview()->windowTitle();
			if (str.contains(Name())) {
				switch (iwptr->Source().Bitdepth()) {
				case 8: {
					iwptr->ShowPreview();
					break;
				}
				case 16: {
					reinterpret_cast<ImageWindow16*>(iwptr)->ShowPreview();
					break;
				}
				case -32: {
					reinterpret_cast<ImageWindow32*>(iwptr)->ShowPreview();
					break;
				}
				}
				str.remove(Name());
				iwptr->Preview()->setWindowTitle(str);
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


	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		iwptr->ShowPreview();
		break;
	}
	case 16: {
		reinterpret_cast<ImageWindow16*>(iwptr)->ShowPreview();
		break;
	}
	case -32: {
		reinterpret_cast<ImageWindow32*>(iwptr)->ShowPreview();
		break;
	}
	}

	iwptr->Preview()->setWindowTitle(iwptr->ImageName() + " Preview: " + Name());
}

bool ProcessDialog::PreviewProcessNameMatches(const QString& preview_name)const {
	return (preview_name.sliced(preview_name.length() - m_name.length(), m_name.length()).compare(m_name) == 0) ? true : false;
}

bool ProcessDialog::isPreviewValid()const {

	if (m_workspace->subWindowList().size() == 0)
		return false;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	if (!iwptr->previewExists())
		return false;

	if (!PreviewProcessNameMatches(iwptr->Preview()->windowTitle()))
		return false;

	return true;
}






void DoubleValidator::fixup(QString& input) const {

	double val = input.toDouble();

	if (val > top())
		val = top();
	else if (val < bottom())
		val = bottom();

	input = QString::number(val, 'd', decimals());
}

//have line edit adjust width based on precision?
DoubleLineEdit::DoubleLineEdit(DoubleValidator* validator, QWidget* parent) : QLineEdit(parent) {
	this->setFocusPolicy(Qt::ClickFocus);
	this->setValidator(validator);
	validator->setParent(this);
	this->resize(85, 30);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::addTrailingZeros);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::removeEndDecimal);
}

DoubleLineEdit::DoubleLineEdit(const QString& contents, DoubleValidator* validator, QWidget* parent) : QLineEdit(contents, parent) {
	this->setFocusPolicy(Qt::ClickFocus);
	this->setValidator(validator);
	validator->setParent(this);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::addTrailingZeros);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::removeEndDecimal);
}

DoubleLineEdit::DoubleLineEdit(const QString& contents, DoubleValidator* validator, int max_length, QWidget* parent) : QLineEdit(contents, parent){
	this->setFocusPolicy(Qt::ClickFocus);
	this->setValidator(validator);
	validator->setParent(this);
	this->setMaxLength(max_length);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::addTrailingZeros);
	connect(this, &QLineEdit::editingFinished, this, &DoubleLineEdit::removeEndDecimal);
}

void DoubleLineEdit::setValue(float value) {
	QString str = QString::number(value);

	this->Validator()->fixup(str);

	if (str.length() > maxLength())
		str = str.sliced(0, maxLength());


	if (str[str.length() - 1] == '.')
		str.removeLast();

	this->setText(str);
}

void DoubleLineEdit::setValue(double value) {
	QString str = QString::number(value);

	this->Validator()->fixup(str);

	if (str.length() > maxLength())
		str = str.sliced(0, maxLength());


	if (str[str.length() - 1] == '.')
		str.removeLast();

	this->setText(str);
}

void DoubleLineEdit::setValue(int value) {
	
	if (value > this->Validator()->top())
		value = this->Validator()->top();
	else if (value < this->Validator()->bottom())
		value = this->Validator()->bottom();

	this->setText(QString::number(value));
}

void DoubleLineEdit::setText_Validated(const QString& text) {
	QString str = text;

	this->Validator()->fixup(str);

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

void DoubleLineEdit::addLabel(QLabel* label)const {
	int x = geometry().x();
	int y = geometry().y();
	int h = geometry().height();
	int fh = label->fontMetrics().height();
	int tw = label->fontMetrics().horizontalAdvance(label->text());

	label->move(x - tw, ((2 * y) + h - fh) / 2);
}
 
void DoubleLineEdit::addSlider(QSlider* slider)const {
	int x = geometry().topRight().x();
	int y = geometry().topRight().y();
	int h = geometry().height();
	int sh = slider->style()->pixelMetric(QStyle::PM_SliderThickness);

	slider->move(x + 15, ((2 * y) + h - sh) / 2);
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
