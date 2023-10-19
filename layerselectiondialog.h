#pragma once

#include <QDialog>
#include "ui_layerselectiondialog.h"
#include <qgsvectorlayer.h>
#include <QListWidgetItem>

class LayerSelectionDialog : public QDialog
{
	Q_OBJECT

public:
	LayerSelectionDialog(QWidget *parent = nullptr);
	~LayerSelectionDialog();

    QVector<QgsVectorLayer*> getSelectedLayers();
    void selectLayer(QVector<QgsVectorLayer*> vec);

signals:
    void finish();

private:
    Ui::LayerSelectionDialog ui;

    QVector<QgsVectorLayer*> selectedLayers;

    QMap<QListWidgetItem*, QgsVectorLayer*> itemLayerMap;


    void initList();
    void addFileDialog();
    void addFile(QString path);
    void addDirDialog();
    void addDir(QString path);
    void selectAllLayers();
    void deselectAllLayers();
    void showPoint();
    void showLine();
    void showPolygon();
};
