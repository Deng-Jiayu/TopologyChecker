#include "layerselectiondialog.h"
#include <qgsproject.h>
#include <qgsapplication.h>
#include <qfiledialog.h>

LayerSelectionDialog::LayerSelectionDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    initList();
    this->setWindowTitle(QStringLiteral("选择图层"));

    connect(ui.btnAddFile, &QPushButton::clicked, this, &LayerSelectionDialog::addFileDialog);
    connect(ui.btnAddDir, &QPushButton::clicked, this, &LayerSelectionDialog::addDirDialog);
    connect(ui.btnSelectAllLayers, &QPushButton::clicked, this, &LayerSelectionDialog::selectAllLayers);
    connect(ui.btnDeselectAllLayers, &QPushButton::clicked, this, &LayerSelectionDialog::deselectAllLayers);
    connect(ui.checkBoxPoint, &QCheckBox::stateChanged, this, &LayerSelectionDialog::showPoint);
    connect(ui.checkBoxLine, &QCheckBox::stateChanged, this, &LayerSelectionDialog::showLine);
    connect(ui.checkBoxPolygon, &QCheckBox::stateChanged, this, &LayerSelectionDialog::showPolygon);
    connect(ui.btnOk, &QPushButton::clicked, this, [&]() {
        this->hide();
        emit finish();
        });
    connect(ui.btnCancel, &QPushButton::clicked, this, [&]() { this->hide(); });
}

LayerSelectionDialog::~LayerSelectionDialog()
{
    qDebug() << "LayerSelectionDialog::~LayerSelectionDialog()";
}

void LayerSelectionDialog::initList()
{
    ui.listWidget->clear();
    mLayers.clear();
    itemLayerMap.clear();
    ui.checkBoxPoint->setChecked(true);
    ui.checkBoxLine->setChecked(true);
    ui.checkBoxPolygon->setChecked(true);

    // Collect layers
    for (QgsVectorLayer* layer : QgsProject::instance()->layers<QgsVectorLayer*>())
    {
        QListWidgetItem* item = new QListWidgetItem(layer->name());
        bool supportedGeometryType = true;
        if (layer->geometryType() == QgsWkbTypes::PointGeometry)
        {
            item->setIcon(QgsApplication::getThemeIcon("/mIconPointLayer.svg"));
        }
        else if (layer->geometryType() == QgsWkbTypes::LineGeometry)
        {
            item->setIcon(QgsApplication::getThemeIcon("/mIconLineLayer.svg"));
        }
        else if (layer->geometryType() == QgsWkbTypes::PolygonGeometry)
        {
            item->setIcon(QgsApplication::getThemeIcon("/mIconPolygonLayer.svg"));
        }
        else
        {
            supportedGeometryType = false;
        }

        if (supportedGeometryType)
        {
            item->setCheckState(Qt::Unchecked);
        }
        else
        {
            item->setCheckState(Qt::Unchecked);
            item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable));
        }
        mLayers.append(layer);
        itemLayerMap[item] = layer;
        ui.listWidget->addItem(item);
    }
}

void LayerSelectionDialog::addFileDialog()
{
    const QStringList filenames = QFileDialog::getOpenFileNames(this, QStringLiteral("选择文件"), QDir::currentPath(), "Shapefiles (*.shp);;All files (*.*)");
    if (filenames.empty())
        return;

    for (const QString& file : filenames)
    {
        addFile(file);
    }
}

void LayerSelectionDialog::addFile(QString path)
{
    if (path.isEmpty())
        return;
    QFileInfo fileInfo(path);
    QgsVectorLayer* layer = new QgsVectorLayer(path, fileInfo.baseName());

    if (!layer || !layer->isValid())
        return;

    QListWidgetItem* item = new QListWidgetItem(path);
    bool supportedGeometryType = true;
    if (layer->geometryType() == QgsWkbTypes::PointGeometry)
    {
        item->setIcon(QgsApplication::getThemeIcon("/mIconPointLayer.svg"));
    }
    else if (layer->geometryType() == QgsWkbTypes::LineGeometry)
    {
        item->setIcon(QgsApplication::getThemeIcon("/mIconLineLayer.svg"));
    }
    else if (layer->geometryType() == QgsWkbTypes::PolygonGeometry)
    {
        item->setIcon(QgsApplication::getThemeIcon("/mIconPolygonLayer.svg"));
    }
    else
    {
        supportedGeometryType = false;
    }

    if (supportedGeometryType)
    {
        item->setCheckState(Qt::Unchecked);
    }
    else
    {
        item->setCheckState(Qt::Unchecked);
        item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable));
    }

    mLayers.append(layer);
    itemLayerMap[item] = layer;
    ui.listWidget->addItem(item);
}

void LayerSelectionDialog::addDirDialog()
{
    QString filepath = QFileDialog::getExistingDirectory(this, QStringLiteral("选择目录"), QDir::currentPath());
    if (filepath.isEmpty())
        return;

    addDir(filepath);
}

void LayerSelectionDialog::addDir(QString path)
{
    if (path.isEmpty())
        return;

    QDir dir(path);

    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    foreach(const QFileInfo & fileInfo, fileInfoList)
    {
        if (fileInfo.isFile())
        {
            addFile(fileInfo.absoluteFilePath());
        }
        else if (fileInfo.isDir())
        {
            addDir(fileInfo.absoluteFilePath());
        }
    }
}

void LayerSelectionDialog::selectAllLayers()
{
    for (int row = 0, nRows = ui.listWidget->count(); row < nRows; ++row)
    {
        QListWidgetItem *item = ui.listWidget->item(row);
        if (item->isHidden())
            continue;
        if (item->flags().testFlag(Qt::ItemIsEnabled))
        {
            item->setCheckState(Qt::Checked);
        }
    }
}

void LayerSelectionDialog::deselectAllLayers()
{
    for (int row = 0, nRows = ui.listWidget->count(); row < nRows; ++row)
    {
        QListWidgetItem* item = ui.listWidget->item(row);
        if (item->flags().testFlag(Qt::ItemIsEnabled))
        {
            item->setCheckState(Qt::Unchecked);
        }
    }
}

void LayerSelectionDialog::showPoint()
{
    if (ui.checkBoxPoint->isChecked())
    {
        for (int row = 0, nRows = ui.listWidget->count(); row < nRows; ++row)
        {
            QListWidgetItem* item = ui.listWidget->item(row);
            if (itemLayerMap[item]->geometryType() == QgsWkbTypes::PointGeometry)
            {
                item->setHidden(false);
            }
        }
    }
    else
    {
        for (int row = 0, nRows = ui.listWidget->count(); row < nRows; ++row)
        {
            QListWidgetItem* item = ui.listWidget->item(row);
            if (itemLayerMap[item]->geometryType() == QgsWkbTypes::PointGeometry)
            {
                item->setCheckState(Qt::Unchecked);
                item->setHidden(true);
            }
        }
    }
}

void LayerSelectionDialog::showLine()
{
    if (ui.checkBoxLine->isChecked())
    {
        for (int row = 0, nRows = ui.listWidget->count(); row < nRows; ++row)
        {
            QListWidgetItem* item = ui.listWidget->item(row);
            if (itemLayerMap[item]->geometryType() == QgsWkbTypes::LineGeometry)
            {
                item->setHidden(false);
            }
        }
    }
    else
    {
        for (int row = 0, nRows = ui.listWidget->count(); row < nRows; ++row)
        {
            QListWidgetItem* item = ui.listWidget->item(row);
            if (itemLayerMap[item]->geometryType() == QgsWkbTypes::LineGeometry)
            {
                item->setCheckState(Qt::Unchecked);
                item->setHidden(true);
            }
        }
    }
}

void LayerSelectionDialog::showPolygon()
{
    if (ui.checkBoxPolygon->isChecked())
    {
        for (int row = 0, nRows = ui.listWidget->count(); row < nRows; ++row)
        {
            QListWidgetItem* item = ui.listWidget->item(row);
            if (itemLayerMap[item]->geometryType() == QgsWkbTypes::PolygonGeometry)
            {
                item->setHidden(false);
            }
        }
    }
    else
    {
        for (int row = 0, nRows = ui.listWidget->count(); row < nRows; ++row)
        {
            QListWidgetItem* item = ui.listWidget->item(row);
            if (itemLayerMap[item]->geometryType() == QgsWkbTypes::PolygonGeometry)
            {
                item->setCheckState(Qt::Unchecked);
                item->setHidden(true);
            }
        }
    }
}

QSet<QgsVectorLayer*> LayerSelectionDialog::getSelectedLayers()
{
    QSet<QgsVectorLayer*> vec;
    for (int row = 0, nRows = ui.listWidget->count(); row < nRows; ++row)
    {
        QListWidgetItem* item = ui.listWidget->item(row);
        if (item->checkState() == Qt::Checked)
        {
            vec.insert(itemLayerMap[item]);
        }
    }
    return vec;
}

void LayerSelectionDialog::selectLayer(QSet<QgsVectorLayer*> vec)
{
    for (int row = 0, nRows = ui.listWidget->count(); row < nRows; ++row)
    {
        QListWidgetItem* item = ui.listWidget->item(row);
        item->setCheckState(Qt::Unchecked);
    }

    for(auto layer : vec)
    {
        for (int row = 0, nRows = ui.listWidget->count(); row < nRows; ++row)
        {
            QListWidgetItem* item = ui.listWidget->item(row);
            if (layer->id() == itemLayerMap[item]->id())
            {
                item->setCheckState(Qt::Checked);
            }
        }
    }
}
