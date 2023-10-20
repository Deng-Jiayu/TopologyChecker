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

QString ResultTab::sSettingsGroup = QStringLiteral( "/TopologyChecker/default_fix_methods/" );

ResultTab::ResultTab( QgisInterface *iface, Checker *checker, QTabWidget *tabWidget, QWidget *parent )
    : QWidget( parent )
    , ui(new Ui::ResultTab)
    , mTabWidget( tabWidget )
    , mIface( iface )
    , mChecker( checker )
{
    ui->setupUi(this);
    mErrorCount = 0;
    mFixedCount = 0;

    connect( checker, &Checker::errorAdded, this, &ResultTab::addError );
    connect( checker, &Checker::errorUpdated, this, &ResultTab::updateError );
    connect( ui->tableWidgetErrors->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ResultTab::onSelectionChanged );
    connect( ui->btnOpenAttributeTable, &QAbstractButton::clicked, this, &ResultTab::openAttributeTable );
    connect( ui->checkBoxHighlight, &QAbstractButton::clicked, this, &ResultTab::highlightErrors );
    connect( QgsProject::instance(), static_cast<void ( QgsProject::* )( const QStringList & )>( &QgsProject::layersWillBeRemoved ), this, &ResultTab::checkRemovedLayer );
    connect( ui->btnExport, &QAbstractButton::clicked, this, &ResultTab::exportErrors );
    connect( ui->btnFix, &QAbstractButton::clicked, this, &ResultTab::fixCurrentError );

    bool allLayersEditable = true;
    for ( const FeaturePool *featurePool : mChecker->featurePools().values() )
    {
        if ( ( featurePool->layer()->dataProvider()->capabilities() & QgsVectorDataProvider::ChangeGeometries ) == 0 )
        {
            allLayersEditable = false;
            break;
        }
    }
    if ( !allLayersEditable )
    {
        ui->btnFix->setEnabled(false);
        ui->btnFixWithDefault->setEnabled(false);
    }

    ui->tableWidgetErrors->horizontalHeader()->setSortIndicator( 0, Qt::AscendingOrder );
    ui->tableWidgetErrors->resizeColumnToContents( 0 );
    ui->tableWidgetErrors->resizeColumnToContents( 1 );
    ui->tableWidgetErrors->horizontalHeader()->setStretchLastSection( true );
    // Not sure why, but this is needed...
    ui->tableWidgetErrors->setSortingEnabled( true );
    ui->tableWidgetErrors->setSortingEnabled( false );
}

ResultTab::~ResultTab()
{
    delete ui;
    delete mChecker;
    qDeleteAll( mCurrentRubberBands );
}

void ResultTab::setRowStatus(int row, const QColor &color, const QString &message, bool selectable)
{
    for ( int col = 0, nCols = ui->tableWidgetErrors->columnCount(); col < nCols; ++col )
    {
        QTableWidgetItem *item = ui->tableWidgetErrors->item( row, col );
        item->setBackground( color );
        if ( !selectable )
        {
            item->setFlags( item->flags() & ~Qt::ItemIsSelectable );
            item->setForeground( Qt::lightGray );
        }
    }
    ui->tableWidgetErrors->item( row, 5 )->setText( message );
}

bool ResultTab::exportErrorsDo(const QString &file)
{
    QList< QPair<QString, QString> > attributes;
    attributes.append( qMakePair( QStringLiteral( "Layer" ), QStringLiteral( "String;30;" ) ) );
    attributes.append( qMakePair( QStringLiteral( "FeatureID" ), QStringLiteral( "String;10;" ) ) );
    attributes.append( qMakePair( QStringLiteral( "ErrorDesc" ), QStringLiteral( "String;80;" ) ) );

    QFileInfo fi( file );
    QString ext = fi.suffix();
    QString driver = QgsVectorFileWriter::driverForExtension( ext );

    QString createError;
    bool success = QgsOgrProviderUtils::createEmptyDataSource( file, driver, "UTF-8", QgsWkbTypes::Point, attributes, QgsProject::instance()->crs(), createError );
    if ( !success )
    {
        return false;
    }

    const QgsVectorLayer::LayerOptions options { QgsProject::instance()->transformContext() };
    QgsVectorLayer *layer = new QgsVectorLayer( file, QFileInfo( file ).baseName(), QStringLiteral( "ogr" ), options );
    if ( !layer->isValid() )
    {
        delete layer;
        return false;
    }

    int fieldLayer = layer->fields().lookupField( QStringLiteral( "Layer" ) );
    int fieldFeatureId = layer->fields().lookupField( QStringLiteral( "FeatureID" ) );
    int fieldErrDesc = layer->fields().lookupField( QStringLiteral( "ErrorDesc" ) );
    for ( int row = 0, nRows = ui->tableWidgetErrors->rowCount(); row < nRows; ++row )
    {
        CheckError *error = ui->tableWidgetErrors->item( row, 0 )->data( Qt::UserRole ).value<CheckError *>();
        QString layerName = QString();
        const QString layerId = error->layerId();
        if ( mChecker->featurePools().keys().contains( layerId ) )
        {
            QgsVectorLayer *srcLayer = mChecker->featurePools()[layerId]->layer();
            layerName = srcLayer->name();
        }

        QgsFeature f( layer->fields() );
        f.setAttribute( fieldLayer, layerName );
        f.setAttribute( fieldFeatureId, error->featureId() );
        f.setAttribute( fieldErrDesc, error->description() );
        QgsGeometry geom( new QgsPoint( error->location() ) );
        f.setGeometry( geom );
        layer->dataProvider()->addFeatures( QgsFeatureList() << f );
    }

    // Remove existing layer with same uri
    QStringList toRemove;
    for ( QgsMapLayer *maplayer : QgsProject::instance()->mapLayers() )
    {
        if ( qobject_cast<QgsVectorLayer *>( maplayer ) &&
            static_cast<QgsVectorLayer *>( maplayer )->dataProvider()->dataSourceUri() == layer->dataProvider()->dataSourceUri() )
        {
            toRemove.append( maplayer->id() );
        }
    }
    if ( !toRemove.isEmpty() )
    {
        QgsProject::instance()->removeMapLayers( toRemove );
    }

    QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>() << layer );
    return true;

}

void ResultTab::updateComboBox()
{
    int row = ui->tableWidgetErrors->currentRow();
    CheckError* error = ui->tableWidgetErrors->item(row, 0)->data(Qt::UserRole).value<CheckError*>();
    const QList<CheckResolutionMethod> resolutionMethods = error->check()->availableResolutionMethods();
    ui->mComboBox->clear();
    int i = 0;
    for (const CheckResolutionMethod& method : resolutionMethods)
    {
        ui->mComboBox->addItem(method.name(), i);
        ++i;
    }
}

void ResultTab::addError(CheckError *error)
{
    bool sortingWasEnabled = ui->tableWidgetErrors->isSortingEnabled();
    if ( sortingWasEnabled )
        ui->tableWidgetErrors->setSortingEnabled( false );

    int row = ui->tableWidgetErrors->rowCount();
    int prec = 7 - std::floor( std::max( 0., std::log10( std::max( error->location().x(), error->location().y() ) ) ) );
    QString posStr = QStringLiteral( "%1, %2" ).arg( error->location().x(), 0, 'f', prec ).arg( error->location().y(), 0, 'f', prec );

    ui->tableWidgetErrors->insertRow( row );
    QTableWidgetItem *idItem = new QTableWidgetItem();
    idItem->setData( Qt::EditRole, error->featureId() != FID_NULL ? QVariant( error->featureId() ) : QVariant() );
    ui->tableWidgetErrors->setItem( row, 0, new QTableWidgetItem( !error->layerId().isEmpty() ? mChecker->featurePools()[error->layerId()]->layer()->name() : "" ) );
    ui->tableWidgetErrors->setItem( row, 1, idItem );
    ui->tableWidgetErrors->setItem( row, 2, new QTableWidgetItem( error->description() ) );
    ui->tableWidgetErrors->setItem( row, 3, new QTableWidgetItem( posStr ) );
    QTableWidgetItem *valueItem = new QTableWidgetItem();
    valueItem->setData( Qt::EditRole, error->value() );
    ui->tableWidgetErrors->setItem( row, 4, valueItem );
    ui->tableWidgetErrors->setItem( row, 5, new QTableWidgetItem( QString() ) );
    ui->tableWidgetErrors->item( row, 0 )->setData( Qt::UserRole, QVariant::fromValue( error ) );
    ++mErrorCount;
    ui->labelErrorCount->setText( tr( "Total errors: %1, fixed errors: %2" ).arg( mErrorCount ).arg( mFixedCount ) );
    mErrorMap.insert( error, QPersistentModelIndex( ui->tableWidgetErrors->model()->index( row, 0 ) ) );

    if ( sortingWasEnabled )
        ui->tableWidgetErrors->setSortingEnabled( true );
}

void ResultTab::updateError(CheckError *error, bool statusChanged)
{
    if ( !mErrorMap.contains( error ) )
    {
        return;
    }
    // Disable sorting to prevent crashes: if i.e. sorting by col 0, as soon as the item(row, 0) is set,
    // the row is potentially moved due to sorting, and subsequent item(row, col) reference wrong item
    bool sortingWasEnabled = ui->tableWidgetErrors->isSortingEnabled();
    if ( sortingWasEnabled )
        ui->tableWidgetErrors->setSortingEnabled( false );

    int row = mErrorMap.value( error ).row();
    int prec = 7 - std::floor( std::max( 0., std::log10( std::max( error->location().x(), error->location().y() ) ) ) );
    QString posStr = QStringLiteral( "%1, %2" ).arg( error->location().x(), 0, 'f', prec ).arg( error->location().y(), 0, 'f', prec );

    ui->tableWidgetErrors->item( row, 3 )->setText( posStr );
    ui->tableWidgetErrors->item( row, 4 )->setData( Qt::EditRole, error->value() );
    if ( error->status() == CheckError::StatusFixed )
    {
        setRowStatus( row, Qt::green, tr( "Fixed: %1" ).arg( error->resolutionMessage() ), true );
        ++mFixedCount;
    }
    else if ( error->status() == CheckError::StatusFixFailed )
    {
        setRowStatus( row, Qt::red, tr( "Fix failed: %1" ).arg( error->resolutionMessage() ), true );
    }
    else if ( error->status() == CheckError::StatusObsolete )
    {
        ui->tableWidgetErrors->setRowHidden( row, true );
        //    setRowStatus( row, Qt::gray, tr( "Obsolete" ), false );
        --mErrorCount;
    }
    ui->labelErrorCount->setText( tr( "Total errors: %1, fixed errors: %2" ).arg( mErrorCount ).arg( mFixedCount ) );

    if ( sortingWasEnabled )
        ui->tableWidgetErrors->setSortingEnabled( true );

}

void ResultTab::onSelectionChanged(const QItemSelection &newSel, const QItemSelection &)
{
    updateComboBox();

    QModelIndex idx = ui->tableWidgetErrors->currentIndex();
    if ( idx.isValid() && !ui->tableWidgetErrors->isRowHidden( idx.row() ) && newSel.contains( idx ) )
    {
        highlightErrors();
    }
    else
    {
        qDeleteAll( mCurrentRubberBands );
        mCurrentRubberBands.clear();
    }
}


void ResultTab::highlightErrors(bool current)
{
    qDeleteAll( mCurrentRubberBands );
    mCurrentRubberBands.clear();

    QList<QTableWidgetItem *> items;
    QVector<QgsPointXY> errorPositions;
    QgsRectangle totextent;

    if ( current )
    {
        items.append( ui->tableWidgetErrors->currentItem() );
    }
    else
    {
        items.append( ui->tableWidgetErrors->selectedItems() );
    }
    for ( QTableWidgetItem *item : qgis::as_const( items ) )
    {
        CheckError *error = ui->tableWidgetErrors->item( item->row(), 0 )->data( Qt::UserRole ).value<CheckError *>();

        const QgsGeometry geom = error->geometry();
        if ( ui->checkBoxHighlight->isChecked() && !geom.isNull() )
        {
            QgsRubberBand *featureRubberBand = new QgsRubberBand( mIface->mapCanvas() );
            featureRubberBand->addGeometry( geom, nullptr );
            featureRubberBand->setWidth( 5 );
            featureRubberBand->setColor( Qt::yellow );
            mCurrentRubberBands.append( featureRubberBand );
        }

        if ( ui->radioButtonError->isChecked() || current || error->status() == CheckError::StatusFixed )
        {
            QgsRubberBand *pointRubberBand = new QgsRubberBand( mIface->mapCanvas(), QgsWkbTypes::PointGeometry );
            pointRubberBand->addPoint( error->location() );
            pointRubberBand->setWidth( 20 );
            pointRubberBand->setColor( Qt::red );
            mCurrentRubberBands.append( pointRubberBand );
            errorPositions.append( error->location() );
        }
        else if ( ui->radioButtonFeature->isChecked() )
        {
            QgsRectangle geomextent = error->affectedAreaBBox();
            if ( totextent.isEmpty() )
            {
                totextent = geomextent;
            }
            else
            {
                totextent.combineExtentWith( geomextent );
            }
        }
    }

    // If error positions positions are marked, pan to the center of all positions,
    // and zoom out if necessary to make all points fit.
    if ( !errorPositions.isEmpty() )
    {
        double cx = 0., cy = 0.;
        QgsRectangle pointExtent( errorPositions.first(), errorPositions.first() );
        Q_FOREACH ( const QgsPointXY &p, errorPositions )
        {
            cx += p.x();
            cy += p.y();
            pointExtent.include( p );
        }
        QgsPointXY center = QgsPointXY( cx / errorPositions.size(), cy / errorPositions.size() );
        if ( totextent.isEmpty() )
        {
            QgsRectangle extent = mIface->mapCanvas()->extent();
            QgsVector diff = center - extent.center();
            extent.setXMinimum( extent.xMinimum() + diff.x() );
            extent.setXMaximum( extent.xMaximum() + diff.x() );
            extent.setYMinimum( extent.yMinimum() + diff.y() );
            extent.setYMaximum( extent.yMaximum() + diff.y() );
            extent.combineExtentWith( pointExtent );
            totextent = extent;
        }
        else
        {
            totextent.combineExtentWith( pointExtent );
        }
    }

    if ( !totextent.isEmpty() )
    {
        mIface->mapCanvas()->setExtent( totextent, true );
    }
    mIface->mapCanvas()->refresh();

}

void ResultTab::openAttributeTable()
{
    QMap<QString, QSet<QgsFeatureId>> ids;
    for ( QModelIndex idx : ui->tableWidgetErrors->selectionModel()->selectedRows() )
    {
        CheckError *error = ui->tableWidgetErrors->item( idx.row(), 0 )->data( Qt::UserRole ).value<CheckError *>();
        QgsFeatureId id = error->featureId();
        if ( id >= 0 )
        {
            ids[error->layerId()].insert( id );
        }
    }
    if ( ids.isEmpty() )
    {
        return;
    }
    for ( const QString &layerId : ids.keys() )
    {
        QStringList expr;
        for ( QgsFeatureId id : ids[layerId] )
        {
            expr.append( QStringLiteral( "$id = %1 " ).arg( id ) );
        }
        if ( mAttribTableDialogs[layerId] )
        {
            mAttribTableDialogs[layerId]->close();
        }
        mAttribTableDialogs[layerId] = mIface->showAttributeTable( mChecker->featurePools()[layerId]->layer(), expr.join( QLatin1String( " or " ) ) );
    }
}

void ResultTab::checkRemovedLayer(const QStringList &ids)
{
    bool requiredLayersRemoved = false;
    for ( const QString &layerId : mChecker->featurePools().keys() )
    {
        if ( ids.contains( layerId ) )
        {
            if ( isEnabled() )
                requiredLayersRemoved = true;
        }
    }
    if ( requiredLayersRemoved )
    {
        if ( mTabWidget->currentWidget() == this )
        {
            QMessageBox::critical( this, tr( "Remove Layer" ), tr( "One or more layers have been removed." ) );
        }
        setEnabled( false );
        qDeleteAll( mCurrentRubberBands );
        mCurrentRubberBands.clear();
    }

}

void ResultTab::exportErrors()
{
    QString initialdir;
    QDir dir = QFileInfo( mChecker->featurePools().first()->layer()->dataProvider()->dataSourceUri() ).dir();
    if ( dir.exists() )
    {
        initialdir = dir.absolutePath();
    }

    QString selectedFilter;
    QString file = QFileDialog::getSaveFileName( this, QStringLiteral( "选择输出文件" ), initialdir, QgsVectorFileWriter::fileFilterString(), &selectedFilter );
    if ( file.isEmpty() )
    {
        return;
    }

    file = QgsFileUtils::addExtensionFromFilter( file, selectedFilter );
    if ( !exportErrorsDo( file ) )
    {
        QMessageBox::critical( this, tr( "Export Errors" ), tr( "Failed to export errors to %1." ).arg( QDir::toNativeSeparators( file ) ) );
    }
}

void ResultTab::fixCurrentError()
{
    int row = ui->tableWidgetErrors->currentRow();
    CheckError* error = ui->tableWidgetErrors->item(row, 0)->data(Qt::UserRole).value<CheckError*>();
    mChecker->fixError(error, ui->mComboBox->currentData().toInt(), true);
}
