#pragma once

#include <QString>
#include <qgsvectorlayer.h>
#include <vector>
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
      tolerance = set->tolerance;
    }

    virtual ~CheckSet() = default;

    QString name;
    QString description;
    vector<QgsVectorLayer *> layersA;
    vector<QgsVectorLayer *> layersB;
    vector<QgsVectorLayer *> excludedLayers;
    double angle = 0.00;
    double upperLimit = 0.00;
    double lowerLimit = 0.00;
    bool excludeEndpoint = false;
    double tolerance;
};

class CheckItem
{
 public:
    CheckItem() = default;
    CheckItem(QString name) : name(name) {}
    CheckItem(QString name, QString description) : name(name), description(description) {}

    QString name;
    QString description;
    vector<CheckSet> sets;
};

class CheckGroup
{
public:
    CheckGroup() = default;
    CheckGroup(QString name) : name(name) {}

    QString name;
    vector<CheckItem> items;
};

class CheckList
{
public:
    CheckList() = default;
    CheckList(QString name) : name(name) {}

    QString name;
    vector<CheckGroup> groups;
};
