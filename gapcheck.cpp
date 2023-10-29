#include "checkcontext.h"
#include "qgsgeometryengine.h"
#include "gapcheck.h"
#include "featurepool.h"
#include "qgsvectorlayer.h"
#include "qgsvectorlayerutils.h"
#include "qgsfeedback.h"
#include "qgsapplication.h"
#include "qgsproject.h"
#include "qgsexpressioncontextutils.h"
#include "qgspolygon.h"
#include "qgscurve.h"

#include "geos_c.h"

GapCheck::GapCheck(const CheckContext *context, const QVariantMap &configuration)
    : Check(context, configuration),
      mAreaMin(configuration.value(QStringLiteral("areaMin")).toDouble()),
      mAreaMax(configuration.value(QStringLiteral("areaMax")).toDouble())
{
    QVariant var = configuration.value("layersA");
    layers = var.value<QSet<QgsVectorLayer *>>();
}

void GapCheck::prepare(const CheckContext *context, const QVariantMap &configuration)
{
    if (configuration.value(QStringLiteral("allowedGapsEnabled")).toBool())
    {
        QgsVectorLayer *layer = context->project()->mapLayer<QgsVectorLayer *>(configuration.value("allowedGapsLayer").toString());
        if (layer)
        {
            mAllowedGapsLayer = layer;
            mAllowedGapsSource = qgis::make_unique<QgsVectorLayerFeatureSource>(layer);

            mAllowedGapsBuffer = configuration.value(QStringLiteral("allowedGapsBuffer")).toDouble();
        }
    }
    else
    {
        mAllowedGapsSource.reset();
    }
}

void GapCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    if (feedback)
        feedback->setProgress(feedback->progress() + 1.0);

    std::unique_ptr<QgsAbstractGeometry> allowedGapsGeom;
    std::unique_ptr<QgsGeometryEngine> allowedGapsGeomEngine;

    if (mAllowedGapsSource)
    {
        QVector<QgsGeometry> allowedGaps;
        QgsFeatureRequest request;
        request.setSubsetOfAttributes(QgsAttributeList());
        QgsFeatureIterator iterator = mAllowedGapsSource->getFeatures(request);
        QgsFeature feature;

        while (iterator.nextFeature(feature))
        {
            QgsGeometry geom = feature.geometry();
            QgsGeometry gg = geom.buffer(mAllowedGapsBuffer, 20);
            allowedGaps.append(gg);
        }

        std::unique_ptr<QgsGeometryEngine> allowedGapsEngine = CheckerUtils::createGeomEngine(nullptr, mContext->tolerance);

        // Create union of allowed gaps
        QString errMsg;
        allowedGapsGeom.reset(allowedGapsEngine->combine(allowedGaps, &errMsg));
        allowedGapsGeomEngine = CheckerUtils::createGeomEngine(allowedGapsGeom.get(), mContext->tolerance);
        allowedGapsGeomEngine->prepareGeometry();
    }

    QVector<QgsGeometry> geomList;
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    const CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), nullptr, mContext, true);
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(layers.begin(), layers.end(), layerFeature.layer()) == layers.end())
            continue;
        geomList.append(layerFeature.geometry());

        if (feedback && feedback->isCanceled())
        {
            geomList.clear();
            break;
        }
    }

    if (geomList.isEmpty())
    {
        return;
    }

    std::unique_ptr<QgsGeometryEngine> geomEngine = CheckerUtils::createGeomEngine(nullptr, mContext->tolerance);

    // Create union of geometry
    QString errMsg;
    std::unique_ptr<QgsAbstractGeometry> unionGeom(geomEngine->combine(geomList, &errMsg));
    if (!unionGeom)
    {
        messages.append(tr("Gap check: %1").arg(errMsg));
        return;
    }

    // Get envelope of union
    geomEngine = CheckerUtils::createGeomEngine(unionGeom.get(), mContext->tolerance);
    geomEngine->prepareGeometry();
    std::unique_ptr<QgsAbstractGeometry> envelope(geomEngine->envelope(&errMsg));
    if (!envelope)
    {
        messages.append(tr("Gap check: %1").arg(errMsg));
        return;
    }

    // Buffer envelope
    geomEngine = CheckerUtils::createGeomEngine(envelope.get(), mContext->tolerance);
    geomEngine->prepareGeometry();
    QgsAbstractGeometry *bufEnvelope = geomEngine->buffer(2, 0, GEOSBUF_CAP_SQUARE, GEOSBUF_JOIN_MITRE, 4.); // #spellok  //#spellok
    envelope.reset(bufEnvelope);

    // Compute difference between envelope and union to obtain gap polygons
    geomEngine = CheckerUtils::createGeomEngine(envelope.get(), mContext->tolerance);
    geomEngine->prepareGeometry();
    std::unique_ptr<QgsAbstractGeometry> diffGeom(geomEngine->difference(unionGeom.get(), &errMsg));
    if (!diffGeom)
    {
        messages.append(tr("Gap check: %1").arg(errMsg));
        return;
    }

    // For each gap polygon which does not lie on the boundary, get neighboring polygons and add error
    QgsGeometryPartIterator parts = diffGeom->parts();
    while (parts.hasNext())
    {
        const QgsAbstractGeometry *gapGeom = parts.next();
        // Skip the gap between features and boundingbox
        const double spacing = context()->tolerance;
        if (gapGeom->boundingBox().snappedToGrid(spacing) == envelope->boundingBox().snappedToGrid(spacing))
        {
            continue;
        }

        // Skip gaps above threshold
        if (gapGeom->area() > mAreaMax || gapGeom->area() < mAreaMin)
        {
            continue;
        }

        QgsRectangle gapAreaBBox = gapGeom->boundingBox();

        // Get neighboring polygons
        QMap<QString, QgsFeatureIds> neighboringIds;
        const CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds.keys(), gapAreaBBox, compatibleGeometryTypes(), mContext);
        for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
        {
            const QgsGeometry geom = layerFeature.geometry();
            if (CheckerUtils::sharedEdgeLength(gapGeom, geom.constGet(), mContext->reducedTolerance) > 0)
            {
                neighboringIds[layerFeature.layer()->id()].insert(layerFeature.feature().id());
                gapAreaBBox.combineExtentWith(layerFeature.geometry().boundingBox());
            }
        }

        if (neighboringIds.isEmpty())
        {
            continue;
        }

        if (allowedGapsGeomEngine && allowedGapsGeomEngine->contains(gapGeom))
        {
            continue;
        }

        // Add error
        double area = gapGeom->area();
        QgsRectangle gapBbox = gapGeom->boundingBox();
        errors.append(new GapCheckError(this, QString(), QgsGeometry(gapGeom->clone()), neighboringIds, area, gapBbox, gapAreaBBox));
    }
}

void GapCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
{
    QMetaEnum metaEnum = QMetaEnum::fromType<GapCheck::ResolutionMethod>();
    if (!metaEnum.isValid() || !metaEnum.valueToKey(method))
    {
        error->setFixFailed(tr("Unknown method"));
    }
    else
    {
        ResolutionMethod methodValue = static_cast<ResolutionMethod>(method);
        switch (methodValue)
        {
        case NoChange:
            error->setFixed(method);
            break;

        case MergeLongestEdge:
        {
            QString errMsg;
            if (mergeWithNeighbor(featurePools, static_cast<GapCheckError *>(error), changes, errMsg, LongestSharedEdge))
            {
                error->setFixed(method);
            }
            else
            {
                error->setFixFailed(tr("Failed to merge with neighbor: %1").arg(errMsg));
            }
            break;
        }

        case AddToAllowedGaps:
        {
            QgsVectorLayer *layer = qobject_cast<QgsVectorLayer *>(mAllowedGapsLayer.data());
            if (layer)
            {
                if (!layer->isEditable() && !layer->startEditing())
                {
                    error->setFixFailed(tr("Could not start editing layer %1").arg(layer->name()));
                }
                else
                {
                    QgsFeature feature = QgsVectorLayerUtils::createFeature(layer, error->geometry());
                    QgsFeatureList features = QgsVectorLayerUtils::makeFeatureCompatible(feature, layer);
                    if (!layer->addFeatures(features))
                    {
                        error->setFixFailed(tr("Could not add feature to layer %1").arg(layer->name()));
                    }
                    else
                    {
                        error->setFixed(method);
                    }
                }
            }
            else
            {
                error->setFixFailed(tr("Allowed gaps layer could not be resolved"));
            }
            break;
        }

        case CreateNewFeature:
        {
            GapCheckError *gapCheckError = static_cast<GapCheckError *>(error);
            QgsProject *project = QgsProject::instance();
            QgsVectorLayer *layer = qobject_cast<QgsVectorLayer *>(project->mapLayer(gapCheckError->neighbors().keys().first()));
            if (layer)
            {
                const QgsGeometry geometry = error->geometry();
                QgsExpressionContext context(QgsExpressionContextUtils::globalProjectLayerScopes(layer));
                QgsFeature feature = QgsVectorLayerUtils::createFeature(layer, geometry, QgsAttributeMap(), &context);
                if (!layer->addFeature(feature))
                {
                    error->setFixFailed(tr("Could not add feature"));
                }
                else
                {
                    error->setFixed(method);
                }
            }
            else
            {
                error->setFixFailed(tr("Could not resolve target layer %1 to add feature").arg(error->layerId()));
            }
            break;
        }

        case MergeLargestArea:
        {
            QString errMsg;
            if (mergeWithNeighbor(featurePools, static_cast<GapCheckError *>(error), changes, errMsg, LargestArea))
            {
                error->setFixed(method);
            }
            else
            {
                error->setFixFailed(tr("Failed to merge with neighbor: %1").arg(errMsg));
            }
            break;
        }
        }
    }
}

bool GapCheck::mergeWithNeighbor(const QMap<QString, FeaturePool *> &featurePools, GapCheckError *err, Changes &changes, QString &errMsg, Condition condition) const
{
    double maxVal = 0.;
    QString mergeLayerId;
    QgsFeature mergeFeature;
    int mergePartIdx = -1;

    const QgsGeometry geometry = err->geometry();
    const QgsAbstractGeometry *errGeometry = CheckerUtils::getGeomPart(geometry.constGet(), 0);

    const auto layerIds = err->neighbors().keys();
    QList<QgsFeature> neighbours;

    // Search for touching neighboring geometries
    for (const QString &layerId : layerIds)
    {
        FeaturePool *featurePool = featurePools.value(layerId);
        std::unique_ptr<QgsAbstractGeometry> errLayerGeom(errGeometry->clone());
        QgsCoordinateTransform ct(featurePool->crs(), mContext->mapCrs, mContext->transformContext);
        errLayerGeom->transform(ct, QgsCoordinateTransform::ReverseTransform);

        const auto featureIds = err->neighbors().value(layerId);

        for (QgsFeatureId testId : featureIds)
        {
            QgsFeature feature;
            if (!featurePool->getFeature(testId, feature))
            {
                continue;
            }

            QgsGeometry transformedGeometry = feature.geometry();
            transformedGeometry.transform(ct);
            feature.setGeometry(transformedGeometry);
            neighbours.append(feature);
        }

        for (const QgsFeature &testFeature : neighbours)
        {
            const QgsGeometry featureGeom = testFeature.geometry();
            const QgsAbstractGeometry *testGeom = featureGeom.constGet();
            for (int iPart = 0, nParts = testGeom->partCount(); iPart < nParts; ++iPart)
            {
                double val = 0;
                switch (condition)
                {
                case LongestSharedEdge:
                    val = CheckerUtils::sharedEdgeLength(errLayerGeom.get(), CheckerUtils::getGeomPart(testGeom, iPart), mContext->reducedTolerance);
                    break;

                case LargestArea:
                    val = CheckerUtils::getGeomPart(testGeom, iPart)->area();
                    break;
                }

                if (val > maxVal)
                {
                    maxVal = val;
                    mergeFeature = testFeature;
                    mergePartIdx = iPart;
                    mergeLayerId = layerId;
                }
            }
        }
    }

    if (maxVal == 0.)
    {
        return false;
    }

    QgsSpatialIndex neighbourIndex(QgsSpatialIndex::Flag::FlagStoreFeatureGeometries);
    neighbourIndex.addFeatures(neighbours);

    QgsPolyline snappedRing;
    QgsVertexIterator iterator = errGeometry->vertices();
    while (iterator.hasNext())
    {
        QgsPoint pt = iterator.next();
        QgsVertexId id;
        QgsGeometry closestGeom = neighbourIndex.geometry(neighbourIndex.nearestNeighbor(QgsPointXY(pt)).first());
        if (!closestGeom.isEmpty())
        {
            QgsPoint closestPoint = QgsGeometryUtils::closestVertex(*closestGeom.constGet(), pt, id);
            snappedRing.append(closestPoint);
        }
    }

    std::unique_ptr<QgsPolygon> snappedErrGeom = qgis::make_unique<QgsPolygon>();
    snappedErrGeom->setExteriorRing(new QgsLineString(snappedRing));

    // Merge geometries
    FeaturePool *featurePool = featurePools[mergeLayerId];
    std::unique_ptr<QgsAbstractGeometry> errLayerGeom(snappedErrGeom->clone());
    QgsCoordinateTransform ct(featurePool->crs(), mContext->mapCrs, mContext->transformContext);
    errLayerGeom->transform(ct, QgsCoordinateTransform::ReverseTransform);
    const QgsGeometry mergeFeatureGeom = mergeFeature.geometry();
    const QgsAbstractGeometry *mergeGeom = mergeFeatureGeom.constGet();
    std::unique_ptr<QgsGeometryEngine> geomEngine = CheckerUtils::createGeomEngine(errLayerGeom.get(), 0);
    std::unique_ptr<QgsAbstractGeometry> combinedGeom(geomEngine->combine(CheckerUtils::getGeomPart(mergeGeom, mergePartIdx), &errMsg));
    if (!combinedGeom || combinedGeom->isEmpty() || !QgsWkbTypes::isSingleType(combinedGeom->wkbType()))
    {
        return false;
    }

    // Add merged polygon to destination geometry
    replaceFeatureGeometryPart(featurePools, mergeLayerId, mergeFeature, mergePartIdx, combinedGeom.release(), changes);

    return true;
}

QStringList GapCheck::resolutionMethods() const
{
    QStringList methods = QStringList()
                          << QStringLiteral("将缝隙加入旁边多边形")
                          << QStringLiteral("无");
    if (mAllowedGapsSource)
        methods << tr("Add gap to allowed exceptions");

    return methods;
}

QList<CheckResolutionMethod> GapCheck::availableResolutionMethods() const
{
    QList<CheckResolutionMethod> fixes;
//        CheckResolutionMethod(MergeLongestEdge, tr("Add to longest shared edge"), tr("Add the gap area to the neighbouring polygon with the longest shared edge."), false),
//        CheckResolutionMethod(CreateNewFeature, tr("Create new feature"), tr("Create a new feature from the gap area."), false),
//        CheckResolutionMethod(MergeLargestArea, tr("Add to largest neighbouring area"), tr("Add the gap area to the neighbouring polygon with the largest area."), false)};

//    if (mAllowedGapsSource)
//        fixes << CheckResolutionMethod(AddToAllowedGaps, tr("Add gap to allowed exceptions"), tr("Create a new feature from the gap geometry on the allowed exceptions layer."), false);

    fixes << CheckResolutionMethod(NoChange, QStringLiteral("无"), tr("Do not perform any action and mark this error as fixed."), false);

    return fixes;
}

QString GapCheck::description() const
{
    return factoryDescription();
}

QString GapCheck::id() const
{
    return factoryId();
}

Check::Flags GapCheck::flags() const
{
    return factoryFlags();
}

QString GapCheck::factoryDescription()
{
    return QStringLiteral("存在空隙");
}

QString GapCheck::factoryId()
{
    return QStringLiteral("QgsGeometryGapCheck");
}

Check::Flags GapCheck::factoryFlags()
{
    return Check::AvailableInValidation;
}

QList<QgsWkbTypes::GeometryType> GapCheck::factoryCompatibleGeometryTypes()
{
    return {QgsWkbTypes::PolygonGeometry};
}

bool GapCheck::factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP
{
    return factoryCompatibleGeometryTypes().contains(layer->geometryType());
}

Check::CheckType GapCheck::factoryCheckType()
{
    return Check::LayerCheck;
}

QgsRectangle GapCheckError::contextBoundingBox() const
{
    return mContextBoundingBox;
}

bool GapCheckError::isEqual(CheckError *other) const
{
    GapCheckError *err = dynamic_cast<GapCheckError *>(other);
    return err && CheckerUtils::pointsFuzzyEqual(err->location(), location(), mCheck->context()->reducedTolerance) && err->neighbors() == neighbors();
}

bool GapCheckError::closeMatch(CheckError *other) const
{
    GapCheckError *err = dynamic_cast<GapCheckError *>(other);
    return err && err->layerId() == layerId() && err->neighbors() == neighbors();
}

void GapCheckError::update(const CheckError *other)
{
    CheckError::update(other);
    // Static cast since this should only get called if isEqual == true
    const GapCheckError *err = static_cast<const GapCheckError *>(other);
    mNeighbors = err->mNeighbors;
    mGapAreaBBox = err->mGapAreaBBox;
}

bool GapCheckError::handleChanges(const Check::Changes &)
{
    return true;
}

QgsRectangle GapCheckError::affectedAreaBBox() const
{
    return mGapAreaBBox;
}

QMap<QString, QgsFeatureIds> GapCheckError::involvedFeatures() const
{
    return mNeighbors;
}

QIcon GapCheckError::icon() const
{

    if (status() == CheckError::StatusFixed)
        return QgsApplication::getThemeIcon(QStringLiteral("/algorithms/mAlgorithmCheckGeometry.svg"));
    else
        return QgsApplication::getThemeIcon(QStringLiteral("/checks/SliverOrGap.svg"));
}
