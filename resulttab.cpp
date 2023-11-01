#include "resulttab.h"
#include "ui_resulttab.h"
#include "checkerror.h"
#include <qgsmapcanvas.h>
#include <QDialog>
#include <qgsproject.h>
#include <QMessageBox>
#include <QFileDialog>
#include <qgsfileutils.h>
#include <qgsvectorfilewriter.h>
#include <qgsogrprovider.h>
#include <qgsvscrollarea.h>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QPlainTextEdit>

QString ResultTab::sSettingsGroup = QStringLiteral("/TopologyChecker/default_fix_methods/");

ResultTab::ResultTab(QgisInterface *iface, Checker *checker, QTabWidget *tabWidget, QWidget *parent)
    : QWidget(parent), ui(new Ui::ResultTab), mTabWidget(tabWidget), mIface(iface), mChecker(checker)
{
    ui->setupUi(this);
    ui->progressBarFixErrors->hide();
    mErrorCount = 0;
    mFixedCount = 0;

    connect(checker, &Checker::errorAdded, this, &ResultTab::addError);
    connect(checker, &Checker::errorUpdated, this, &ResultTab::updateError);
    connect(ui->tableWidgetErrors->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ResultTab::onSelectionChanged);
    connect(ui->btnOpenAttributeTable, &QAbstractButton::clicked, this, &ResultTab::openAttributeTable);
    connect(ui->checkBoxHighlight, &QAbstractButton::clicked, this, &ResultTab::highlightErrors);
    connect(QgsProject::instance(), static_cast<void (QgsProject::*)(const QStringList &)>(&QgsProject::layersWillBeRemoved), this, &ResultTab::checkRemovedLayer);
    connect(ui->btnExport, &QAbstractButton::clicked, this, &ResultTab::exportErrors);
    connect(ui->btnFix, &QAbstractButton::clicked, this, &ResultTab::fixCurrentError);
    connect(ui->btnErrorResolutionSettings, &QAbstractButton::clicked, this, &ResultTab::setDefaultResolutionMethods);
    connect(ui->btnFixWithDefault, &QAbstractButton::clicked, this, &ResultTab::fixErrorsWithDefault);
    connect(ui->btnClassify, &QPushButton::clicked, this, &ResultTab::classify);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &ResultTab::RedoCheck);

    bool allLayersEditable = true;
    for (const FeaturePool *featurePool : mChecker->featurePools().values())
    {
        if ((featurePool->layer()->dataProvider()->capabilities() & QgsVectorDataProvider::ChangeGeometries) == 0)
        {
            allLayersEditable = false;
            break;
        }
    }
    if (!allLayersEditable)
    {
        ui->btnFix->setEnabled(false);
        ui->btnFixWithDefault->setEnabled(false);
    }

    ui->tableWidgetErrors->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
    ui->tableWidgetErrors->resizeColumnToContents(0);
    ui->tableWidgetErrors->resizeColumnToContents(1);
    ui->tableWidgetErrors->horizontalHeader()->setStretchLastSection(true);
    // Not sure why, but this is needed...
    ui->tableWidgetErrors->setSortingEnabled(true);
    ui->tableWidgetErrors->setSortingEnabled(false);
}

ResultTab::~ResultTab()
{
    delete ui;
    delete mChecker;
    qDeleteAll(mCurrentRubberBands);
}

void ResultTab::finalize()
{
    ui->tableWidgetErrors->setSortingEnabled(true);
    if (!mChecker->getMessages().isEmpty())
    {
        QDialog dialog;
        dialog.setLayout(new QVBoxLayout());
        dialog.layout()->addWidget(new QLabel(QStringLiteral("以下检查出现错误：")));
        dialog.layout()->addWidget(new QPlainTextEdit(mChecker->getMessages().join(QLatin1Char('\n'))));
        QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal);
        dialog.layout()->addWidget(bbox);
        connect(bbox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(bbox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        dialog.setWindowTitle(tr("Check Errors Occurred"));
        dialog.exec();
    }
}

void ResultTab::setRowStatus(int row, const QColor &color, const QString &message, bool selectable)
{
    for (int col = 0, nCols = ui->tableWidgetErrors->columnCount(); col < nCols; ++col)
    {
        QTableWidgetItem *item = ui->tableWidgetErrors->item(row, col);
        item->setBackground(color);
        if (!selectable)
        {
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
            item->setForeground(Qt::lightGray);
        }
    }
    ui->tableWidgetErrors->item(row, 5)->setText(message);
}
#include <qgspolygon.h>
bool ResultTab::exportErrorsDo(const QString &file)
{
    QList<QPair<QString, QString>> attributes;
    attributes.append(qMakePair(QStringLiteral("layer"), QStringLiteral("String;30;")));
    attributes.append(qMakePair(QStringLiteral("FeatureID"), QStringLiteral("String;20;")));
    attributes.append(qMakePair(QStringLiteral("Error"), QStringLiteral("String;80;")));
    attributes.append(qMakePair(QStringLiteral("coordinate"), QStringLiteral("String;80;")));
    attributes.append(qMakePair(QStringLiteral("value"), QStringLiteral("String;80;")));
    attributes.append(qMakePair(QStringLiteral("resolution"), QStringLiteral("String;80;")));

    QFileInfo fi(file);
    QString ext = fi.suffix();
    QString driver = QgsVectorFileWriter::driverForExtension(ext);

    QString createError;
    bool success = QgsOgrProviderUtils::createEmptyDataSource(file, driver, "UTF-8", QgsWkbTypes::Point, attributes, QgsProject::instance()->crs(), createError);
    if (!success)
    {
        return false;
    }

    const QgsVectorLayer::LayerOptions options{QgsProject::instance()->transformContext()};
    QgsVectorLayer *layer = new QgsVectorLayer(file, QFileInfo(file).baseName(), QStringLiteral("ogr"), options);
    if (!layer->isValid())
    {
        delete layer;
        return false;
    }

    int fieldLayer = layer->fields().lookupField(QStringLiteral("layer"));
    int fieldFeatureId = layer->fields().lookupField(QStringLiteral("FeatureID"));
    int fieldErrDesc = layer->fields().lookupField(QStringLiteral("Error"));
    int fieldErrPos = layer->fields().lookupField(QStringLiteral("coordinate"));
    int fieldErrValue = layer->fields().lookupField(QStringLiteral("value"));
    int fieldErrFix = layer->fields().lookupField(QStringLiteral("resolution"));
    for (int row = 0, nRows = ui->tableWidgetErrors->rowCount(); row < nRows; ++row)
    {
        CheckError *error = ui->tableWidgetErrors->item(row, 0)->data(Qt::UserRole).value<CheckError *>();
        QString layerName = QString();
        const QString layerId = error->layerId();
        if (mChecker->featurePools().keys().contains(layerId))
        {
            QgsVectorLayer *srcLayer = mChecker->featurePools()[layerId]->layer();
            layerName = srcLayer->name();
        }

        QgsFeature f(layer->fields());
        f.setAttribute(fieldLayer, layerName);
        f.setAttribute(fieldFeatureId, error->featureId());
        f.setAttribute(fieldErrDesc, error->description());
        int prec = 7 - std::floor(std::max(0., std::log10(std::max(error->location().x(), error->location().y()))));
        QString posStr = QStringLiteral("%1, %2").arg(error->location().x(), 0, 'f', prec).arg(error->location().y(), 0, 'f', prec);
        f.setAttribute(fieldErrPos, posStr);
        f.setAttribute(fieldErrValue, error->value());
        QString status;
        if (error->status() == CheckError::StatusFixed)
        {
            status = QStringLiteral("修复：") + error->resolutionMessage();
        }
        else if (error->status() == CheckError::StatusFixFailed)
        {
            status = QStringLiteral("修复失败：") + error->resolutionMessage();
        }
        f.setAttribute(fieldErrFix, status);
        f.setGeometry(QgsGeometry::fromPointXY(error->location()));
        layer->dataProvider()->addFeatures(QgsFeatureList() << f);
    }

    // Remove existing layer with same uri
    QStringList toRemove;
    for (QgsMapLayer *maplayer : QgsProject::instance()->mapLayers())
    {
        if (qobject_cast<QgsVectorLayer *>(maplayer) &&
            static_cast<QgsVectorLayer *>(maplayer)->dataProvider()->dataSourceUri() == layer->dataProvider()->dataSourceUri())
        {
            toRemove.append(maplayer->id());
        }
    }
    if (!toRemove.isEmpty())
    {
        QgsProject::instance()->removeMapLayers(toRemove);
    }

    QgsProject::instance()->addMapLayers(QList<QgsMapLayer *>() << layer);
    return true;
}

void ResultTab::updateComboBox()
{
    int row = ui->tableWidgetErrors->currentRow();
    CheckError *error = ui->tableWidgetErrors->item(row, 0)->data(Qt::UserRole).value<CheckError *>();
    const QList<CheckResolutionMethod> resolutionMethods = error->check()->availableResolutionMethods();
    ui->mComboBox->clear();
    int i = 0;
    for (const CheckResolutionMethod &method : resolutionMethods)
    {
        ui->mComboBox->addItem(method.name(), i);
        ++i;
    }
}

void ResultTab::addError(CheckError *error)
{
    bool sortingWasEnabled = ui->tableWidgetErrors->isSortingEnabled();
    if (sortingWasEnabled)
        ui->tableWidgetErrors->setSortingEnabled(false);

    int row = ui->tableWidgetErrors->rowCount();
    int prec = 7 - std::floor(std::max(0., std::log10(std::max(error->location().x(), error->location().y()))));
    QString posStr = QStringLiteral("%1, %2").arg(error->location().x(), 0, 'f', prec).arg(error->location().y(), 0, 'f', prec);

    ui->tableWidgetErrors->insertRow(row);
    QTableWidgetItem *idItem = new QTableWidgetItem();
    idItem->setData(Qt::EditRole, error->featureId() != FID_NULL ? QVariant(error->featureId()) : QVariant());
    ui->tableWidgetErrors->setItem(row, 0, new QTableWidgetItem(!error->layerId().isEmpty() ? mChecker->featurePools()[error->layerId()]->layer()->name() : ""));
    ui->tableWidgetErrors->setItem(row, 1, idItem);
    ui->tableWidgetErrors->setItem(row, 2, new QTableWidgetItem(error->description()));
    ui->tableWidgetErrors->setItem(row, 3, new QTableWidgetItem(posStr));
    QTableWidgetItem *valueItem = new QTableWidgetItem();
    valueItem->setData(Qt::EditRole, error->value());
    ui->tableWidgetErrors->setItem(row, 4, valueItem);
    ui->tableWidgetErrors->setItem(row, 5, new QTableWidgetItem(QString()));
    ui->tableWidgetErrors->item(row, 0)->setData(Qt::UserRole, QVariant::fromValue(error));
    ++mErrorCount;
    ui->labelErrorCount->setText(QStringLiteral("错误数目：") + QString::number(mErrorCount) + QStringLiteral("，修复数目：") + QString::number(mFixedCount));
    mErrorMap.insert(error, QPersistentModelIndex(ui->tableWidgetErrors->model()->index(row, 0)));

    if (sortingWasEnabled)
        ui->tableWidgetErrors->setSortingEnabled(true);
}

void ResultTab::updateError(CheckError *error, bool statusChanged)
{
    if (!mErrorMap.contains(error))
    {
        return;
    }
    // Disable sorting to prevent crashes: if i.e. sorting by col 0, as soon as the item(row, 0) is set,
    // the row is potentially moved due to sorting, and subsequent item(row, col) reference wrong item
    bool sortingWasEnabled = ui->tableWidgetErrors->isSortingEnabled();
    if (sortingWasEnabled)
        ui->tableWidgetErrors->setSortingEnabled(false);

    int row = mErrorMap.value(error).row();
    int prec = 7 - std::floor(std::max(0., std::log10(std::max(error->location().x(), error->location().y()))));
    QString posStr = QStringLiteral("%1, %2").arg(error->location().x(), 0, 'f', prec).arg(error->location().y(), 0, 'f', prec);

    ui->tableWidgetErrors->item(row, 3)->setText(posStr);
    ui->tableWidgetErrors->item(row, 4)->setData(Qt::EditRole, error->value());
    if (error->status() == CheckError::StatusFixed)
    {
        setRowStatus(row, Qt::green, tr("Fixed: %1").arg(error->resolutionMessage()), true);
        ++mFixedCount;
    }
    else if (error->status() == CheckError::StatusFixFailed)
    {
        setRowStatus(row, Qt::red, tr("Fix failed: %1").arg(error->resolutionMessage()), true);
    }
    else if (error->status() == CheckError::StatusObsolete)
    {
        //ui->tableWidgetErrors->setRowHidden(row, true);
        setRowStatus( row, Qt::gray, tr( "Obsolete" ), false );
        --mErrorCount;
    }
    ui->labelErrorCount->setText(QStringLiteral("错误数目：") + QString::number(mErrorCount) + QStringLiteral("，修复数目：") + QString::number(mFixedCount));

    if (sortingWasEnabled)
        ui->tableWidgetErrors->setSortingEnabled(true);
}

void ResultTab::onSelectionChanged(const QItemSelection &newSel, const QItemSelection &)
{
    updateComboBox();

    QModelIndex idx = ui->tableWidgetErrors->currentIndex();
    if (idx.isValid() && !ui->tableWidgetErrors->isRowHidden(idx.row()) && newSel.contains(idx))
    {
        highlightErrors();
    }
    else
    {
        qDeleteAll(mCurrentRubberBands);
        mCurrentRubberBands.clear();
    }
    ui->btnOpenAttributeTable->setEnabled(!newSel.isEmpty());
}

void ResultTab::highlightErrors(bool current)
{
    qDeleteAll(mCurrentRubberBands);
    mCurrentRubberBands.clear();

    QList<QTableWidgetItem *> items;
    QVector<QgsPointXY> errorPositions;
    QgsRectangle totextent;

    if (current)
    {
        items.append(ui->tableWidgetErrors->currentItem());
    }
    else
    {
        items.append(ui->tableWidgetErrors->selectedItems());
    }
    for (QTableWidgetItem *item : qgis::as_const(items))
    {
        CheckError *error = ui->tableWidgetErrors->item(item->row(), 0)->data(Qt::UserRole).value<CheckError *>();

        const QgsGeometry geom = error->geometry();
        if (ui->checkBoxHighlight->isChecked() && !geom.isNull())
        {
            QgsRubberBand *featureRubberBand = new QgsRubberBand(mIface->mapCanvas());
            featureRubberBand->addGeometry(geom, nullptr);
            featureRubberBand->setWidth(5);
            featureRubberBand->setColor(Qt::yellow);
            mCurrentRubberBands.append(featureRubberBand);
        }

        if (ui->radioButtonError->isChecked() || current || error->status() == CheckError::StatusFixed)
        {
            QgsRubberBand *pointRubberBand = new QgsRubberBand(mIface->mapCanvas(), QgsWkbTypes::PointGeometry);
            pointRubberBand->addPoint(error->location());
            pointRubberBand->setWidth(20);
            pointRubberBand->setColor(Qt::red);
            mCurrentRubberBands.append(pointRubberBand);
            errorPositions.append(error->location());
        }
        else if (ui->radioButtonFeature->isChecked())
        {
            QgsRectangle geomextent = error->affectedAreaBBox();
            if (totextent.isEmpty())
            {
                totextent = geomextent;
            }
            else
            {
                totextent.combineExtentWith(geomextent);
            }
        }
    }

    // If error positions positions are marked, pan to the center of all positions,
    // and zoom out if necessary to make all points fit.
    if (!errorPositions.isEmpty())
    {
        double cx = 0., cy = 0.;
        QgsRectangle pointExtent(errorPositions.first(), errorPositions.first());
        Q_FOREACH (const QgsPointXY &p, errorPositions)
        {
            cx += p.x();
            cy += p.y();
            pointExtent.include(p);
        }
        QgsPointXY center = QgsPointXY(cx / errorPositions.size(), cy / errorPositions.size());
        if (totextent.isEmpty())
        {
            QgsRectangle extent = mIface->mapCanvas()->extent();
            QgsVector diff = center - extent.center();
            extent.setXMinimum(extent.xMinimum() + diff.x());
            extent.setXMaximum(extent.xMaximum() + diff.x());
            extent.setYMinimum(extent.yMinimum() + diff.y());
            extent.setYMaximum(extent.yMaximum() + diff.y());
            extent.combineExtentWith(pointExtent);
            totextent = extent;
        }
        else
        {
            totextent.combineExtentWith(pointExtent);
        }
    }

    if (!totextent.isEmpty())
    {
        mIface->mapCanvas()->setExtent(totextent, true);
    }
    mIface->mapCanvas()->refresh();
}

void ResultTab::openAttributeTable()
{
    QMap<QString, QSet<QgsFeatureId>> ids;
    for (QModelIndex idx : ui->tableWidgetErrors->selectionModel()->selectedRows())
    {
        CheckError *error = ui->tableWidgetErrors->item(idx.row(), 0)->data(Qt::UserRole).value<CheckError *>();
        QgsFeatureId id = error->featureId();
        if (id >= 0)
        {
            ids[error->layerId()].insert(id);
        }
    }
    if (ids.isEmpty())
    {
        return;
    }
    for (const QString &layerId : ids.keys())
    {
        QStringList expr;
        for (QgsFeatureId id : ids[layerId])
        {
            expr.append(QStringLiteral("$id = %1 ").arg(id));
        }
        if (mAttribTableDialogs[layerId])
        {
            mAttribTableDialogs[layerId]->close();
        }
        mAttribTableDialogs[layerId] = mIface->showAttributeTable(mChecker->featurePools()[layerId]->layer(), expr.join(QLatin1String(" or ")));
    }
}

void ResultTab::checkRemovedLayer(const QStringList &ids)
{
    bool requiredLayersRemoved = false;
    for (const QString &layerId : mChecker->featurePools().keys())
    {
        if (ids.contains(layerId))
        {
            if (isEnabled())
                requiredLayersRemoved = true;
        }
    }
    if (requiredLayersRemoved)
    {
        if (mTabWidget->currentWidget() == this)
        {
            QMessageBox::critical(this, tr("Remove Layer"), tr("One or more layers have been removed."));
        }
        setEnabled(false);
        qDeleteAll(mCurrentRubberBands);
        mCurrentRubberBands.clear();
    }
}

void ResultTab::exportErrors()
{
    QString initialdir;
    QDir dir = QFileInfo(mChecker->featurePools().first()->layer()->dataProvider()->dataSourceUri()).dir();
    if (dir.exists())
    {
        initialdir = dir.absolutePath();
    }

    QString selectedFilter;
    QString file = QFileDialog::getSaveFileName(this, QStringLiteral("选择输出文件"), initialdir, QgsVectorFileWriter::fileFilterString(), &selectedFilter);
    if (file.isEmpty())
    {
        return;
    }

    file = QgsFileUtils::addExtensionFromFilter(file, selectedFilter);
    if (!exportErrorsDo(file))
    {
        QMessageBox::critical(this, tr("Export Errors"), tr("Failed to export errors to %1.").arg(QDir::toNativeSeparators(file)));
    }
}

void ResultTab::fixCurrentError()
{
    int row = ui->tableWidgetErrors->currentRow();
    CheckError *error = ui->tableWidgetErrors->item(row, 0)->data(Qt::UserRole).value<CheckError *>();
    mChecker->fixError(error, ui->mComboBox->currentData().toInt(), true);
}

void ResultTab::fixErrorsWithDefault()
{
    int sum = ui->tableWidgetErrors->rowCount();
    QList<CheckError *> errors;

    for (int i = 0; i < sum; ++i)
    {
        CheckError *error = ui->tableWidgetErrors->item(i, 0)->data(Qt::UserRole).value<CheckError *>();
        if (error->status() < CheckError::StatusFixed)
        {
            errors.append(error);
        }
    }

    if (errors.isEmpty())
    {
        return;
    }
    if (QMessageBox::Yes != QMessageBox::question(this, tr("Fix Errors"), tr("Do you want to fix %1 errors?").arg(errors.size()), QMessageBox::Yes, QMessageBox::No))
    {
        return;
    }

    // Disable sorting while fixing errors
    ui->tableWidgetErrors->setSortingEnabled(false);

    // clear rubberbands
    qDeleteAll(mCurrentRubberBands);
    mCurrentRubberBands.clear();

    //! Fix errors
    mCloseable = false;

    setCursor(Qt::WaitCursor);
    ui->progressBarFixErrors->setVisible(true);
    ui->progressBarFixErrors->setRange(0, errors.size());

    for (CheckError *error : qgis::as_const(errors))
    {
        int fixMethod = QgsSettings().value(sSettingsGroup + error->check()->id(), QVariant::fromValue<int>(0)).toInt();

        mChecker->fixError(error, fixMethod);
        ui->progressBarFixErrors->setValue(ui->progressBarFixErrors->value() + 1);
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    ui->progressBarFixErrors->hide();

    unsetCursor();

    for (const QString &layerId : mChecker->featurePools().keys())
    {
        mChecker->featurePools()[layerId]->layer()->triggerRepaint();
    }

    mCloseable = true;
    ui->tableWidgetErrors->setSortingEnabled(true);
}

void ResultTab::setDefaultResolutionMethods()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("设置修复函数"));

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QgsVScrollArea *scrollArea = new QgsVScrollArea(&dialog);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(new QLabel(QStringLiteral("选择修复方法：")));
    layout->addWidget(scrollArea);

    QWidget *scrollAreaContents = new QWidget(scrollArea);
    QVBoxLayout *scrollAreaLayout = new QVBoxLayout(scrollAreaContents);

    for (const Check *check : mChecker->getChecks())
    {
        QGroupBox *groupBox = new QGroupBox(scrollAreaContents);
        groupBox->setTitle(check->description());
        groupBox->setFlat(true);

        QVBoxLayout *groupBoxLayout = new QVBoxLayout(groupBox);
        groupBoxLayout->setContentsMargins(2, 0, 2, 2);
        QButtonGroup *radioGroup = new QButtonGroup(groupBox);
        radioGroup->setProperty("errorType", check->id());
        int checkedId = QgsSettings().value(sSettingsGroup + check->id(), QVariant::fromValue<int>(0)).toInt();
        const QList<CheckResolutionMethod> resolutionMethods = check->availableResolutionMethods();
        for (const CheckResolutionMethod &method : resolutionMethods)
        {
            QRadioButton *radio = new QRadioButton(method.name(), groupBox);
            radio->setChecked(method.id() == checkedId);
            groupBoxLayout->addWidget(radio);
            radioGroup->addButton(radio, method.id());
        }
        connect(radioGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &ResultTab::storeDefaultResolutionMethod);

        scrollAreaLayout->addWidget(groupBox);
    }
    scrollAreaLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));
    scrollArea->setWidget(scrollAreaContents);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttonBox);
    dialog.exec();
}

void ResultTab::storeDefaultResolutionMethod(int id) const
{
    QString errorType = qobject_cast<QButtonGroup *>(QObject::sender())->property("errorType").toString();
    QgsSettings().setValue(sSettingsGroup + errorType, id);
}


void ResultTab::classify()
{
    if(mChecker == nullptr) return;

    if(mClassifyDialog == nullptr)
    {
        mClassifyDialog = new ClassifyDialog(this);
        connect(mClassifyDialog, &ClassifyDialog::ClassifyDone, this, &ResultTab::doClassify);
        mClassifyDialog->initList(mChecker->getChecks());
    }

    mClassifyDialog->show();

}

void ResultTab::doClassify()
{
    mErrorCount = 0;
    mFixedCount = 0;
    mClassifyDialog->hide();
    QStringList selectedTypes = mClassifyDialog->selectedType();

    int sum = ui->tableWidgetErrors->rowCount();

    for (int i = 0; i < sum; ++i)
    {
        ui->tableWidgetErrors->showRow(i);
        CheckError *error = ui->tableWidgetErrors->item(i, 0)->data(Qt::UserRole).value<CheckError *>();
        if (!selectedTypes.contains(error->check()->description()))
        {
            ui->tableWidgetErrors->setRowHidden(i, true);
        }
        else
        {
            ++mErrorCount;
            if (error->status() == CheckError::StatusFixed)
            {
                ++mFixedCount;
            }
            else if (error->status() == CheckError::StatusObsolete)
            {
                --mErrorCount;
            }
        }
    }
    ui->labelErrorCount->setText(QStringLiteral("错误数目：") + QString::number(mErrorCount) + QStringLiteral("，修复数目：") + QString::number(mFixedCount));
}

void ResultTab::RedoCheck()
{

}
