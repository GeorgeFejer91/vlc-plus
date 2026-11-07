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


#ifdef Q_OS_WIN
static int checkTranslatedDedicatedKeys( const QString &directory )
{
    QSettings settings( directory + "/translated.ini", QSettings::IniFormat );
    FlubberPanel panel( &settings );
    QJsonObject baseline = panel.settingsJson(); baseline.insert( "stepPercent", 37 ); baseline.insert( "primary", "face" );
    const QStringList names{ "oscillationFrequency", "edgeSmoothness", "projectionAmplitude", "pulseSynchrony", "waveSizeVariation", "saturation" };
    for( const QString &name : names ) replaceSetting( baseline, { "appearance", "mappings", name, "drivenBy" }, "x-axis" );
    replaceSetting( baseline, { "appearance", "presentation", "halo", "steepness" }, 1 );
    QFile fixture( directory + "/translated.json" );
    if( !fixture.open( QIODevice::WriteOnly ) ) return 125;
    fixture.write( QJsonDocument( baseline ).toJson() ); fixture.close();
    const auto reset = [&]() { QString error; return panel.loadSettingsFile( fixture.fileName(), error ); };
    const Qt::KeyboardModifiers chord = Qt::ControlModifier | Qt::AltModifier;
    struct Probe { int symbol, virtualKey, mapping; bool shift, keypad; };
    const Probe probes[] = {
        { '[', 0x38, 4, false, false }, { ']', 0x39, 5, false, false },
        { '$', 0x34, 0, true, false }, { '%', 0x35, 1, true, false }, { '&', 0x36, 2, true, false },
        { '/', 0x37, 3, true, false }, { '(', 0x38, 4, true, false }, { ')', 0x39, 5, true, false },
        { Qt::Key_4, 0x64, 0, false, true }, { Qt::Key_5, 0x65, 1, false, true },
        { Qt::Key_6, 0x66, 2, false, true }, { Qt::Key_7, 0x67, 3, false, true },
        { Qt::Key_8, 0x68, 4, false, true }, { Qt::Key_9, 0x69, 5, false, true }
    };
    for( const Probe &probe : probes )
    {
        if( !reset() ) return 126;
        Qt::KeyboardModifiers modifiers = probe.shift ? chord | Qt::ShiftModifier : chord;
        if( probe.keypad ) modifiers |= Qt::KeypadModifier;
        QKeyEvent claim( QEvent::ShortcutOverride, probe.symbol, modifiers, 0, probe.virtualKey, 0 );
        const QJsonObject before = panel.settingsJson();
        if( !panel.handleInputKey( &claim ) || !claim.isAccepted() || panel.settingsJson() != before ) return 127;
        QKeyEvent press( QEvent::KeyPress, probe.symbol, modifiers, 0, probe.virtualKey, 0 );
        if( !panel.handleInputKey( &press ) || panel.settingsJson().value( "stepPercent" ).toInt() != 37 ||
            settingAt( panel.settingsJson(), { "appearance", "mappings", names[probe.mapping], "drivenBy" } ).toString() !=
                QLatin1String( probe.shift ? "radius" : "y-axis" ) || panel.primary() != FlubberPanel::FlubberKind ) return 128;
    }
    if( !reset() ) return 129;
    QKeyEvent euroClaim( QEvent::ShortcutOverride, 0x20ac, chord, 0, 0x45, 0 );
    QKeyEvent euro( QEvent::KeyPress, 0x20ac, chord, 0, 0x45, 0 );
    const QJsonObject beforeEuro = panel.settingsJson();
    if( !panel.handleInputKey( &euroClaim ) || panel.settingsJson() != beforeEuro || !panel.handleInputKey( &euro ) ||
        qAbs( settingAt( panel.settingsJson(), { "appearance", "presentation", "halo", "steepness" } ).toDouble() - 1.1 ) > 1e-8 ||
        panel.settingsJson().value( "stepPercent" ).toInt() != 37 ) return 130;
    if( !reset() ) return 131;
    QKeyEvent atClaim( QEvent::ShortcutOverride, '@', chord, 0, 0x51, 0 );
    QKeyEvent at( QEvent::KeyPress, '@', chord, 0, 0x51, 0 );
    const QJsonObject beforeAt = panel.settingsJson();
    if( !panel.handleInputKey( &atClaim ) || panel.settingsJson() != beforeAt ) return 132;
    bool opened = false;
    QTimer::singleShot( 0, [&]() {
        QColorDialog *dialog = qobject_cast<QColorDialog *>( QApplication::activeModalWidget() );
        if( dialog ) { opened = true; dialog->setCurrentColor( QColor( "#654321" ) ); dialog->accept(); }
    } );
    if( !panel.handleInputKey( &at ) || !opened ||
        settingAt( panel.settingsJson(), { "appearance", "visual", "colors", "halo" } ).toString() != QLatin1String( "#654321" ) ||
        panel.settingsJson().value( "stepPercent" ).toInt() != 37 || panel.primary() != FlubberPanel::FlubberKind ) return 133;
    struct StepProbe { int symbol, virtualKey, delta; };
    const StepProbe brackets[] = { { 0x00b4, 0xdd, 1 }, { '\\', 0xdb, -1 } };
    for( const StepProbe &probe : brackets )
    {
        if( !reset() ) return 150;
        const QJsonObject before = panel.settingsJson();
        QKeyEvent claim( QEvent::ShortcutOverride, probe.symbol, chord, 0, probe.virtualKey, 0 );
        claim.ignore();
        if( !panel.handleInputKey( &claim ) || !claim.isAccepted() || panel.settingsJson() != before ) return 151;
        QKeyEvent press( QEvent::KeyPress, probe.symbol, chord, 0, probe.virtualKey, 0 );
        QJsonObject expected = before; expected.insert( "stepPercent", 37 + probe.delta );
        if( !panel.handleInputKey( &press ) || panel.settingsJson() != expected ) return 152;
        QKeyEvent repeat( QEvent::KeyPress, probe.symbol, chord, 0, probe.virtualKey, 0, QString(), true );
        if( !panel.handleInputKey( &repeat ) || panel.settingsJson() != expected ) return 153;
        QKeyEvent other( QEvent::KeyPress, probe.symbol, Qt::ControlModifier, 0, probe.virtualKey, 0 );
        if( panel.handleInputKey( &other ) || panel.settingsJson() != expected ) return 154;
    }
    return 0;
}
#endif


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
    const Qt::KeyboardModifiers chord = Qt::ControlModifier | Qt::AltModifier;
    for( QWidget *editor : editors )
    {
        editor->setFocus(); QApplication::processEvents();
        QWidget *focus = QApplication::focusWidget();
        if( !focus || ( focus != editor && !editor->isAncestorOf( focus ) ) ) return 134;
        const QJsonObject before = panel.settingsJson();
        QKeyEvent key( QEvent::KeyPress, Qt::Key_F1, chord );
        QApplication::sendEvent( editor, &key );
        if( panel.settingsJson() != before ) return 135;
        QKeyEvent claim( QEvent::ShortcutOverride, Qt::Key_F1, chord );
        if( panel.handleInputKey( &claim ) || panel.settingsJson() != before ) return 136;
    }
    if( host.bubbled == 0 ) return 137;
    combo.setEditable( false ); combo.setFocus(); QApplication::processEvents();
    const QJsonObject beforeCombo = panel.settingsJson();
    QKeyEvent setting( QEvent::KeyPress, Qt::Key_T, chord );
    if( !panel.handleInputKey( &setting ) || panel.settingsJson() == beforeCombo ) return 138;
    line.setFocus(); QApplication::processEvents();
    const QJsonObject beforeNative = panel.settingsJson();
    FlubberNativeKeyEvent queued( Qt::Key_F, Qt::AltModifier, false, 1 );
    QCoreApplication::sendEvent( &panel, &queued );
    if( panel.settingsJson() != beforeNative || panel.arrowMode() ) return 139;
    return 0;
}

static int checkDedicatedSettings( const QString &directory )
{
    QSettings settings( directory + "/dedicated.ini", QSettings::IniFormat );
    FlubberPanel panel( &settings );
    if( panel.settingsJson().value( "schema" ).toString() != QLatin1String( "vlc-feedback-settings/v3" ) ||
        panel.settingsJson().value( "stepPercent" ).toInt() != 10 ) return 101;
    const Qt::KeyboardModifiers chord = Qt::ControlModifier | Qt::AltModifier;
    {
        const QJsonObject before = panel.settingsJson();
        QDialog modal( &panel ); modal.setModal( true ); modal.show(); QApplication::processEvents();
        FlubberNativeKeyEvent queued( Qt::Key_T, chord, false, -1 );
        QCoreApplication::sendEvent( &panel, &queued ); modal.reject();
        if( panel.settingsJson() != before ) return 121;
    }
    const auto press = [&]( int key, Qt::KeyboardModifiers modifiers ) {
        QKeyEvent event( QEvent::KeyPress, key, modifiers ); return panel.handleInputKey( &event );
    };
    const auto load = [&]( const QJsonObject &root ) {
        QFile file( directory + "/dedicated.json" );
        if( !file.open( QIODevice::WriteOnly ) ) return false;
        file.write( QJsonDocument( root ).toJson() ); file.close();
        QString error; return panel.loadSettingsFile( file.fileName(), error );
    };
    const auto exclusive = [&]( FlubberPanel::FeedbackKind kind ) {
        for( int i = 0; i < 3; ++i ) if( panel.visible( static_cast<FlubberPanel::FeedbackKind>( i ) ) != ( i == kind ) ) return false;
        return panel.primary() == kind;
    };
    const auto unchangedProbes = [&]( int key, Qt::KeyboardModifiers modifiers ) {
        const QJsonObject before = panel.settingsJson();
        QKeyEvent claim( QEvent::ShortcutOverride, key, modifiers );
        QKeyEvent repeat( QEvent::KeyPress, key, modifiers, QString(), true );
        return panel.handleInputKey( &claim ) && claim.isAccepted() && panel.handleInputKey( &repeat ) && panel.settingsJson() == before;
    };
    panel.setAffect( .25, -.5 );
    QJsonObject baseline = panel.settingsJson(); baseline.insert( "primary", "face" );
    struct Numeric { int key; const char *path; double start, step, low, high; bool grid; };
    const Numeric numbers[] = {
        { Qt::Key_T, "appearance/visual/transparency", .5, .05, 0, 1, false },
        { Qt::Key_B, "appearance/visual/flubber/outlineThickness", 2, .25, 0, 20, false },
        { Qt::Key_W, "appearance/presentation/halo/widthPercent", 15, 5, 0, 10000, false },
        { Qt::Key_E, "appearance/presentation/halo/steepness", 1, .1, .1, 10, false },
        { Qt::Key_X, "grid/columns", 21, 2, 3, 2001, true },
        { Qt::Key_Y, "grid/rows", 21, 2, 3, 2001, true },
        { Qt::Key_J, "grid/lineThickness", 1, .25, 0, 20, true },
        { Qt::Key_P, "grid/outlineThickness", 2, .25, 0, 20, true },
        { Qt::Key_Z, "grid/cursorSize", 14, 1, 0, 100, true }
    };
    for( const Numeric &control : numbers )
    {
        const QStringList path = QString::fromLatin1( control.path ).split( '/' );
        QJsonObject initial = baseline; replaceSetting( initial, path, control.start );
        if( !load( initial ) || !unchangedProbes( control.key, chord ) || !press( control.key, chord ) ||
            qAbs( settingAt( panel.settingsJson(), path ).toDouble() - control.start - control.step ) > 1e-8 ||
            !exclusive( control.grid ? FlubberPanel::GridKind : FlubberPanel::FlubberKind ) ) return 102;
        if( !press( control.key, chord | Qt::ShiftModifier ) ||
            qAbs( settingAt( panel.settingsJson(), path ).toDouble() - control.start ) > 1e-8 ) return 103;
        for( bool upper : { false, true } )
        {
            QJsonObject limit = baseline; replaceSetting( limit, path, upper ? control.high : control.low );
            if( !load( limit ) || !press( control.key, upper ? chord : chord | Qt::ShiftModifier ) ||
                settingAt( panel.settingsJson(), path ).toDouble() != ( upper ? control.high : control.low ) ) return 104;
        }
    }
    struct Toggle { int key; const char *path; bool grid; };
    const Toggle toggles[] = {
        { Qt::Key_O, "appearance/visual/flubber/showOutline", false },
        { Qt::Key_H, "appearance/visual/flubber/showHalo", false },
        { Qt::Key_G, "appearance/presentation/halo/gradient", false },
        { Qt::Key_V, "grid/showOutline", true }
    };
    for( const Toggle &control : toggles )
    {
        const QStringList path = QString::fromLatin1( control.path ).split( '/' );
        QJsonObject initial = baseline; replaceSetting( initial, path, false );
        if( !load( initial ) || !unchangedProbes( control.key, chord ) || !press( control.key, chord ) ||
            !settingAt( panel.settingsJson(), path ).toBool() ||
            !exclusive( control.grid ? FlubberPanel::GridKind : FlubberPanel::FlubberKind ) ||
            !press( control.key, chord ) || settingAt( panel.settingsJson(), path ).toBool() ) return 105;
    }
    QJsonObject anchors = baseline;
    replaceSetting( anchors, { "appearance", "presentation", "colorAnchors" }, "axes" );
    if( !load( anchors ) || !press( Qt::Key_A, chord ) ||
        settingAt( panel.settingsJson(), { "appearance", "presentation", "colorAnchors" } ).toString() != QLatin1String( "corners" ) ||
        !press( Qt::Key_A, chord ) || !exclusive( FlubberPanel::FlubberKind ) ||
        settingAt( panel.settingsJson(), { "appearance", "presentation", "colorAnchors" } ).toString() != QLatin1String( "axes" ) ) return 106;
    const QStringList mappingNames{ "oscillationFrequency", "edgeSmoothness", "projectionAmplitude",
        "pulseSynchrony", "waveSizeVariation", "saturation" };
    for( int i = 0; i < 6; ++i )
    {
        const QStringList path{ "appearance", "mappings", mappingNames[i] };
        QJsonObject initial = baseline;
        replaceSetting( initial, path, QJsonObject{ { "min", i == 0 ? 1 : .2 }, { "max", i == 0 ? 3 : .8 },
            { "drivenBy", "x-axis" }, { "reverse", false } } );
        for( bool minimum : { false, true } )
        {
            const int key = Qt::Key_F1 + i + ( minimum ? 6 : 0 );
            QStringList bound = path; bound << ( minimum ? "min" : "max" );
            const double start = minimum ? ( i == 0 ? 1 : .2 ) : ( i == 0 ? 3 : .8 );
            const double step = i == 0 ? .1 : .05;
            if( !load( initial ) || !unchangedProbes( key, chord ) || !press( key, chord ) ||
                qAbs( settingAt( panel.settingsJson(), bound ).toDouble() - start - step ) > 1e-8 ||
                !press( key, chord | Qt::ShiftModifier ) ||
                qAbs( settingAt( panel.settingsJson(), bound ).toDouble() - start ) > 1e-8 ||
                !exclusive( FlubberPanel::FlubberKind ) ) return 107;
            QJsonObject closed = initial;
            QStringList lo = path; lo << "min"; QStringList hi = path; hi << "max";
            replaceSetting( closed, lo, .5 ); replaceSetting( closed, hi, .5 );
            if( !load( closed ) || !press( key, minimum ? chord : chord | Qt::ShiftModifier ) ||
                settingAt( panel.settingsJson(), bound ).toDouble() != .5 ) return 108;
            for( bool upper : { false, true } )
            {
                QJsonObject limit = initial;
                replaceSetting( limit, lo, upper ? ( i == 0 ? 10 : 1 ) : 0 );
                replaceSetting( limit, hi, upper ? ( i == 0 ? 10 : 1 ) : 0 );
                if( !load( limit ) || !press( key, upper ? chord : chord | Qt::ShiftModifier ) ||
                    settingAt( panel.settingsJson(), bound ).toDouble() != ( upper ? ( i == 0 ? 10 : 1 ) : 0 ) ) return 109;
            }
        }
        QStringList driver = path; driver << "drivenBy"; QStringList reverse = path; reverse << "reverse";
        if( !load( initial ) || !press( Qt::Key_4 + i, chord ) || settingAt( panel.settingsJson(), driver ).toString() != QLatin1String( "y-axis" ) ||
            !press( Qt::Key_4 + i, chord | Qt::ShiftModifier ) || settingAt( panel.settingsJson(), driver ).toString() != QLatin1String( "x-axis" ) ||
            !press( Qt::Key_4 + i, chord | Qt::ShiftModifier ) || settingAt( panel.settingsJson(), driver ).toString() != QLatin1String( "radius" ) ||
            !press( Qt::Key_4 + i, chord ) || settingAt( panel.settingsJson(), driver ).toString() != QLatin1String( "x-axis" ) ||
            !unchangedProbes( Qt::Key_F1 + i, Qt::ControlModifier | Qt::ShiftModifier ) ||
            !press( Qt::Key_F1 + i, Qt::ControlModifier | Qt::ShiftModifier ) || !settingAt( panel.settingsJson(), reverse ).toBool() ||
            !press( Qt::Key_F1 + i, Qt::ControlModifier | Qt::ShiftModifier ) || settingAt( panel.settingsJson(), reverse ).toBool() ||
            !exclusive( FlubberPanel::FlubberKind ) ) return 110;
    }
    const int colorKeys[] = { Qt::Key_U, Qt::Key_D, Qt::Key_L, Qt::Key_R, Qt::Key_I, Qt::Key_K, Qt::Key_Q, Qt::Key_C };
    const QStringList colorNames{ "up", "down", "left", "right", "idle", "outline", "halo", "cursor" };
    for( int i = 0; i < 8; ++i )
    {
        if( !load( baseline ) || !unchangedProbes( colorKeys[i], chord ) ) return 111;
        const QJsonObject before = panel.settingsJson(); bool opened = false;
        QTimer::singleShot( 0, [&]() {
            QColorDialog *dialog = qobject_cast<QColorDialog *>( QApplication::activeModalWidget() );
            if( dialog ) { opened = true; dialog->reject(); }
        } );
        if( !press( colorKeys[i], chord ) || !opened || panel.settingsJson() != before ) return 112;
        opened = false;
        QTimer::singleShot( 0, [&]() {
            QColorDialog *dialog = qobject_cast<QColorDialog *>( QApplication::activeModalWidget() );
            if( dialog ) { opened = true; dialog->setCurrentColor( QColor( "#123456" ) ); dialog->accept(); }
        } );
        if( !press( colorKeys[i], chord ) || !opened || !exclusive( FlubberPanel::FlubberKind ) ||
            settingAt( panel.settingsJson(), { "appearance", "visual", "colors", colorNames[i] } ).toString() != QLatin1String( "#123456" ) ) return 113;
    }
    if( panel.valence() != .25f || panel.arousal() != -.5f ) return 114;
    const QJsonObject beforeInvalid = panel.settingsJson();
    for( const QJsonValue &step : { QJsonValue( 0 ), QJsonValue( 101 ), QJsonValue( 10.5 ), QJsonValue( "20" ), QJsonValue() } )
    {
        QJsonObject invalid = beforeInvalid; invalid.insert( "stepPercent", step );
        if( load( invalid ) || panel.settingsJson() != beforeInvalid ) return 115;
    }
    QJsonObject missingStep = beforeInvalid; missingStep.remove( "stepPercent" );
    if( load( missingStep ) || panel.settingsJson() != beforeInvalid ) return 122;
    for( int step : { 1, 100 } )
    {
        QJsonObject endpoint = beforeInvalid; endpoint.insert( "stepPercent", step );
        if( !load( endpoint ) || !press( step == 1 ? Qt::Key_BracketLeft : Qt::Key_BracketRight, chord ) ||
            panel.settingsJson().value( "stepPercent" ).toInt() != step ) return 123;
    }
    QJsonObject desired = panel.settingsJson(); desired.insert( "stepPercent", 37 );
    if( !load( desired ) || settings.value( "Flubber/stepPercent" ).toInt() != 37 ) return 116;
    QString error;
    if( !panel.saveSettingsSnapshot( error ) ) return 117;
    press( Qt::Key_BracketRight, chord );
    FlubberPanel restored( &settings );
    if( restored.settingsJson() != desired ) return 118;
    QJsonObject legacy = desired; legacy.insert( "schema", "vlc-feedback-settings/v2" ); legacy.remove( "stepPercent" );
    if( !load( legacy ) || panel.settingsJson().value( "stepPercent" ).toInt() != 38 ) return 119;
    legacy.insert( "stepPercent", 37 );
    const QJsonObject beforeExtra = panel.settingsJson();
    if( load( legacy ) || panel.settingsJson() != beforeExtra ) return 120;
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
    const int editorShortcuts = checkEditorShortcutOwnership( temp.path() );
    if( editorShortcuts ) return editorShortcuts;
#ifdef Q_OS_WIN
    const int translatedKeys = checkTranslatedDedicatedKeys( temp.path() );
    if( translatedKeys ) return translatedKeys;
#endif
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
    QTest::keyClick( &textEntry, Qt::Key_T, Qt::ControlModifier | Qt::AltModifier );
    QTest::keyClick( &textEntry, Qt::Key_F1, Qt::ControlModifier | Qt::AltModifier );
    QTest::keyClick( &textEntry, Qt::Key_F1, Qt::ControlModifier | Qt::ShiftModifier );
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
    return checkDedicatedSettings( temp.path() );
}
