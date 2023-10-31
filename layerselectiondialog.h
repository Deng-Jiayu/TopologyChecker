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

    QSet<QgsVectorLayer*> getSelectedLayers();
    void selectLayer(QSet<QgsVectorLayer*> vec);

    QVector<QgsVectorLayer*> mLayers;
    void selectType(QString str);

signals:
    void finish();

private:
    Ui::LayerSelectionDialog ui;

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
