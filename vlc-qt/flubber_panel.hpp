/* Flubber extension for VLC 3.0.20 Qt. GPL-2.0-or-later, as the host module. */
#ifndef VLC_FLUBBER_PANEL_HPP
#define VLC_FLUBBER_PANEL_HPP

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QCursor>
#include <QEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QSettings>
#include <QMessageBox>
#include <QImage>
#include <QImageReader>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QTabWidget>
#include <QTabBar>
#include <QScreen>
#include <QScopedValueRollback>
#include <QSlider>
#include <QIcon>
#include <QGroupBox>
#include <QColorDialog>
#include <QSaveFile>
#include <QCoreApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <algorithm>
#include <QSet>
#include <QVector>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QUuid>
#include <QToolTip>
#include <QSizePolicy>
#include <QtMath>
#include <cstdint>
#include <cmath>
#include <functional>
#include <memory>
#include <QPointer>
#include "flubber_video_keys.hpp"
#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

/* The Planner's vlc-flubber-appearance/v1 contract is deliberately closed.
 * Parse into a temporary object and change the live preset only after every
 * field has passed validation. Position and size are player-owned settings. */
class FlubberPanel : public QWidget
{
public:
    enum FeedbackKind { FlubberKind = 0, GridKind = 1, FaceKind = 2 };
    struct ElementLayout { bool visible; int size, x, y; };
    explicit FlubberPanel( QSettings *settings, QWidget *parent = NULL )
        : QWidget( parent ), settings_( settings ), affectX_( 0 ), affectY_( 0 ),
          phase_( 0 ), primary_( FlubberKind ), faceRate_( 10 ), faceX_( 0 ), faceY_( 0 ),
          panelHeight_( 130 ), panelPercent_( -1 ), stepPercent_( 10 ), referenceSurface_( NULL ),
          arrowMode_( settings_->value( "Flubber/inputMode", "mouse" ).toString() == QLatin1String( "arrows" ) ),
          ratingActive_( false ), capturePending_( false ), cursorHideCalls_( 0 )
    {
        setObjectName( "vlc-flubber-panel" );
        setMouseTracking( true );
        setFocusPolicy( Qt::ClickFocus );
        updateInputTooltip();
        setAutoFillBackground( true );
        QPalette p = palette();
        p.setColor( QPalette::Window, Qt::black );
        setPalette( p );
        setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
        const char *names[] = { "flubber", "grid", "face" };
        const int defaultX[] = { 50, 18, 82 };
        for( int i = 0; i < 3; ++i )
        {
            const QString key = QStringLiteral( "Flubber/layers/" ) + QLatin1String( names[i] ) + '/';
            layers_[i] = { settings_->value( key + "visible", i == 0 ).toBool(),
                qBound( 10, settings_->value( key + "size", i == 0
                    ? settings_->value( "Flubber/sizePercent", 65 ) : QVariant( 55 ) ).toInt(), 100 ),
                qBound( 0, settings_->value( key + "x", i == 0
                    ? settings_->value( "Flubber/xPercent", 50 ) : QVariant( defaultX[i] ) ).toInt(), 100 ),
                qBound( 0, settings_->value( key + "y", i == 0
                    ? settings_->value( "Flubber/yPercent", 50 ) : QVariant( 50 ) ).toInt(), 100 ) };
        }
        primary_ = kindFromName( settings_->value( "Flubber/primary", "flubber" ).toString() );
        normalizeVisibility();
        grid_ = defaultGrid();
        grid_.insert( "lineThickness", settings_->value( "Flubber/gridLineThickness", 1.0 ).toDouble() );
        grid_.insert( "showOutline", settings_->value( "Flubber/gridShowOutline", true ).toBool() );
        grid_.insert( "outlineThickness", settings_->value( "Flubber/gridOutlineThickness", 2.0 ).toDouble() );
        grid_.insert( "cursorSize", settings_->value( "Flubber/gridCursorSize", 14.0 ).toDouble() );
        gridColumns_ = qBound( 3, settings_->value( "Flubber/gridColumns", 21 ).toInt() | 1, 2001 );
        gridRows_ = qBound( 3, settings_->value( "Flubber/gridRows", 21 ).toInt() | 1, 2001 );
        tiledGrid_ = settings_->value( "Flubber/gridStyle" ).toString() != QLatin1String( "continuous" );
        const int holdMs = settings_->value( "Flubber/arrowHoldMs" ).toInt();
        arrowHoldMs_ = holdMs >= MinHoldMs && holdMs <= MaxHoldMs ? holdMs : 0;
        faceId_ = settings_->value( "Flubber/faceId", "photo-reference-v3" ).toString();
        faceRate_ = qBound( .5, settings_->value( "Flubber/faceRate", 10.0 ).toDouble(), 20.0 );
        panelHeight_ = qBound( 48, settings_->value( "Flubber/panelHeight", 130 ).toInt(), 2160 );
        bool parsed = false;
        const int requestedPanel = qEnvironmentVariableIntValue( "VLC_FLUBBER_PANEL_PERCENT", &parsed );
        if( parsed && requestedPanel >= 10 && requestedPanel <= 100 ) panelPercent_ = requestedPanel;
        const int requestedStep = qEnvironmentVariableIntValue( "VLC_FLUBBER_STEP_PERCENT", &parsed );
        stepPercent_ = qBound( 1, settings_->value( "Flubber/stepPercent", 10 ).toInt(), 100 );
        if( parsed && requestedStep >= 1 && requestedStep <= 100 ) stepPercent_ = requestedStep;
        setFixedHeight( panelHeight_ );
        initializeShape();
        preset_ = defaultPreset();
        const QByteArray storedAppearance = settings_->value( "Flubber/appearanceJson" ).toByteArray();
        const QJsonDocument savedAppearance = QJsonDocument::fromJson( storedAppearance );
        if( savedAppearance.isObject() && savedAppearance.object().value( "schema" ).toString() ==
            QLatin1String( "vlc-flubber-appearance/v1" ) ) preset_ = savedAppearance.object();
        const QString path = settings_->value( "Flubber/appearancePath" ).toString();
        if( storedAppearance.isEmpty() && !path.isEmpty() )
        {
            QString error;
            QJsonObject loaded;
            if( readAppearance( path, loaded, error ) ) preset_ = loaded;
            else appearanceLoadError_ = error;
        }
        QString faceError;
        if( !setFaceId( faceId_, &faceError ) ) appearanceLoadError_ = faceError;
        const QString lastSettings = settings_->value( "Flubber/lastSettingsPath" ).toString();
        if( !lastSettings.isEmpty() )
        {
            QString error;
            if( !loadSettingsFile( lastSettings, error ) )
                appearanceLoadError_ = tr( "Could not restore saved settings from %1: %2" ).arg( lastSettings, error );
        }
        if( parsed && requestedStep >= 1 && requestedStep <= 100 ) stepPercent_ = requestedStep;
        clock_.start();
        faceClock_.start();
        arrowRepeat_.setTimerType( Qt::PreciseTimer );
        arrowRepeat_.setInterval( 33 );
        connect( &arrowRepeat_, &QTimer::timeout, this, [this]() {
            if( !ratingWindowIsForeground() || QApplication::activeModalWidget() ||
                QApplication::activePopupWidget() || textEditorOwnsShortcut() ||
                ( QApplication::queryKeyboardModifiers() & ~Qt::KeypadModifier ) ||
                ( rateAllowed_ && !rateAllowed_() ) )
                clearHeldArrows();
            else
            {
                const qint64 now = holdClock_.nsecsElapsed();
                moveHeldArrows( flubberHoldDistance( stepPercent_, arrowHoldMs_, now - holdLastNs_ ) );
                holdLastNs_ = now;
            }
        } );
        qApp->installEventFilter( this );
        nativeKeys_.reset( new FlubberVideoKeys( this, arrowMode_ ) );
        QTimer *timer = new QTimer( this );
        timer->setTimerType( Qt::PreciseTimer );
        connect( timer, &QTimer::timeout, this, [this]() {
            if( !nativeKeys_->refresh( referenceSurface_ ? referenceSurface_->winId() : WId( 0 ) ) && arrowMode_ )
            {
                setArrowMode( false );
                QMessageBox::warning( window(), tr( "Affect arrow control unavailable" ),
                    tr( "Exclusive arrow control could not be enabled for this video. Mouse control has been selected.\n%1" ).arg( nativeKeys_->error() ) );
            }
            if( ratingActive_ && ( !ratingWindowIsForeground()
#ifdef Q_OS_WIN
                || GetCapture() != reinterpret_cast<HWND>( winId() )
#endif
                ) ) interruptRating();
            if( ratingActive_ && rateAllowed_ && !rateAllowed_() ) releaseRating();
            if( panelPercent_ > 0 && referenceSurface_ )
            {
                const int combined = referenceSurface_->height() + this->height();
                const int height = qBound( 48,
                    qRound( combined * panelPercent_ / ( 100.0 + panelPercent_ ) ), 2160 );
                if( height != this->height() ) setFixedHeight( height );
            }
            const qint64 elapsed = clock_.restart();
            const double frequency = mapped( "oscillationFrequency" );
            phase_ = std::fmod( phase_ + qMin<qint64>( elapsed, 100 ) * .001 * 2 * M_PI * frequency, 2 * M_PI );
            update();
        } );
        timer->start( 33 );
    }

    ~FlubberPanel() Q_DECL_OVERRIDE
    {
        nativeKeys_.reset();
        releaseRating();
        qApp->removeEventFilter( this );
    }

    /* Call only on the Qt GUI thread. Other threads must use a queued call. */
    void setAffect( double valence, double arousal )
    {
        const double x = qBound( -1.0, valence, 1.0 );
        const double y = qBound( -1.0, arousal, 1.0 );
        if( x == affectX_ && y == affectY_ ) return;
        affectX_ = x;
        affectY_ = y;
        if( affectChanged_ ) affectChanged_( this->valence(), this->arousal() );
        update();
    }
    float valence() const { return float( affectX_ ); }
    float arousal() const { return float( affectY_ ); }
    FeedbackKind primary() const { return primary_; }
    bool visible( FeedbackKind kind ) const { return layers_[kind].visible; }
    ElementLayout layoutFor( FeedbackKind kind ) const { return layers_[kind]; }
    QString faceId() const { return faceId_; }
    static QString kindName( FeedbackKind kind )
    { return kind == GridKind ? QStringLiteral( "grid" ) : kind == FaceKind ? QStringLiteral( "face" ) : QStringLiteral( "flubber" ); }
    static FeedbackKind kindFromName( const QString &name )
    { return name == QLatin1String( "grid" ) ? GridKind : name == QLatin1String( "face" ) ? FaceKind : FlubberKind; }
    static QStringList faceIds()
    {
        QStringList ids{ QStringLiteral( "photo-reference-v3" ) };
        for( int i = 1; i <= 8; ++i ) ids << QStringLiteral( "photo-synthetic-%1" ).arg( i, 2, 10, QLatin1Char( '0' ) );
        return ids;
    }
    static QString faceLabel( const QString &id )
    {
        if( id == QLatin1String( "photo-reference-v3" ) ) return QObject::tr( "Original portrait" );
        const int index = faceIds().indexOf( id );
        return index > 0 ? QObject::tr( "Synthetic preset %1" ).arg( index + 1 ) : QString();
    }
    bool setFaceId( const QString &id, QString *error = NULL )
    {
        if( rejectAppearanceChange() ) return false;
        const QImage image = readFaceAtlas( id, error );
        if( image.isNull() ) return false;
        faceAtlas_ = image;
        faceId_ = id;
        settings_->setValue( "Flubber/faceId", id );
        update();
        return true;
    }
    static QImage readFaceAtlas( const QString &id, QString *error = NULL )
    {
        if( !faceIds().contains( id ) )
        { if( error ) *error = tr( "Unknown face preset." ); return {}; }
        const QString path = QCoreApplication::applicationDirPath() + "/assets/face/" + id + ".png";
        QImageReader reader( path, "png" );
        QImage image = reader.read();
        if( image.size() != QSize( 3360, 3360 ) )
        { if( error ) *error = tr( "The selected 21×21 face atlas is missing or invalid: %1 (%2)" )
            .arg( path, reader.errorString() ); return {}; }
        return image;
    }
    void setPrimary( FeedbackKind kind )
    {
        if( rejectAppearanceChange() ) return;
        if( kind != primary_ )
        {
            std::swap( layers_[kind].size, layers_[primary_].size );
            std::swap( layers_[kind].x, layers_[primary_].x );
            std::swap( layers_[kind].y, layers_[primary_].y );
            primary_ = kind;
            settings_->setValue( "Flubber/primary", kindName( kind ) );
            recordInteraction( QStringLiteral( "representation" ), { { "kind", kindName( kind ) } } );
        }
        normalizeVisibility();
        update();
    }
    void setVisible( FeedbackKind kind, bool enabled )
    {
        if( enabled ) setPrimary( kind );
        else if( kind == primary_ ) setPrimary( FlubberKind );
    }
    void setLayout( FeedbackKind kind, ElementLayout layout )
    {
        if( rejectAppearanceChange() ) return;
        layout.size = qBound( 10, layout.size, 100 );
        layout.x = qBound( 0, layout.x, 100 );
        layout.y = qBound( 0, layout.y, 100 );
        layout.visible = kind == primary_;
        layers_[kind] = layout;
        persistLayout( kind );
        update();
    }
    QJsonObject settingsJson() const
    {
        QJsonObject layers;
        for( int i = 0; i < 3; ++i )
        {
            const ElementLayout layer = layers_[i];
            layers.insert( kindName( static_cast<FeedbackKind>( i ) ), QJsonObject{
                { "visible", layer.visible }, { "size", layer.size }, { "x", layer.x }, { "y", layer.y } } );
        }
        QJsonObject grid = grid_;
        grid.insert( "columns", gridColumns_ );
        grid.insert( "rows", gridRows_ );
        return QJsonObject{ { "schema", "vlc-feedback-settings/v4" }, { "appearance", preset_ },
            { "stepPercent", stepPercent_ }, { "arrowHoldMs", arrowHoldMs_ },
            { "gridStyle", tiledGrid_ ? "tiles" : "continuous" },
            { "panelHeight", panelHeight_ }, { "primary", kindName( primary_ ) },
            { "inputMode", arrowMode_ ? "arrows" : "mouse" }, { "layers", layers },
            { "grid", grid }, { "face", QJsonObject{ { "id", faceId_ }, { "transitionRate", faceRate_ } } } };
    }
    bool saveSettingsFile( const QString &path, QString &error ) const
    {
        QSaveFile file( path );
        if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) ) { error = file.errorString(); return false; }
        if( file.write( QJsonDocument( settingsJson() ).toJson( QJsonDocument::Indented ) ) < 0 || !file.commit() )
        { error = file.errorString(); return false; }
        return true;
    }
    bool saveSettingsSnapshot( QString &error )
    {
        const QDir profile( QFileInfo( settings_->fileName() ).absolutePath() );
        const QString directory = profile.filePath( "flubber-presets" );
        if( !QDir().mkpath( directory ) )
        { error = tr( "Could not create settings directory: %1" ).arg( directory ); return false; }
        const QString filename = QStringLiteral( "flubber-settings-%1-%2.json" )
            .arg( QDateTime::currentDateTimeUtc().toString( "yyyyMMdd-HHmmsszzz" ),
                  QUuid::createUuid().toString( QUuid::WithoutBraces ) );
        const QString path = QDir( directory ).filePath( filename );
        if( !saveSettingsFile( path, error ) ) return false;
        QSettings profileSettings( settings_->fileName(), settings_->format() );
        const QVariant previousPath = profileSettings.value( "Flubber/lastSettingsPath" );
        profileSettings.setValue( "Flubber/lastSettingsPath", path );
        profileSettings.sync();
        if( profileSettings.status() != QSettings::NoError )
        {
            if( previousPath.isValid() ) profileSettings.setValue( "Flubber/lastSettingsPath", previousPath );
            else profileSettings.remove( "Flubber/lastSettingsPath" );
            error = tr( "Saved %1, but could not remember it in the VLC profile." ).arg( path );
            return false;
        }
        return true;
    }
    bool loadSettingsFile( const QString &path, QString &error )
    {
        if( rejectAppearanceChange() )
        { error = tr( "Flubber appearance is locked during recording." ); return false; }
        QFile file( path );
        if( !file.open( QIODevice::ReadOnly ) ) { error = file.errorString(); return false; }
        if( file.size() > 131072 ) { error = tr( "Settings JSON exceeds 128 KiB." ); return false; }
        const QByteArray bytes = file.readAll();
        QJsonParseError parse;
        const QJsonDocument document = QJsonDocument::fromJson( bytes, &parse );
        if( parse.error != QJsonParseError::NoError || !document.isObject() || !uniqueKeys( bytes ) )
        { error = tr( "Invalid settings JSON or duplicate key." ); return false; }
        const QJsonObject root = document.object();
        if( root.value( "schema" ).toString() == QLatin1String( "vlc-flubber-appearance/v1" ) )
        {
            QJsonObject appearance;
            if( !readAppearance( path, appearance, error ) ) return false;
            preset_ = appearance;
            settings_->setValue( "Flubber/appearancePath", QFileInfo( path ).absoluteFilePath() );
            settings_->setValue( "Flubber/appearanceJson", QJsonDocument( preset_ ).toJson( QJsonDocument::Compact ) );
            update();
            return true;
        }
        const QString schema = root.value( "schema" ).toString();
        const bool version4 = schema == QLatin1String( "vlc-feedback-settings/v4" );
        const bool version3 = schema == QLatin1String( "vlc-feedback-settings/v3" );
        const bool version2 = schema == QLatin1String( "vlc-feedback-settings/v2" );
        QJsonObject base = root;
        const QJsonValue hold = base.take( "arrowHoldMs" ), style = base.take( "gridStyle" );
        const QJsonValue step = base.take( "stepPercent" );
        if( ( !version4 && !version3 && !version2 ) ||
            !keys( base, { "schema", "appearance", "panelHeight", "primary", "inputMode", "layers", "grid", "face" } ) ||
            ( version2 ? !step.isUndefined() : ( !number( step, 1, 100 ) || step.toDouble() != step.toInt() ) ) ||
            ( version4 ? ( ( !number( hold, 0, 0 ) && ( !number( hold, MinHoldMs, MaxHoldMs ) ||
                         hold.toDouble() != hold.toInt() ) ) ||
                         ( style.toString() != QLatin1String( "tiles" ) && style.toString() != QLatin1String( "continuous" ) ) )
                       : ( !hold.isUndefined() || !style.isUndefined() ) ) ||
            !root.value( "appearance" ).isObject() || !number( root.value( "panelHeight" ), 48, 2160 ) ||
            !root.value( "layers" ).isObject() || !root.value( "grid" ).isObject() || !root.value( "face" ).isObject() )
        { error = tr( "Unexpected settings schema or fields." ); return false; }
        const QString primaryName = root.value( "primary" ).toString();
        const QString input = root.value( "inputMode" ).toString();
        if( !QStringList{ "flubber", "grid", "face" }.contains( primaryName ) ||
            !QStringList{ "arrows", "mouse" }.contains( input ) )
        { error = tr( "Invalid primary feedback or input mode." ); return false; }
        QJsonObject appearance;
        if( !validateAppearanceObject( root.value( "appearance" ).toObject(), appearance, error ) ) return false;
        const QJsonObject layers = root.value( "layers" ).toObject();
        if( !keys( layers, { "flubber", "grid", "face" } ) )
        { error = tr( "Expected three feedback layers." ); return false; }
        ElementLayout parsedLayers[3];
        for( int i = 0; i < 3; ++i )
        {
            const QJsonObject layer = layers.value( kindName( static_cast<FeedbackKind>( i ) ) ).toObject();
            if( !keys( layer, { "visible", "size", "x", "y" } ) ||
                !layer.value( "visible" ).isBool() || !number( layer.value( "size" ), 10, 100 ) ||
                !number( layer.value( "x" ), 0, 100 ) || !number( layer.value( "y" ), 0, 100 ) )
            { error = tr( "Invalid feedback layer layout." ); return false; }
            parsedLayers[i] = { layer.value( "visible" ).toBool(), layer.value( "size" ).toInt(),
                layer.value( "x" ).toInt(), layer.value( "y" ).toInt() };
        }
        const QJsonObject grid = root.value( "grid" ).toObject();
        if( !keys( grid, { "columns", "rows", "lineThickness", "showOutline", "outlineThickness", "cursorSize" } ) ||
            !number( grid.value( "columns" ), 3, 2001 ) || !number( grid.value( "rows" ), 3, 2001 ) ||
            !( grid.value( "columns" ).toInt() & 1 ) || !( grid.value( "rows" ).toInt() & 1 ) ||
            !number( grid.value( "lineThickness" ), 0, 20 ) || !grid.value( "showOutline" ).isBool() ||
            !number( grid.value( "outlineThickness" ), 0, 20 ) || !number( grid.value( "cursorSize" ), 0, 100 ) )
        { error = tr( "Invalid grid settings." ); return false; }
        const QJsonObject face = root.value( "face" ).toObject();
        if( !keys( face, { "id", "transitionRate" } ) || !faceIds().contains( face.value( "id" ).toString() ) ||
            !number( face.value( "transitionRate" ), .5, 20 ) )
        { error = tr( "Invalid face settings." ); return false; }
        QImageReader atlasReader( QCoreApplication::applicationDirPath() + "/assets/face/" + face.value( "id" ).toString() + ".png", "png" );
        QImage atlas = atlasReader.read();
        if( atlas.size() != QSize( 3360, 3360 ) )
        { error = tr( "The chosen face atlas is missing or invalid: %1" ).arg( atlasReader.errorString() ); return false; }
        preset_ = appearance;
        if( !version2 )
        {
            stepPercent_ = step.toInt();
            settings_->setValue( "Flubber/stepPercent", stepPercent_ );
        }
        if( version4 )
        {
            arrowHoldMs_ = hold.toInt();
            tiledGrid_ = style.toString() == QLatin1String( "tiles" );
            persistTravelSettings();
        }
        primary_ = kindFromName( primaryName );
        for( int i = 0; i < 3; ++i ) layers_[i] = parsedLayers[i];
        normalizeVisibility();
        grid_ = grid; grid_.remove( "columns" ); grid_.remove( "rows" );
        gridColumns_ = grid.value( "columns" ).toInt(); gridRows_ = grid.value( "rows" ).toInt();
        faceId_ = face.value( "id" ).toString(); faceAtlas_ = atlas;
        faceRate_ = face.value( "transitionRate" ).toDouble();
        if( panelPercent_ < 0 ) { panelHeight_ = root.value( "panelHeight" ).toInt(); setFixedHeight( panelHeight_ ); }
        setArrowMode( input == QLatin1String( "arrows" ) );
        settings_->setValue( "Flubber/primary", primaryName );
        settings_->setValue( "Flubber/panelHeight", panelHeight_ );
        settings_->setValue( "Flubber/gridColumns", gridColumns_ ); settings_->setValue( "Flubber/gridRows", gridRows_ );
        settings_->setValue( "Flubber/gridLineThickness", grid_.value( "lineThickness" ).toDouble() );
        settings_->setValue( "Flubber/gridShowOutline", grid_.value( "showOutline" ).toBool() );
        settings_->setValue( "Flubber/gridOutlineThickness", grid_.value( "outlineThickness" ).toDouble() );
        settings_->setValue( "Flubber/gridCursorSize", grid_.value( "cursorSize" ).toDouble() );
        settings_->setValue( "Flubber/faceId", faceId_ ); settings_->setValue( "Flubber/faceRate", faceRate_ );
        settings_->setValue( "Flubber/appearancePath", QString() );
        settings_->setValue( "Flubber/appearanceJson", QJsonDocument( preset_ ).toJson( QJsonDocument::Compact ) );
        update();
        return true;
    }
    QString appearanceLoadError() const { return appearanceLoadError_; }
    bool ratingActive() const { return ratingActive_; }
    void stopRating() { clearHeldArrows(); releaseRating(); }
    void setAffectChangedHandler( const std::function<void(float, float)> &handler ) { affectChanged_ = handler; }
    void setInteractionHandler( const std::function<void(const QJsonObject &)> &handler ) { interaction_ = handler; }
    void recordInteraction( const QString &event, QJsonObject data = QJsonObject() )
    {
        if( !interaction_ ) return;
        data.insert( "event", event );
        interaction_( data );
    }
    bool appearanceLocked() const { return appearanceLocked_; }
    void setAppearanceLocked( bool locked )
    {
        if( appearanceLocked_ == locked ) return;
        appearanceLocked_ = locked;
        if( preview_ ) update();
        recordInteraction( QStringLiteral( "appearance-lock" ), { { "locked", locked } } );
    }
    bool rejectAppearanceChange()
    {
        if( !appearanceLocked_ ) return false;
        recordInteraction( QStringLiteral( "appearance-blocked" ) );
        return true;
    }
    bool arrowMode() const { return arrowMode_; }
    void setArrowMode( bool enabled, bool syncNative = true )
    {
        if( enabled != arrowMode_ )
            recordInteraction( QStringLiteral( "affect-input" ),
                               { { "mode", enabled ? QStringLiteral( "arrows" ) : QStringLiteral( "mouse" ) } } );
        stopRating();
        arrowMode_ = enabled;
        if( syncNative && nativeKeys_ ) nativeKeys_->setArrowMode( enabled );
        settings_->setValue( "Flubber/inputMode", enabled ? "arrows" : "mouse" );
        updateInputTooltip();
        if( enabled && ratingWindowIsForeground() ) setFocus( Qt::ShortcutFocusReason );
    }
    void setVideoAffect( double valence, double arousal )
    {
        if( !arrowMode_ && !ratingActive_ ) setAffect( valence, arousal );
    }
    bool handleInputKey( QKeyEvent *event )
    {
        const unsigned arrow = flubberArrow( event->key() );
        if( event->type() == QEvent::KeyRelease )
        {
            if( !arrow || !arrowMode_ ) return false;
            if( !event->isAutoRepeat() ) releaseArrow( arrow );
            event->accept();
            return true;
        }
        if( event->type() != QEvent::KeyPress && event->type() != QEvent::ShortcutOverride ) return false;
        if( textEditorOwnsShortcut() ) return false;
        const Qt::KeyboardModifiers modifiers = event->modifiers() & ~Qt::KeypadModifier;
        if( handleSettingsKey( event, modifiers ) ) return true;
        if( event->key() == Qt::Key_F && modifiers == Qt::AltModifier )
        {
            if( event->type() == QEvent::KeyPress && !event->isAutoRepeat() ) setArrowMode( !arrowMode_ );
            event->accept();
            return true;
        }
        if( !arrowMode_ || !arrow ) return false;
        // Claim arrows before Qt/VLC shortcuts. Keep the existing supported-input
        // gate (including pause); ShortcutOverride never changes affect.
        if( event->type() == QEvent::KeyPress && !event->isAutoRepeat() )
        {
            if( modifiers || ( rateAllowed_ && !rateAllowed_() ) ) clearHeldArrows();
            else if( !( heldArrows_ & arrow ) )
            {
                heldArrows_ |= arrow;
                moveHeldArrows( tapDistance() );
                holdClock_.start();
                holdLastNs_ = 0;
                arrowRepeat_.start();
            }
        }
        event->accept();
        return true;
    }
    bool handleSettingsKey( QKeyEvent *event, Qt::KeyboardModifiers modifiers )
    {
        const int key = event->key();
        const bool cycle = key == Qt::Key_G && ( modifiers == Qt::AltModifier ||
            modifiers == ( Qt::AltModifier | Qt::ShiftModifier ) );
        const bool faceCycle = key == Qt::Key_H && ( modifiers == Qt::AltModifier ||
            modifiers == ( Qt::AltModifier | Qt::ShiftModifier ) );
        const bool altLayout = ( modifiers == Qt::AltModifier &&
            ( key == Qt::Key_W || key == Qt::Key_A || key == Qt::Key_S || key == Qt::Key_D ||
              key == Qt::Key_Plus || key == Qt::Key_Equal || key == Qt::Key_Minus ) ) ||
            ( modifiers == ( Qt::AltModifier | Qt::ShiftModifier ) && key == Qt::Key_Plus );
        const bool snapshot = key == Qt::Key_J && modifiers == Qt::AltModifier;
        const bool layoutArrow = arrowMode_ && flubberArrow( key ) &&
            modifiers == ( Qt::ControlModifier | Qt::AltModifier );
        int settingsKey = key;
#ifdef Q_OS_WIN
        if( event->nativeVirtualKey() >= '1' && event->nativeVirtualKey() <= '3' )
            settingsKey = Qt::Key_1 + int( event->nativeVirtualKey() - '1' );
#endif
        const int target = flubberSettingsTarget( settingsKey, modifiers );
        const bool controls = target >= 0 ||
            ( key == Qt::Key_S && modifiers == ( Qt::ControlModifier | Qt::ShiftModifier ) );
        if( !cycle && !faceCycle && !layoutArrow && !altLayout && !snapshot && !controls ) return false;
        event->accept();
        if( event->type() != QEvent::KeyPress || event->isAutoRepeat() ) return true;
        if( !snapshot && rejectAppearanceChange() ) return true;
        if( snapshot )
        {
            QString error;
            if( !saveSettingsSnapshot( error ) )
            {
                releaseRating();
                QMessageBox::warning( window(), tr( "Could not save Flubber settings" ), error );
            }
            else QToolTip::showText( mapToGlobal( rect().center() ),
                tr( "Settings saved. Restored automatically next launch.\n%1" )
                    .arg( settings_->value( "Flubber/lastSettingsPath" ).toString() ), this );
        }
        else if( cycle )
        {
            const int direction = modifiers.testFlag( Qt::ShiftModifier ) ? -1 : 1;
            setPrimary( static_cast<FeedbackKind>( ( int( primary_ ) + direction + 3 ) % 3 ) );
        }
        else if( faceCycle )
        {
            const QStringList ids = faceIds();
            const int direction = modifiers.testFlag( Qt::ShiftModifier ) ? -1 : 1;
            const int index = qMax( 0, ids.indexOf( faceId_ ) );
            QString error;
            if( setFaceId( ids[( index + direction + ids.size() ) % ids.size()], &error ) ) setPrimary( FaceKind );
            else setToolTip( error );
        }
        else if( controls ) showControls( window(), target );
        else
        {
            ElementLayout changed = layers_[primary_];
            if( key == Qt::Key_Left || key == Qt::Key_A ) changed.x -= 2;
            if( key == Qt::Key_Right || key == Qt::Key_D ) changed.x += 2;
            if( key == Qt::Key_Up || key == Qt::Key_W ) changed.y -= 2;
            if( key == Qt::Key_Down || key == Qt::Key_S ) changed.y += 2;
            if( key == Qt::Key_Equal || key == Qt::Key_Plus ) changed.size += 2;
            if( key == Qt::Key_Minus ) changed.size -= 2;
            setLayout( primary_, changed );
        }
        update();
        return true;
    }
    void setInterruptHandler( const std::function<void()> &handler ) { interrupted_ = handler; }
    void setRateAllowedHandler( const std::function<bool()> &handler ) { rateAllowed_ = handler; }
    void setReferenceSurface( QWidget *surface ) { referenceSurface_ = surface; }

    void showControls( QWidget *parent, int target = -1 )
    {
        if( rejectAppearanceChange() ) return;
        stopRating();
        const bool scoped = target >= FlubberKind && target <= FaceKind;
        const bool editsFlubber = !scoped || target == FlubberKind;
        const bool editsGrid = !scoped || target == GridKind;
        const bool editsFace = !scoped || target == FaceKind;
        QDialog dialog( parent );
        const QString titles[] = { tr( "Flubber appearance" ), tr( "2D grid settings" ), tr( "Face morph settings" ) };
        dialog.setWindowTitle( scoped ? titles[target] : tr( "Flubber Settings" ) );
        if( !scoped ) dialog.resize( 660, 460 );
        QJsonObject pendingPreset = preset_;
        QString pendingPath = settings_->value( "Flubber/appearancePath" ).toString();
        QVBoxLayout *layout = new QVBoxLayout( &dialog );
        QTabWidget *tabs = new QTabWidget( &dialog );
        layout->addWidget( tabs );
        auto page = [tabs]( const QString &title, bool shown ) {
            QWidget *body = new QWidget( tabs );
            QVBoxLayout *layout = new QVBoxLayout( body );
            QFormLayout *form = new QFormLayout;
            form->setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );
            layout->addLayout( form );
            layout->addStretch();
            if( shown ) tabs->addTab( body, title ); else body->hide();
            return form;
        };
        auto real = [&dialog]( double min, double max, double value, int decimals = 2 ) {
            QDoubleSpinBox *box = new QDoubleSpinBox( &dialog );
            box->setRange( min, max ); box->setDecimals( decimals ); box->setValue( value );
            box->setSingleStep( decimals == 1 ? .1 : .05 );
            return box;
        };
        QFormLayout *display = page( tr( "Input && display" ), !scoped );
        QFormLayout *flubberForm = page( scoped ? tr( "Appearance" ) : tr( "Flubber" ), editsFlubber );
        QFormLayout *colorForm = !scoped || target == FlubberKind ? page( tr( "Colors" ), true ) : NULL;
        QFormLayout *gridForm = page( tr( "2D grid" ), editsGrid );
        if( !colorForm ) colorForm = page( tr( "Colors" ), editsGrid );
        QFormLayout *layoutForm = page( tr( "Layout" ), !scoped );
        QFormLayout *mappingForm = page( tr( "Mappings" ), editsFlubber );
        QFormLayout *faceForm = page( tr( "Face morph" ), editsFace );
        if( tabs->count() == 1 ) tabs->tabBar()->hide();
        std::function<void()> refreshPreview;
        QSpinBox *height = spin( &dialog, 48, 2160, panelHeight_, tr( " px" ) );
        height->setObjectName( "flubber-panel-height" );
        if( panelPercent_ > 0 )
        {
            height->setValue( this->height() );
            height->setEnabled( false );
            height->setToolTip( tr( "Panel height is controlled by this Recorder session." ) );
        }
        layoutForm->addRow( tr( "Bottom panel height" ), height );
        QComboBox *primaryBox = new QComboBox( &dialog );
        primaryBox->setObjectName( "flubber-representation" );
        primaryBox->addItems( { tr( "Flubber" ), tr( "2D grid" ), tr( "Face morph" ) } );
        primaryBox->setCurrentIndex( primary_ ); display->addRow( tr( "Visible representation" ), primaryBox );
        QComboBox *inputBox = new QComboBox( &dialog );
        inputBox->addItems( { tr( "Mouse" ), tr( "Arrow keys" ) } );
        inputBox->setCurrentIndex( arrowMode_ ? 1 : 0 ); display->addRow( tr( "Input mode" ), inputBox );
        QSpinBox *tap = spin( &dialog, 1, 100, stepPercent_, tr( " %" ) );
        display->addRow( tr( "Tap step (% of half-axis)" ), tap );
        QCheckBox *customHold = new QCheckBox( tr( "Set a target hold time" ), &dialog );
        customHold->setObjectName( "flubber-custom-hold" );
        customHold->setChecked( arrowHoldMs_ > 0 );
        display->addRow( customHold );
        QSpinBox *hold = spin( &dialog, MinHoldMs, MaxHoldMs, arrowHoldMs_ ? arrowHoldMs_ : 2000, tr( " ms" ) );
        hold->setObjectName( "flubber-hold-duration" );
        hold->setSingleStep( 250 );
        hold->setEnabled( customHold->isChecked() );
        connect( customHold, &QCheckBox::toggled, hold, &QWidget::setEnabled );
        display->addRow( tr( "Full-axis hold time" ), hold );
        QLabel *holdHelp = new QLabel( tr( "Time from one edge to the other while holding one arrow. "
            "Turn off the target time to repeat the tap step at the original rate." ), &dialog );
        holdHelp->setWordWrap( true );
        display->addRow( holdHelp );
        QJsonObject visual = pendingPreset.value( "visual" ).toObject();
        QJsonObject shape = visual.value( "flubber" ).toObject();
        QJsonObject presentation = pendingPreset.value( "presentation" ).toObject();
        QJsonObject halo = presentation.value( "halo" ).toObject();
        QWidget *transparencyRow = new QWidget( &dialog );
        QHBoxLayout *transparencyLayout = new QHBoxLayout( transparencyRow );
        transparencyLayout->setContentsMargins( 0, 0, 0, 0 );
        QSlider *transparencySlider = new QSlider( Qt::Horizontal, transparencyRow );
        transparencySlider->setRange( 0, 100 );
        transparencySlider->setObjectName( "flubber-transparency" );
        transparencySlider->setAccessibleName( tr( "Transparency" ) );
        QSpinBox *transparency = spin( transparencyRow, 0, 100,
            qRound( 100 * visual.value( "transparency" ).toDouble() ), tr( " %" ) );
        transparency->setAccessibleName( tr( "Transparency percentage" ) );
        transparencySlider->setValue( transparency->value() );
        transparencyLayout->addWidget( transparencySlider, 1 );
        transparencyLayout->addWidget( transparency );
        connect( transparencySlider, &QSlider::valueChanged, transparency, &QSpinBox::setValue );
        connect( transparency, QOverload<int>::of( &QSpinBox::valueChanged ), transparencySlider, &QSlider::setValue );
        QCheckBox *outline = new QCheckBox( &dialog ); outline->setChecked( shape.value( "showOutline" ).toBool() );
        QDoubleSpinBox *outlineThickness = real( 0, 20, shape.value( "outlineThickness" ).toDouble() );
        QCheckBox *showHalo = new QCheckBox( &dialog ); showHalo->setChecked( shape.value( "showHalo" ).toBool() );
        QDoubleSpinBox *haloWidth = real( 0, 10000, halo.value( "widthPercent" ).toDouble() );
        QCheckBox *haloGradient = new QCheckBox( &dialog ); haloGradient->setChecked( halo.value( "gradient" ).toBool() );
        QDoubleSpinBox *haloSteepness = real( .1, 10, halo.value( "steepness" ).toDouble() );
        QComboBox *anchors = new QComboBox( &dialog ); anchors->addItems( { tr( "Axes" ), tr( "Corners" ) } );
        anchors->setCurrentIndex( presentation.value( "colorAnchors" ).toString() == QLatin1String( "corners" ) ? 1 : 0 );
        QFormLayout *sharedAppearance = scoped && target == GridKind ? gridForm : flubberForm;
        if( sharedAppearance == flubberForm )
            flubberForm->addRow( scoped ? tr( "Transparency (shared)" ) : tr( "Transparency" ), transparencyRow );
        flubberForm->addRow( tr( "Outline" ), outline ); flubberForm->addRow( tr( "Outline thickness" ), outlineThickness );
        flubberForm->addRow( tr( "Halo" ), showHalo ); flubberForm->addRow( tr( "Halo width (%)" ), haloWidth );
        flubberForm->addRow( tr( "Halo gradient" ), haloGradient ); flubberForm->addRow( tr( "Halo steepness" ), haloSteepness );
        if( sharedAppearance == flubberForm )
            flubberForm->addRow( scoped ? tr( "Color anchors (shared)" ) : tr( "Color anchors" ), anchors );
        const char *colorNames[] = { "up", "down", "left", "right", "idle", "outline", "halo", "cursor" };
        const QString colorLabels[] = { tr( "High arousal" ), tr( "Low arousal" ), tr( "Negative valence" ), tr( "Positive valence" ),
            tr( "Idle" ), tr( "Outline" ), tr( "Halo" ), tr( "Cursor" ) };
        const auto setColor = []( QPushButton *button, const QString &color ) {
            button->setText( color );
            QPixmap swatch( 16, 16 );
            swatch.fill( QColor( color ) );
            button->setIcon( QIcon( swatch ) );
        };
        QPushButton *colorButtons[8];
        for( int i = 0; i < 8; ++i )
        {
            colorButtons[i] = new QPushButton( &dialog );
            const QString color = visual.value( "colors" ).toObject().value( QLatin1String( colorNames[i] ) ).toString();
            colorButtons[i]->setAccessibleName( tr( "%1 color" ).arg( colorLabels[i] ) );
            setColor( colorButtons[i], color );
            const bool showColor = !scoped || ( target == GridKind && i != 4 && i != 6 ) ||
                ( target == FlubberKind && i != 7 );
            const bool sharedColor = i < 4 || i == 5;
            if( showColor ) colorForm->addRow( scoped && sharedColor ?
                tr( "%1 (shared)" ).arg( colorLabels[i] ) : colorLabels[i], colorButtons[i] );
            else colorButtons[i]->hide();
            connect( colorButtons[i], &QPushButton::clicked, &dialog, [&, i]() {
                const QColor chosen = QColorDialog::getColor( QColor( colorButtons[i]->text() ), &dialog );
                if( chosen.isValid() )
                {
                    setColor( colorButtons[i], chosen.name() );
                    if( refreshPreview ) refreshPreview();
                }
            } );
        }
        const char *mappingNames[] = { "oscillationFrequency", "edgeSmoothness", "projectionAmplitude",
            "pulseSynchrony", "waveSizeVariation", "saturation" };
        const QString mappingLabels[] = { tr( "Oscillation speed" ), tr( "Edge smoothness" ),
            tr( "Projection size" ), tr( "Pulse synchrony" ), tr( "Wave size variation" ),
            tr( "Color saturation" ) };
        QDoubleSpinBox *mappingMin[6], *mappingMax[6]; QComboBox *mappingDriver[6]; QCheckBox *mappingReverse[6];
        const QString drivers[] = { "x-axis", "y-axis", "angle", "radius" };
        for( int i = 0; i < 6; ++i )
        {
            const QJsonObject entry = pendingPreset.value( "mappings" ).toObject().value( QLatin1String( mappingNames[i] ) ).toObject();
            QWidget *row = new QWidget( &dialog ); QHBoxLayout *line = new QHBoxLayout( row ); line->setContentsMargins( 0, 0, 0, 0 );
            mappingMin[i] = real( 0, i == 0 ? 10 : 1, entry.value( "min" ).toDouble() );
            mappingMax[i] = real( 0, i == 0 ? 10 : 1, entry.value( "max" ).toDouble() );
            mappingDriver[i] = new QComboBox( row ); mappingDriver[i]->addItems( { "x-axis", "y-axis", "angle", "radius" } );
            for( int d = 0; d < 4; ++d ) if( entry.value( "drivenBy" ).toString() == drivers[d] ) mappingDriver[i]->setCurrentIndex( d );
            mappingReverse[i] = new QCheckBox( tr( "Reverse" ), row ); mappingReverse[i]->setChecked( entry.value( "reverse" ).toBool() );
            line->addWidget( new QLabel( tr( "Min" ), row ) ); line->addWidget( mappingMin[i] );
            line->addWidget( new QLabel( tr( "Max" ), row ) ); line->addWidget( mappingMax[i] );
            line->addWidget( mappingDriver[i] ); line->addWidget( mappingReverse[i] );
            mappingForm->addRow( mappingLabels[i], row );
        }
        QComboBox *gridStyle = new QComboBox( &dialog );
        gridStyle->setObjectName( "flubber-grid-style" );
        gridStyle->addItem( tr( "Tiles" ), "tiles" );
        gridStyle->addItem( tr( "Continuous" ), "continuous" );
        gridStyle->setCurrentIndex( tiledGrid_ ? 0 : 1 );
        gridForm->addRow( tr( "Grid style" ), gridStyle );
        QSpinBox *columns = spin( &dialog, 3, 2001, gridColumns_, QString() ); columns->setSingleStep( 2 );
        QSpinBox *rows = spin( &dialog, 3, 2001, gridRows_, QString() ); rows->setSingleStep( 2 );
        QDoubleSpinBox *gridLine = real( 0, 20, grid_.value( "lineThickness" ).toDouble() );
        QCheckBox *gridOutline = new QCheckBox( &dialog ); gridOutline->setChecked( grid_.value( "showOutline" ).toBool() );
        QDoubleSpinBox *gridOutlineThickness = real( 0, 20, grid_.value( "outlineThickness" ).toDouble() );
        QDoubleSpinBox *cursorSize = real( 0, 100, grid_.value( "cursorSize" ).toDouble() );
        gridForm->addRow( tr( "Tile columns (odd)" ), columns );
        gridForm->addRow( tr( "Tile rows (odd)" ), rows );
        gridForm->addRow( tr( "Line thickness" ), gridLine );
        gridOutline->setText( tr( "Show outline" ) );
        gridForm->addRow( gridOutline );
        gridForm->addRow( tr( "Outline thickness" ), gridOutlineThickness );
        gridForm->addRow( tr( "Cursor size" ), cursorSize );
        if( sharedAppearance == gridForm )
        {
            gridForm->addRow( tr( "Transparency (shared)" ), transparencyRow );
            gridForm->addRow( tr( "Color anchors (shared)" ), anchors );
        }
        const auto updateGridControls = [=]( int index ) {
            columns->setEnabled( index == 0 );
            rows->setEnabled( index == 0 );
            gridLine->setEnabled( index == 0 );
        };
        connect( gridStyle, QOverload<int>::of( &QComboBox::currentIndexChanged ), &dialog, updateGridControls );
        updateGridControls( gridStyle->currentIndex() );
        QComboBox *face = new QComboBox( &dialog );
        for( const QString &id : faceIds() ) face->addItem( faceLabel( id ), id );
        face->setCurrentIndex( qMax( 0, faceIds().indexOf( faceId_ ) ) );
        QDoubleSpinBox *faceRate = real( .5, 20, faceRate_, 1 );
        faceForm->addRow( tr( "Portrait preset" ), face ); faceForm->addRow( tr( "Transition speed (states/s)" ), faceRate );
        QSpinBox *size[3], *x[3], *y[3];
        for( int i = 0; i < 3; ++i )
        {
            const QString name = i == 0 ? tr( "Flubber" ) : i == 1 ? tr( "2D grid" ) : tr( "Face morph" );
            size[i] = spin( &dialog, 10, 100, layers_[i].size, tr( " %" ) );
            x[i] = spin( &dialog, 0, 100, layers_[i].x, tr( " %" ) );
            y[i] = spin( &dialog, 0, 100, layers_[i].y, tr( " %" ) );
            size[i]->setObjectName( QStringLiteral( "flubber-layer-%1-size" ).arg( i ) );
            x[i]->setObjectName( QStringLiteral( "flubber-layer-%1-x" ).arg( i ) );
            y[i]->setObjectName( QStringLiteral( "flubber-layer-%1-y" ).arg( i ) );
            QWidget *row = new QWidget( &dialog ); QHBoxLayout *line = new QHBoxLayout( row );
            line->setContentsMargins( 0, 0, 0, 0 );
            line->addWidget( new QLabel( tr( "Size" ), row ) ); line->addWidget( size[i] );
            line->addWidget( new QLabel( tr( "X" ), row ) ); line->addWidget( x[i] );
            line->addWidget( new QLabel( tr( "Y" ), row ) ); line->addWidget( y[i] );
            size[i]->setAccessibleName( tr( "%1 size" ).arg( name ) );
            x[i]->setAccessibleName( tr( "%1 horizontal position" ).arg( name ) );
            y[i]->setAccessibleName( tr( "%1 vertical position" ).arg( name ) );
            QFormLayout *forms[] = { flubberForm, gridForm, faceForm };
            if( !scoped ) layoutForm->addRow( name, row );
            else if( i == target && i != FlubberKind ) forms[i]->addRow( row );
            else if( i == target ) forms[i]->addRow( tr( "Layout" ), row );
            else row->hide();
        }
        QWidget *fileControls = new QWidget( &dialog );
        QHBoxLayout *fileButtons = new QHBoxLayout( fileControls );
        fileButtons->setContentsMargins( 0, 0, 0, 0 );
        QPushButton *loadAppearance = new QPushButton( tr( "Load appearance JSON…" ), &dialog );
        QPushButton *loadSettings = new QPushButton( tr( "Load settings JSON…" ), &dialog );
        QPushButton *saveAppearance = new QPushButton( tr( "Export appearance…" ), &dialog );
        QPushButton *saveSettings = new QPushButton( tr( "Export settings…" ), &dialog );
        fileButtons->addWidget( loadAppearance ); fileButtons->addWidget( loadSettings );
        fileButtons->addWidget( saveAppearance ); fileButtons->addWidget( saveSettings );
        layout->addWidget( fileControls );
        fileControls->setVisible( !scoped );
        QLabel *current = new QLabel( pendingPath.isEmpty() ? tr( "Default Flubber appearance" ) : pendingPath, &dialog );
        current->setWordWrap( true );
        current->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Preferred );
        current->setToolTip( pendingPath );
        layout->addWidget( current );
        current->setVisible( !scoped );
        connect( loadAppearance, &QPushButton::clicked, &dialog, [&]() {
            const QString path = QFileDialog::getOpenFileName( &dialog,
                tr( "Load Flubber appearance" ), QString(), tr( "JSON files (*.json)" ) );
            if( path.isEmpty() ) return;
            QJsonObject next;
            QString error;
            if( !readAppearance( path, next, error ) )
            {
                QMessageBox::warning( &dialog, tr( "Invalid Flubber appearance" ), error );
                return;
            }
            pendingPreset = next; pendingPath = QFileInfo( path ).absoluteFilePath(); current->setText( path ); current->setToolTip( path );
            const QJsonObject v = next.value( "visual" ).toObject(), s = v.value( "flubber" ).toObject();
            const QJsonObject p = next.value( "presentation" ).toObject(), h = p.value( "halo" ).toObject();
            transparency->setValue( qRound( 100 * v.value( "transparency" ).toDouble() ) );
            outline->setChecked( s.value( "showOutline" ).toBool() ); outlineThickness->setValue( s.value( "outlineThickness" ).toDouble() );
            showHalo->setChecked( s.value( "showHalo" ).toBool() ); haloWidth->setValue( h.value( "widthPercent" ).toDouble() );
            haloGradient->setChecked( h.value( "gradient" ).toBool() ); haloSteepness->setValue( h.value( "steepness" ).toDouble() );
            anchors->setCurrentIndex( p.value( "colorAnchors" ).toString() == QLatin1String( "corners" ) ? 1 : 0 );
            for( int i = 0; i < 8; ++i )
            { const QString c = v.value( "colors" ).toObject().value( QLatin1String( colorNames[i] ) ).toString();
                setColor( colorButtons[i], c ); }
            for( int i = 0; i < 6; ++i )
            { const QJsonObject m = next.value( "mappings" ).toObject().value( QLatin1String( mappingNames[i] ) ).toObject();
                mappingMin[i]->setValue( m.value( "min" ).toDouble() ); mappingMax[i]->setValue( m.value( "max" ).toDouble() );
                mappingDriver[i]->setCurrentText( m.value( "drivenBy" ).toString() );
                mappingReverse[i]->setChecked( m.value( "reverse" ).toBool() ); }
        } );
        QString requestedFile;
        connect( loadSettings, &QPushButton::clicked, &dialog, [&]() {
            requestedFile = QFileDialog::getOpenFileName( &dialog, tr( "Load Flubber settings" ), QString(), tr( "JSON files (*.json)" ) );
            if( !requestedFile.isEmpty() ) dialog.done( 2 );
        } );
        connect( saveAppearance, &QPushButton::clicked, &dialog, [&]() {
            requestedFile = QFileDialog::getSaveFileName( &dialog, tr( "Export Flubber appearance" ), "flubber-appearance.json", tr( "JSON files (*.json)" ) );
            if( !requestedFile.isEmpty() ) dialog.done( 3 );
        } );
        connect( saveSettings, &QPushButton::clicked, &dialog, [&]() {
            requestedFile = QFileDialog::getSaveFileName( &dialog, tr( "Export Flubber settings" ), "flubber-settings.json", tr( "JSON files (*.json)" ) );
            if( !requestedFile.isEmpty() ) dialog.done( 4 );
        } );
        QDialogButtonBox *buttons = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog );
        layout->addWidget( buttons );
        connect( buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept );
        connect( buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject );
        const auto appearanceDraft = [&]() {
            QJsonObject next = pendingPreset;
            QJsonObject visual = next.value( "visual" ).toObject();
            QJsonObject presentation = next.value( "presentation" ).toObject();
            QJsonObject colors = visual.value( "colors" ).toObject();
            if( transparency->value() != qRound( 100 * visual.value( "transparency" ).toDouble() ) )
                visual.insert( "transparency", transparency->value() / 100.0 );
            for( int i = 0; i < 8; ++i ) colors.insert( QLatin1String( colorNames[i] ), colorButtons[i]->text() );
            visual.insert( "colors", colors );
            presentation.insert( "colorAnchors", anchors->currentIndex() == 1 ? "corners" : "axes" );
            if( editsFlubber )
            {
                QJsonObject shape = visual.value( "flubber" ).toObject();
                shape.insert( "showOutline", outline->isChecked() ); shape.insert( "outlineThickness", outlineThickness->value() );
                shape.insert( "showHalo", showHalo->isChecked() ); visual.insert( "flubber", shape );
                QJsonObject halo = presentation.value( "halo" ).toObject();
                halo.insert( "widthPercent", haloWidth->value() ); halo.insert( "gradient", haloGradient->isChecked() );
                halo.insert( "steepness", haloSteepness->value() ); presentation.insert( "halo", halo );
                QJsonObject mappings = next.value( "mappings" ).toObject();
                for( int i = 0; i < 6; ++i ) mappings.insert( QLatin1String( mappingNames[i] ), QJsonObject{
                    { "min", mappingMin[i]->value() }, { "max", mappingMax[i]->value() },
                    { "drivenBy", mappingDriver[i]->currentText() }, { "reverse", mappingReverse[i]->isChecked() } } );
                next.insert( "mappings", mappings );
            }
            next.insert( "visual", visual );
            next.insert( "presentation", presentation );
            return next;
        };
        const auto gridDraft = [&]() {
            return QJsonObject{ { "lineThickness", gridLine->value() }, { "showOutline", gridOutline->isChecked() },
                { "outlineThickness", gridOutlineThickness->value() }, { "cursorSize", cursorSize->value() } };
        };
        const auto layoutDraft = [&]( int i ) {
            return ElementLayout{ i == primary_, size[i]->value(), x[i]->value(), y[i]->value() };
        };
        FeedbackPreview draft;
        QString previewFace = faceId_;
        if( scoped )
        {
            draft.kind = static_cast<FeedbackKind>( target );
            draft.atlas = faceAtlas_;
            refreshPreview = [&]() {
                draft.layout = layoutDraft( target );
                if( editsFlubber || editsGrid ) draft.appearance = appearanceDraft();
                if( editsGrid )
                {
                    draft.grid = gridDraft();
                    draft.tiles = gridStyle->currentIndex() == 0 ? QSize( columns->value(), rows->value() ) : QSize();
                }
                if( editsFace )
                {
                    draft.faceRate = faceRate->value();
                    const QString id = face->currentData().toString();
                    if( id != previewFace )
                    {
                        draft.atlas = readFaceAtlas( id );
                        previewFace = id;
                    }
                }
                update();
            };
            for( auto *box : dialog.findChildren<QSpinBox *>() )
                connect( box, QOverload<int>::of( &QSpinBox::valueChanged ), &dialog, refreshPreview );
            for( auto *box : dialog.findChildren<QDoubleSpinBox *>() )
                connect( box, QOverload<double>::of( &QDoubleSpinBox::valueChanged ), &dialog, refreshPreview );
            for( auto *box : dialog.findChildren<QComboBox *>() )
                connect( box, QOverload<int>::of( &QComboBox::currentIndexChanged ), &dialog, refreshPreview );
            for( auto *box : dialog.findChildren<QCheckBox *>() )
                connect( box, &QCheckBox::toggled, &dialog, refreshPreview );
            refreshPreview();
            const QSize preferred[] = { QSize( 590, 460 ), QSize( 560, 460 ), QSize( 470, 205 ) };
            placeFeedbackPopup( dialog, parent, preferred[target] );
        }
        int result;
        {
            QScopedValueRollback<const FeedbackPreview *> preview( preview_, scoped ? &draft : NULL );
            result = dialog.exec();
        }
        if( scoped ) update();
        if( result == 2 )
        {
            QString error;
            if( !loadSettingsFile( requestedFile, error ) ) QMessageBox::warning( parent, tr( "Invalid Flubber settings" ), error );
            return;
        }
        if( result == QDialog::Accepted || result == 3 || result == 4 )
        {
            if( rejectAppearanceChange() ) return;
            if( editsGrid && ( ( columns->value() & 1 ) == 0 || ( rows->value() & 1 ) == 0 ) )
            { QMessageBox::warning( parent, tr( "Invalid grid" ), tr( "Grid columns and rows must be odd." ) ); return; }
            for( int i = 0; editsFlubber && i < 6; ++i ) if( mappingMin[i]->value() > mappingMax[i]->value() )
            { QMessageBox::warning( parent, tr( "Invalid mapping" ), tr( "Each mapping minimum must not exceed its maximum." ) ); return; }
            QString faceError;
            if( editsFace && !setFaceId( face->currentData().toString(), &faceError ) )
            { QMessageBox::warning( parent, tr( "Invalid face atlas" ), faceError ); return; }
            if( editsFlubber || editsGrid )
            {
                preset_ = appearanceDraft();
                settings_->setValue( "Flubber/appearancePath", pendingPath );
                settings_->setValue( "Flubber/appearanceJson", QJsonDocument( preset_ ).toJson( QJsonDocument::Compact ) );
            }
            const FeedbackKind selected = static_cast<FeedbackKind>( scoped ? target : primaryBox->currentIndex() );
            if( scoped && selected != primary_ )
                recordInteraction( QStringLiteral( "representation" ), { { "kind", kindName( selected ) } } );
            primary_ = selected;
            settings_->setValue( "Flubber/primary", kindName( primary_ ) );
            if( scoped )
            {
                normalizeVisibility();
                setLayout( selected, layoutDraft( target ) );
            }
            else
            {
                if( panelPercent_ < 0 ) { panelHeight_ = height->value(); setFixedHeight( panelHeight_ );
                    settings_->setValue( "Flubber/panelHeight", panelHeight_ ); }
                for( int i = 0; i < 3; ++i ) setLayout( static_cast<FeedbackKind>( i ), layoutDraft( i ) );
                stepPercent_ = tap->value();
                arrowHoldMs_ = customHold->isChecked() ? hold->value() : 0;
                settings_->setValue( "Flubber/stepPercent", stepPercent_ );
                settings_->setValue( "Flubber/arrowHoldMs", arrowHoldMs_ );
                setArrowMode( inputBox->currentIndex() == 1 );
            }
            if( editsGrid )
            {
                tiledGrid_ = gridStyle->currentIndex() == 0;
                gridColumns_ = columns->value(); gridRows_ = rows->value();
                grid_ = gridDraft();
                settings_->setValue( "Flubber/gridStyle", tiledGrid_ ? "tiles" : "continuous" );
                settings_->setValue( "Flubber/gridColumns", gridColumns_ ); settings_->setValue( "Flubber/gridRows", gridRows_ );
                settings_->setValue( "Flubber/gridLineThickness", gridLine->value() );
                settings_->setValue( "Flubber/gridShowOutline", gridOutline->isChecked() );
                settings_->setValue( "Flubber/gridOutlineThickness", gridOutlineThickness->value() );
                settings_->setValue( "Flubber/gridCursorSize", cursorSize->value() );
            }
            if( editsFace ) { faceRate_ = faceRate->value(); settings_->setValue( "Flubber/faceRate", faceRate_ ); }
            update();
            if( result == 3 || result == 4 )
            {
                QSaveFile file( requestedFile );
                QString error;
                if( result == 4 ) { if( !saveSettingsFile( requestedFile, error ) )
                    QMessageBox::warning( parent, tr( "Could not export settings" ), error ); }
                else if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) ||
                    file.write( QJsonDocument( preset_ ).toJson( QJsonDocument::Indented ) ) < 0 || !file.commit() )
                    QMessageBox::warning( parent, tr( "Could not export appearance" ), file.errorString() );
            }
        }
    }

    static bool readAppearance( const QString &path, QJsonObject &out, QString &error )
    {
        QFile file( path );
        if( !file.open( QIODevice::ReadOnly ) ) { error = file.errorString(); return false; }
        if( file.size() > 65536 ) { error = QObject::tr( "Appearance JSON exceeds 64 KiB." ); return false; }
        const QByteArray bytes = file.readAll();
        QJsonParseError parse;
        QJsonDocument document = QJsonDocument::fromJson( bytes, &parse );
        if( parse.error != QJsonParseError::NoError || !document.isObject() )
        { error = QObject::tr( "Appearance JSON is not an object: %1" ).arg( parse.errorString() ); return false; }
        if( !uniqueKeys( bytes ) )
        { error = QObject::tr( "Appearance JSON contains a duplicate key." ); return false; }
        return validateAppearanceObject( document.object(), out, error );
    }
    static bool validateAppearanceObject( const QJsonObject &root, QJsonObject &out, QString &error )
    {
        if( !keys( root, { "schema", "visual", "presentation", "mappings" } ) ||
            root.value( "schema" ).toString() != QLatin1String( "vlc-flubber-appearance/v1" ) ||
            !root.value( "visual" ).isObject() || !root.value( "presentation" ).isObject() ||
            !root.value( "mappings" ).isObject() )
        { error = QObject::tr( "Unexpected appearance schema or top-level fields." ); return false; }
        const QJsonObject visual = root.value( "visual" ).toObject();
        const QJsonObject presentation = root.value( "presentation" ).toObject();
        const QJsonObject mappings = root.value( "mappings" ).toObject();
        if( !keys( visual, { "transparency", "flubber", "colors" } ) ||
            !number( visual.value( "transparency" ), 0, 1 ) ||
            !visual.value( "flubber" ).isObject() || !visual.value( "colors" ).isObject() ||
            !keys( presentation, { "colorAnchors", "halo" } ) ||
            !presentation.value( "halo" ).isObject() )
        { error = QObject::tr( "Invalid visual or presentation fields." ); return false; }
        const QJsonObject flubber = visual.value( "flubber" ).toObject();
        const QJsonObject colors = visual.value( "colors" ).toObject();
        const QJsonObject halo = presentation.value( "halo" ).toObject();
        if( !keys( flubber, { "showOutline", "outlineThickness", "showHalo" } ) ||
            !flubber.value( "showOutline" ).isBool() || !flubber.value( "showHalo" ).isBool() ||
            !number( flubber.value( "outlineThickness" ), 0, 20 ) ||
            !keys( colors, { "up", "down", "left", "right", "idle", "outline", "halo", "cursor" } ) ||
            !keys( halo, { "widthPercent", "gradient", "steepness" } ) ||
            !number( halo.value( "widthPercent" ), 0, 10000 ) ||
            !halo.value( "gradient" ).isBool() || !number( halo.value( "steepness" ), .1, 10 ) ||
            ( presentation.value( "colorAnchors" ).toString() != QLatin1String( "axes" ) &&
              presentation.value( "colorAnchors" ).toString() != QLatin1String( "corners" ) ) )
        { error = QObject::tr( "Invalid Flubber visual settings." ); return false; }
        for( QJsonObject::const_iterator it = colors.begin(); it != colors.end(); ++it )
        {
            const QString c = it.value().toString();
            if( c.size() != 7 || !c.startsWith( '#' ) )
            { error = QObject::tr( "Invalid color: %1" ).arg( it.key() ); return false; }
            for( int i = 1; i < 7; ++i )
                if( !QStringLiteral( "0123456789abcdefABCDEF" ).contains( c.at( i ) ) )
                { error = QObject::tr( "Invalid color: %1" ).arg( it.key() ); return false; }
        }
        const char *names[] = { "oscillationFrequency", "edgeSmoothness", "projectionAmplitude",
                                "pulseSynchrony", "waveSizeVariation", "saturation" };
        if( mappings.size() != 6 ) { error = QObject::tr( "Expected six Flubber mappings." ); return false; }
        for( int i = 0; i < 6; ++i )
        {
            const QString name = QLatin1String( names[i] );
            if( !mappings.value( name ).isObject() ) { error = QObject::tr( "Missing mapping: %1" ).arg( name ); return false; }
            const QJsonObject mapping = mappings.value( name ).toObject();
            const double maximum = i == 0 ? 10 : 1;
            const QString driver = mapping.value( "drivenBy" ).toString();
            if( !keys( mapping, { "min", "max", "drivenBy", "reverse" } ) ||
                !number( mapping.value( "min" ), 0, maximum ) || !number( mapping.value( "max" ), 0, maximum ) ||
                mapping.value( "min" ).toDouble() > mapping.value( "max" ).toDouble() ||
                ( driver != QLatin1String( "x-axis" ) && driver != QLatin1String( "y-axis" ) &&
                  driver != QLatin1String( "angle" ) && driver != QLatin1String( "radius" ) ) ||
                !mapping.value( "reverse" ).isBool() )
            { error = QObject::tr( "Invalid mapping: %1" ).arg( name ); return false; }
        }
        out = root;
        return true;
    }

protected:
    void customEvent( QEvent *event ) Q_DECL_OVERRIDE
    {
        if( event->type() == FlubberNativeKeyEvent::eventType() )
        {
            if( !nativeKeys_ ) { event->accept(); return; }
            if( QApplication::activeModalWidget() || QApplication::activePopupWidget() || textEditorOwnsShortcut() )
            { clearHeldArrows(); nativeKeys_->setArrowMode( arrowMode_ ); event->accept(); return; }
            const FlubberNativeKeyEvent *native = static_cast<FlubberNativeKeyEvent *>( event );
            if( native->guiRevision != nativeKeys_->guiRevision() ) { event->accept(); return; }
            recordKey( native->key, native->modifiers, native->down, native->repeat );
            if( native->arrowModeAfter >= 0 ) setArrowMode( native->arrowModeAfter != 0, false );
            else
            {
                QKeyEvent key( native->down ? QEvent::KeyPress : QEvent::KeyRelease,
                               native->key, native->modifiers, QString(), native->repeat );
                if( !flubberArrow( native->key ) || !native->down || ratingWindowIsForeground() )
                    handleInputKey( &key );
            }
            event->accept(); return;
        }
        QWidget::customEvent( event );
    }
    bool eventFilter( QObject *watched, QEvent *event ) Q_DECL_OVERRIDE
    {
        if( interaction_ ) recordPlayerInput( watched, event );
        if( event->type() == QEvent::KeyRelease && heldArrows_ )
        {
            const auto *key = static_cast<QKeyEvent *>( event );
            if( !key->isAutoRepeat() ) releaseArrow( flubberArrow( key->key() ) );
        }
        if( ( event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride ) &&
            !QApplication::activeModalWidget() && !QApplication::activePopupWidget() &&
            ratingWindowIsForeground() )
        {
            QWidget *target = qobject_cast<QWidget *>( watched );
            if( target && target->window() == window() && !textEditorOwnsShortcut( target ) )
            {
                QKeyEvent *key = static_cast<QKeyEvent *>( event );
                if( handleInputKey( key ) ) return true;
            }
        }
        if( event->type() == QEvent::ApplicationDeactivate ||
            ( watched == window() && event->type() == QEvent::WindowDeactivate ) )
        {
            clearHeldArrows();
            if( nativeKeys_ ) nativeKeys_->setArrowMode( arrowMode_ );
            if( ratingActive_ ) interruptRating();
        }
        if( ratingActive_ && watched == window() &&
            ( event->type() == QEvent::Resize || event->type() == QEvent::Move ) )
            releaseRating();
        return QWidget::eventFilter( watched, event );
    }
    void keyPressEvent( QKeyEvent *event ) Q_DECL_OVERRIDE
    {
        if( event->key() == Qt::Key_Escape && ratingActive_ )
        {
            releaseRating(); event->accept(); return;
        }
        QWidget::keyPressEvent( event );
    }
    void mouseMoveEvent( QMouseEvent *event ) Q_DECL_OVERRIDE
    {
        if( arrowMode_ ) { QWidget::mouseMoveEvent( event ); return; }
        if( ratingActive_ )
        {
            const QPoint center = mapToGlobal( rect().center() );
            const QPoint delta = event->globalPos() - center;
            if( !delta.isNull() )
            {
                const double travel = qMax( 1.0, window()->height() * .6 );
                setAffect( affectX_ + 2 * qBound( -150, delta.x(), 150 ) / travel,
                           affectY_ - 2 * qBound( -150, delta.y(), 150 ) / travel );
                QCursor::setPos( center );
            }
            event->accept();
            return;
        }
        setAffect( event->localPos().x() * 2 / qMax( 1, width() ) - 1,
                   1 - event->localPos().y() * 2 / qMax( 1, height() ) );
        QWidget::mouseMoveEvent( event );
    }
    void mousePressEvent( QMouseEvent *event ) Q_DECL_OVERRIDE
    {
        if( event->button() == Qt::LeftButton && !arrowMode_ && !ratingActive_ )
        {
            capturePending_ = true;
            event->accept();
            return;
        }
        QWidget::mousePressEvent( event );
    }
    void mouseReleaseEvent( QMouseEvent *event ) Q_DECL_OVERRIDE
    {
        if( event->button() == Qt::LeftButton && capturePending_ )
        {
            capturePending_ = false;
            /* Qt releases its own implicit button capture after this event.
             * Install our explicit rating capture on the next GUI turn. */
            QTimer::singleShot( 0, this, [this]() {
                if( !ratingWindowIsForeground() || ( rateAllowed_ && !rateAllowed_() ) ) return;
                if( !captureRating() )
                    QMessageBox::warning( this, tr( "Flubber rating unavailable" ),
                        tr( "Could not confine and hide the rating pointer." ) );
            } );
            event->accept();
            return;
        }
        QWidget::mouseReleaseEvent( event );
    }
    void paintEvent( QPaintEvent * ) Q_DECL_OVERRIDE
    {
        const FeedbackPreview *preview = renderPreview();
        const double rate = preview && preview->kind == FaceKind ? preview->faceRate : faceRate_;
        const double faceStep = qMin<qint64>( faceClock_.nsecsElapsed(), 100000000 ) * 1e-9 * rate / 10;
        faceClock_.restart();
        faceX_ += qBound( -faceStep, affectX_ - faceX_, faceStep );
        faceY_ += qBound( -faceStep, affectY_ - faceY_, faceStep );
        QPainter painter( this );
        painter.fillRect( rect(), Qt::black );
        painter.setRenderHint( QPainter::Antialiasing );
        drawElement( painter, preview ? preview->kind : primary_ );
    }

private:
    struct FeedbackPreview
    {
        FeedbackKind kind = FlubberKind;
        ElementLayout layout{};
        QJsonObject appearance, grid;
        QSize tiles;
        QImage atlas;
        double faceRate = 0;
    };
    const FeedbackPreview *renderPreview() const { return appearanceLocked_ ? NULL : preview_; }
    const QJsonObject &renderAppearance() const
    {
        const auto *preview = renderPreview();
        return preview && preview->kind != FaceKind ? preview->appearance : preset_;
    }
    static void placeFeedbackPopup( QDialog &dialog, QWidget *parent, const QSize &preferred )
    {
        dialog.ensurePolished();
        dialog.layout()->setSizeConstraint( QLayout::SetMinimumSize );
        dialog.resize( dialog.sizeHint().expandedTo( preferred ) );
        if( !parent ) return;
        const QRect screen = parent->screen()->availableGeometry();
        const QPoint upperRight = parent->mapToGlobal( parent->rect().topRight() );
        const int right = qMin( screen.right(), upperRight.x() ) - 16;
        dialog.move( qMax( screen.left(), right - dialog.width() ),
                     qBound( screen.top(), upperRight.y() + 32, qMax( screen.top(), screen.bottom() - dialog.height() ) ) );
    }
    void recordKey( int key, Qt::KeyboardModifiers modifiers, bool down, bool repeat )
    {
        recordInteraction( QStringLiteral( "key" ), {
            { "key", key }, { "modifiers", int( modifiers ) }, { "down", down }, { "repeat", repeat }
        } );
    }
    void recordPlayerInput( QObject *watched, QEvent *event )
    {
        const QEvent::Type type = event->type();
        if( type != QEvent::KeyPress && type != QEvent::KeyRelease && type != QEvent::MouseButtonPress &&
            type != QEvent::MouseButtonRelease && type != QEvent::MouseButtonDblClick && type != QEvent::Wheel ) return;
        QWidget *target = qobject_cast<QWidget *>( watched );
        if( !target || QApplication::activeModalWidget() ) return;
        QWidget *root = target;
        while( root && root != window() ) root = root->parentWidget();
        if( !root ) return;
        if( type == QEvent::KeyPress || type == QEvent::KeyRelease )
        {
            if( target != QApplication::focusWidget() || textEditorOwnsShortcut( target ) ) return;
            const auto *key = static_cast<QKeyEvent *>( event );
            recordKey( key->key(), key->modifiers(), type == QEvent::KeyPress, key->isAutoRepeat() );
        }
        else if( type == QEvent::Wheel )
        {
            const auto *wheel = static_cast<QWheelEvent *>( event );
            if( target != QApplication::widgetAt( wheel->globalPosition().toPoint() ) ) return;
            recordInteraction( QStringLiteral( "wheel" ),
                               { { "x", wheel->angleDelta().x() }, { "y", wheel->angleDelta().y() } } );
        }
        else
        {
            const auto *mouse = static_cast<QMouseEvent *>( event );
            QWidget *grabber = QWidget::mouseGrabber();
            if( target != ( grabber ? grabber : QApplication::widgetAt( mouse->globalPos() ) ) ) return;
            recordInteraction( QStringLiteral( "mouse-button" ),
                { { "button", int( mouse->button() ) }, { "down", type != QEvent::MouseButtonRelease },
                  { "double", type == QEvent::MouseButtonDblClick } } );
        }
    }
    void clearHeldArrows()
    {
        heldArrows_ = 0;
        arrowRepeat_.stop();
    }
    void releaseArrow( unsigned arrow )
    {
        if( !( heldArrows_ & arrow ) ) return;
        heldArrows_ &= ~arrow;
        if( !heldArrows_ ) arrowRepeat_.stop();
        else holdLastNs_ = holdClock_.nsecsElapsed();
    }
    double tapDistance() const { return stepPercent_ / 100.0; }
    void moveHeldArrows( double distance )
    {
        const int x = bool( heldArrows_ & FlubberRight ) - bool( heldArrows_ & FlubberLeft );
        const int y = bool( heldArrows_ & FlubberUp ) - bool( heldArrows_ & FlubberDown );
        const double step = x && y ? distance / qSqrt( 2.0 ) : distance;
        setAffect( affectX_ + x * step, affectY_ + y * step );
    }
    bool textEditorOwnsShortcut( const QWidget *target = NULL ) const
    {
        const QWidget *candidates[] = { target, QApplication::focusWidget() };
        for( const QWidget *candidate : candidates )
        {
            if( !candidate || candidate->window() != window() ) continue;
            for( const QWidget *widget = candidate; widget; widget = widget->parentWidget() )
            {
                if( widget->inherits( "QLineEdit" ) || widget->inherits( "QTextEdit" ) ||
                    widget->inherits( "QPlainTextEdit" ) || widget->inherits( "QAbstractSpinBox" ) ) return true;
                const QComboBox *combo = qobject_cast<const QComboBox *>( widget );
                if( combo && combo->isEditable() ) return true;
                if( widget == window() ) break;
            }
        }
        return false;
    }
    void persistTravelSettings()
    {
        settings_->setValue( "Flubber/arrowHoldMs", arrowHoldMs_ );
        settings_->setValue( "Flubber/gridStyle", tiledGrid_ ? "tiles" : "continuous" );
    }
    void normalizeVisibility()
    {
        for( int i = 0; i < 3; ++i )
        {
            layers_[i].visible = i == primary_;
            persistLayout( static_cast<FeedbackKind>( i ) );
        }
    }
    void drawElement( QPainter &painter, FeedbackKind kind )
    {
        painter.save();
        if( kind == GridKind ) drawGrid( painter );
        else if( kind == FaceKind ) drawFace( painter );
        else drawFlubber( painter );
        painter.restore();
    }
    const ElementLayout &renderLayout( FeedbackKind kind ) const
    {
        const auto *preview = renderPreview();
        return preview && preview->kind == kind ? preview->layout : layers_[kind];
    }
    QPointF elementCenter( FeedbackKind kind ) const
    {
        const ElementLayout &layout = renderLayout( kind );
        return QPointF( width() * layout.x / 100.0, height() * layout.y / 100.0 );
    }
    double elementSide( FeedbackKind kind ) const
    { return qMin( double( height() ), double( width() ) / 2.0 ) * renderLayout( kind ).size / 100.0; }
    void drawFlubber( QPainter &painter )
    {
        const double radius = elementSide( FlubberKind ) * .47;
        const QPointF center = elementCenter( FlubberKind );
        QPainterPath path;
        const double smooth = mapped( "edgeSmoothness" );
        const double amplitude = mapped( "projectionAmplitude" );
        const double synchrony = mapped( "pulseSynchrony" );
        const double variation = mapped( "waveSizeVariation" );
        for( int i = 0; i < 192; ++i )
        {
            const double theta = i * 2 * M_PI / 192;
            const int waveIndex = ( i + 6 ) / 12 % 16;
            const double shape = pointy_[i] * ( 1 - smooth ) + rounded_[i] * smooth;
            const double wave = .5 + .5 * qSin( phase_ + ( 1 - synchrony ) * phases_[waveIndex] );
            const double asymmetry = 1 + variation * amplitudes_[waveIndex];
            const double deformation = ( 1 + shape * amplitude * wave * asymmetry ) * ( .95 + .05 * qSin( phase_ ) );
            const QPointF point( center.x() + radius * deformation * qCos( theta ),
                                 center.y() + radius * deformation * qSin( theta ) );
            if( i == 0 ) path.moveTo( point ); else path.lineTo( point );
        }
        path.closeSubpath();
        const QJsonObject visual = renderAppearance().value( "visual" ).toObject();
        const QJsonObject colors = visual.value( "colors" ).toObject();
        const QJsonObject flubber = visual.value( "flubber" ).toObject();
        const QJsonObject halo = renderAppearance().value( "presentation" ).toObject().value( "halo" ).toObject();
        const QColor fill = currentColor();
        const double opacity = 1 - visual.value( "transparency" ).toDouble();
        painter.setOpacity( opacity );
        if( flubber.value( "showHalo" ).toBool() && halo.value( "widthPercent" ).toDouble() > 0 )
        {
            QColor glow( colors.value( "halo" ).toString() );
            const double width = qMin( double( qMax( this->width(), this->height() ) * 2 ),
                qMax( 1.0, flubber.value( "outlineThickness" ).toDouble() * 3 )
                * halo.value( "widthPercent" ).toDouble() / 100 );
            painter.setBrush( Qt::NoBrush );
            if( halo.value( "gradient" ).toBool() )
            {
                const double steepness = halo.value( "steepness" ).toDouble();
                for( int layer = 6; layer >= 1; --layer )
                {
                    const double distance = double( layer ) / 6;
                    glow.setAlphaF( .13 * qPow( 1 - distance / 1.2, steepness ) );
                    painter.setPen( QPen( glow, width * distance,
                                           Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
                    painter.drawPath( path );
                }
            }
            else
            {
                glow.setAlphaF( .65 );
                painter.setPen( QPen( glow, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
                painter.drawPath( path );
            }
        }
        painter.setBrush( fill );
        if( flubber.value( "showOutline" ).toBool() )
            painter.setPen( QPen( QColor( colors.value( "outline" ).toString() ),
                                  flubber.value( "outlineThickness" ).toDouble() ) );
        else painter.setPen( Qt::NoPen );
        painter.drawPath( path );
    }
    const QImage &gridBackground( int size, const QJsonObject &colors, bool corners, double opacity, QSize tiles )
    {
        if( gridBackground_.width() == size && gridBackgroundColors_ == colors &&
            gridBackgroundTiles_ == tiles &&
            gridBackgroundCorners_ == corners && gridBackgroundOpacity_ == opacity )
            return gridBackground_;
        const QColor up( colors.value( "up" ).toString() ), down( colors.value( "down" ).toString() );
        const QColor left( colors.value( "left" ).toString() ), right( colors.value( "right" ).toString() );
        const QColor anchors[4] = { up, down, left, right };
        QImage fieldImage( size, size, QImage::Format_ARGB32_Premultiplied );
        for( int py = 0; py < fieldImage.height(); ++py )
        {
            QRgb *line = reinterpret_cast<QRgb *>( fieldImage.scanLine( py ) );
            const double v = tiles.isValid() ? 1 - 2.0 * ( ( 2 * py + 1 ) * tiles.height() / ( 2 * size ) ) / ( tiles.height() - 1 )
                                        : 1 - 2.0 * py / qMax( 1, size - 1 );
            for( int px = 0; px < fieldImage.width(); ++px )
            {
                const double u = tiles.isValid() ? 2.0 * ( ( 2 * px + 1 ) * tiles.width() / ( 2 * size ) ) / ( tiles.width() - 1 ) - 1
                                            : 2.0 * px / qMax( 1, size - 1 ) - 1;
                double w[4];
                if( corners )
                {
                    const double x = ( u + 1 ) / 2, y = ( v + 1 ) / 2;
                    w[0] = ( 1 - x ) * y; w[1] = x * ( 1 - y );
                    w[2] = ( 1 - x ) * ( 1 - y ); w[3] = x * y;
                }
                else
                {
                    w[0] = qMax( 0.0, v ); w[1] = qMax( 0.0, -v );
                    w[2] = qMax( 0.0, -u ); w[3] = qMax( 0.0, u );
                    const double total = w[0] + w[1] + w[2] + w[3];
                    for( double &weight : w ) weight = total > 0 ? weight / total : 0;
                }
                double rgb[3] = { 183, 183, 183 };
                for( int a = 0; a < 4; ++a )
                {
                    rgb[0] += ( anchors[a].red() - 183 ) * w[a];
                    rgb[1] += ( anchors[a].green() - 183 ) * w[a];
                    rgb[2] += ( anchors[a].blue() - 183 ) * w[a];
                }
                line[px] = qRgba( qBound( 0, qRound( rgb[0] * opacity ), 255 ),
                                  qBound( 0, qRound( rgb[1] * opacity ), 255 ),
                                  qBound( 0, qRound( rgb[2] * opacity ), 255 ), qRound( opacity * 255 ) );
            }
        }
        gridBackground_ = fieldImage;
        gridBackgroundColors_ = colors;
        gridBackgroundTiles_ = tiles;
        gridBackgroundCorners_ = corners;
        gridBackgroundOpacity_ = opacity;
        return gridBackground_;
    }
    void drawGrid( QPainter &painter )
    {
        const auto *preview = renderPreview();
        const QJsonObject &grid = preview ? preview->grid : grid_;
        const QSize tiles = preview ? preview->tiles : ( tiledGrid_ ? QSize( gridColumns_, gridRows_ ) : QSize() );
        const double side = qMax( 20.0, elementSide( GridKind ) );
        const QPointF center = elementCenter( GridKind );
        const QRectF field( center.x() - side / 2, center.y() - side / 2, side, side );
        const QJsonObject visual = renderAppearance().value( "visual" ).toObject();
        const QJsonObject colors = visual.value( "colors" ).toObject();
        const bool corners = renderAppearance().value( "presentation" ).toObject().value( "colorAnchors" ).toString() == QLatin1String( "corners" );
        painter.drawImage( field, gridBackground( int( side ), colors, corners, 1 - visual.value( "transparency" ).toDouble(), tiles ) );
        const double lineWidth = grid.value( "lineThickness" ).toDouble();
        if( tiles.isValid() && lineWidth > 0 )
        {
            painter.setPen( QPen( QColor( 255, 255, 255, 80 ), lineWidth ) );
            if( side / tiles.width() >= 2 )
                for( int col = 1; col < tiles.width(); ++col )
                {
                    const double x = field.left() + side * col / tiles.width();
                    painter.drawLine( QPointF( x, field.top() ), QPointF( x, field.bottom() ) );
                }
            if( side / tiles.height() >= 2 )
                for( int row = 1; row < tiles.height(); ++row )
                {
                    const double y = field.top() + side * row / tiles.height();
                    painter.drawLine( QPointF( field.left(), y ), QPointF( field.right(), y ) );
                }
        }
        if( grid.value( "showOutline" ).toBool() )
        {
            painter.setBrush( Qt::NoBrush );
            painter.setPen( QPen( QColor( colors.value( "outline" ).toString() ), grid.value( "outlineThickness" ).toDouble() ) );
            painter.drawRect( field );
        }
        double x = ( affectX_ + 1 ) / 2, y = ( 1 - affectY_ ) / 2;
        if( tiles.isValid() )
        {
            x = ( qMin( int( x * tiles.width() ), tiles.width() - 1 ) + .5 ) / tiles.width();
            y = ( qMin( int( y * tiles.height() ), tiles.height() - 1 ) + .5 ) / tiles.height();
        }
        const QPointF marker( field.left() + x * side, field.top() + y * side );
        painter.setPen( Qt::NoPen );
        painter.setBrush( QColor( colors.value( "cursor" ).toString() ) );
        painter.drawEllipse( marker, grid.value( "cursorSize" ).toDouble() / 2,
                             grid.value( "cursorSize" ).toDouble() / 2 );
    }
    void drawFace( QPainter &painter )
    {
        const auto *preview = renderPreview();
        const QImage &atlas = preview ? preview->atlas : faceAtlas_;
        if( atlas.isNull() ) return;
        const double side = qMax( 20.0, elementSide( FaceKind ) );
        const QPointF center = elementCenter( FaceKind );
        const QRectF target( center.x() - side / 2, center.y() - side / 2, side, side );
        const double gx = ( faceX_ + 1 ) * 10, gy = ( 1 - faceY_ ) * 10;
        const int x0 = qBound( 0, int( qFloor( gx ) ), 20 ), y0 = qBound( 0, int( qFloor( gy ) ), 20 );
        const int x1 = qMin( 20, x0 + 1 ), y1 = qMin( 20, y0 + 1 );
        const double dx = gx - x0, dy = gy - y0;
        const int col[4] = { x0, x1, x0, x1 }, row[4] = { y0, y0, y1, y1 };
        const double weight[4] = { ( 1 - dx ) * ( 1 - dy ), dx * ( 1 - dy ), ( 1 - dx ) * dy, dx * dy };
        QImage mixed( 160, 160, QImage::Format_ARGB32_Premultiplied );
        mixed.fill( Qt::transparent );
        QPainter mix( &mixed );
        mix.setCompositionMode( QPainter::CompositionMode_Plus );
        for( int i = 0; i < 4; ++i )
        {
            if( weight[i] <= 0 ) continue;
            mix.setOpacity( weight[i] );
            mix.drawImage( QRectF( 0, 0, 160, 160 ), atlas, QRectF( col[i] * 160, row[i] * 160, 160, 160 ) );
        }
        mix.end();
        painter.drawImage( target, mixed );
    }
    void persistLayout( FeedbackKind kind )
    {
        const QString key = QStringLiteral( "Flubber/layers/" ) + kindName( kind ) + '/';
        const ElementLayout layout = layers_[kind];
        settings_->setValue( key + "visible", layout.visible );
        settings_->setValue( key + "size", layout.size );
        settings_->setValue( key + "x", layout.x );
        settings_->setValue( key + "y", layout.y );
    }
    static QJsonObject defaultGrid()
    { return QJsonObject{ { "lineThickness", 1.0 }, { "showOutline", true },
        { "outlineThickness", 2.0 }, { "cursorSize", 14.0 } }; }
    void updateInputTooltip()
    {
        setToolTip( arrowMode_
            ? QObject::tr( "Affect arrows are ON: Left/Right changes valence; Up/Down changes arousal. Alt+F restores VLC arrow controls." )
            : QObject::tr( "VLC arrow controls are ON. Alt+F switches arrows to valence/arousal. Click for mouse rating; Escape releases it." ) );
    }
    bool ratingWindowIsForeground() const
    {
#ifdef Q_OS_WIN
        const HWND owner = reinterpret_cast<HWND>( window()->winId() );
        const HWND foreground = GetForegroundWindow();
        return foreground && GetAncestor( foreground, GA_ROOT ) == owner;
#else
        return window()->isActiveWindow();
#endif
    }
    bool captureRating()
    {
        if( !ratingWindowIsForeground() ) return false;
#ifdef Q_OS_WIN
        const HWND owner = reinterpret_cast<HWND>( window()->winId() );
        RECT client = {};
        POINT topLeft = { 0, 0 };
        if( !GetClientRect( owner, &client ) || !ClientToScreen( owner, &topLeft ) ) return false;
        POINT bottomRight = { client.right, client.bottom };
        if( !ClientToScreen( owner, &bottomRight ) ) return false;
        const RECT bounds = { topLeft.x, topLeft.y, bottomRight.x, bottomRight.y };
        if( !ClipCursor( &bounds ) ) return false;
        if( SetCapture( reinterpret_cast<HWND>( winId() ) ) == NULL && GetCapture() == NULL )
        { ClipCursor( NULL ); return false; }
        for( ; cursorHideCalls_ < 32; ++cursorHideCalls_ )
        {
            const int count = ShowCursor( FALSE );
            if( count < 0 ) { ++cursorHideCalls_; break; }
        }
        if( cursorHideCalls_ == 32 ) { releaseRating(); return false; }
#else
        QApplication::setOverrideCursor( Qt::BlankCursor );
#endif
        ratingActive_ = true;
        setFocus( Qt::MouseFocusReason );
        QCursor::setPos( mapToGlobal( rect().center() ) );
        return true;
    }
    void releaseRating()
    {
        capturePending_ = false;
        if( !ratingActive_ && cursorHideCalls_ == 0 ) return;
#ifdef Q_OS_WIN
        if( GetCapture() == reinterpret_cast<HWND>( winId() ) ) ReleaseCapture();
        ClipCursor( NULL );
        while( cursorHideCalls_-- > 0 ) ShowCursor( TRUE );
        cursorHideCalls_ = 0;
#else
        QApplication::restoreOverrideCursor();
#endif
        ratingActive_ = false;
    }
    void interruptRating()
    {
        if( !ratingActive_ ) return;
        releaseRating();
        if( interrupted_ ) interrupted_();
    }
    void initializeShape()
    {
        double minimum = 1e100, maximum = -1e100;
        const double alpha = 3 * M_PI / 4;
        for( int i = 0; i < 192; ++i )
        {
            const double theta = i * 2 * M_PI / 192;
            rounded_[i] = ( qCos( 16 * theta ) + 1 ) / 2;
            const int distance = qAbs( i % 12 - 6 );
            pointy_[i] = qSin( alpha ) / qSin( M_PI - alpha - ( 2 * M_PI / 192 ) * distance );
            minimum = qMin( minimum, pointy_[i] );
            maximum = qMax( maximum, pointy_[i] );
        }
        for( int i = 0; i < 192; ++i ) pointy_[i] = ( pointy_[i] - minimum ) / ( maximum - minimum );

        // Exact 32-bit mulberry32 sequence used by Planner math.js for this seed.
        uint32_t seed = 2166136261u;
        const char *label = "affect-research-v1-preview";
        for( const unsigned char *p = reinterpret_cast<const unsigned char *>( label ); *p; ++p )
            seed = ( seed ^ *p ) * 16777619u;
        uint32_t value = seed;
        auto random = [&value]() {
            value += 0x6d2b79f5u;
            uint32_t result = value;
            result = ( result ^ ( result >> 15 ) ) * ( result | 1u );
            result ^= result + ( ( result ^ ( result >> 7 ) ) * ( result | 61u ) );
            return double( result ^ ( result >> 14 ) ) / 4294967296.0;
        };
        for( int i = 0; i < 16; ++i )
        {
            phases_[i] = ( random() * 2 - 1 ) * M_PI;
            amplitudes_[i] = random() * 2 - 1;
        }
    }
    static bool uniqueKeys( const QByteArray &bytes )
    {
        struct Frame { bool object; bool keyExpected; QSet<QString> keys; };
        QVector<Frame> frames;
        for( int i = 0; i < bytes.size(); ++i )
        {
            const char c = bytes.at( i );
            if( c == '{' ) frames.push_back( Frame{ true, true, {} } );
            else if( c == '[' ) frames.push_back( Frame{ false, false, {} } );
            else if( c == '}' || c == ']' ) { if( !frames.isEmpty() ) frames.removeLast(); }
            else if( c == ',' && !frames.isEmpty() && frames.last().object ) frames.last().keyExpected = true;
            else if( c == '"' )
            {
                const int start = i;
                ++i;
                for( ; i < bytes.size(); ++i )
                {
                    if( bytes.at( i ) == '\\' ) { ++i; continue; }
                    if( bytes.at( i ) == '"' ) break;
                }
                if( !frames.isEmpty() && frames.last().object && frames.last().keyExpected )
                {
                    const QByteArray quoted = bytes.mid( start, i - start + 1 );
                    const QString key = QJsonDocument::fromJson( QByteArray( "[" ) + quoted + "]" ).array().at( 0 ).toString();
                    if( frames.last().keys.contains( key ) ) return false;
                    frames.last().keys.insert( key );
                    frames.last().keyExpected = false;
                }
            }
        }
        return true;
    }
    static QSpinBox *spin( QWidget *parent, int min, int max, int value, const QString &suffix )
    {
        QSpinBox *box = new QSpinBox( parent );
        box->setRange( min, max ); box->setValue( value ); box->setSuffix( suffix );
        return box;
    }
    static bool keys( const QJsonObject &object, std::initializer_list<const char *> expected )
    {
        if( object.size() != int( expected.size() ) ) return false;
        for( const char *key : expected ) if( !object.contains( QLatin1String( key ) ) ) return false;
        return true;
    }
    static bool number( const QJsonValue &value, double lo, double hi )
    { return value.isDouble() && value.toDouble() >= lo && value.toDouble() <= hi; }
    double mapped( const char *name ) const
    {
        const QJsonObject m = renderAppearance().value( "mappings" ).toObject().value( QLatin1String( name ) ).toObject();
        const QString driver = m.value( "drivenBy" ).toString();
        double t = 0;
        if( driver == QLatin1String( "x-axis" ) ) t = ( affectX_ + 1 ) / 2;
        else if( driver == QLatin1String( "y-axis" ) ) t = ( affectY_ + 1 ) / 2;
        else if( driver == QLatin1String( "radius" ) ) t = qMin( 1.0, qSqrt( affectX_*affectX_ + affectY_*affectY_ ) );
        else { t = qAtan2( affectY_, affectX_ ) / ( 2 * M_PI ); if( t < 0 ) t += 1; }
        if( m.value( "reverse" ).toBool() ) t = 1 - t;
        return m.value( "min" ).toDouble() + ( m.value( "max" ).toDouble() - m.value( "min" ).toDouble() ) * t;
    }
    QColor currentColor() const
    {
        const QJsonObject colors = renderAppearance().value( "visual" ).toObject().value( "colors" ).toObject();
        if( qSqrt( affectX_*affectX_ + affectY_*affectY_ ) < .005 )
            return QColor( colors.value( "idle" ).toString() );
        const bool corners = renderAppearance().value( "presentation" ).toObject().value( "colorAnchors" ).toString() == QLatin1String( "corners" );
        double weights[4];
        if( corners )
        {
            const double u = ( affectX_ + 1 ) / 2, v = ( affectY_ + 1 ) / 2;
            weights[0] = ( 1-u ) * v; weights[1] = u * ( 1-v );
            weights[2] = ( 1-u ) * ( 1-v ); weights[3] = u * v;
        }
        else
        {
            weights[0] = qMax( 0.0, affectY_ ); weights[1] = qMax( 0.0, -affectY_ );
            weights[2] = qMax( 0.0, -affectX_ ); weights[3] = qMax( 0.0, affectX_ );
            const double total = weights[0] + weights[1] + weights[2] + weights[3];
            if( total > 0 ) for( int i = 0; i < 4; ++i ) weights[i] /= total;
        }
        const char *names[] = { "up", "down", "left", "right" };
        const double saturation = mapped( "saturation" );
        double red = 183, green = 183, blue = 183;
        for( int i = 0; i < 4; ++i )
        {
            const QColor c( colors.value( QLatin1String( names[i] ) ).toString() );
            red += ( c.red() - 183 ) * weights[i] * saturation;
            green += ( c.green() - 183 ) * weights[i] * saturation;
            blue += ( c.blue() - 183 ) * weights[i] * saturation;
        }
        return QColor( qRound( red ), qRound( green ), qRound( blue ) );
    }
    static QJsonObject mapping( double lo, double hi, const char *driver, bool reverse = false )
    { return QJsonObject{ { "min", lo }, { "max", hi }, { "drivenBy", driver }, { "reverse", reverse } }; }
    static QJsonObject defaultPreset()
    {
        return QJsonObject{
            { "schema", "vlc-flubber-appearance/v1" },
            { "visual", QJsonObject{
                { "transparency", .05 },
                { "flubber", QJsonObject{ { "showOutline", true }, { "outlineThickness", 2 }, { "showHalo", true } } },
                { "colors", QJsonObject{ { "up", "#f2c94c" }, { "down", "#2f80ed" },
                    { "left", "#eb5757" }, { "right", "#27ae60" }, { "idle", "#9ca3af" },
                    { "outline", "#f8fafc" }, { "halo", "#93c5fd" }, { "cursor", "#ffffff" } } }
            } },
            { "presentation", QJsonObject{ { "colorAnchors", "axes" },
                { "halo", QJsonObject{ { "widthPercent", 150 }, { "gradient", true }, { "steepness", 1 } } } } },
            { "mappings", QJsonObject{
                { "oscillationFrequency", mapping( .5, 2.5, "y-axis" ) },
                { "edgeSmoothness", mapping( 0, 1, "x-axis" ) },
                { "projectionAmplitude", mapping( .2, .4, "y-axis" ) },
                { "pulseSynchrony", mapping( .2, 1, "x-axis" ) },
                { "waveSizeVariation", mapping( 0, .8, "x-axis", true ) },
                { "saturation", mapping( 0, 1, "radius" ) }
            } }
        };
    }
    QSettings *settings_;  // owned by VLC's main interface
    QJsonObject preset_;
    QString appearanceLoadError_;
    double affectX_, affectY_, phase_;
    FeedbackKind primary_;
    ElementLayout layers_[3];
    QJsonObject grid_;
    QImage gridBackground_;
    QJsonObject gridBackgroundColors_;
    QSize gridBackgroundTiles_;
    bool gridBackgroundCorners_ = false;
    double gridBackgroundOpacity_ = 0.;
    int gridColumns_, gridRows_;
    QString faceId_;
    QImage faceAtlas_;
    double faceRate_, faceX_, faceY_;
    int panelHeight_;
    int panelPercent_, stepPercent_;
    enum { MinHoldMs = 250, MaxHoldMs = 15000 };
    int arrowHoldMs_ = 0;
    bool tiledGrid_ = true;
    QElapsedTimer holdClock_;
    qint64 holdLastNs_ = 0;
    QPointer<QWidget> referenceSurface_; // VLC owns the sibling central surface
    std::unique_ptr<FlubberVideoKeys> nativeKeys_;
    bool arrowMode_;
    bool ratingActive_;
    bool capturePending_;
    int cursorHideCalls_;
    std::function<void()> interrupted_;
    std::function<void(float, float)> affectChanged_;
    std::function<void(const QJsonObject &)> interaction_;
    bool appearanceLocked_ = false;
    const FeedbackPreview *preview_ = NULL;
    std::function<bool()> rateAllowed_;
    unsigned heldArrows_ = 0;
    QTimer arrowRepeat_;
    QElapsedTimer clock_, faceClock_;
    double pointy_[192], rounded_[192], phases_[16], amplitudes_[16];
};

#endif
