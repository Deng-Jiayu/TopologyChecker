#include "setuptab.h"
#include "ui_setuptab.h"

#include <qgscollapsiblegroupbox.h>
#include <QFileDialog>
#include <QJsonObject>
#include <QJsonArray>

SetupTab::SetupTab(QgisInterface *iface, QDockWidget *checkDock, QWidget *parent)
    : QWidget(parent), ui(new Ui::SetupTab), mIface(iface), mCheckDock(checkDock)
{
    ui->setupUi(this);

    //initLists();
    initUi();
    connect(ui->btnSave, &QPushButton::clicked, this, &SetupTab::save);
    connect(ui->btnRead, &QPushButton::clicked, this, &SetupTab::read);

}

SetupTab::~SetupTab()
{
    delete ui;
}


void SetupTab::initLists()
{
    CheckSet pointOnLineCheckSet(QStringLiteral("点必须在线上	"),QStringLiteral("点必须在线对象上，包括在线内、线节点和线端点上，但是不能在线外。"));
    CheckSet pointOnEndpointCheckSet(QStringLiteral("点必须被线端点覆盖"),QStringLiteral("点只能在线对象的端点上，而不能在线的节点上、线内其它位置和线外。"));
    CheckSet pointOnLineNodeCheckSet(QStringLiteral("点必须被线节点覆盖"),QStringLiteral("点只能在线对象的节点上，而不能在线的端点上、线内其它位置和线外。"));
    CheckSet pointDuplicateCheckSet(QStringLiteral("无重复点"),QStringLiteral("检查是否存在重复的点对象。"));

    CheckItem pointLineItem(QStringLiteral("点线矛盾检查"),QStringLiteral("检查点对象与线对象的位置关系"));
    pointLineItem.sets.push_back(pointOnLineCheckSet);
    pointLineItem.sets.push_back(pointOnEndpointCheckSet);
    pointLineItem.sets.push_back(pointOnLineNodeCheckSet);
    CheckItem duplicateNodeItem(QStringLiteral("无重复点"),QStringLiteral("是否存在重复的点对象"));
    duplicateNodeItem.sets.push_back(pointDuplicateCheckSet);

    CheckGroup pointGroup(QStringLiteral("点拓扑检查"));
    pointGroup.items.push_back(pointLineItem);
    pointGroup.items.push_back(duplicateNodeItem);

    CheckSet turnbackCheckSet(QStringLiteral("线内无打折"),QStringLiteral("检查线数据集中是否存在连续四个节点构成的两个夹角的角度都小于所给的角度值（单位为度），若两个夹角都小于角度值，则认为线对象在中间两个节点处打折。"));
    CheckSet angleCheckSet(QStringLiteral("最小锐角"),QStringLiteral("用来检查矢量地物上节点之间的角度是否小于设定值。"));
    CheckSet lineEndCoveredByPointCheckSet(QStringLiteral("线端点必须有井"),QStringLiteral("检查线数据集中是否存在线端点未被参考点数据集的点覆盖的线对象。"));
    CheckSet lineIntersectionCheckSet(QStringLiteral("线内无相交"),QStringLiteral("检查一个线数据集中是否存在两个（或两个以上）相交且共享交点，但并未从交点处打断的线对象。"));
    CheckSet lineLayerIntersectionCheckSet(QStringLiteral("线与线无相交"),QStringLiteral("检查线数据集中是否存在与参考线数据集的线相交的线对象，即两个线数据集中的所有线对象必须相互分离。"));
    CheckSet lineSelfIntersectionCheckSet(QStringLiteral("线内无自相交"),QStringLiteral("检查一个线数据集中是否存在与自身相交的线对象，即线对象中不能有重叠（坐标相同）的节点。"));
    CheckSet lineSelfOverlapCheckSet(QStringLiteral("线内无自交叠"),QStringLiteral("检查一个线数据集中是否存在与自身重叠的线对象，即一个线对象本身不能有重叠部分。"));
    CheckSet lineOverlapCheckSet(QStringLiteral("线内无重叠"),QStringLiteral("检查一个线数据集中是否存在两个（或两个以上）线对象之间有相互重叠的部分。"));
    CheckSet lineLayerOverlapCheckSet(QStringLiteral("线与线无重叠"),QStringLiteral("检查线数据集中是否存在与参考线数据集的线重叠的线对象，即两个线数据集之间的线对象不能有重合的部分。"));
    CheckSet lineCoveredByBoundaryCheckSet(QStringLiteral("线被面边界覆盖"),QStringLiteral("检查线数据集中是否存在没有被参考面数据集的面边界（可以是一个或多个面边界）覆盖的线对象。"));
    CheckSet lineCoveredByLineCheckSet(QStringLiteral("线被多条线完全覆盖"),QStringLiteral("检查线数据集中是否存在没有被参考线数据集的一条或多条线覆盖的线对象。"));
    CheckSet lineDuplicateCheckSet(QStringLiteral("线与线不重复"),QStringLiteral("检查是否存在重复的线对象。"));

    CheckItem turnbackItem(QStringLiteral("折角检查"),QStringLiteral("检查线对象/面边界折角"));
    turnbackItem.sets.push_back(turnbackCheckSet);
    turnbackItem.sets.push_back(angleCheckSet);
    CheckItem lineIntersectionItem(QStringLiteral("线相交"),QStringLiteral("检查线对象是否相交"));
    lineIntersectionItem.sets.push_back(lineIntersectionCheckSet);
    lineIntersectionItem.sets.push_back(lineLayerIntersectionCheckSet);
    lineIntersectionItem.sets.push_back(lineSelfIntersectionCheckSet);
    CheckItem lineOverlapItem(QStringLiteral("线重叠"),QStringLiteral("检查线对象是否重叠/重复"));
    lineOverlapItem.sets.push_back(lineSelfOverlapCheckSet);
    lineOverlapItem.sets.push_back(lineOverlapCheckSet);
    lineOverlapItem.sets.push_back(lineLayerOverlapCheckSet);
    lineOverlapItem.sets.push_back(lineDuplicateCheckSet);
    CheckItem lineCoveredByItem(QStringLiteral("线被覆盖"),QStringLiteral("检查线对象是否被覆盖"));
    lineCoveredByItem.sets.push_back(lineCoveredByBoundaryCheckSet);
    lineCoveredByItem.sets.push_back(lineCoveredByLineCheckSet);


    CheckGroup lineGroup(QStringLiteral("线拓扑检查"));
    lineGroup.items.push_back(turnbackItem);
    lineGroup.items.push_back(lineIntersectionItem);
    lineGroup.items.push_back(lineOverlapItem);
    lineGroup.items.push_back(lineCoveredByItem);


    CheckSet holeCheckSet(QStringLiteral("面内无岛"),QStringLiteral("面对象内部不能出现被挖空（岛洞多边形）的情况。"));
    CheckSet gapCheckSet(QStringLiteral("面内无缝隙"),QStringLiteral("检查相邻面对象之间是否存在空隙，即相邻面对象之间的边界必须重合。"));
    CheckSet polygonDuplicateCheckSet(QStringLiteral("面不重复"),QStringLiteral("检查是否存在重复的面对象。"));
    CheckSet convexhullCheckSet(QStringLiteral("凸包图斑"),QStringLiteral("检查面对象是否为凸包。"));
    CheckSet notWithinCheckSet(QStringLiteral("面不相互包含"),QStringLiteral("检查是否存在被面对象包含的面对象。"));
    CheckSet withinCheckSet(QStringLiteral("面被面包含"),QStringLiteral("检查是否存在被面对象包含的面对象。"));
    CheckSet polygonOverlapCheck(QStringLiteral("面内无重叠"),QStringLiteral("检查一个面数据集中是否存在两个（或两个以上）相互重叠的面对象，包括部分重叠和完全重叠。"));
    CheckSet polygonLayerOverlapCheck(QStringLiteral("面与面无重叠"),QStringLiteral("检查面数据集中是否存在与参考面数据集的面重叠的面对象，包括部分重叠和完全重叠，即两个面数据集内的面对象之间不能存在交集。"));

    CheckItem withinItem(QStringLiteral("包含检查"),QStringLiteral("检查面对象之间的重复、重叠、包含关系"));
    withinItem.sets.push_back(withinCheckSet);
    withinItem.sets.push_back(notWithinCheckSet);
    withinItem.sets.push_back(polygonDuplicateCheckSet);
    withinItem.sets.push_back(polygonOverlapCheck);
    withinItem.sets.push_back(polygonLayerOverlapCheck);
    CheckItem holeItem(QStringLiteral("面内无岛/缝隙"),QStringLiteral("检查相邻面对象之间是否存在空隙，面对象内是否存在孔洞"));
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
    for (int i = 0; i < mlists.size(); ++i)
    {
        if (mlists[i].name == ui->comboBox->currentText())
        {
            curList = &mlists[i];
        }
    }

    if(curList == nullptr) return;

    this->groupBoxs.clear();
    this->btns.clear();

    if (ui->widgetInputs)
        delete ui->widgetInputs;
    ui->widgetInputs = new QWidget(this);
    ui->widgetInputs->setObjectName(QString::fromUtf8("widgetInputs"));
    ui->widgetInputs->setGeometry(QRect(0, 0, 800, 600));
    ui->scrollArea->setWidget(ui->widgetInputs);

    QVBoxLayout *verticalLayout;
    verticalLayout = new QVBoxLayout(ui->widgetInputs);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));

    for (int i = 0; i < curList->groups.size(); ++i)
    {
        CollapsibleGroupBox *groupBox;
        groupBox = new CollapsibleGroupBox(curList->groups[i].name, ui->widgetInputs);

        groupBox->setObjectName(QString::fromUtf8("groupBoxGeometryProperties"));
        groupBox->setProperty("flat", QVariant(true));

        QVBoxLayout *verticalLayout_2;
        verticalLayout_2 = new QVBoxLayout(groupBox);
        verticalLayout_2->setSpacing(2);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(2, 2, 2, 2);

        for (int j = 0; j < curList->groups[i].items.size(); ++j)
        {
            QPushButton *btn = new QPushButton(groupBox);

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

                QJsonObject object({ { "name", set.name }, { "description", set.description} });
                setArray.append(object);
            }

            QJsonObject itemObj;
            itemObj.insert("name",item.name);
            itemObj.insert("description",item.description);
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
    for(int i = 0; i<btns.size();++i){
        qDebug()<<btns[i]->pos();
        qDebug()<<btns[i]->text();
    }
    for(int i = 0;i<groupBoxs.size();++i){
        qDebug()<<groupBoxs[i]->pos();
        qDebug()<<groupBoxs[i]->title();
    }

    QString fileName = QFileDialog::getOpenFileName(this, QStringLiteral("选择文件"), "./", "Json(*.json)");

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
            CheckItem item(obj["name"].toString(),obj["description"].toString());


            QJsonArray sets = obj["sets"].toArray();
            for(int k = 0;k<sets.size();++k){
                val = sets[k];
                obj = val.toObject();

                CheckSet set(obj["name"].toString(),obj["description"].toString());
                item.sets.push_back(set);
            }

            group.items.push_back(item);
        }

        list.groups.push_back(group);

    }


    list.name  = QFileInfo(fileName).completeBaseName();

    this->mlists.push_back(list);

    ui->comboBox->addItem(mlists.back().name);
    initUi();
}
