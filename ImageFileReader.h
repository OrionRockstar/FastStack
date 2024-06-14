#pragma once
struct Status {
	bool m_success = true;
	QString m_message = "";

	explicit operator bool()const { return m_success; }
};

class ImageFileReader {
	QMdiArea* m_workspace;
public:
	ImageFileReader(QMdiArea* workspace);

	Status Read(std::filesystem::path file_path);
};

