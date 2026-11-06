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
    QLineEdit textEntry( &shortcutHost );
    shortcutLayout.addWidget( &textEntry ); textEntry.show(); textEntry.setFocus(); app.processEvents();
    const auto primaryBeforeTyping = shortcutPanel.primary();
    QTest::keyClick( &textEntry, Qt::Key_G, Qt::AltModifier );
    if( shortcutPanel.primary() != primaryBeforeTyping ) return 55;
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
    return 0;
}
