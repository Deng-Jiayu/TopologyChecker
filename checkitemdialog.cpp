#include "checkitemdialog.h"
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

    ui->tableWidgetPara->setRowCount(8);

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
    comboBoxLayerMode->addItem(QStringLiteral("一对一"));
    comboBoxLayerMode->addItem(QStringLiteral("一对多"));
    ui->tableWidgetPara->setCellWidget(5, 1, comboBoxLayerMode);
    //connect(comboBoxLayerMode, &QComboBox::currentTextChanged, this, &CheckItemDialog::setLayerMode);

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

void CheckItemDialog::setParaUi(QPushButton *btn)
{
    for (int i = 0; i < ui->tableWidgetPara->rowCount(); ++i)
        ui->tableWidgetPara->showRow(i);
    if(btn == ui->btnPointOnLine || btn == ui->btnPointOnLineEnd || btn == ui->btnPointOnLineNode || btn == ui->btnPointOnBoundary) {
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
        if(btn == ui->btnPointOnBoundary) ui->tableWidgetPara->item(1, 0)->setText(QStringLiteral("面图层"));
    }
    else if(btn == ui->btnPointDuplicate || btn == ui->btnLineDuplicate || btn == ui->btnPolygonDuplicate){
        ui->tableWidgetPara->hideRow(1);
        ui->tableWidgetPara->hideRow(2);
        ui->tableWidgetPara->hideRow(3);
        ui->tableWidgetPara->hideRow(4);
        ui->tableWidgetPara->hideRow(5);
        ui->tableWidgetPara->hideRow(6);
        ui->tableWidgetPara->hideRow(7);
        if(btn == ui->btnPointDuplicate)
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("点图层"));
        else if(btn == ui->btnLineDuplicate)
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("线图层"));
        else
            ui->tableWidgetPara->item(0, 0)->setText(QStringLiteral("面图层"));
        setBtnText(layerA, mCheckSet->layersA);
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

void CheckItemDialog::saveEdit()
{
    if (!mCheckSet)
        return;

    mCheckSet->angle = doubleSpinBoxAngle->value();
    mCheckSet->lowerLimit = doubleSpinBoxMin->value();
    mCheckSet->upperLimit = doubleSpinBoxMax->value();
    mCheckSet->excludeEndpoint = excludeEndpoint->isChecked();
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
            qDebug()<<"390";
            checks.append(new PointOnBoundaryCheck(context, configuration));
        }
    }

    return checks;
}
