#pragma once

#include <QString>
#include <qgsvectorlayer.h>
#include <QVector>
using namespace std;

class CheckSet
{
    Q_DECLARE_TR_FUNCTIONS(CheckSet)
public:
    CheckSet() = default;
    CheckSet(QString name) : name(name) {}
    CheckSet(QString name, QString description) : name(name), description(description) {}

    void fromPoint(CheckSet *set) {
        if (!set)
          return;
        name = set->name;
        layersA = set->layersA;
        layersB = set->layersB;
        excludedLayers = set->excludedLayers;
        angle = set->angle;
        upperLimit = set->upperLimit;
        lowerLimit = set->lowerLimit;
        excludeEndpoint = set->excludeEndpoint;
        oneToOne = set->oneToOne;
        attr = set->attr;
        tolerance = set->tolerance;
    }

    virtual ~CheckSet() = default;

    QString name;
    QString description;
    QVector<QgsVectorLayer *> layersA;
    QVector<QgsVectorLayer *> layersB;
    QVector<QgsVectorLayer *> excludedLayers;
    double angle = 0.00;
    double upperLimit = 0.00;
    double lowerLimit = 0.00;
    bool excludeEndpoint = false;
    bool oneToOne = false;
    QString attr = "";
    double tolerance;
};

class CheckItem
{
 public:
    CheckItem() = default;
    CheckItem(QString name) : name(name) {}

    QString name;
    QVector<CheckSet> sets;

    QString getDescription() {
        QString ans;
        for (auto &it : as_const(sets)) {
            ans += it.name;
            ans += ", ";
        }
        if(!ans.isEmpty())
            ans.remove(ans.length() - 2, 2);
        return ans;
    }
};

class CheckGroup
{
public:
    CheckGroup() = default;
    CheckGroup(QString name) : name(name) {}

    QString name;
    QVector<CheckItem> items;
};

class CheckList
{
public:
    CheckList() = default;
    CheckList(QString name) : name(name) {}

    QString name;
    QVector<CheckGroup> groups;
};
