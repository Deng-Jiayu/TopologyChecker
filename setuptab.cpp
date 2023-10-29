#include "setuptab.h"
#include "checkcontext.h"
#include "ui_setuptab.h"

#include "vectordataproviderfeaturepool.h"
#include "widget.h"
#include "checkitemdialog.h"
#include "checker.h"

#include <QFileDialog>
#include <QLineEdit>
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include <qgsproject.h>

SetupTab::SetupTab(QgisInterface *iface, QDockWidget *checkDock, QWidget *parent)
    : QWidget(parent), ui(new Ui::SetupTab), mIface(iface), mCheckDock(checkDock)
{
    ui->setupUi(this);
    ui->widgetProgress->hide();
    mAbortButton = new QPushButton( QStringLiteral( "取消" ) );
    connect(ui->widgetInputs, &Widget::addGroup, this, &SetupTab::addGroup);

    initUi();
    connect(ui->btnSave, &QPushButton::clicked, this, &SetupTab::save);
    connect(ui->btnRead, &QPushButton::clicked, this, &SetupTab::read);
    connect(ui->btnCreateList, &QPushButton::clicked, this, &SetupTab::createList);
    connect(ui->btnDeleteList, &QPushButton::clicked, this, &SetupTab::deleteList);
    connect(ui->comboBox, &QComboBox::currentTextChanged, this, &SetupTab::initUi);
    connect(QgsProject::instance(), &QgsProject::layersAdded, this, &SetupTab::updateLayers);
    connect(QgsProject::instance(), static_cast<void ( QgsProject::* )( const QStringList & )>( &QgsProject::layersRemoved ), this, &SetupTab::updateLayers);

    updateLayers();
}

SetupTab::~SetupTab()
{
    delete mAbortButton;
    delete ui;
}


void SetupTab::initLists()
{
    CheckSet pointOnLineCheckSet(QStringLiteral("点必须在线上"));
    CheckSet pointOnEndpointCheckSet(QStringLiteral("点必须被线端点覆盖"));
    CheckSet pointOnLineNodeCheckSet(QStringLiteral("点必须被线节点覆盖"));
    CheckSet pointDuplicateCheckSet(QStringLiteral("无重复点"));

    CheckItem pointLineItem(QStringLiteral("点线矛盾检查"));
    pointLineItem.sets.push_back(pointOnLineCheckSet);
    pointLineItem.sets.push_back(pointOnEndpointCheckSet);
    pointLineItem.sets.push_back(pointOnLineNodeCheckSet);
    CheckItem duplicateNodeItem(QStringLiteral("无重复点"));
    duplicateNodeItem.sets.push_back(pointDuplicateCheckSet);

    CheckGroup pointGroup(QStringLiteral("点拓扑检查"));
    pointGroup.items.push_back(pointLineItem);
    pointGroup.items.push_back(duplicateNodeItem);

    CheckSet turnbackCheckSet(QStringLiteral("线内无打折"));
    CheckSet angleCheckSet(QStringLiteral("最小锐角"));
    CheckSet lineEndCoveredByPointCheckSet(QStringLiteral("线端点必须有井"));
    CheckSet lineIntersectionCheckSet(QStringLiteral("线内无相交"));
    CheckSet lineLayerIntersectionCheckSet(QStringLiteral("线与线无相交"));
    CheckSet lineSelfIntersectionCheckSet(QStringLiteral("线内无自相交"));
    CheckSet lineSelfOverlapCheckSet(QStringLiteral("线内无自交叠"));
    CheckSet lineOverlapCheckSet(QStringLiteral("线内无重叠"));
    CheckSet lineLayerOverlapCheckSet(QStringLiteral("线与线无重叠"));
    CheckSet lineCoveredByBoundaryCheckSet(QStringLiteral("线被面边界覆盖"));
    CheckSet lineCoveredByLineCheckSet(QStringLiteral("线被多条线完全覆盖"));
    CheckSet lineDuplicateCheckSet(QStringLiteral("线与线不重复"));

    CheckItem turnbackItem(QStringLiteral("折角检查"));
    turnbackItem.sets.push_back(turnbackCheckSet);
    turnbackItem.sets.push_back(angleCheckSet);
    CheckItem lineIntersectionItem(QStringLiteral("线相交"));
    lineIntersectionItem.sets.push_back(lineIntersectionCheckSet);
    lineIntersectionItem.sets.push_back(lineLayerIntersectionCheckSet);
    lineIntersectionItem.sets.push_back(lineSelfIntersectionCheckSet);
    CheckItem lineOverlapItem(QStringLiteral("线重叠"));
    lineOverlapItem.sets.push_back(lineSelfOverlapCheckSet);
    lineOverlapItem.sets.push_back(lineOverlapCheckSet);
    lineOverlapItem.sets.push_back(lineLayerOverlapCheckSet);
    lineOverlapItem.sets.push_back(lineDuplicateCheckSet);
    CheckItem lineCoveredByItem(QStringLiteral("线被覆盖"));
    lineCoveredByItem.sets.push_back(lineCoveredByBoundaryCheckSet);
    lineCoveredByItem.sets.push_back(lineCoveredByLineCheckSet);


    CheckGroup lineGroup(QStringLiteral("线拓扑检查"));
    lineGroup.items.push_back(turnbackItem);
    lineGroup.items.push_back(lineIntersectionItem);
    lineGroup.items.push_back(lineOverlapItem);
    lineGroup.items.push_back(lineCoveredByItem);


    CheckSet holeCheckSet(QStringLiteral("面内无岛"));
    CheckSet gapCheckSet(QStringLiteral("面内无缝隙"));
    CheckSet polygonDuplicateCheckSet(QStringLiteral("面不重复"));
    CheckSet convexhullCheckSet(QStringLiteral("凸包图斑"));
    CheckSet notWithinCheckSet(QStringLiteral("面不相互包含"));
    CheckSet withinCheckSet(QStringLiteral("面被面包含"));
    CheckSet polygonOverlapCheck(QStringLiteral("面内无重叠"));
    CheckSet polygonLayerOverlapCheck(QStringLiteral("面与面无重叠"));

    CheckItem withinItem(QStringLiteral("包含检查"));
    withinItem.sets.push_back(withinCheckSet);
    withinItem.sets.push_back(notWithinCheckSet);
    withinItem.sets.push_back(polygonDuplicateCheckSet);
    withinItem.sets.push_back(polygonOverlapCheck);
    withinItem.sets.push_back(polygonLayerOverlapCheck);
    CheckItem holeItem(QStringLiteral("面内无岛/缝隙"));
    holeItem.sets.push_back(holeCheckSet);
    holeItem.sets.push_back(gapCheckSet);
    CheckItem convexhullItem(QStringLiteral("凸包图斑"));
    convexhullItem.sets.push_back(convexhullCheckSet);

    CheckGroup polygonGroup(QStringLiteral("面拓扑检查"));
    polygonGroup.items.push_back(withinItem);
    polygonGroup.items.push_back(holeItem);
    polygonGroup.items.push_back(convexhullItem);

    CheckList pipelineList(QStringLiteral("管线拓扑检查"));
    pipelineList.groups.push_back(pointGroup);
    pipelineList.groups.push_back(lineGroup);
    pipelineList.groups.push_back(polygonGroup);

    this->mlists.push_back(pipelineList);

    ui->comboBox->addItem(mlists.back().name);
}


void SetupTab::initUi()
{
    curList = nullptr;
    for (int i = 0; i < mlists.size(); ++i)
    {
        if (mlists[i].name == ui->comboBox->currentText())
        {
            curList = &mlists[i];
        }
    }

    this->groupBoxs.clear();
    this->btns.clear();

    if (ui->widgetInputs)
        delete ui->widgetInputs;
    ui->widgetInputs = new Widget(this);
    connect(ui->widgetInputs, &Widget::addGroup, this, &SetupTab::addGroup);
    ui->widgetInputs->setObjectName(QString::fromUtf8("widgetInputs"));
    ui->scrollArea->setWidget(ui->widgetInputs);

    QVBoxLayout *verticalLayout;
    verticalLayout = new QVBoxLayout(ui->widgetInputs);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));

    if(curList == nullptr) return;

    for (int i = 0; i < curList->groups.size(); ++i)
    {
        CollapsibleGroupBox *groupBox;
        groupBox = new CollapsibleGroupBox(curList->groups[i].name, ui->widgetInputs);
        connect(groupBox, &CollapsibleGroupBox::addItem, this, &SetupTab::addItem);
        connect(groupBox, &CollapsibleGroupBox::addGroup, this, &SetupTab::addGroup);
        connect(groupBox, &CollapsibleGroupBox::remove, this, &SetupTab::remove);
        connect(groupBox, &CollapsibleGroupBox::rename, this, &SetupTab::rename);
        groupBox->setObjectName(QString::fromUtf8("groupBoxGeometryProperties"));
        boxToGroup[groupBox] = &curList->groups[i];

        QVBoxLayout *verticalLayout_2;
        verticalLayout_2 = new QVBoxLayout(groupBox);
        verticalLayout_2->setSpacing(2);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(2, 2, 2, 2);

        for (int j = 0; j < curList->groups[i].items.size(); ++j)
        {
            PushButton *btn = new PushButton(groupBox);
            CheckItem &item = curList->groups[i].items[j];
            btn->setToolTip(item.getDescription());
            connect(btn, &PushButton::rename, this, &SetupTab::rename);
            connect(btn, &PushButton::remove, this, &SetupTab::remove);
            connect(btn, &PushButton::addItem, this, &SetupTab::addItem);
            connect(btn, &PushButton::addGroup, this, &SetupTab::addGroup);
            connect(btn, &PushButton::run, this, &SetupTab::runItem);
            connect(btn, &PushButton::runAll, this, &SetupTab::setPara);

            btnToCheck[btn] = &curList->groups[i].items[j];

            btn->setStyleSheet("text-align : left;");
            btn->setObjectName(curList->groups[i].items[j].name);
            btn->setText(curList->groups[i].items[j].name);

            verticalLayout_2->addWidget(btn);
            this->btns.push_back(btn);
        }

        verticalLayout->addWidget(groupBox);
        this->groupBoxs.push_back(groupBox);
    }

    initConnection();
}

void SetupTab::initConnection()
{
    for (auto it = boxToGroup.begin(); it != boxToGroup.end(); ) {
        if (it.key()) {

            ++it;
        } else {
            it = boxToGroup.erase(it);
        }
    }
    for (auto it = btnToCheck.begin(); it != btnToCheck.end(); ) {
        if (it.key()) {

            ++it;
        } else {
            it = btnToCheck.erase(it);
        }
    }

    for (auto it : as_const(btns)) {
        if(!it) continue;
        connect(it, &PushButton::clicked, this, [=]() {
            CheckItemDialog *dialog = new CheckItemDialog(mIface, btnToCheck[it], this);
            connect(dialog, &CheckItemDialog::checkerStarted, this, &SetupTab::checkerStarted);
            connect(dialog, &CheckItemDialog::checkerFinished, this, &SetupTab::checkerFinished);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
            dialog->set();
        });
    }
}


void SetupTab::save()
{
    QJsonArray groupArray;
    for (int i = 0; i < curList->groups.size(); ++i) {

        CheckGroup group = curList->groups[i];
        QJsonArray itemArray;

        for (int j = 0; j < group.items.size(); ++j) {

            CheckItem item = group.items[j];
            QJsonArray setArray;

            for(int k = 0; k < item.sets.size(); ++k){

                CheckSet set = item.sets[k];

                QJsonObject object({{ "name", set.name }});
                object.insert("layersA",QJsonArray::fromStringList(set.layersAStr));
                object.insert("layersB",QJsonArray::fromStringList(set.layersBStr));
                object.insert("angle", set.angle);
                object.insert("upperLimit", set.upperLimit);
                object.insert("lowerLimit", set.lowerLimit);
                object.insert("excludeEndpoint", set.excludeEndpoint);
                object.insert("oneToOne", set.oneToOne);
                object.insert("attr", set.attr);
                setArray.append(object);
            }

            QJsonObject itemObj;
            itemObj.insert("name",item.name);
            itemObj.insert("sets",setArray);
            itemArray.append(itemObj);
        }

        QJsonObject groupObj;
        groupObj.insert("name",group.name);
        groupObj.insert("items",itemArray);
        groupArray.append(groupObj);
    }

    qDebug()<<curList->name;
    QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("保存"), "./"+curList->name, "Json(*.json)");

    QJsonDocument doc(groupArray);
    QByteArray jsonData = doc.toJson();

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(jsonData);
        file.close();
    }

}

void SetupTab::read()
{
    QString fileName = QFileDialog::getOpenFileName(this, QStringLiteral("选择文件"), "./", "Json(*.json)");
    if (fileName.isEmpty())
        return;

    QFile file(fileName);

    file.open(QFile::ReadOnly | QFile::Text);
    QString str = file.readAll();
    QJsonDocument d = QJsonDocument::fromJson(str.toUtf8());

    CheckList list;
    QJsonArray arr = d.array();
    for(int i = 0;i<arr.size();++i){
        QJsonValue val = arr[i];
        QJsonObject obj = val.toObject();
        CheckGroup group(obj["name"].toString());

        QJsonArray items = obj["items"].toArray();
        for(int j = 0;j<items.size();++j){
            val = items[j];
            obj = val.toObject();
            CheckItem item(obj["name"].toString());

            QJsonArray sets = obj["sets"].toArray();
            for(int k = 0;k<sets.size();++k){
                val = sets[k];
                obj = val.toObject();

                //CheckSet set(obj["name"].toString(),obj["description"].toString());
                CheckSet set(obj["name"].toString());
                QStringList list;
                QJsonArray array = obj["layersA"].toArray();
                for (auto it : as_const(array))
                  list.append(it.toString());
                set.layersAStr = list;
                list.clear();
                array = obj["layersB"].toArray();
                for (auto it : as_const(array))
                  list.append(it.toString());
                set.layersBStr = list;
                set.angle = obj["angle"].toDouble();
                set.upperLimit = obj["upperLimit"].toDouble();
                set.lowerLimit = obj["lowerLimit"].toDouble();
                set.excludeEndpoint = obj["excludeEndpoint"].toBool();
                set.oneToOne = obj["oneToOne"].toBool();
                set.attr = obj["attr"].toString();

                item.sets.push_back(set);
            }

            group.items.push_back(item);
        }

        list.groups.push_back(group);

    }

    list.name  = QFileInfo(fileName).completeBaseName();

    int i = mlists.size();
    mlists.push_back(list);
    ui->comboBox->addItem(mlists[i].name);
    ui->comboBox->setCurrentIndex(i);

    initItemLayers();

    qDebug() << curList->groups[0].items[0].sets[0].layersA.size();
}

void SetupTab::createList()
{
    QString dlgTitle = QStringLiteral("新建方案");
    QString txtLabel = QStringLiteral("请输入检查方案名");
    QLineEdit::EchoMode echoMode = QLineEdit::Normal;
    bool ok = false;
    QString text = QInputDialog::getText(this, dlgTitle, txtLabel, echoMode, QString(), &ok);
    if (!ok || text.isEmpty())
        return;
    CheckList newList(text);
    int i = mlists.size();
    mlists.push_back(newList);
    ui->comboBox->addItem(mlists[i].name);
    ui->comboBox->setCurrentIndex(i);
}

void SetupTab::deleteList()
{
    if (QMessageBox::No ==
        QMessageBox::question(this, QStringLiteral("拓扑检查"), QStringLiteral("删除后不可恢复，您确认删除？"),
            QMessageBox::Yes, QMessageBox::No)) {
        return;
    }

    int i = ui->comboBox->currentIndex();
    mlists.remove(i);
    ui->comboBox->removeItem(i);
}

void SetupTab::updateLayers()
{
    layers.clear();
    for ( QgsVectorLayer *layer : QgsProject::instance()->layers<QgsVectorLayer *>() )
    {
        if(!layer || !layer->isValid()) continue;

        if(layer->geometryType() == QgsWkbTypes::PointGeometry ||
            layer->geometryType() == QgsWkbTypes::LineGeometry ||
            layer->geometryType() == QgsWkbTypes::PolygonGeometry)
        {
            layers.insert(layer);
        }
    }

    initItemLayers();
}

void SetupTab::addGroup()
{
    if (curList == nullptr) {
        QMessageBox::critical(this, QStringLiteral("拓扑检查"), QStringLiteral("请先新建方案"));
        return;
    }

    QString dlgTitle = QStringLiteral("新建组");
    QString txtLabel = QStringLiteral("请输入检查组名");
    QLineEdit::EchoMode echoMode = QLineEdit::Normal;
    bool ok = false;
    QString text = QInputDialog::getText(this, dlgTitle, txtLabel, echoMode, QString(), &ok);
    if (!ok || text.isEmpty())
        return;

    for (int i = 0; i < curList->groups.size(); ++i)
    {
        if (text == curList->groups[i].name) {
            QMessageBox::critical(this, QStringLiteral("拓扑检查"), QStringLiteral("组已存在"));
            return;
        }
    }

    curList->groups.push_back(CheckGroup(text));
    initUi();
}

void SetupTab::addItem()
{
    QString dlgTitle = QStringLiteral("新建项");
    QString txtLabel = QStringLiteral("请输入检查项名");
    QLineEdit::EchoMode echoMode = QLineEdit::Normal;
    bool ok = false;
    QString text = QInputDialog::getText(this, dlgTitle, txtLabel, echoMode, QString(), &ok);
    if (!ok || text.isEmpty())
        return;

    CheckGroup *group = nullptr;
    CollapsibleGroupBox *p = qobject_cast<CollapsibleGroupBox *>(sender());

    if (p) {
        group = boxToGroup[p];
    }
    else
    {
        PushButton *p = qobject_cast<PushButton *>(sender());
        if(!p) return;

        CheckItem *item = btnToCheck[p];
        bool found = false;
        for (int i = 0; i < curList->groups.size(); ++i)
        {
            for (int j = 0; j < curList->groups[i].items.size(); ++j)
            {
                if (item == &curList->groups[i].items[j]) {
                    group = &curList->groups[i];
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }

    if (!group) return;

    for (auto &it : as_const(group->items)) {
        if (it.name == text) {
            QMessageBox::critical(this, QStringLiteral("拓扑检查"), QStringLiteral("项已存在"));
            return;
        }
    }

    CheckItem item(text);
    group->items.push_back(item);

    initUi();
}

void SetupTab::remove()
{
    if (QMessageBox::No ==
        QMessageBox::question(this, QStringLiteral("拓扑检查"), QStringLiteral("确认删除？"),
            QMessageBox::Yes, QMessageBox::No)) {
        return;
    }

    CollapsibleGroupBox *p = qobject_cast<CollapsibleGroupBox *>(sender());

    if (p) {
        for (int i = 0; i < curList->groups.size(); ++i)
        {
            if (curList->groups[i].name == p->title())
            {
                curList->groups.remove(i);
                initUi();
                return;
            }
        }
    } else {
        PushButton *p = qobject_cast<PushButton *>(sender());
        if(!p) return;

        CheckItem *item = btnToCheck[p];
        for (int i = 0; i < curList->groups.size(); ++i)
        {
            for (int j = 0; j < curList->groups[i].items.size(); ++j)
            {
                if (item == &curList->groups[i].items[j])
                {
                    curList->groups[i].items.remove(j);
                    initUi();
                    return;
                }
            }
        }
    }
}

void SetupTab::rename()
{
    CollapsibleGroupBox *p = qobject_cast<CollapsibleGroupBox *>(sender());

    if (p) {
        QString dlgTitle = QStringLiteral("重命名");
        QString txtLabel = QStringLiteral("请输入检查组名");
        QLineEdit::EchoMode echoMode = QLineEdit::Normal;
        bool ok = false;
        QString text = QInputDialog::getText(this, dlgTitle, txtLabel, echoMode, p->title(), &ok);
        if (!ok || text.isEmpty() || text == p->title())
            return;

        for (int i = 0; i < curList->groups.size(); ++i)
        {
            if (text == curList->groups[i].name) {
                QMessageBox::critical(this, QStringLiteral("拓扑检查"), QStringLiteral("组已存在"));
                return;
            }
        }

        boxToGroup[p]->name = text;
        initUi();
    } else
    {
        PushButton *p = qobject_cast<PushButton *>(sender());
        if(!p) return;

        QString dlgTitle = QStringLiteral("重命名");
        QString txtLabel = QStringLiteral("请输入检查项名");
        QLineEdit::EchoMode echoMode = QLineEdit::Normal;
        bool ok = false;
        QString text = QInputDialog::getText(this, dlgTitle, txtLabel, echoMode, p->text(), &ok);
        if (!ok || text.isEmpty() || text == p->text())
            return;

        CheckItem *item = btnToCheck[p];
        CheckGroup *group = nullptr;
        bool found = false;
        for (int i = 0; i < curList->groups.size(); ++i)
        {
            for (int j = 0; j < curList->groups[i].items.size(); ++j)
            {
                if (item == &curList->groups[i].items[j]) {
                    group = &curList->groups[i];
                    found = true;
                    break;
                }
            }
            if(found) break;
        }
        if(group == nullptr) return;

        for (int i = 0; i < group->items.size(); ++i)
        {
            if (group->items[i].name == text)
            {
                QMessageBox::critical(this, QStringLiteral("拓扑检查"), QStringLiteral("项已存在"));
                return;
            }
        }

        item->name = text;
        initUi();
    }
}

void SetupTab::runItem()
{
    PushButton *p = qobject_cast<PushButton *>(sender());
    CheckItemDialog *dialog = new CheckItemDialog(mIface, btnToCheck[p], this);
    dialog->setWindowTitle(QStringLiteral("执行检查"));
    connect(dialog, &CheckItemDialog::checkerStarted, this, &SetupTab::checkerStarted);
    connect(dialog, &CheckItemDialog::checkerFinished, this, &SetupTab::checkerFinished);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->resize(600, 500);
    dialog->hideBtnList();
    dialog->show();
}

void SetupTab::setPara()
{
    if(mMessageBox == nullptr){
        mMessageBox = new MessageBox(this);
        connect(mMessageBox, &MessageBox::run, this, &SetupTab::run);
    }
    mMessageBox->show();
}

void SetupTab::run()
{
    mMessageBox->hide();
    bool exist = false;
    for (auto i : curList->groups)
    {
        for (auto j : i.items)
        {
            if (j.sets.size() != 0)
            {
                exist = true;
                break;
            }
        }
    }
    if (!exist){
        QMessageBox::critical( this, QStringLiteral( "一键检查项" ), QStringLiteral( "检查列表为空" ) );
        return;
    }

    // Get process layer
    QSet<QgsVectorLayer *> processLayers;
    for (auto i : curList->groups)
    {
        for (auto j : (i.items))
        {
            for (auto set : (j.sets))
            {
                for (auto layer : as_const(set.layersA)) processLayers.insert(layer);
                for (auto layer : as_const(set.layersB)) processLayers.insert(layer);
                for (auto layer : as_const(set.excludedLayers)) processLayers.insert(layer);
            }
        }
    }

    if(processLayers.empty()) return;

    for (QgsVectorLayer *layer : processLayers)
    {
        if (layer->isEditable())
        {
            QMessageBox::critical(this, tr("Check Geometries"), tr("Input layer '%1' is not allowed to be in editing mode.").arg(layer->name()));
            return;
        }
    }
    bool selectedOnly = mMessageBox->selectedOnly();
    double tolerance = mMessageBox->tolerance();

    // Setup checker
    setCursor( Qt::WaitCursor );
    ui->labelStatus->setText( QStringLiteral( "<b>Building spatial index…</b>" ) );
    ui->labelStatus->show();
    QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );

    QMap<QString, FeaturePool *> featurePools;
    for ( QgsVectorLayer *layer : qgis::as_const( processLayers ) )
    {
        featurePools.insert( layer->id(), new VectorDataProviderFeaturePool( layer, selectedOnly ) );
    }

    QgsProject::instance()->setCrs((*processLayers.begin())->crs());

    CheckContext *context = new CheckContext( tolerance, QgsProject::instance()->crs(), QgsProject::instance()->transformContext(), QgsProject::instance() );

    QList<Check *> checks = getChecks(context);

    Checker *checker = new Checker( checks, context, featurePools );

    emit checkerStarted( checker );

    // Run
    ui->buttonBox->addButton( mAbortButton, QDialogButtonBox::ActionRole );
    ui->progressBar->setRange( 0, 0 );
    ui->labelStatus->hide();
    ui->progressBar->show();
    QEventLoop evLoop;
    QFutureWatcher<void> futureWatcher;
    connect( checker, &Checker::progressValue, ui->progressBar, &QProgressBar::setValue );
    connect( &futureWatcher, &QFutureWatcherBase::finished, &evLoop, &QEventLoop::quit );
    connect( mAbortButton, &QAbstractButton::clicked, &futureWatcher, &QFutureWatcherBase::cancel );

    mIsRunningInBackground = true;

    int maxSteps = 0;
    futureWatcher.setFuture( checker->execute( &maxSteps ) );
    ui->progressBar->setRange( 0, maxSteps );
    evLoop.exec();

    mIsRunningInBackground = false;

    // Restore window
    unsetCursor();
    mAbortButton->setEnabled( true );
    ui->buttonBox->removeButton( mAbortButton );
    ui->progressBar->hide();
    ui->labelStatus->hide();

    // Show result
    emit checkerFinished( !futureWatcher.isCanceled() );

}

QList<Check *> SetupTab::getChecks(CheckContext *context)
{
    QList<Check *> checks;
    for(int i = 0;i<curList->groups.size();++i)
    {
        CheckGroup &group = curList->groups[i];
        for(int j = 0;j<group.items.size();++j)
        {
            CheckItem &item = group.items[j];
            CheckItemDialog *dialog = new CheckItemDialog(mIface, &item);

            checks.append(dialog->getChecks(context));

            delete dialog;
        }
    }
    return checks;
}

void SetupTab::initItemLayers()
{
    if(curList == nullptr) return;
    for (auto &i : curList->groups)
    {
        for (auto &j : (i.items))
        {
            for (auto &set : (j.sets))
            {
                set.layersA.clear();
                set.layersB.clear();
                for(auto layerName : set.layersAStr)
                {
                    for(auto layer : layers)
                    {
                        if(layer->name() == layerName)
                        {
                            set.layersA.insert(layer);
                            break;
                        }
                    }
                }
                for(auto layerName : set.layersBStr)
                {
                    for(auto layer : layers)
                    {
                        if(layer->name() == layerName)
                        {
                            set.layersB.insert(layer);
                            break;
                        }
                    }
                }
            }
        }
    }
}

void SetupTab::setupItemLayers()
{
    if(curList == nullptr) return;
    for (auto &i : curList->groups)
    {
        for (auto &j : (i.items))
        {
            for (auto &set : (j.sets))
            {
                set.layersA.intersect(layers);
                set.layersB.intersect(layers);
            }
        }
    }
}
