#pragma once
#include "pch.h"
#include "Image.h"


class MenuBar  : public QMenuBar
{
	Q_OBJECT

public:
	MenuBar(QWidget*parent);
	~MenuBar();

	QMenu* filemenu;// = addMenu(tr("&File"));
	QAction* open;
    QAction* save;
    QAction* save_as;

    Image8 img8;
    Image16 img16;
    Image32 img32;

    QMenu* processmenu;

    static void OpenImageDialog(QFileDialog& dialog, QFileDialog::AcceptMode  acceptMode) {
        static bool firstDialog = true;

        if (firstDialog) {
            firstDialog = false;
            const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
            dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
        }

        //QTextStream qout(stdout);
        //qout << "AAA" << Qt::endl;
        /*QStringList mimeTypeFilters;
        const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
            ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();

        for (const QByteArray& mimeTypeName : supportedMimeTypes)
            mimeTypeFilters.append(mimeTypeName);

        mimeTypeFilters.append("image/FITS");
        for (auto mimeTypeName : mimeTypeFilters)
            qout << mimeTypeName << Qt::endl;
        mimeTypeFilters.sort();

        dialog.setMimeTypeFilters(mimeTypeFilters);*/
        QStringList typelist = {
            "FITS file(*.fits *.fts *.fit)",
            "XISF file(*.xisf)",
            "TIFF file(*.tiff *tif)"
        };
        dialog.setNameFilters(typelist);

        dialog.selectNameFilter("FITS");
        dialog.setAcceptMode(acceptMode);
        if (acceptMode == QFileDialog::AcceptSave)
            dialog.setDefaultSuffix("fits");

    }

    static void SaveImageDialog(QFileDialog& dialog, QFileDialog::AcceptMode  acceptMode) {

    }

    static void SaveAsImageDialog(QFileDialog& dialog, QFileDialog::AcceptMode  acceptMode) {

    }

    void Open();

    void Save();

    void AddFileMenu();

    void AddProcessMenu();

	void AddAction();
};
