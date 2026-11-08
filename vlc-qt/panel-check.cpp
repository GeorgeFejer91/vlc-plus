#include "flubber_panel.hpp"
#include "flubber_bridge.hpp"
#include <QApplication>
#include <QImage>
#include <QTemporaryDir>
#include <QFile>
#include <QEventLoop>
#include <QTimer>
#include <QSpinBox>
#include <QSlider>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPlainTextEdit>
#ifndef Q_OS_WIN
#include <QShortcut>
#include <QtTest/QTest>
#endif
#include <cstdio>


static QJsonValue settingAt( QJsonObject root, const QStringList &path )
{
    QJsonValue value( root );
    for( const QString &key : path ) value = value.toObject().value( key );
    return value;
}

static void replaceSetting( QJsonObject &root, QStringList path, const QJsonValue &value )
{
    const QString key = path.takeFirst();
    if( path.isEmpty() ) root.insert( key, value );
    else { QJsonObject child = root.value( key ).toObject(); replaceSetting( child, path, value ); root.insert( key, child ); }
}


static int checkRecordingLock( const QString &directory )
{
    QSettings settings( directory + "/recording-lock.ini", QSettings::IniFormat );
    FlubberPanel panel( &settings );
    const QJsonObject unlockedAppearance = panel.settingsJson();
    QStringList lockEvents;
    panel.setInteractionHandler( [&lockEvents]( const QJsonObject &action ) {
        lockEvents << action.value( "event" ).toString();
    } );
    panel.setAppearanceLocked( true );
    QKeyEvent cycleDuringRecording( QEvent::KeyPress, Qt::Key_G, Qt::AltModifier );
    panel.handleSettingsKey( &cycleDuringRecording, Qt::AltModifier );
    panel.setPrimary( FlubberPanel::FaceKind );
    auto moved = panel.layoutFor( panel.primary() );
    moved.x += 10;
    panel.setLayout( panel.primary(), moved );
    if( panel.settingsJson() != unlockedAppearance ||
        lockEvents.count( "appearance-blocked" ) != 3 ) return 193;
    panel.setAffect( -.25, .5 );
    if( panel.valence() != -.25 || panel.arousal() != .5 ) return 194;
    panel.setAppearanceLocked( false );
    panel.setPrimary( FlubberPanel::GridKind );
    if( panel.primary() != FlubberPanel::GridKind ) return 195;
    return 0;
}

class BubblingEditor : public QLineEdit
{
public:
    explicit BubblingEditor( QWidget *parent ) : QLineEdit( parent ) {}
protected:
    void keyPressEvent( QKeyEvent *event ) Q_DECL_OVERRIDE { event->ignore(); }
};
class ShortcutParent : public QWidget
{
public:
    FlubberPanel *panel = NULL;
    int bubbled = 0;
protected:
    void keyPressEvent( QKeyEvent *event ) Q_DECL_OVERRIDE
    {
        ++bubbled;
        if( !panel || !panel->handleInputKey( event ) ) QWidget::keyPressEvent( event );
    }
};
static int checkEditorShortcutOwnership( const QString &directory )
{
    QSettings settings( directory + "/editor-shortcuts.ini", QSettings::IniFormat );
    ShortcutParent host;
    QVBoxLayout layout( &host );
    FlubberPanel panel( &settings, &host ); host.panel = &panel;
    BubblingEditor line( &host ); QTextEdit rich( &host ); QPlainTextEdit plain( &host );
    QComboBox combo( &host ); combo.setEditable( true ); QSpinBox spin( &host );
    QWidget *editors[] = { &line, &rich, &plain, &combo, &spin };
    layout.addWidget( &panel );
    for( QWidget *editor : editors ) layout.addWidget( editor );
    host.show(); QApplication::setActiveWindow( &host );
    const Qt::KeyboardModifiers chord = Qt::AltModifier;
    for( QWidget *editor : editors )
    {
        editor->setFocus(); QApplication::processEvents();
        QWidget *focus = QApplication::focusWidget();
        if( !focus || ( focus != editor && !editor->isAncestorOf( focus ) ) ) return 134;
        const QJsonObject before = panel.settingsJson();
        QKeyEvent key( QEvent::KeyPress, Qt::Key_G, chord );
        QApplication::sendEvent( editor, &key );
        if( panel.settingsJson() != before ) return 135;
        QKeyEvent claim( QEvent::ShortcutOverride, Qt::Key_G, chord );
        if( panel.handleInputKey( &claim ) || panel.settingsJson() != before ) return 136;
    }
    if( host.bubbled == 0 ) return 137;
    combo.setEditable( false ); combo.setFocus(); QApplication::processEvents();
    const QJsonObject beforeCombo = panel.settingsJson();
    QKeyEvent setting( QEvent::KeyPress, Qt::Key_G, chord );
    if( !panel.handleInputKey( &setting ) || panel.settingsJson() == beforeCombo ) return 138;
    line.setFocus(); QApplication::processEvents();
    const QJsonObject beforeNative = panel.settingsJson();
    FlubberNativeKeyEvent queued( Qt::Key_F, Qt::AltModifier, false, 1 );
    QCoreApplication::sendEvent( &panel, &queued );
    if( panel.settingsJson() != beforeNative || panel.arrowMode() ) return 139;
    return 0;
}

static int checkRepresentationPopups( const QString &directory, const QDir &evidence )
{
    QSettings settings( directory + "/representation-popups.ini", QSettings::IniFormat );
    FlubberPanel panel( &settings );
    panel.resize( 720, panel.height() );
    panel.setPrimary( FlubberPanel::GridKind );
    panel.setAffect( .25, .4 );
    panel.show(); QApplication::setActiveWindow( &panel );
    const Qt::KeyboardModifiers modifiers = Qt::ControlModifier | Qt::ShiftModifier;
    const auto popup = [&]( int digit, bool keep ) {
        const QJsonObject before = panel.settingsJson();
        const QImage original = panel.grab().toImage();
        bool verified = false;
        bool visited = false;
        QTimer::singleShot( 0, &panel, [&]() {
            visited = true;
            auto *dialog = qobject_cast<QDialog *>( QApplication::activeModalWidget() );
            if( !dialog ) return;
            auto *tabs = dialog->findChild<QTabWidget *>();
            const QStringList titles{ "Flubber appearance", "2D grid settings", "Face morph settings" };
            const int expectedTabs[] = { 3, 2, 1 };
            if( tabs && tabs->count() == expectedTabs[digit] && dialog->windowTitle() == titles[digit] )
            {
                const QImage initial = panel.grab().toImage();
                auto *size = dialog->findChild<QSpinBox *>( QStringLiteral( "flubber-layer-%1-size" ).arg( digit ) );
                if( size && size->isVisible() )
                {
                    size->setValue( 100 );
                    const QImage resized = panel.grab().toImage();
                    verified = resized != initial && panel.settingsJson() == before &&
                        !dialog->findChild<QSpinBox *>( "flubber-hold-duration" )->isVisible() &&
                        tabs->currentWidget()->rect().contains( QRect( size->mapTo( tabs->currentWidget(), QPoint() ), size->size() ) );
                    if( digit == 0 ) dialog->findChild<QSlider *>( "flubber-transparency" )->setValue( 100 );
                    if( digit == 1 ) dialog->findChild<QComboBox *>( "flubber-grid-style" )->setCurrentIndex( 1 );
                    if( digit == 2 ) tabs->currentWidget()->findChild<QComboBox *>()->setCurrentIndex( 1 );
                    verified = verified && panel.grab().toImage() != resized && panel.settingsJson() == before;
                    const auto fits = []( QWidget *control ) {
                        return !control->isVisible() || ( control->width() >= control->minimumSizeHint().width() &&
                            control->height() >= control->minimumSizeHint().height() );
                    };
                    for( int i = 0; i < tabs->count(); ++i )
                    {
                        tabs->setCurrentIndex( i );
                        QApplication::processEvents();
                        for( auto *control : tabs->currentWidget()->findChildren<QWidget *>() )
                            if( qobject_cast<QLabel *>( control ) || qobject_cast<QAbstractSpinBox *>( control ) ||
                                qobject_cast<QComboBox *>( control ) || qobject_cast<QPushButton *>( control ) )
                                if( !fits( control ) )
                                {
                                    std::fprintf( stderr, "Popup %d tab %d: %s %dx%d, minimum %dx%d\n",
                                        digit, i, control->metaObject()->className(), control->width(), control->height(),
                                        control->minimumSizeHint().width(), control->minimumSizeHint().height() );
                                    verified = false;
                                }
                    }
                    tabs->setCurrentIndex( 0 );
                    dialog->grab().save( evidence.filePath( QStringLiteral( "representation-popup-%1.png" ).arg( digit ) ) );
                }
            }
            if( keep && verified ) dialog->accept();
            else
            {
                QKeyEvent escape( QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier );
                QApplication::sendEvent( dialog, &escape );
            }
        } );
        QKeyEvent press( QEvent::KeyPress, Qt::Key_1 + digit, modifiers );
        if( !panel.handleInputKey( &press ) )
        {
            QApplication::processEvents();
            return false;
        }
        if( !visited || !verified ) return false;
        if( !keep ) return panel.settingsJson() == before && panel.grab().toImage() == original;
        QJsonObject expected = before;
        expected.insert( "gridStyle", "continuous" );
        replaceSetting( expected, { "layers", "grid", "size" }, 100 );
        return panel.settingsJson() == expected;
    };
    if( !popup( 0, false ) || !popup( 1, false ) || !popup( 2, false ) || !popup( 1, true ) ) return 212;
    return 0;
}

static int checkFeedbackSettingsDialog( const QString &directory, const QDir &evidence )
{
    QSettings settings( directory + "/feedback-settings.ini", QSettings::IniFormat );
    FlubberPanel panel( &settings );
    panel.resize( 720, panel.height() );
    panel.show(); QApplication::setActiveWindow( &panel );
    const Qt::KeyboardModifiers shortcut = Qt::ControlModifier | Qt::ShiftModifier;
    QKeyEvent claim( QEvent::ShortcutOverride, Qt::Key_S, shortcut );
    const QJsonObject initial = panel.settingsJson();
    if( !panel.handleInputKey( &claim ) || !claim.isAccepted() || panel.settingsJson() != initial ) return 196;
    for( bool accept : { false, true } )
    {
        bool edited = false;
        QTimer::singleShot( 0, &panel, [&]() {
            auto *dialog = qobject_cast<QDialog *>( QApplication::activeModalWidget() );
            if( !dialog ) { std::fputs( "No settings dialog\n", stderr ); return; }
            auto *custom = dialog->findChild<QCheckBox *>( "flubber-custom-hold" );
            auto *duration = dialog->findChild<QSpinBox *>( "flubber-hold-duration" );
            auto *gridStyle = dialog->findChild<QComboBox *>( "flubber-grid-style" );
            auto *transparency = dialog->findChild<QSlider *>( "flubber-transparency" );
            auto *tabs = dialog->findChild<QTabWidget *>();
            QPushButton *color = NULL;
            if( !custom || !duration || !gridStyle || !transparency || !tabs )
            { std::fputs( "Missing settings control\n", stderr ); dialog->reject(); return; }
            for( auto *button : dialog->findChildren<QPushButton *>() )
                if( button->accessibleName() == QLatin1String( "Idle color" ) ) { color = button; break; }
            if( !color )
            { std::fputs( "Missing idle color\n", stderr ); dialog->reject(); return; }
            QTimer::singleShot( 0, [&]() {
                if( auto *picker = qobject_cast<QColorDialog *>( QApplication::activeModalWidget() ) )
                { picker->setCurrentColor( QColor( "#123456" ) ); picker->accept(); }
            } );
            color->click();
            if( color->text() != QLatin1String( "#123456" ) )
            { std::fputs( "Color picker did not apply\n", stderr ); dialog->reject(); return; }
            custom->setChecked( true );
            duration->setValue( 600 );
            gridStyle->setCurrentIndex( gridStyle->findData( "continuous" ) );
            transparency->setValue( 38 );
            for( int i = 0; i < tabs->count(); ++i )
            {
                tabs->setCurrentIndex( i );
                QApplication::processEvents();
                auto *body = tabs->currentWidget();
                if( body->minimumSizeHint().width() > body->width() ||
                    body->minimumSizeHint().height() > body->height() )
                {
                    std::fprintf( stderr, "Settings tab %d: %dx%d needs %dx%d\n", i, body->width(), body->height(),
                        body->minimumSizeHint().width(), body->minimumSizeHint().height() );
                    dialog->reject(); return;
                }
                if( accept && i < 4 )
                    dialog->grab().save( evidence.filePath( QStringLiteral( "feedback-settings-%1.png" ).arg( i ) ) );
            }
            edited = true;
            if( accept ) dialog->accept(); else dialog->reject();
        } );
        QKeyEvent open( QEvent::KeyPress, Qt::Key_S, shortcut );
        if( !accept )
        {
            FlubberNativeKeyEvent nativeOpen( Qt::Key_S, shortcut, false, -1 );
            QCoreApplication::sendEvent( &panel, &nativeOpen );
        }
        else if( !panel.handleInputKey( &open ) ) return 197;
        if( !edited ) return 197;
        if( !accept && panel.settingsJson() != initial ) return 198;
    }
    const QJsonObject changed = panel.settingsJson();
    if( changed.value( "arrowHoldMs" ).toInt() != 600 ||
        changed.value( "gridStyle" ).toString() != QLatin1String( "continuous" ) ||
        settingAt( changed, { "appearance", "visual", "transparency" } ).toDouble() != .38 ||
        settingAt( changed, { "appearance", "visual", "colors", "idle" } ).toString() != QLatin1String( "#123456" ) ) return 199;
    FlubberPanel reopened( &settings );
    if( reopened.settingsJson() != changed ) return 200;

    panel.setArrowMode( true );
    panel.setAffect( -1, .25 );
    panel.setFocus();
    QKeyEvent right( QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier );
    QKeyEvent release( QEvent::KeyRelease, Qt::Key_Right, Qt::NoModifier );
    if( !panel.handleInputKey( &right ) || qAbs( panel.valence() + .9 ) > .00001 ) return 201;
#ifndef Q_OS_WIN
    QElapsedTimer hold; hold.start();
    while( panel.valence() < 1 && hold.elapsed() < 1000 )
        QTest::qWait( 5 );
    if( panel.valence() != 1 || panel.arousal() != .25 || hold.elapsed() < 590 || hold.elapsed() > 700 ) return 202;
#endif
    panel.handleInputKey( &release );
    panel.setAffect( -1, .25 );
    panel.handleInputKey( &right );
    panel.handleInputKey( &release );
    QEventLoop wait;
    QTimer::singleShot( 70, &wait, &QEventLoop::quit ); wait.exec();
    if( qAbs( panel.valence() + .9 ) > .00001 ) return 203;

    panel.setPrimary( FlubberPanel::GridKind );
    panel.setAffect( .25, 0 );
    const QImage continuous = panel.grab().toImage();
    QString error;
    QJsonObject snapshot = panel.settingsJson();
    const QString file = directory + "/import-feedback.json";
    const auto load = [&]( const QJsonObject &json ) {
        QFile fixture( file );
        if( !fixture.open( QIODevice::WriteOnly | QIODevice::Truncate ) ||
            fixture.write( QJsonDocument( json ).toJson() ) < 0 ) return false;
        fixture.close();
        return panel.loadSettingsFile( file, error );
    };
    snapshot.insert( "gridStyle", "tiles" );
    replaceSetting( snapshot, { "grid", "columns" }, 3 );
    replaceSetting( snapshot, { "grid", "rows" }, 3 );
    if( !load( snapshot ) || panel.settingsJson() != snapshot || panel.grab().toImage() == continuous ||
        panel.valence() != .25 ) return 204;
    const QImage firstTile = panel.grab().toImage();
    panel.setAffect( .30, 0 );
    if( panel.grab().toImage() != firstTile ) return 205;
    for( const QJsonValue &bad : { QJsonValue( 1 ), QJsonValue( 15001 ), QJsonValue( 600.25 ), QJsonValue( "600" ) } )
    {
        QJsonObject invalid = snapshot; invalid.insert( "arrowHoldMs", bad );
        if( load( invalid ) || panel.settingsJson() != snapshot ) return 206;
    }
    QJsonObject invalid = snapshot; invalid.insert( "gridStyle", "unknown" );
    if( load( invalid ) || panel.settingsJson() != snapshot ) return 207;
    invalid = snapshot; invalid.remove( "stepPercent" );
    if( load( invalid ) || panel.settingsJson() != snapshot ) return 208;
    for( int version : { 3, 2 } )
    {
        QJsonObject legacy = snapshot;
        legacy.insert( "schema", QStringLiteral( "vlc-feedback-settings/v%1" ).arg( version ) );
        legacy.remove( "arrowHoldMs" ); legacy.remove( "gridStyle" );
        if( version == 2 ) legacy.remove( "stepPercent" );
        if( !load( legacy ) || panel.settingsJson() != snapshot ) return 209;
    }
    if( !panel.saveSettingsSnapshot( error ) ) return 210;
    settings.setValue( "Flubber/arrowHoldMs", 0 );
    settings.setValue( "Flubber/gridStyle", "continuous" );
    FlubberPanel restored( &settings );
    if( restored.settingsJson() != snapshot ) return 211;
    return 0;
}

int main( int argc, char **argv )
{
    QApplication::setAttribute( Qt::AA_DontUseNativeDialogs );
    QApplication app( argc, argv );
    {
        QTemporaryDir inputRaceRoot;
        QSettings inputRaceSettings( inputRaceRoot.filePath( "native-mode-ordering.ini" ), QSettings::IniFormat );
        FlubberPanel inputRace( &inputRaceSettings );
        QCoreApplication::postEvent( &inputRace, new FlubberNativeKeyEvent( Qt::Key_F, Qt::AltModifier, false, 1 ) );
        inputRace.setArrowMode( false );
        QCoreApplication::sendPostedEvents( &inputRace, FlubberNativeKeyEvent::eventType() );
        if( inputRace.arrowMode() || inputRaceSettings.value( "Flubber/inputMode" ).toString() != QLatin1String( "mouse" ) ) return 81;
        inputRaceSettings.clear();
    }
    if( argc != 3 ) return 2;
    const auto onlyVisible = []( const FlubberPanel &checked, FlubberPanel::FeedbackKind selected ) {
        const QJsonObject layers = checked.settingsJson().value( "layers" ).toObject();
        for( auto kind : { FlubberPanel::FlubberKind, FlubberPanel::GridKind, FlubberPanel::FaceKind } )
            if( checked.visible( kind ) != ( kind == selected ) ||
                checked.layoutFor( kind ).visible != ( kind == selected ) ||
                layers.value( FlubberPanel::kindName( kind ) ).toObject().value( "visible" ).toBool() != ( kind == selected ) )
                return false;
        return checked.primary() == selected;
    };
    QJsonObject appearance;
    QString error;
    if( !FlubberPanel::readAppearance( QString::fromLocal8Bit( argv[1] ), appearance, error ) )
    {
        std::fprintf( stderr, "Valid appearance rejected: %s\n", error.toUtf8().constData() );
        return 3;
    }
    QTemporaryDir temp;
    const QDir evidence = QFileInfo( QString::fromLocal8Bit( argv[2] ) ).dir();
    if( const int result = checkFeedbackSettingsDialog( temp.path(), evidence ) ) return result;
    if( const int result = checkRepresentationPopups( temp.path(), evidence ) ) return result;
    const int editorShortcuts = checkEditorShortcutOwnership( temp.path() );
    if( editorShortcuts ) return editorShortcuts;
    if( const int result = checkRecordingLock( temp.path() ) ) return result;
    QSettings settings( temp.filePath( "flubber.ini" ), QSettings::IniFormat );
    settings.setValue( "Flubber/appearancePath", QString::fromLocal8Bit( argv[1] ) );
    FlubberPanel panel( &settings );
    panel.resize( 640, panel.height() );
    panel.show();
    app.processEvents();
    QImage image = panel.grab().toImage();
    if( image.isNull() || image.pixelColor( 0, 0 ) != QColor( Qt::black ) ) return 4;
    if( !image.save( QString::fromLocal8Bit( argv[2] ) ) ) return 5;
    QFile original( QString::fromLocal8Bit( argv[1] ) );
    if( !original.open( QIODevice::ReadOnly ) ) return 6;
    QByteArray duplicate = original.readAll();
    duplicate.insert( duplicate.indexOf( '{' ) + 1, "\"\\u0073chema\":\"hidden-duplicate\"," );
    QFile bad( temp.filePath( "duplicate.json" ) );
    if( !bad.open( QIODevice::WriteOnly ) ) return 7;
    bad.write( duplicate ); bad.close();
    if( FlubberPanel::readAppearance( bad.fileName(), appearance, error ) || !error.contains( "duplicate" ) ) return 8;
    QJsonObject colors = appearance.value( "visual" ).toObject().value( "colors" ).toObject();
    colors.insert( "idle", "#ff0000" );
    colors.insert( "cursor", "#00ff00" );
    QJsonObject visual = appearance.value( "visual" ).toObject();
    visual.insert( "colors", colors );
    visual.insert( "transparency", 0 );
    appearance.insert( "visual", visual );
    QFile red( temp.filePath( "red.json" ) );
    if( !red.open( QIODevice::WriteOnly ) ) return 9;
    red.write( QJsonDocument( appearance ).toJson() ); red.close();
    settings.setValue( "Flubber/appearancePath", red.fileName() );
    FlubberPanel redPanel( &settings );
    redPanel.resize( 640, redPanel.height() ); redPanel.show(); app.processEvents();
    const QImage redImage = redPanel.grab().toImage();
    if( redImage.pixelColor( 320, 65 ) != QColor( "#ff0000" ) ) return 10;
    visual.insert( "transparency", 1 );
    appearance.insert( "visual", visual );
    QFile transparent( temp.filePath( "transparent.json" ) );
    if( !transparent.open( QIODevice::WriteOnly ) ) return 11;
    transparent.write( QJsonDocument( appearance ).toJson() ); transparent.close();
    settings.setValue( "Flubber/appearancePath", transparent.fileName() );
    FlubberPanel clearPanel( &settings );
    clearPanel.resize( 640, clearPanel.height() ); clearPanel.show(); app.processEvents();
    if( clearPanel.grab().toImage().pixelColor( 320, 65 ) != QColor( Qt::black ) ) return 12;
    QTimer::singleShot( 0, &app, [&]() {
        QDialog *dialog = qobject_cast<QDialog *>( QApplication::activeModalWidget() );
        if( !dialog ) { std::fprintf( stderr, "No modal dialog\n" ); return; }
        QSpinBox *height = dialog->findChild<QSpinBox *>( "flubber-panel-height" );
        QSpinBox *size = dialog->findChild<QSpinBox *>( "flubber-layer-0-size" );
        QSpinBox *x = dialog->findChild<QSpinBox *>( "flubber-layer-0-x" );
        QSpinBox *y = dialog->findChild<QSpinBox *>( "flubber-layer-0-y" );
        QComboBox *selection = dialog->findChild<QComboBox *>( "flubber-representation" );
        for( QCheckBox *box : dialog->findChildren<QCheckBox *>() )
            if( box->text() == QLatin1String( "Show" ) ) { dialog->reject(); return; }
        if( !height || !size || !x || !y || !selection ) { dialog->reject(); return; }
        height->setValue( 200 ); size->setValue( 80 );
        x->setValue( 25 ); y->setValue( 75 );
        dialog->accept();
    } );
    panel.showControls( NULL );
    if( settings.value( "Flubber/panelHeight" ).toInt() != 200 ||
        settings.value( "Flubber/layers/flubber/size" ).toInt() != 80 ||
        settings.value( "Flubber/layers/flubber/x" ).toInt() != 25 ||
        settings.value( "Flubber/layers/flubber/y" ).toInt() != 75 ) return 13;
    qputenv( "VLC_FLUBBER_STEP_PERCENT", "20" );
    const auto ratioCheck = [&]( const char *ratio, int expected ) {
        qputenv( "VLC_FLUBBER_PANEL_PERCENT", ratio );
        QWidget container;
        QVBoxLayout layout( &container );
        layout.setContentsMargins( 0, 0, 0, 0 ); layout.setSpacing( 0 );
        QWidget reference;
        FlubberPanel sessionPanel( &settings, &container );
        sessionPanel.setReferenceSurface( &reference );
        layout.addWidget( &reference, 1 );
        layout.addWidget( &sessionPanel );
        container.resize( 640, 800 ); container.show();
        QEventLoop loop;
        QTimer::singleShot( 160, &loop, &QEventLoop::quit );
        loop.exec();
        if( sessionPanel.height() != expected ) return false;
        QKeyEvent right( QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier );
        QApplication::sendEvent( &sessionPanel, &right );
        if( !qFuzzyCompare( sessionPanel.valence() + 1.f, 1.f ) ) return false;
        sessionPanel.setRateAllowedHandler( []() { return true; } );
        sessionPanel.setArrowMode( true );
        if( !sessionPanel.handleInputKey( &right ) ||
            !qFuzzyCompare( sessionPanel.valence() + 1.f, 1.2f ) ) return false;
        sessionPanel.setVideoAffect( -.8, -.8 );
        if( !qFuzzyCompare( sessionPanel.valence() + 1.f, 1.2f ) ) return false;
        QKeyEvent modified( QEvent::KeyPress, Qt::Key_Right, Qt::ControlModifier );
        if( !sessionPanel.handleInputKey( &modified ) ||
            !qFuzzyCompare( sessionPanel.valence() + 1.f, 1.2f ) ) return false;
        sessionPanel.setArrowMode( false );
        if( sessionPanel.handleInputKey( &right ) ) return false;
        sessionPanel.setVideoAffect( -.5, .5 );
        return qFuzzyCompare( sessionPanel.valence() + 1.f, .5f ) &&
               qFuzzyCompare( sessionPanel.arousal() + 1.f, 1.5f ) &&
               settings.value( "Flubber/inputMode" ).toString() == QLatin1String( "mouse" );
    };
    if( !ratioCheck( "100", 400 ) || !ratioCheck( "10", 73 ) ) return 14;
    for( bool oldVisible : { true, false } )
    {
        QSettings legacy( temp.filePath( oldVisible ? "all-visible.ini" : "all-hidden.ini" ), QSettings::IniFormat );
        legacy.setValue( "Flubber/primary", "grid" );
        for( const QString &kind : { QStringLiteral( "flubber" ), QStringLiteral( "grid" ), QStringLiteral( "face" ) } )
            legacy.setValue( "Flubber/layers/" + kind + "/visible", oldVisible );
        legacy.setValue( "Flubber/layers/face/x", 73 );
        FlubberPanel migrated( &legacy );
        if( !onlyVisible( migrated, FlubberPanel::GridKind ) || migrated.layoutFor( FlubberPanel::FaceKind ).x != 73 ||
            legacy.value( "Flubber/layers/flubber/visible" ).toBool() ||
            !legacy.value( "Flubber/layers/grid/visible" ).toBool() ||
            legacy.value( "Flubber/layers/face/visible" ).toBool() ) return 61;
    }
    {
        QSettings chordSettings( temp.filePath( "chord.ini" ), QSettings::IniFormat );
        FlubberPanel chord( &chordSettings );
        chord.setArrowMode( true );
        QKeyEvent left( QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier );
        QKeyEvent up( QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier );
        QKeyEvent leftUp( QEvent::KeyRelease, Qt::Key_Left, Qt::NoModifier );
        if( !chord.handleInputKey( &left ) || chord.valence() >= 0 || chord.arousal() != 0 ) return 158;
        const float first = chord.valence();
        if( !chord.handleInputKey( &up ) || chord.valence() >= first || chord.arousal() <= 0 ||
            qAbs( chord.valence() - first + chord.arousal() ) > .00001f ||
            qAbs( chord.arousal() * qSqrt( 2.0 ) + first ) > .00001 ) return 159;
        const float diagonal = chord.valence(), arousal = chord.arousal();
        QKeyEvent repeat( QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier, QString(), true );
        if( !chord.handleInputKey( &repeat ) || chord.valence() != diagonal || chord.arousal() != arousal ) return 160;
        QKeyEvent right( QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier );
        if( !chord.handleInputKey( &right ) || chord.valence() != diagonal || chord.arousal() <= arousal ) return 161;
        QKeyEvent upUp( QEvent::KeyRelease, Qt::Key_Up, Qt::NoModifier );
        QKeyEvent down( QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier );
        const float beforeRelease = chord.arousal();
        if( !chord.handleInputKey( &leftUp ) || !chord.handleInputKey( &upUp ) ||
            !chord.handleInputKey( &down ) || chord.valence() <= diagonal || chord.arousal() >= beforeRelease ) return 162;
        chord.stopRating();
        chord.setAffect( 0, 0 );
        if( !chord.handleInputKey( &up ) || chord.valence() != 0 || chord.arousal() <= 0 ) return 163;
    }
    QSettings keySettings( temp.filePath( "keys.ini" ), QSettings::IniFormat );
    FlubberPanel keysPanel( &keySettings );
    bool inputAllowed = true;
    keysPanel.setRateAllowedHandler( [&]() { return inputAllowed; } );
    QKeyEvent toggle( QEvent::KeyPress, Qt::Key_F, Qt::AltModifier );
    if( !keysPanel.handleInputKey( &toggle ) || !keysPanel.arrowMode() ) return 23;
    QKeyEvent claim( QEvent::ShortcutOverride, Qt::Key_Up, Qt::NoModifier );
    if( !keysPanel.handleInputKey( &claim ) || !claim.isAccepted() || keysPanel.arousal() != 0 ) return 24;
    QKeyEvent up( QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier );
    if( !keysPanel.handleInputKey( &up ) || keysPanel.arousal() <= 0 ) return 25;
    const float arousal = keysPanel.arousal();
    inputAllowed = false;
    if( !keysPanel.handleInputKey( &up ) || keysPanel.arousal() != arousal ) return 26;
    QKeyEvent repeat( QEvent::KeyPress, Qt::Key_F, Qt::AltModifier, QString(), true );
    if( !keysPanel.handleInputKey( &repeat ) || !keysPanel.arrowMode() ) return 27;
    if( !keysPanel.handleInputKey( &toggle ) || keysPanel.arrowMode() || keysPanel.handleInputKey( &up ) ) return 28;
    QKeyEvent cycleClaim( QEvent::ShortcutOverride, Qt::Key_G, Qt::AltModifier );
    if( !keysPanel.handleInputKey( &cycleClaim ) || keysPanel.primary() != FlubberPanel::FlubberKind ) return 35;
    QKeyEvent cycle( QEvent::KeyPress, Qt::Key_G, Qt::AltModifier );
    if( !keysPanel.handleInputKey( &cycle ) || !onlyVisible( keysPanel, FlubberPanel::GridKind ) ) return 36;
    const QStringList expectedFaces{ "photo-reference-v3", "photo-synthetic-01", "photo-synthetic-02",
        "photo-synthetic-03", "photo-synthetic-04", "photo-synthetic-05", "photo-synthetic-06",
        "photo-synthetic-07", "photo-synthetic-08" };
    if( !keysPanel.handleInputKey( &cycle ) || !onlyVisible( keysPanel, FlubberPanel::FaceKind ) ) return 37;
    const QString retainedFace = keysPanel.faceId();
    if( !keysPanel.handleInputKey( &cycle ) || !onlyVisible( keysPanel, FlubberPanel::FlubberKind ) ) return 38;
    QKeyEvent cycleRepeat( QEvent::KeyPress, Qt::Key_G, Qt::AltModifier, QString(), true );
    if( !keysPanel.handleInputKey( &cycleRepeat ) || keysPanel.primary() != FlubberPanel::FlubberKind ) return 39;
    QKeyEvent previous( QEvent::KeyPress, Qt::Key_G, Qt::AltModifier | Qt::ShiftModifier );
    if( !keysPanel.handleInputKey( &previous ) || keysPanel.primary() != FlubberPanel::FaceKind ||
        keysPanel.faceId() != retainedFace ) return 40;
    keysPanel.setPrimary( FlubberPanel::FlubberKind );
    if( !keysPanel.setFaceId( "photo-reference-v3" ) || !onlyVisible( keysPanel, FlubberPanel::FlubberKind ) ) return 62;
    QKeyEvent faceClaim( QEvent::ShortcutOverride, Qt::Key_H, Qt::AltModifier );
    QKeyEvent faceRepeat( QEvent::KeyPress, Qt::Key_H, Qt::AltModifier, QString(), true );
    QKeyEvent wrongFaceModifier( QEvent::KeyPress, Qt::Key_H, Qt::ControlModifier );
    if( !keysPanel.handleInputKey( &faceClaim ) || !faceClaim.isAccepted() ||
        !keysPanel.handleInputKey( &faceRepeat ) || keysPanel.handleInputKey( &wrongFaceModifier ) ||
        keysPanel.faceId() != QLatin1String( "photo-reference-v3" ) ||
        !onlyVisible( keysPanel, FlubberPanel::FlubberKind ) ) return 63;
    QKeyEvent nextFace( QEvent::KeyPress, Qt::Key_H, Qt::AltModifier );
    QKeyEvent previousFace( QEvent::KeyPress, Qt::Key_H, Qt::AltModifier | Qt::ShiftModifier );
    for( int i = 1; i <= 9; ++i )
        if( !keysPanel.handleInputKey( &nextFace ) || keysPanel.faceId() != expectedFaces[i % 9] ||
            !onlyVisible( keysPanel, FlubberPanel::FaceKind ) ) return 64;
    for( int i = 8; i >= 0; --i )
        if( !keysPanel.handleInputKey( &previousFace ) || keysPanel.faceId() != expectedFaces[i] ||
            !onlyVisible( keysPanel, FlubberPanel::FaceKind ) ) return 65;
    if( !keysPanel.handleInputKey( &nextFace ) || !keysPanel.handleInputKey( &cycle ) ||
        !onlyVisible( keysPanel, FlubberPanel::FlubberKind ) || !keysPanel.handleInputKey( &previous ) ||
        !onlyVisible( keysPanel, FlubberPanel::FaceKind ) || keysPanel.faceId() != QLatin1String( "photo-synthetic-01" ) ||
        keySettings.value( "Flubber/faceId" ).toString() != QLatin1String( "photo-synthetic-01" ) ) return 66;
    keysPanel.setArrowMode( true );
    for( auto modifier : { Qt::ShiftModifier, Qt::ControlModifier, Qt::AltModifier } )
    {
        QKeyEvent modifiedArrow( QEvent::KeyPress, Qt::Key_Right, modifier );
        if( !keysPanel.handleInputKey( &modifiedArrow ) || keysPanel.valence() != 0 ) return 56;
    }
    keysPanel.setArrowMode( false );
    const auto beforeMove = keysPanel.layoutFor( FlubberPanel::FaceKind );
    QKeyEvent moveRight( QEvent::KeyPress, Qt::Key_Right, Qt::ControlModifier | Qt::AltModifier );
    if( keysPanel.handleInputKey( &moveRight ) ) return 57;
    keysPanel.setArrowMode( true );
    if( !keysPanel.handleInputKey( &moveRight ) ||
        keysPanel.layoutFor( FlubberPanel::FaceKind ).x != qMin( 100, beforeMove.x + 2 ) ) return 41;
    QKeyEvent bigger( QEvent::KeyPress, Qt::Key_Equal, Qt::AltModifier );
    if( !keysPanel.handleInputKey( &bigger ) ||
        keysPanel.layoutFor( FlubberPanel::FaceKind ).size != qMin( 100, beforeMove.size + 2 ) ) return 42;
    if( keysPanel.valence() != 0 || keysPanel.arousal() != arousal ) return 43;
    const QJsonObject beforeRetired = keysPanel.settingsJson();
    for( int key : { Qt::Key_BracketRight, Qt::Key_Period, Qt::Key_PageUp, Qt::Key_T, Qt::Key_F1, Qt::Key_Q } )
    {
        QKeyEvent retired( QEvent::KeyPress, key, Qt::ControlModifier | Qt::AltModifier );
        if( keysPanel.handleInputKey( &retired ) || keysPanel.settingsJson() != beforeRetired ) return 44;
    }
    qunsetenv( "VLC_FLUBBER_PANEL_PERCENT" );
    QSettings manualSettings( temp.filePath( "manual.ini" ), QSettings::IniFormat );
    FlubberPanel manualPanel( &manualSettings );
    const auto moved = keysPanel.layoutFor( FlubberPanel::FaceKind );
    QKeyEvent layoutClaim( QEvent::ShortcutOverride, Qt::Key_Right, Qt::ControlModifier | Qt::AltModifier );
    if( !keysPanel.handleInputKey( &layoutClaim ) || keysPanel.layoutFor( FlubberPanel::FaceKind ).x != moved.x ) return 49;
    QKeyEvent wrongModifier( QEvent::KeyPress, Qt::Key_G, Qt::ControlModifier );
    if( keysPanel.handleInputKey( &wrongModifier ) ) return 50;
    bool controlsOpened = false;
    QTimer::singleShot( 0, [&]() {
        QDialog *dialog = qobject_cast<QDialog *>( QApplication::activeModalWidget() );
        if( dialog ) { controlsOpened = dialog->windowTitle() == QLatin1String( "Flubber Settings" ); dialog->reject(); }
    } );
    QKeyEvent controls( QEvent::KeyPress, Qt::Key_S, Qt::ControlModifier | Qt::ShiftModifier );
    if( !manualPanel.handleInputKey( &controls ) || !controlsOpened ) return 52;
    manualPanel.resize( 600, manualPanel.height() );
    const auto renderedChange = [&]() {
        manualPanel.setAffect( -1, -1 );
        QEventLoop first; QTimer::singleShot( 200, &first, &QEventLoop::quit ); first.exec();
        const QImage low = manualPanel.grab().toImage();
        manualPanel.setAffect( 1, 1 );
        QEventLoop second; QTimer::singleShot( 200, &second, &QEventLoop::quit ); second.exec();
        return low != manualPanel.grab().toImage();
    };
    for( auto kind : { FlubberPanel::FlubberKind, FlubberPanel::GridKind, FlubberPanel::FaceKind } )
    {
        manualPanel.setPrimary( kind );
        if( !onlyVisible( manualPanel, kind ) ) return 67;
        if( kind != FlubberPanel::FaceKind ) { if( !renderedChange() ) return 53; }
        else for( const QString &id : expectedFaces )
        {
            QString loadError;
            if( !manualPanel.setFaceId( id, &loadError ) || !renderedChange() ) return 54;
        }
    }
    keysPanel.setArrowMode( false );
#ifndef Q_OS_WIN
    // Exercise Qt's real shortcut map from a video-like sibling widget. The
    // Windows offscreen platform has no native foreground HWND; its dispatch
    // logic is covered above, while this integration runs in Linux Qt CI.
    QWidget shortcutHost;
    QVBoxLayout shortcutLayout( &shortcutHost );
    QWidget videoFocus;
    videoFocus.setFocusPolicy( Qt::StrongFocus );
    FlubberPanel shortcutPanel( &keySettings, &shortcutHost );
    shortcutPanel.setRateAllowedHandler( [&]() { return inputAllowed; } );
    shortcutLayout.addWidget( &videoFocus );
    shortcutLayout.addWidget( &shortcutPanel );
    int vlcCommands = 0;
    int modifiedVlcCommands = 0;
    const Qt::KeyboardModifiers arrowModifiers[] = { Qt::ShiftModifier, Qt::ControlModifier,
        Qt::AltModifier, Qt::ControlModifier | Qt::AltModifier };
    for( auto modifiers : arrowModifiers )
    {
        QShortcut *shortcut = new QShortcut( QKeySequence( int( modifiers ) | Qt::Key_Left ), &shortcutHost );
        QObject::connect( shortcut, &QShortcut::activated, [&]() { ++modifiedVlcCommands; } );
    }
    for( int key : { Qt::Key_Left, Qt::Key_Right, Qt::Key_Up, Qt::Key_Down } )
    {
        QShortcut *shortcut = new QShortcut( QKeySequence( key ), &shortcutHost );
        QObject::connect( shortcut, &QShortcut::activated, [&]() { ++vlcCommands; } );
    }
    shortcutHost.show();
    QApplication::setActiveWindow( &shortcutHost );
    videoFocus.setFocus(); app.processEvents();
    QTest::keyClick( &videoFocus, Qt::Key_Up );
    if( vlcCommands != 1 ) return 29;
    for( auto modifiers : arrowModifiers ) QTest::keyClick( &videoFocus, Qt::Key_Left, modifiers );
    if( modifiedVlcCommands != 4 ) return 58;
    QTest::keyClick( &videoFocus, Qt::Key_F, Qt::AltModifier );
    if( !shortcutPanel.arrowMode() ) return 30;
    for( auto modifiers : arrowModifiers ) QTest::keyClick( &videoFocus, Qt::Key_Left, modifiers );
    if( modifiedVlcCommands != 4 ) return 59;
    QTest::keyClick( &videoFocus, Qt::Key_G, Qt::AltModifier );
    if( shortcutPanel.primary() != FlubberPanel::FlubberKind ) return 51;
    inputAllowed = true;
    QTest::keyClick( &videoFocus, Qt::Key_Up );
    QTest::keyClick( &videoFocus, Qt::Key_Right );
    if( vlcCommands != 1 || shortcutPanel.arousal() <= 0 || shortcutPanel.valence() <= 0 ) return 31;
    inputAllowed = false;
    const float held = shortcutPanel.arousal();
    QTest::keyClick( &videoFocus, Qt::Key_Up );
    if( vlcCommands != 1 || shortcutPanel.arousal() != held ) return 32;
    inputAllowed = true;
    QTest::keyClick( &videoFocus, Qt::Key_Up );
    if( vlcCommands != 1 || shortcutPanel.arousal() <= held ) return 34;
    QTest::keyClick( &videoFocus, Qt::Key_F, Qt::AltModifier );
    QTest::keyClick( &videoFocus, Qt::Key_Up );
    if( shortcutPanel.arrowMode() || vlcCommands != 2 ) return 33;
    for( auto modifiers : arrowModifiers ) QTest::keyClick( &videoFocus, Qt::Key_Left, modifiers );
    if( modifiedVlcCommands != 8 ) return 60;
    int vlcLayoutCommands = 0;
    QShortcut vlcAltW( QKeySequence( Qt::ALT | Qt::Key_W ), &shortcutHost );
    QObject::connect( &vlcAltW, &QShortcut::activated, [&]() { ++vlcLayoutCommands; } );
    const auto videoLayout = shortcutPanel.layoutFor( shortcutPanel.primary() );
    const float videoValence = shortcutPanel.valence(), videoArousal = shortcutPanel.arousal();
    QTest::keyClick( &videoFocus, Qt::Key_W, Qt::AltModifier );
    if( vlcLayoutCommands != 0 || shortcutPanel.layoutFor( shortcutPanel.primary() ).y != videoLayout.y - 2 ||
        shortcutPanel.valence() != videoValence || shortcutPanel.arousal() != videoArousal ) return 76;
    QTest::keyClick( &videoFocus, Qt::Key_J, Qt::AltModifier );
    const QString videoSnapshot = keySettings.value( "Flubber/lastSettingsPath" ).toString();
    if( !QFileInfo::exists( videoSnapshot ) ) return 77;
    QLineEdit textEntry( &shortcutHost );
    shortcutLayout.addWidget( &textEntry ); textEntry.show(); textEntry.setFocus(); app.processEvents();
    const auto primaryBeforeTyping = shortcutPanel.primary();
    QTest::keyClick( &textEntry, Qt::Key_G, Qt::AltModifier );
    if( shortcutPanel.primary() != primaryBeforeTyping ) return 55;
    const QJsonObject beforeTextShortcuts = shortcutPanel.settingsJson();
    QTest::keyClick( &textEntry, Qt::Key_G, Qt::AltModifier );
    if( shortcutPanel.settingsJson() != beforeTextShortcuts ) return 124;
    QTest::keyClick( &textEntry, Qt::Key_J, Qt::AltModifier );
    if( keySettings.value( "Flubber/lastSettingsPath" ).toString() != videoSnapshot ) return 78;
#endif
    if( settings.value( "Flubber/panelHeight" ).toInt() != 200 ) return 15;
    panel.setPrimary( FlubberPanel::GridKind );
    panel.setLayout( FlubberPanel::GridKind, { true, 45, 18, 50 } );
    panel.setLayout( FlubberPanel::FaceKind, { true, 45, 82, 50 } );
    if( !onlyVisible( panel, FlubberPanel::GridKind ) ) return 16;
    QString faceError;
    if( !panel.setFaceId( "photo-synthetic-01", &faceError ) ) return 17;
    panel.resize( 900, panel.height() ); app.processEvents();
    const QImage exclusive = panel.grab().toImage();
    exclusive.save( QFileInfo( QString::fromLocal8Bit( argv[2] ) ).dir().filePath( "exclusive-grid.png" ) );
    if( exclusive.pixelColor( 0, 0 ) != QColor( Qt::black ) ||
        exclusive.pixelColor( 162, 100 ) == QColor( Qt::black ) ||
        exclusive.pixelColor( 738, 100 ) != QColor( Qt::black ) ) return 18;
    const QString fullSettings = temp.filePath( "full-settings.json" );
    if( !panel.saveSettingsFile( fullSettings, error ) ) return 19;
    FlubberPanel restored( &settings );
    if( !restored.loadSettingsFile( fullSettings, error ) ||
        restored.primary() != FlubberPanel::GridKind ||
        restored.faceId() != QLatin1String( "photo-synthetic-01" ) ||
        !onlyVisible( restored, FlubberPanel::GridKind ) ) return 20;
    for( bool oldVisible : { true, false } )
    {
        QJsonObject legacy = panel.settingsJson();
        QJsonObject layers = legacy.value( "layers" ).toObject();
        for( const QString &kind : layers.keys() )
        {
            QJsonObject layer = layers.value( kind ).toObject();
            layer.insert( "visible", oldVisible ); layers.insert( kind, layer );
        }
        legacy.insert( "layers", layers );
        QFile file( temp.filePath( "legacy-settings.json" ) );
        if( !file.open( QIODevice::WriteOnly ) ) return 68;
        file.write( QJsonDocument( legacy ).toJson() ); file.close();
        if( !restored.loadSettingsFile( file.fileName(), error ) || !onlyVisible( restored, FlubberPanel::GridKind ) ||
            restored.layoutFor( FlubberPanel::GridKind ).size != 45 ||
            restored.layoutFor( FlubberPanel::GridKind ).x != 18 ||
            restored.layoutFor( FlubberPanel::FaceKind ).x != 82 ||
            restored.faceId() != QLatin1String( "photo-synthetic-01" ) ) return 69;
    }
    restored.setVisible( FlubberPanel::FaceKind, true );
    if( !onlyVisible( restored, FlubberPanel::FaceKind ) ) return 70;
    restored.setVisible( FlubberPanel::GridKind, false );
    if( !onlyVisible( restored, FlubberPanel::FaceKind ) ) return 71;
    restored.setVisible( FlubberPanel::FaceKind, false );
    restored.setVisible( FlubberPanel::FlubberKind, false );
    if( !onlyVisible( restored, FlubberPanel::FlubberKind ) ) return 72;
    restored.setPrimary( FlubberPanel::GridKind );
    QJsonObject invalid = panel.settingsJson(); invalid.insert( "primary", "unknown" );
    QFile invalidFile( temp.filePath( "invalid-settings.json" ) );
    if( !invalidFile.open( QIODevice::WriteOnly ) ) return 21;
    invalidFile.write( QJsonDocument( invalid ).toJson() ); invalidFile.close();
    if( restored.loadSettingsFile( invalidFile.fileName(), error ) ||
        restored.primary() != FlubberPanel::GridKind ) return 22;
    qunsetenv( "VLC_FLUBBER_PANEL_PERCENT" );
    qunsetenv( "VLC_FLUBBER_STEP_PERCENT" );
    QSettings snapshotSettings( temp.filePath( "snapshots.ini" ), QSettings::IniFormat );
    FlubberPanel snapshotPanel( &snapshotSettings );
    snapshotPanel.setPrimary( FlubberPanel::GridKind );
    snapshotPanel.setAffect( .25, -.5 );
    struct LayoutKey { int key; Qt::KeyboardModifiers modifiers; int size, x, y; };
    const LayoutKey layoutKeys[] = {
        { Qt::Key_W, Qt::AltModifier, 60, 50, 48 },
        { Qt::Key_A, Qt::AltModifier, 60, 48, 50 },
        { Qt::Key_S, Qt::AltModifier, 60, 50, 52 },
        { Qt::Key_D, Qt::AltModifier, 60, 52, 50 },
        { Qt::Key_Plus, Qt::AltModifier, 62, 50, 50 },
        { Qt::Key_Equal, Qt::AltModifier, 62, 50, 50 },
        { Qt::Key_Plus, Qt::AltModifier | Qt::ShiftModifier, 62, 50, 50 },
        { Qt::Key_Plus, Qt::AltModifier | Qt::KeypadModifier, 62, 50, 50 },
        { Qt::Key_Minus, Qt::AltModifier, 58, 50, 50 },
        { Qt::Key_Minus, Qt::AltModifier | Qt::KeypadModifier, 58, 50, 50 }
    };
    for( bool arrows : { false, true } )
    {
        snapshotPanel.setArrowMode( arrows );
        for( const auto &action : layoutKeys )
        {
            snapshotPanel.setLayout( FlubberPanel::GridKind, { true, 60, 50, 50 } );
            QKeyEvent probe( QEvent::ShortcutOverride, action.key, action.modifiers );
            if( !snapshotPanel.handleInputKey( &probe ) ||
                snapshotPanel.layoutFor( FlubberPanel::GridKind ).size != 60 ||
                snapshotPanel.layoutFor( FlubberPanel::GridKind ).x != 50 ||
                snapshotPanel.layoutFor( FlubberPanel::GridKind ).y != 50 ) return 61;
            QKeyEvent press( QEvent::KeyPress, action.key, action.modifiers );
            if( !snapshotPanel.handleInputKey( &press ) ) return 62;
            const auto moved = snapshotPanel.layoutFor( FlubberPanel::GridKind );
            if( moved.size != action.size || moved.x != action.x || moved.y != action.y ||
                snapshotPanel.valence() != .25f || snapshotPanel.arousal() != -.5f ) return 63;
        }
    }
    snapshotPanel.setLayout( FlubberPanel::GridKind, { true, 100, 0, 0 } );
    QKeyEvent top( QEvent::KeyPress, Qt::Key_W, Qt::AltModifier );
    QKeyEvent grow( QEvent::KeyPress, Qt::Key_Plus, Qt::AltModifier );
    snapshotPanel.handleInputKey( &top ); snapshotPanel.handleInputKey( &grow );
    if( snapshotPanel.layoutFor( FlubberPanel::GridKind ).y != 0 ||
        snapshotPanel.layoutFor( FlubberPanel::GridKind ).size != 100 ) return 64;
    QKeyEvent saveProbe( QEvent::ShortcutOverride, Qt::Key_J, Qt::AltModifier );
    if( !snapshotPanel.handleInputKey( &saveProbe ) || snapshotSettings.contains( "Flubber/lastSettingsPath" ) ) return 65;
    QKeyEvent saveSnapshot( QEvent::KeyPress, Qt::Key_J, Qt::AltModifier );
    if( !snapshotPanel.handleInputKey( &saveSnapshot ) ) return 66;
    const QString firstSnapshot = snapshotSettings.value( "Flubber/lastSettingsPath" ).toString();
    QFile snapshotFile( firstSnapshot );
    if( !snapshotFile.open( QIODevice::ReadOnly ) ||
        QJsonDocument::fromJson( snapshotFile.readAll() ).object() != snapshotPanel.settingsJson() ) return 67;
    snapshotFile.close();
    QKeyEvent saveRepeat( QEvent::KeyPress, Qt::Key_J, Qt::AltModifier, QString(), true );
    snapshotPanel.handleInputKey( &saveRepeat );
    if( snapshotSettings.value( "Flubber/lastSettingsPath" ).toString() != firstSnapshot ) return 68;
    snapshotPanel.setLayout( FlubberPanel::GridKind, { true, 72, 34, 68 } );
    snapshotPanel.handleInputKey( &saveSnapshot );
    const QString lastSnapshot = snapshotSettings.value( "Flubber/lastSettingsPath" ).toString();
    if( lastSnapshot == firstSnapshot || !QFileInfo::exists( firstSnapshot ) ) return 69;
    const QJsonObject savedSnapshot = snapshotPanel.settingsJson();
    snapshotPanel.setLayout( FlubberPanel::GridKind, { true, 20, 2, 4 } );
    FlubberPanel autoRestored( &snapshotSettings );
    if( autoRestored.settingsJson() != savedSnapshot ) return 70;
    snapshotSettings.setValue( "Flubber/lastSettingsPath", temp.filePath( "missing.json" ) );
    FlubberPanel missingSnapshot( &snapshotSettings );
    if( missingSnapshot.settingsJson() != savedSnapshot || missingSnapshot.appearanceLoadError().isEmpty() ) return 71;
    snapshotSettings.setValue( "Flubber/lastSettingsPath", invalidFile.fileName() );
    FlubberPanel corruptSnapshot( &snapshotSettings );
    if( corruptSnapshot.settingsJson() != savedSnapshot || corruptSnapshot.appearanceLoadError().isEmpty() ) return 72;
    if( !corruptSnapshot.loadSettingsFile( fullSettings, error ) ||
        corruptSnapshot.primary() != FlubberPanel::GridKind || corruptSnapshot.faceId() != QLatin1String( "photo-synthetic-01" ) ) return 73;
    const QString blockedProfile = temp.filePath( "blocked" );
    QDir().mkpath( blockedProfile );
    QFile blockedDirectory( blockedProfile + "/flubber-presets" );
    if( !blockedDirectory.open( QIODevice::WriteOnly ) ) return 74;
    blockedDirectory.close();
    QSettings blockedSettings( blockedProfile + "/settings.ini", QSettings::IniFormat );
    blockedSettings.setValue( "Flubber/lastSettingsPath", lastSnapshot );
    FlubberPanel blockedPanel( &blockedSettings );
    bool saveErrorShown = false;
    QTimer::singleShot( 0, [&]() {
        auto *warning = qobject_cast<QMessageBox *>( QApplication::activeModalWidget() );
        if( warning ) { saveErrorShown = true; warning->accept(); }
    } );
    blockedPanel.handleInputKey( &saveSnapshot );
    if( !saveErrorShown || blockedSettings.value( "Flubber/lastSettingsPath" ).toString() != lastSnapshot ) return 75;
    const QString bookmarkProfile = temp.filePath( "bookmark" );
    QDir().mkpath( bookmarkProfile );
    QSettings bookmarkSettings( bookmarkProfile + "/settings.ini", QSettings::IniFormat );
    bookmarkSettings.setValue( "Flubber/lastSettingsPath", lastSnapshot );
    bookmarkSettings.sync();
    QFile::remove( bookmarkSettings.fileName() );
    QDir().mkdir( bookmarkSettings.fileName() );
    FlubberPanel bookmarkPanel( &bookmarkSettings );
    if( bookmarkPanel.saveSettingsSnapshot( error ) ||
        bookmarkSettings.value( "Flubber/lastSettingsPath" ).toString() != lastSnapshot ) return 79;
    QDir().rmdir( bookmarkSettings.fileName() );
    bookmarkSettings.sync();
    if( !bookmarkPanel.saveSettingsSnapshot( error ) ||
        bookmarkSettings.value( "Flubber/lastSettingsPath" ).toString() == lastSnapshot ) return 80;
    return 0;
}
