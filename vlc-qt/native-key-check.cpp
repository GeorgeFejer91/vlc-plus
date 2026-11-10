#include "flubber_video_keys.hpp"
#include <QCoreApplication>
#include <atomic>
#include <cmath>
#include <future>
#include <thread>
#include <cstdio>

int main( int argc, char **argv )
{
    QCoreApplication app( argc, argv );
    for( auto mods : { Qt::KeyboardModifiers(), Qt::KeyboardModifiers( Qt::ShiftModifier ),
        Qt::KeyboardModifiers( Qt::ControlModifier ), Qt::KeyboardModifiers( Qt::AltModifier ),
        Qt::KeyboardModifiers( Qt::ControlModifier | Qt::AltModifier ) } )
        if( !flubberReservedKey( Qt::Key_Left, mods, true ) || flubberReservedKey( Qt::Key_Left, mods, false ) ) return 1;
    if( !flubberReservedKey( Qt::Key_H, Qt::AltModifier | Qt::ShiftModifier, false ) ||
        flubberReservedKey( Qt::Key_H, Qt::NoModifier, true ) ||
        flubberReservedKey( Qt::Key_F, Qt::NoModifier, true ) ) return 2;
    for( bool arrows : { false, true } )
    {
        for( int key : { Qt::Key_W, Qt::Key_A, Qt::Key_S, Qt::Key_D, Qt::Key_J,
            Qt::Key_Equal, Qt::Key_Plus, Qt::Key_Minus } )
            if( !flubberReservedKey( key, Qt::AltModifier, arrows ) ) return 10;
        if( !flubberReservedKey( Qt::Key_Plus, Qt::AltModifier | Qt::ShiftModifier, arrows ) ) return 11;
    }
    const Qt::KeyboardModifiers settings = Qt::ControlModifier | Qt::ShiftModifier;
    const Qt::KeyboardModifiers retired = Qt::ControlModifier | Qt::AltModifier;
    for( bool arrows : { false, true } )
    {
        if( !flubberReservedKey( Qt::Key_S, settings, arrows ) ||
            flubberReservedKey( Qt::Key_S, Qt::ControlModifier, arrows ) ) return 12;
        for( int key : { Qt::Key_T, Qt::Key_F1, Qt::Key_Q, Qt::Key_1, Qt::Key_8, Qt::Key_PageUp,
                         Qt::Key_Equal, Qt::Key_BracketRight, Qt::Key_Period } )
            if( flubberReservedKey( key, retired, arrows ) ||
                flubberReservedKey( key, retired | Qt::ShiftModifier, arrows ) ) return 13;
        for( int key = Qt::Key_1; key <= Qt::Key_3; ++key )
            if( !flubberReservedKey( key, settings, arrows ) ||
                flubberReservedKey( key, Qt::ControlModifier, arrows ) ) return 14;
        if( flubberSettingsTarget( Qt::Key_QuoteDbl, settings ) != 1 ||
            flubberSettingsTarget( Qt::Key_section, settings ) != 2 ) return 14;
        for( int key : { Qt::Key_F1, Qt::Key_PageUp, Qt::Key_A, Qt::Key_M } )
            if( flubberReservedKey( key, settings, arrows ) ) return 14;
    }
    for( int duration : { 250, 600, 15000 } )
    {
        const double start = -1 + .37;
        const qint64 ns = qint64( duration ) * 1000000;
        const double middle = start + flubberHoldDistance( 37, duration, ns / 2 );
        const double end = middle + flubberHoldDistance( 37, duration, ns - ns / 2 );
        if( std::abs( middle - .185 ) > 1e-9 || std::abs( end - 1 ) > 1e-9 ) return 15;
    }
    if( flubberHoldDistance( 37, 0, 33000000 ) != .37 ) return 16;
#ifdef Q_OS_WIN
    if( flubberQtKey( '1' ) != Qt::Key_1 || flubberQtKey( VK_NUMPAD3 ) != Qt::Key_3 ||
        flubberQtKey( 'S' ) != Qt::Key_S || flubberQtKey( VK_ADD ) != Qt::Key_Plus ||
        flubberQtKey( VK_SUBTRACT ) != Qt::Key_Minus ) return 17;
    // Hidden, owned HWNDs: exercise the same separate GetMessage loop as VLC
    // without taking foreground focus or synthesizing input into another app.
    struct Windows { HWND surface, child, other; DWORD thread; };
    std::promise<Windows> ready;
    std::atomic<int> commands( 0 ), barriers( 0 );
    std::thread video( [&]() {
        HWND surface = CreateWindowExW( 0, L"STATIC", L"VLC+ hidden keyboard test", WS_OVERLAPPED,
            0, 0, 64, 64, NULL, NULL, GetModuleHandleW( NULL ), NULL );
        HWND child = CreateWindowExW( 0, L"STATIC", L"Owned video", WS_CHILD,
            0, 0, 32, 32, surface, NULL, GetModuleHandleW( NULL ), NULL );
        HWND other = CreateWindowExW( 0, L"STATIC", L"Other owned window", WS_OVERLAPPED,
            0, 0, 32, 32, NULL, NULL, GetModuleHandleW( NULL ), NULL );
        ready.set_value( Windows{ surface, child, other, GetCurrentThreadId() } );
        MSG message;
        while( GetMessageW( &message, NULL, 0, 0 ) > 0 )
        {
            if( message.message == WM_APP + 12 ) ++barriers;
            else if( message.message == WM_KEYDOWN ) ++commands;
            else { TranslateMessage( &message ); DispatchMessageW( &message ); }
        }
        if( child ) DestroyWindow( child );
        if( other ) DestroyWindow( other );
        if( surface ) DestroyWindow( surface );
    } );
    const Windows windows = ready.get_future().get();
    auto barrier = [&]( int expected ) {
        PostThreadMessageW( windows.thread, WM_APP + 12, 0, 0 );
        for( int i = 0; i < 500 && barriers < expected; ++i ) { QCoreApplication::processEvents(); Sleep( 2 ); }
        return barriers >= expected;
    };
    int result = 0;
    QObject recipient;
    if( !windows.surface || !windows.child || !windows.other ) result = 4;
    else
    {
        FlubberVideoKeys unavailable( NULL, true );
        if( unavailable.refresh( reinterpret_cast<WId>( windows.surface ) ) || unavailable.error().isEmpty() ) result = 9;
        {
            FlubberVideoKeys bridge( &recipient, true );
            bridge.refresh( reinterpret_cast<WId>( windows.surface ) );
            PostMessageW( windows.child, WM_KEYDOWN, VK_LEFT, 0 );
            if( !barrier( 1 ) || commands != 0 ) result = 5;
            PostMessageW( windows.other, WM_KEYDOWN, VK_LEFT, 0 );
            if( !barrier( 2 ) || commands != 1 ) result = 6;
            bridge.setArrowMode( false );
            PostMessageW( windows.child, WM_KEYDOWN, VK_LEFT, 0 );
            if( !barrier( 3 ) || commands != 2 ) result = 7;
            bridge.setArrowMode( true );
        }
        PostMessageW( windows.child, WM_KEYDOWN, VK_LEFT, 0 );
        if( !barrier( 4 ) || commands != 3 ) result = 8;
    }
    PostThreadMessageW( windows.thread, WM_QUIT, 0, 0 );
    video.join();
    if( result ) std::fprintf( stderr, "Native video-key routing failed: %d\n", result );
    else std::puts( "PASS: own video GetMessage routing, other-window isolation, VLC restoration and teardown." );
    return result;
#else
    return 0;
#endif
}
