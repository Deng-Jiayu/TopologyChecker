/***************************************************************************
 *  qgsgeometrychecker.h                                                   *
 *  -------------------                                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define SIP_NO_FILE

#ifndef CHECKER_H
#define CHECKER_H

#include <QFuture>
#include <QList>
#include <QMutex>
#include <QStringList>


#include "qgsfeedback.h"
#include "qgsfeatureid.h"

typedef qint64 QgsFeatureId;
class CheckContext;
class Check;
class CheckError;
class QgsMapLayer;
class QgsVectorLayer;
class FeaturePool;
class QMutex;

/**
 * \ingroup analysis
 *
 * Manages and runs a set of geometry checks.
 *
 * \since QGIS 3.4
 */
class Checker : public QObject
{
    Q_OBJECT
  public:
    Checker( const QList<Check *> &checks, CheckContext *context SIP_TRANSFER, const QMap<QString, FeaturePool *> &featurePools );
    ~Checker() override;
    QFuture<void> execute( int *totalSteps = nullptr );
    bool fixError( CheckError *error, int method, bool triggerRepaint = false );
    const QList<Check *> getChecks() const { return mChecks; }
    QStringList getMessages() const { return mMessages; }
    void setMergeAttributeIndices( const QMap<QString, int> &mergeAttributeIndices ) { mMergeAttributeIndices = mergeAttributeIndices; }
    CheckContext *getContext() const { return mContext; }
    const QMap<QString, FeaturePool *> featurePools() const {return mFeaturePools;}

  signals:
    void errorAdded( CheckError *error );
    void errorUpdated( CheckError *error, bool statusChanged );
    void progressValue( int value );

  private:
    class RunCheckWrapper
    {
      public:
        explicit RunCheckWrapper( Checker *instance );
        void operator()( const Check *check );
      private:
        Checker *mInstance = nullptr;
    };

    QList<Check *> mChecks;
    CheckContext *mContext = nullptr;
    QList<CheckError *> mCheckErrors;
    QStringList mMessages;
    QMutex mErrorListMutex;
    QMap<QString, int> mMergeAttributeIndices;
    QgsFeedback mFeedback;
    QMap<QString, FeaturePool *> mFeaturePools;

    void runCheck( const QMap<QString, FeaturePool *> &featurePools, const Check *check );

  private slots:
    void emitProgressValue();
};

#endif // CHECKER_H
