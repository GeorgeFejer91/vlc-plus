#include "flubber_video_keys.hpp"
#include <QCoreApplication>
#include <atomic>
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
#ifdef Q_OS_WIN
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
