﻿#include "checkitemdialog.h"
#include "ui_checkitemdialog.h"
#include <QDebug>

CheckItemDialog::CheckItemDialog(QgisInterface *iface, CheckItem *item, QWidget *parent)
    : QDialog(parent), ui(new Ui::CheckItemDialog), iface(iface), mcheckItem(item)
{
    ui->setupUi(this);
    ui->textShortHelp->setAcceptRichText(true);
    ui->btnCancel->hide();
    ui->progressBar->hide();
    ui->labelStatus->hide();
    this->resize(900, 650);

    layerSelectDialog = new LayerSelectionDialog(this);

    connect(ui->tableWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CheckItemDialog::onSelectionChanged);
    connect(ui->btnSaveEdit, &QPushButton::clicked, this, &CheckItemDialog::saveEdit);
    connect(ui->btnDeleteCheck, &QPushButton::clicked, this, &CheckItemDialog::deleteCheck);
    connect(ui->btnRun, &QPushButton::clicked, this, &CheckItemDialog::run);

    initTable();
    initParaTable();
    initConnection();
}

CheckItemDialog::~CheckItemDialog()
{
    qDebug() << "CheckItemDialog::~CheckItemDialog()";
    delete ui;
}

void CheckItemDialog::set()
{
    ui->groupBoxPoint->setCollapsed(true);
    ui->groupBoxLine->setCollapsed(true);
    ui->groupBoxPolygon->setCollapsed(true);
    ui->groupBoxProperties->setCollapsed(true);
}

void CheckItemDialog::initTable()
{
    while (ui->tableWidget->rowCount() > 0)
    {
        ui->tableWidget->removeRow(0);
    }
    mCheckMap.clear();

    for (int i = 0; i < mcheckItem->sets.size(); ++i)
    {
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);

        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(mcheckItem->sets[i].name));
        mCheckMap[row] = &mcheckItem->sets[i];
    }
}

void CheckItemDialog::initParaTable()
{
    while (ui->tableWidgetPara->rowCount() > 0)
        ui->tableWidgetPara->removeRow(0);

    ui->tableWidgetPara->setRowCount(9);

    ui->tableWidgetPara->setItem(0, 0, new QTableWidgetItem(QStringLiteral("点图层")));
    layerA = new QPushButton(QStringLiteral("..."), this);
    ui->tableWidgetPara->setCellWidget(0, 1, layerA);
    connect(layerA, &QPushButton::clicked, this, &CheckItemDialog::selectLayerA);

    ui->tableWidgetPara->setItem(1, 0, new QTableWidgetItem(QStringLiteral("线图层")));
    layerB = new QPushButton(QStringLiteral("..."), this);
    ui->tableWidgetPara->setCellWidget(1, 1, layerB);
    connect(layerB, &QPushButton::clicked, this, &CheckItemDialog::selectLayerB);

    ui->tableWidgetPara->setItem(2, 0, new QTableWidgetItem(QStringLiteral("其他图层")));
    layerC = new QPushButton(QStringLiteral("..."), this);
    ui->tableWidgetPara->setCellWidget(2, 1, layerC);

    ui->tableWidgetPara->setItem(3, 0, new QTableWidgetItem(QStringLiteral("长度上限")));
    doubleSpinBoxMax = new QgsDoubleSpinBox(this);
    doubleSpinBoxMax->setDecimals(6);
    doubleSpinBoxMax->setMaximum(999999999.000000000000000);
    ui->tableWidgetPara->setCellWidget(3, 1, doubleSpinBoxMax);
    ui->tableWidgetPara->setItem(4, 0, new QTableWidgetItem(QStringLiteral("长度下限")));
    doubleSpinBoxMin = new QgsDoubleSpinBox(this);
    doubleSpinBoxMin->setDecimals(6);
    doubleSpinBoxMin->setMaximum(999999999.000000000000000);
    ui->tableWidgetPara->setCellWidget(4, 1, doubleSpinBoxMin);

    ui->tableWidgetPara->setItem(5, 0, new QTableWidgetItem(QStringLiteral("图层对应模式")));
    comboBoxLayerMode = new QComboBox(this);
    comboBoxLayerMode->addItem(QStringLiteral("一对多"));
    comboBoxLayerMode->addItem(QStringLiteral("一对一"));
    ui->tableWidgetPara->setCellWidget(5, 1, comboBoxLayerMode);
    connect(comboBoxLayerMode, &QComboBox::currentTextChanged, this, &CheckItemDialog::setLayerMode);

    ui->tableWidgetPara->setItem(6, 0, new QTableWidgetItem(QStringLiteral("角度")));
    doubleSpinBoxAngle = new QgsDoubleSpinBox(this);
    doubleSpinBoxAngle->setObjectName(QString::fromUtf8("doubleSpinBoxAngle"));
    doubleSpinBoxAngle->setDecimals(6);
    doubleSpinBoxAngle->setMaximum(360.000000000000000);
    ui->tableWidgetPara->setCellWidget(6, 1, doubleSpinBoxAngle);

    ui->tableWidgetPara->setItem(7, 0, new QTableWidgetItem(QStringLiteral("排除端点")));
    excludeEndpoint = new QCheckBox(this);
    excludeEndpoint->setObjectName(QString::fromUtf8("excludeEndpoint"));
    ui->tableWidgetPara->setCellWidget(7, 1, excludeEndpoint);

    ui->tableWidgetPara->setItem(8, 0, new QTableWidgetItem(QStringLiteral("字段名称")));
    attr = new QLineEdit(this);
    attr->setObjectName(QString::fromUtf8("attr"));
    ui->tableWidgetPara->setCellWidget(8, 1, attr);

    for (int i = 0; i < ui->tableWidgetPara->rowCount(); ++i)
    {
        ui->tableWidgetPara->hideRow(i);
    }

}

void CheckItemDialog::initConnection()
{
    QList<QPushButton*> list = ui->scrollArea->findChildren<QPushButton*>();
    for (int i = 0; i < list.size(); i++)
    {
        QPushButton* btn = list.at(i);
        nameToBtn[btn->text()] = btn;
        connect(btn, &QPushButton::clicked, this, [&](){
            QPushButton *p = qobject_cast<QPushButton *>(sender());
            if(mCheckSet) delete mCheckSet;
            mCheckSet = new CheckSet(p->text());
            setParaUi(p);
            setText(p);
        });
    }
}

void CheckItemDialog::setBtnText(QPushButton* btn, QVector<QgsVectorLayer*> vec)
{
    if (vec.empty()) {
        btn->setText(QStringLiteral("..."));
        return;
    }
    QString str;
    for (auto layer : vec)
    {
        str += layer->name();
        str += ", ";
    }
    str.remove(str.length() - 2, 2);
    btn->setText(str);
}

void CheckItemDialog::setLayerModeText()
{
    if (mCheckSet->oneToOne == false)
        comboBoxLayerMode->setCurrentIndex(0);
    else
        comboBoxLayerMode->setCurrentIndex(1);
}

void CheckItemDialog::setAttrText()
{
    attr->setText(mCheckSet->attr);
}

void CheckItemDialog::setParaUi(QPushButton *btn)
{
    for (int i = 0; i < ui->tableWidgetPara->rowCount(); ++i)
        ui->tableWidgetPara->showRow(i);
    if(btn == ui->btnPointOnLine || btn == ui->btnPointOnLineEnd || btn == ui->btnPointOnLineNode
        || btn == ui->btnPointOnBoundary || btn == ui->btnPointInPolygon || btn == ui->btnLineLayerIntersection
        || btn == ui->btnLineLayerOverlap || btn == ui->btnLineCoveredByLine || btn == ui->btnLineCoveredByBoundary
        || btn == ui->btnLineEndOnPoint || btn == ui->btnLineInPolygon || btn == ui->btnPolygonLayerOverlap
        || btn == ui->btnPolygonCoveredByPolygon || btn == ui->btnPolygonInPolygon) {
        ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("点图层"));
        ui->tableWidgetPara->item(1, 0)->setText(QStringLiteral("线图层"));
        setBtnText(layerA, mCheckSet->layersA);
        setBtnText(layerB, mCheckSet->layersB);

        ui->tableWidgetPara->hideRow(2);
        ui->tableWidgetPara->hideRow(3);
        ui->tableWidgetPara->hideRow(4);
        ui->tableWidgetPara->hideRow(5);
        ui->tableWidgetPara->hideRow(6);
        ui->tableWidgetPara->hideRow(7);
        ui->tableWidgetPara->hideRow(8);
        if(btn == ui->btnPointOnBoundary || btn == ui->btnPointInPolygon) ui->tableWidgetPara->item(1, 0)->setText(QStringLiteral("面图层"));
        else if(btn == ui->btnLineLayerIntersection) {
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("线层1"));
            ui->tableWidgetPara->item(1, 0)->setText(QStringLiteral("线层2"));
            ui->tableWidgetPara->showRow(7);
            ui->tableWidgetPara->item(7, 0)->setText(QStringLiteral("排除端点"));
            excludeEndpoint->setChecked(mCheckSet->excludeEndpoint);
        }else if(btn == ui->btnLineLayerOverlap || btn == ui->btnLineCoveredByLine){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("线层1"));
            ui->tableWidgetPara->item(1, 0)->setText(QStringLiteral("线层2"));
        }else if(btn == ui->btnLineCoveredByBoundary || btn == ui->btnLineInPolygon){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("线图层"));
            ui->tableWidgetPara->item(1, 0)->setText(QStringLiteral("面图层"));
        }else if(btn == ui->btnLineEndOnPoint){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("线图层"));
            ui->tableWidgetPara->item(1, 0)->setText(QStringLiteral("点图层"));
        }else if(btn == ui->btnPolygonLayerOverlap){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("面层1"));
            ui->tableWidgetPara->item(1, 0)->setText(QStringLiteral("面层2"));
            ui->tableWidgetPara->showRow(3);
            ui->tableWidgetPara->item(3 ,0)->setText(QStringLiteral("面积上限 (map units sqr.)"));
            doubleSpinBoxMax->setValue(mCheckSet->upperLimit);
        }else if(btn == ui->btnPolygonCoveredByPolygon || btn == ui->btnPolygonInPolygon){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("面层1"));
            ui->tableWidgetPara->item(1, 0)->setText(QStringLiteral("面层2"));
        }
    }
    else if(btn == ui->btnPointDuplicate || btn == ui->btnLineDuplicate || btn == ui->btnPolygonDuplicate
               || btn == ui->btnLineIntersection || btn == ui->btnLineSelfIntersection || btn == ui->btnLineOverlap
               || btn == ui->btnLineSelfOverlap || btn == ui->btnDangle || btn == ui->btnTurnback
               || btn == ui->btnSegmentLength || btn == ui->btnLength || btn == ui->btnPolygonOverlap
               || btn == ui->btnGap || btn == ui->btnHole || btn == ui->btnConvexhull
               || btn == ui->btnArea || btn == ui->btnClockwise || btn == ui->btnAngle
               || btn == ui->btnInvalid || btn == ui->btnDuplicate || btn == ui->btnCollinear
               || btn == ui->btnDuplicateNode || btn == ui->btnInvalidAttr)
    {
        ui->tableWidgetPara->hideRow(1);
        ui->tableWidgetPara->hideRow(2);
        ui->tableWidgetPara->hideRow(3);
        ui->tableWidgetPara->hideRow(4);
        ui->tableWidgetPara->hideRow(5);
        ui->tableWidgetPara->hideRow(6);
        ui->tableWidgetPara->hideRow(7);
        ui->tableWidgetPara->hideRow(8);
        if(btn == ui->btnPointDuplicate)
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("点图层"));
        else if(btn == ui->btnLineDuplicate || btn == ui->btnLineOverlap || btn == ui->btnDangle)
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("线图层"));
        else if(btn == ui->btnPolygonDuplicate || btn == ui->btnHole)
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("面图层"));
        else if(btn == ui->btnLineIntersection || btn == ui->btnLineSelfIntersection){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("线图层"));
            ui->tableWidgetPara->showRow(7);
            ui->tableWidgetPara->item(7, 0)->setText(QStringLiteral("排除端点"));
        }else if(btn == ui->btnLineSelfOverlap){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("线图层"));
            ui->tableWidgetPara->showRow(7);
            ui->tableWidgetPara->item(7, 0)->setText(QStringLiteral("多部份要素"));
        }else if(btn == ui->btnTurnback){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("线图层"));
            ui->tableWidgetPara->showRow(6);
            doubleSpinBoxAngle->setValue(mCheckSet->angle);
        }else if(btn == ui->btnAngle){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("面图层"));
            ui->tableWidgetPara->showRow(6);
            doubleSpinBoxAngle->setValue(mCheckSet->angle);
        }else if(btn == ui->btnSegmentLength || btn == ui->btnLength){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("线图层"));
            ui->tableWidgetPara->showRow(3);
            ui->tableWidgetPara->item(3 ,0)->setText(QStringLiteral("长度上限 (map units)"));
            doubleSpinBoxMax->setValue(mCheckSet->upperLimit);
            ui->tableWidgetPara->showRow(4);
            ui->tableWidgetPara->item(4 ,0)->setText(QStringLiteral("长度下限 (map units)"));
            doubleSpinBoxMin->setValue(mCheckSet->lowerLimit);
        }else if(btn == ui->btnPolygonOverlap){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("面图层"));
            ui->tableWidgetPara->showRow(3);
            ui->tableWidgetPara->item(3 ,0)->setText(QStringLiteral("面积上限 (map units sqr.)"));
            doubleSpinBoxMax->setValue(mCheckSet->upperLimit);
        }else if(btn == ui->btnGap){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("面图层"));
            ui->tableWidgetPara->showRow(3);
            ui->tableWidgetPara->item(3 ,0)->setText(QStringLiteral("空隙面积上限 (map units sqr.)"));
            doubleSpinBoxMax->setValue(mCheckSet->upperLimit);
            ui->tableWidgetPara->showRow(4);
            ui->tableWidgetPara->item(4 ,0)->setText(QStringLiteral("空隙面积下限 (map units sqr.)"));
            doubleSpinBoxMin->setValue(mCheckSet->lowerLimit);
        }else if(btn == ui->btnArea){
            ui->tableWidgetPara->showRow(3);
            ui->tableWidgetPara->item(3 ,0)->setText(QStringLiteral("面积上限 (map units sqr.)"));
            doubleSpinBoxMax->setValue(mCheckSet->upperLimit);
            ui->tableWidgetPara->showRow(4);
            ui->tableWidgetPara->item(4 ,0)->setText(QStringLiteral("面积下限 (map units sqr.)"));
            doubleSpinBoxMin->setValue(mCheckSet->lowerLimit);
        }else if(btn == ui->btnClockwise){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("线/面图层"));
            ui->tableWidgetPara->showRow(7);
            ui->tableWidgetPara->item(7, 0)->setText(QStringLiteral("反向检查"));
        }else if(btn == ui->btnInvalid || btn == ui->btnCollinear || btn == ui->btnDuplicateNode){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("线/面图层"));
        }else if(btn == ui->btnDuplicate){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("图层"));
            ui->tableWidgetPara->item(7, 0)->setText(QStringLiteral("只检查同层地物"));
            ui->tableWidgetPara->showRow(7);
            ui->tableWidgetPara->item(5, 0)->setText(QStringLiteral("检查类型"));
            comboBoxLayerMode->setItemText(0, QStringLiteral("图形一致"));
            comboBoxLayerMode->setItemText(1, QStringLiteral("节点一致"));
            ui->tableWidgetPara->showRow(5);
        }else if(btn == ui->btnConvexhull){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("面图层"));
            ui->tableWidgetPara->showRow(7);
            ui->tableWidgetPara->item(7, 0)->setText(QStringLiteral("反向检查"));
            excludeEndpoint->setChecked(mCheckSet->excludeEndpoint);
        }else if(btn == ui->btnInvalidAttr){
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("图层"));
            ui->tableWidgetPara->showRow(8);
            setAttrText();
        }
        setBtnText(layerA, mCheckSet->layersA);
        setLayerModeText();
        excludeEndpoint->setChecked(mCheckSet->excludeEndpoint);
    }
}

void CheckItemDialog::setText(QPushButton *btn)
{
    QString str = "<html><body><span style=\" font-size:16pt; font-weight:700;\">";
    str += btn->text();
    str += "</span><br><br><span style=\" font-size:13pt;\">";
    str += btn->toolTip();
    str += "</span><br><br></body></html>";
    ui->textShortHelp->setText(str);
}

void CheckItemDialog::onSelectionChanged(const QItemSelection &, const QItemSelection &)
{
    int row = ui->tableWidget->currentRow();
    if (!mCheckMap[row])
        return;
    if (mCheckSet != nullptr) {
        delete mCheckSet;
    }
    mCheckSet = new CheckSet;
    mCheckSet->fromPoint(mCheckMap[row]);

    QPushButton *btn = nameToBtn[mCheckSet->name];
    if(btn == nullptr) return;
    setParaUi(btn);
    setText(btn);
}

void CheckItemDialog::selectLayerA()
{
    layerSelectDialog->disconnect();
    connect(layerSelectDialog, &LayerSelectionDialog::finish, this, [&]() {
        setBtnText(layerA, layerSelectDialog->getSelectedLayers());
        mCheckSet->layersA = layerSelectDialog->getSelectedLayers();
    });
    layerSelectDialog->selectLayer(mCheckSet->layersA);
    layerSelectDialog->show();
}

void CheckItemDialog::selectLayerB()
{
    layerSelectDialog->disconnect();
    connect(layerSelectDialog, &LayerSelectionDialog::finish, this, [&]() {
        setBtnText(layerB, layerSelectDialog->getSelectedLayers());
        mCheckSet->layersB = layerSelectDialog->getSelectedLayers();
    });
    layerSelectDialog->selectLayer(mCheckSet->layersB);
    layerSelectDialog->show();
}

void CheckItemDialog::setLayerMode()
{
    if (!mCheckSet)
        return;
    if (comboBoxLayerMode->currentIndex() == 0)
        mCheckSet->oneToOne = false;
    else
        mCheckSet->oneToOne = true;
}

void CheckItemDialog::saveEdit()
{
    if (!mCheckSet)
        return;

    mCheckSet->angle = doubleSpinBoxAngle->value();
    mCheckSet->lowerLimit = doubleSpinBoxMin->value();
    mCheckSet->upperLimit = doubleSpinBoxMax->value();
    mCheckSet->excludeEndpoint = excludeEndpoint->isChecked();
    mCheckSet->oneToOne = (comboBoxLayerMode->currentIndex() == 1);
    mCheckSet->attr = attr->text();
    for (int i = 0; i < mcheckItem->sets.size(); ++i)
    {
        if (mcheckItem->sets[i].name == mCheckSet->name)
        {
            mcheckItem->sets[i] = *mCheckSet;
            return;
        }
    }

    mcheckItem->sets.push_back(*mCheckSet);
    initTable();
}

void CheckItemDialog::deleteCheck()
{
    int row = ui->tableWidget->currentRow();
    if (!mCheckMap[row])
        return;

    QString name = mCheckMap[row]->name;

    for (auto it = mcheckItem->sets.begin(); it != mcheckItem->sets.end(); ++it) {
        if (it->name == name){
            mcheckItem->sets.erase(it);
            break;
        }
    }
    initTable();
}

#include <QMessageBox>
#include "vectordataproviderfeaturepool.h"
#include "checker.h"
#include "checkcontext.h"
#include <qgsproject.h>
void CheckItemDialog::run()
{
    if (mcheckItem == nullptr || mcheckItem->sets.empty())
        return;

    // Get all layer
    QSet<QgsVectorLayer*> layers;
    for (int i = 0; i < mcheckItem->sets.size(); ++i) {
        CheckSet& set = mcheckItem->sets[i];
        for (auto layer : as_const(set.layersA))layers.insert(layer);
        for (auto layer : as_const(set.layersB))layers.insert(layer);
        for (auto layer : as_const(set.excludedLayers))layers.insert(layer);
    }
    if (layers.isEmpty()) return;

    for (QgsVectorLayer* layer : layers)
    {
        if (layer->isEditable())
        {
            QMessageBox::critical(this, tr("Check Geometries"), tr("Input layer '%1' is not allowed to be in editing mode.").arg(layer->name()));
            return;
        }
    }
    bool selectedOnly = ui->checkBoxInputSelectedOnly->isChecked();

    // Setup checker
    setCursor(Qt::WaitCursor);
    ui->btnRun->setEnabled(false);
    ui->labelStatus->setText(tr("<b>Building spatial index…</b>"));
    ui->labelStatus->show();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    QMap<QString, FeaturePool *> featurePools;
    for (QgsVectorLayer *layer : qgis::as_const(layers)) {
        featurePools.insert(layer->id(), new VectorDataProviderFeaturePool(layer, selectedOnly));
    }

    CheckContext *context = new CheckContext( ui->spinBoxTolerance->value(), QgsProject::instance()->crs(), QgsProject::instance()->transformContext(), QgsProject::instance() );

    QList<Check *> checks = getChecks(context);

    Checker *checker = new Checker( checks, context, featurePools );

    emit checkerStarted( checker );

    // Run
    ui->progressBar->setRange( 0, 0 );
    ui->labelStatus->hide();
    ui->progressBar->show();
    ui->btnCancel->show();
    QEventLoop evLoop;
    QFutureWatcher<void> futureWatcher;
    connect( checker, &Checker::progressValue, ui->progressBar, &QProgressBar::setValue );
    connect( &futureWatcher, &QFutureWatcherBase::finished, &evLoop, &QEventLoop::quit );
    connect( ui->btnCancel, &QAbstractButton::clicked, &futureWatcher, &QFutureWatcherBase::cancel );

    mIsRunningInBackground = true;

    int maxSteps = 0;
    futureWatcher.setFuture( checker->execute( &maxSteps ) );
    ui->progressBar->setRange( 0, maxSteps );
    evLoop.exec();

    mIsRunningInBackground = false;

    // Restore window
    unsetCursor();
    ui->progressBar->hide();
    ui->btnCancel->hide();
    ui->btnRun->setEnabled( true );

    // Show result
    emit checkerFinished( !futureWatcher.isCanceled() );
    this->hide();
}

#include "pointonlinecheck.h"
#include "pointonlineendcheck.h"
#include "pointonlinenodecheck.h"
#include "duplicatecheck.h"
#include "pointonboundarycheck.h"
#include "pointinpolygoncheck.h"
#include "linelayerintersectioncheck.h"
#include "lineintersectioncheck.h"
#include "lineselfintersectioncheck.h"
#include "linelayeroverlapcheck.h"
#include "lineoverlapcheck.h"
#include "lineselfoverlapcheck.h"
#include "danglecheck.h"
#include "linecoveredbylinecheck.h"
#include "linecoveredbyboundarycheck.h"
#include "lineendonpointcheck.h"
#include "lineinpolygoncheck.h"
#include "turnbackcheck.h"
#include "segmentlengthcheck.h"
#include "lengthcheck.h"
#include "polygonoverlapcheck.h"
#include "gapcheck.h"
#include "polygonlayeroverlapcheck.h"
#include "polygoncoveredbypolygoncheck.h"
#include "polygoninpolygoncheck.h"
#include "holecheck.h"
#include "convexhullcheck.h"
#include "areacheck.h"
#include "clockwisecheck.h"
#include "anglecheck.h"
#include "isvalidcheck.h"
#include "samecheck.h"
#include "collinearcheck.h"
#include "duplicatenodecheck.h"
#include "attrvalidcheck.h"
QList<Check *> CheckItemDialog::getChecks(CheckContext *context)
{
    QList<Check *> checks;
    for (auto &checkset : as_const(mcheckItem->sets))
    {
        QVariantMap configuration;
        QVariant var;
        var.setValue(checkset.layersA);
        configuration.insert("layersA", var);
        var.setValue(checkset.layersB);
        configuration.insert("layersB", var);
        configuration.insert( "excludeEndpoint", checkset.excludeEndpoint );
        if (checkset.name == ui->btnPointOnLine->text()) {
            checks.append(new PointOnLineCheck(context, configuration));
        }else if(checkset.name == ui->btnPointOnLineEnd->text()){
            checks.append(new PointOnLineEndCheck(context, configuration));
        }else if(checkset.name == ui->btnPointOnLineNode->text()){
            checks.append(new PointOnLineNodeCheck(context, configuration));
        }else if(checkset.name == ui->btnPointDuplicate->text()){
            var.setValue(QgsWkbTypes::PointGeometry);
            configuration.insert("type", var);
            checks.append(new DuplicateCheck(context, configuration));
        }else if(checkset.name == ui->btnLineDuplicate->text()){
            var.setValue(QgsWkbTypes::LineGeometry);
            configuration.insert("type", var);
            checks.append(new DuplicateCheck(context, configuration));
        }else if(checkset.name == ui->btnPolygonDuplicate->text()){
            var.setValue(QgsWkbTypes::PolygonGeometry);
            configuration.insert("type", var);
            checks.append(new DuplicateCheck(context, configuration));
        }else if(checkset.name == ui->btnPointOnBoundary->text()){
            checks.append(new PointOnBoundaryCheck(context, configuration));
        }else if(checkset.name == ui->btnPointInPolygon->text()){
            checks.append(new PointInPolygonCheck(context, configuration));
        }else if(checkset.name == ui->btnLineLayerIntersection->text()){
            checks.append(new LineLayerIntersectionCheck(context, configuration));
        }else if(checkset.name == ui->btnLineIntersection->text()){
            checks.append(new LineIntersectionCheck(context, configuration));
        }else if(checkset.name == ui->btnLineSelfIntersection->text()){
            checks.append(new LineSelfIntersectionCheck(context, configuration));
        }else if(checkset.name == ui->btnLineLayerOverlap->text()){
            checks.append(new LineLayerOverlapCheck(context, configuration));
        }else if(checkset.name == ui->btnLineOverlap->text()){
            checks.append(new LineOverlapCheck(context, configuration));
        }else if(checkset.name == ui->btnLineSelfOverlap->text()){
            checks.append(new LineSelfOverlapCheck(context, configuration));
        }else if(checkset.name == ui->btnDangle->text()){
            checks.append(new DangleCheck(context, configuration));
        }else if(checkset.name == ui->btnLineCoveredByLine->text()){
            checks.append(new LineCoveredByLineCheck(context, configuration));
        }else if(checkset.name == ui->btnLineCoveredByBoundary->text()){
            checks.append(new LineCoveredByBoundaryCheck(context, configuration));
        }else if(checkset.name == ui->btnLineEndOnPoint->text()){
            checks.append(new LineEndOnPointCheck(context, configuration));
        }else if(checkset.name == ui->btnLineInPolygon->text()){
            checks.append(new LineInPolygonCheck(context, configuration));
        }else if(checkset.name == ui->btnTurnback->text()){
            configuration.insert( "minAngle", checkset.angle );
            checks.append(new TurnbackCheck(context, configuration));
        }else if(checkset.name == ui->btnSegmentLength->text()){
            configuration.insert( "lengthMax", checkset.upperLimit );
            configuration.insert( "lengthMin", checkset.lowerLimit );
            checks.append(new SegmentLengthCheck(context, configuration));
        }else if(checkset.name == ui->btnLength->text()){
            configuration.insert( "lengthMax", checkset.upperLimit );
            configuration.insert( "lengthMin", checkset.lowerLimit );
            checks.append(new LengthCheck(context, configuration));
        }else if(checkset.name == ui->btnPolygonOverlap->text()){
            configuration.insert( "areaMax", checkset.upperLimit );
            checks.append(new PolygonOverlapCheck(context, configuration));
        }else if(checkset.name == ui->btnGap->text()){
            configuration.insert( "areaMax", checkset.upperLimit );
            configuration.insert( "areaMin", checkset.lowerLimit );
            checks.append(new GapCheck(context, configuration));
        }else if(checkset.name == ui->btnPolygonLayerOverlap->text()){
            configuration.insert( "areaMax", checkset.upperLimit );
            checks.append(new PolygonLayerOverlapCheck(context, configuration));
        }else if(checkset.name == ui->btnPolygonCoveredByPolygon->text()){
            checks.append(new PolygonCoveredByPolygonCheck(context, configuration));
        }else if(checkset.name == ui->btnPolygonInPolygon->text()){
            checks.append(new PolygonInPolygonCheck(context, configuration));
        }else if(checkset.name == ui->btnHole->text()){
            checks.append(new HoleCheck(context, configuration));
        }else if(checkset.name == ui->btnConvexhull->text()){
            checks.append(new ConvexHullCheck(context, configuration));
        }else if(checkset.name == ui->btnArea->text()){
            configuration.insert( "areaMax", checkset.upperLimit );
            configuration.insert( "areaMin", checkset.lowerLimit );
            checks.append(new AreaCheck(context, configuration));
        }else if(checkset.name == ui->btnClockwise->text()){
            checks.append(new ClockwiseCheck(context, configuration));
        }else if(checkset.name == ui->btnAngle->text()){
            configuration.insert( "minAngle", checkset.angle );
            checks.append(new AngleCheck(context, configuration));
        }else if(checkset.name == ui->btnInvalid->text()){
            checks.append(new IsValidCheck(context, configuration));
        }else if(checkset.name == ui->btnDuplicate->text()){
            configuration.insert( "sameNode", checkset.oneToOne );
            checks.append(new SameCheck(context, configuration));
        }else if(checkset.name == ui->btnCollinear->text()){
            checks.append(new CollinearCheck(context, configuration));
        }else if(checkset.name == ui->btnDuplicateNode->text()){
            checks.append(new DuplicateNodeCheck(context, configuration));
        }else if(checkset.name == ui->btnInvalidAttr->text()){
            configuration.insert( "attr", checkset.attr );
            checks.append(new AttrValidCheck(context, configuration));
        }else if(checkset.name == ui->btnUniqueAttr->text()){

        }
    }

    return checks;
}
