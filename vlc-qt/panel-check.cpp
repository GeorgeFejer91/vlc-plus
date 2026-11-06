#include "flubber_panel.hpp"
#include "flubber_bridge.hpp"
#include <QApplication>
#include <QImage>
#include <QTemporaryDir>
#include <QFile>
#include <QEventLoop>
#include <QTimer>
#include <QSpinBox>
#include <QVBoxLayout>
#ifndef Q_OS_WIN
#include <QShortcut>
#include <QtTest/QTest>
#endif
#include <cstdio>

int main( int argc, char **argv )
{
    QApplication app( argc, argv );
    if( argc != 3 ) return 2;
    QJsonObject appearance;
    QString error;
    if( !FlubberPanel::readAppearance( QString::fromLocal8Bit( argv[1] ), appearance, error ) )
    {
        std::fprintf( stderr, "Valid appearance rejected: %s\n", error.toUtf8().constData() );
        return 3;
    }
    QTemporaryDir temp;
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
        if( !height || !size || !x || !y ) { dialog->reject(); return; }
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
    if( !keysPanel.handleInputKey( &cycle ) || keysPanel.primary() != FlubberPanel::GridKind ) return 36;
    const QStringList expectedFaces{ "photo-reference-v3", "photo-synthetic-01", "photo-synthetic-02",
        "photo-synthetic-03", "photo-synthetic-04", "photo-synthetic-05", "photo-synthetic-06",
        "photo-synthetic-07", "photo-synthetic-08" };
    if( !keysPanel.handleInputKey( &cycle ) || keysPanel.primary() != FlubberPanel::FaceKind ||
        !keysPanel.visible( FlubberPanel::FaceKind ) ) return 37;
    const QString retainedFace = keysPanel.faceId();
    if( !keysPanel.handleInputKey( &cycle ) || keysPanel.primary() != FlubberPanel::FlubberKind ) return 38;
    QKeyEvent cycleRepeat( QEvent::KeyPress, Qt::Key_G, Qt::AltModifier, QString(), true );
    if( !keysPanel.handleInputKey( &cycleRepeat ) || keysPanel.primary() != FlubberPanel::FlubberKind ) return 39;
    QKeyEvent previous( QEvent::KeyPress, Qt::Key_G, Qt::AltModifier | Qt::ShiftModifier );
    if( !keysPanel.handleInputKey( &previous ) || keysPanel.primary() != FlubberPanel::FaceKind ||
        keysPanel.faceId() != retainedFace ) return 40;
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
    QKeyEvent bigger( QEvent::KeyPress, Qt::Key_Equal, Qt::ControlModifier | Qt::AltModifier );
    if( !keysPanel.handleInputKey( &bigger ) ||
        keysPanel.layoutFor( FlubberPanel::FaceKind ).size != qMin( 100, beforeMove.size + 2 ) ) return 42;
    if( keysPanel.valence() != 0 || keysPanel.arousal() != arousal ) return 43;
    QKeyEvent finer( QEvent::KeyPress, Qt::Key_BracketRight, Qt::ControlModifier | Qt::AltModifier );
    if( !keysPanel.handleInputKey( &finer ) || keySettings.value( "Flubber/stepPercent" ).toInt() != 21 ) return 44;
    QKeyEvent faster( QEvent::KeyPress, Qt::Key_Period, Qt::ControlModifier | Qt::AltModifier );
    if( !keysPanel.handleInputKey( &faster ) ||
        keysPanel.settingsJson().value( "face" ).toObject().value( "transitionRate" ).toDouble() != 10.5 ) return 45;
    QKeyEvent denser( QEvent::KeyPress, Qt::Key_PageUp, Qt::ControlModifier | Qt::AltModifier );
    if( !keysPanel.handleInputKey( &denser ) || keySettings.value( "Flubber/gridColumns" ).toInt() != 23 ||
        keySettings.value( "Flubber/gridRows" ).toInt() != 23 ) return 46;
    QKeyEvent taller( QEvent::KeyPress, Qt::Key_PageUp, Qt::ControlModifier | Qt::ShiftModifier );
    if( !keysPanel.handleInputKey( &taller ) || keysPanel.height() != 130 ) return 47;
    qunsetenv( "VLC_FLUBBER_PANEL_PERCENT" );
    QSettings manualSettings( temp.filePath( "manual.ini" ), QSettings::IniFormat );
    FlubberPanel manualPanel( &manualSettings );
    if( !manualPanel.handleInputKey( &taller ) || manualPanel.height() != 146 ||
        manualSettings.value( "Flubber/panelHeight" ).toInt() != 146 ) return 48;
    const auto moved = keysPanel.layoutFor( FlubberPanel::FaceKind );
    QKeyEvent layoutClaim( QEvent::ShortcutOverride, Qt::Key_Right, Qt::ControlModifier | Qt::AltModifier );
    if( !keysPanel.handleInputKey( &layoutClaim ) || keysPanel.layoutFor( FlubberPanel::FaceKind ).x != moved.x ) return 49;
    QKeyEvent wrongModifier( QEvent::KeyPress, Qt::Key_G, Qt::ControlModifier );
    if( keysPanel.handleInputKey( &wrongModifier ) ) return 50;
    bool controlsOpened = false;
    QTimer::singleShot( 0, [&]() {
        QDialog *dialog = qobject_cast<QDialog *>( QApplication::activeModalWidget() );
        if( dialog ) { controlsOpened = dialog->windowTitle() == QLatin1String( "Flubber Controls" ); dialog->reject(); }
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
        for( auto other : { FlubberPanel::FlubberKind, FlubberPanel::GridKind, FlubberPanel::FaceKind } )
            manualPanel.setVisible( other, false );
        manualPanel.setPrimary( kind );
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
    QTest::keyClick( &textEntry, Qt::Key_J, Qt::AltModifier );
    if( keySettings.value( "Flubber/lastSettingsPath" ).toString() != videoSnapshot ) return 78;
#endif
    if( settings.value( "Flubber/panelHeight" ).toInt() != 200 ) return 15;
    panel.setLayout( FlubberPanel::GridKind, { true, 45, 18, 50 } );
    panel.setLayout( FlubberPanel::FaceKind, { true, 45, 82, 50 } );
    panel.setPrimary( FlubberPanel::GridKind );
    if( panel.primary() != FlubberPanel::GridKind || !panel.visible( FlubberPanel::FaceKind ) ||
        !panel.visible( FlubberPanel::FlubberKind ) ) return 16;
    QString faceError;
    if( !panel.setFaceId( "photo-synthetic-01", &faceError ) ) return 17;
    panel.resize( 900, panel.height() ); app.processEvents();
    const QImage three = panel.grab().toImage();
    three.save( QFileInfo( QString::fromLocal8Bit( argv[2] ) ).dir().filePath( "three-layers.png" ) );
    if( three.pixelColor( 0, 0 ) != QColor( Qt::black ) ||
        three.pixelColor( 162, 100 ) == QColor( Qt::black ) ||
        three.pixelColor( 738, 100 ) == QColor( Qt::black ) ) return 18;
    const QString fullSettings = temp.filePath( "full-settings.json" );
    if( !panel.saveSettingsFile( fullSettings, error ) ) return 19;
    FlubberPanel restored( &settings );
    if( !restored.loadSettingsFile( fullSettings, error ) ||
        restored.primary() != FlubberPanel::GridKind ||
        restored.faceId() != QLatin1String( "photo-synthetic-01" ) ||
        !restored.visible( FlubberPanel::FaceKind ) ) return 20;
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
