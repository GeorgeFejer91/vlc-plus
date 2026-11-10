/* Own-video-thread keyboard routing. GPL-2.0-or-later. */
#ifndef VLC_FLUBBER_VIDEO_KEYS_HPP
#define VLC_FLUBBER_VIDEO_KEYS_HPP

#include <QCoreApplication>
#include <QEvent>
#include <QWidget>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <cstdio>

struct FlubberNativeKeyEvent : QEvent
{
    static QEvent::Type eventType()
    {
        static const int type = QEvent::registerEventType();
        return static_cast<QEvent::Type>( type );
    }
    FlubberNativeKeyEvent( int value, Qt::KeyboardModifiers mods, bool repeated, int mode,
                           quint64 revision = 0, bool pressed = true )
        : QEvent( eventType() ), key( value ), modifiers( mods ), repeat( repeated ),
          arrowModeAfter( mode ), guiRevision( revision ), down( pressed ) {}
    int key;
    Qt::KeyboardModifiers modifiers;
    bool repeat;
    int arrowModeAfter;
    quint64 guiRevision;
    bool down;
};

enum FlubberArrow { FlubberLeft = 1, FlubberRight = 2, FlubberUp = 4, FlubberDown = 8 };

inline unsigned flubberArrow( int key )
{
    switch( key )
    {
        case Qt::Key_Left: return FlubberLeft;
        case Qt::Key_Right: return FlubberRight;
        case Qt::Key_Up: return FlubberUp;
        case Qt::Key_Down: return FlubberDown;
        default: return 0;
    }
}

inline double flubberHoldDistance( int tapPercent, int holdMs, qint64 elapsedNs )
{
    const double tap = tapPercent / 100.0;
    return holdMs ? ( 2.0 - tap ) * ( elapsedNs / 1e6 ) / holdMs : tap;
}

inline int flubberSettingsTarget( int key, Qt::KeyboardModifiers mods )
{
    if( ( mods & ~Qt::KeypadModifier ) != ( Qt::ControlModifier | Qt::ShiftModifier ) ) return -1;
    switch( key )
    {
        case Qt::Key_1: case Qt::Key_Exclam: return 0;
        case Qt::Key_2: case Qt::Key_At: case Qt::Key_QuoteDbl: return 1;
        case Qt::Key_3: case Qt::Key_NumberSign: case Qt::Key_section: return 2;
        default: return -1;
    }
}

inline bool flubberReservedKey( int key, Qt::KeyboardModifiers mods, bool arrows )
{
    mods &= ~Qt::KeypadModifier;
    if( flubberArrow( key ) ) return arrows;
    if( mods == Qt::AltModifier && key == Qt::Key_F ) return true;
    if( mods == Qt::AltModifier && ( key == Qt::Key_W || key == Qt::Key_A || key == Qt::Key_S ||
        key == Qt::Key_D || key == Qt::Key_J || key == Qt::Key_Equal || key == Qt::Key_Plus || key == Qt::Key_Minus ) ) return true;
    if( mods == ( Qt::AltModifier | Qt::ShiftModifier ) && key == Qt::Key_Plus ) return true;
    if( ( mods == Qt::AltModifier || mods == ( Qt::AltModifier | Qt::ShiftModifier ) ) &&
        ( key == Qt::Key_G || key == Qt::Key_H ) ) return true;
    return flubberSettingsTarget( key, mods ) >= 0 ||
        ( mods == ( Qt::ControlModifier | Qt::ShiftModifier ) && key == Qt::Key_S );
}

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

inline int flubberQtKey( WPARAM key )
{
    if( key >= '1' && key <= '3' ) return Qt::Key_1 + int( key - '1' );
    if( key >= VK_NUMPAD1 && key <= VK_NUMPAD3 ) return Qt::Key_1 + int( key - VK_NUMPAD1 );
    if( key >= 'A' && key <= 'Z' ) return Qt::Key_A + int( key - 'A' );
    switch( key )
    {
        case VK_LEFT: return Qt::Key_Left; case VK_RIGHT: return Qt::Key_Right;
        case VK_UP: return Qt::Key_Up; case VK_DOWN: return Qt::Key_Down;
        case VK_OEM_PLUS: return Qt::Key_Equal; case VK_ADD: return Qt::Key_Plus;
        case VK_OEM_MINUS: case VK_SUBTRACT: return Qt::Key_Minus;
        default: return 0;
    }
}

class FlubberVideoKeys
{
    struct Entry { FlubberVideoKeys *owner; QObject *target; HWND surface; bool arrows, altChord; };
    struct Hook { HHOOK handle; HWND window; };
    struct Discovery { DWORD gui; QMap<DWORD, HWND> windows; };
    static QMutex &mutex() { static QMutex value; return value; }
    static QMap<DWORD, Entry> &entries() { static QMap<DWORD, Entry> value; return value; }
    static bool keepCallbackCodeAlive()
    {
        // Unhook can return with a callback still in flight. Keep this module's
        // code mapped until process exit; targets and hooks still detach normally.
        HMODULE module = NULL;
        return GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>( reinterpret_cast<quintptr>( &receive ) ), &module ) != 0;
    }
    static BOOL CALLBACK collect( HWND window, LPARAM parameter )
    {
        Discovery &found = *reinterpret_cast<Discovery *>( parameter );
        DWORD process = 0;
        const DWORD thread = GetWindowThreadProcessId( window, &process );
        if( thread && thread != found.gui && process == GetCurrentProcessId() && !found.windows.contains( thread ) )
            found.windows.insert( thread, window );
        return TRUE;
    }
    static LRESULT CALLBACK receive( int code, WPARAM removed, LPARAM parameter )
    {
        if( code >= 0 && removed == PM_REMOVE )
        {
            MSG &message = *reinterpret_cast<MSG *>( parameter );
            const bool down = message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN;
            const bool up = message.message == WM_KEYUP || message.message == WM_SYSKEYUP;
            if( down || up )
            {
                QMutexLocker lock( &mutex() );
                auto found = entries().find( GetCurrentThreadId() );
                if( found != entries().end() && found->target && message.hwnd &&
                    ( message.hwnd == found->surface || IsChild( found->surface, message.hwnd ) ) )
                {
                    Entry &entry = found.value();
                    GUITHREADINFO state = {}; state.cbSize = sizeof( state );
                    const bool menu = GetGUIThreadInfo( GetCurrentThreadId(), &state ) &&
                        ( state.flags & ( GUI_INMENUMODE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE ) );
                    int key = flubberQtKey( message.wParam );
                    Qt::KeyboardModifiers mods = Qt::NoModifier;
                    if( GetKeyState( VK_CONTROL ) & 0x8000 ) mods |= Qt::ControlModifier;
                    if( GetKeyState( VK_SHIFT ) & 0x8000 ) mods |= Qt::ShiftModifier;
                    if( ( GetKeyState( VK_MENU ) & 0x8000 ) || ( message.lParam & ( LPARAM( 1 ) << 29 ) ) ) mods |= Qt::AltModifier;
                    if( key == Qt::Key_Equal && ( mods & Qt::ShiftModifier ) ) key = Qt::Key_Plus;
                    const bool foreground = GetAncestor( GetForegroundWindow(), GA_ROOT ) == GetAncestor( message.hwnd, GA_ROOT );
                    const bool arrow = flubberArrow( key ) != 0;
                    bool consume = !menu && arrow && entry.arrows;
                    if( down && message.wParam == VK_MENU && !( message.lParam & ( LPARAM( 1 ) << 30 ) ) ) entry.altChord = false;
                    if( up && message.wParam == VK_MENU && entry.altChord ) { consume = true; entry.altChord = false; }
                    if( !menu && foreground && flubberReservedKey( key, mods, entry.arrows ) )
                    {
                        consume = true;
                        if( down || arrow )
                        {
                            const bool repeat = down && ( message.lParam & ( LPARAM( 1 ) << 30 ) );
                            int mode = -1;
                            if( key == Qt::Key_F && mods == Qt::AltModifier ) mode = repeat ? int( entry.arrows ) : int( !entry.arrows );
                            if( mode >= 0 )
                            {
                                entry.owner->arrows_ = mode != 0;
                                for( auto it = entries().begin(); it != entries().end(); ++it )
                                    if( it->owner == entry.owner ) it->arrows = mode != 0;
                            }
                            if( mods & Qt::AltModifier ) entry.altChord = true;
                            // Only queue to Qt; never block the video thread on GUI work.
                            QCoreApplication::postEvent( entry.target,
                                new FlubberNativeKeyEvent( key, mods, repeat, mode, entry.owner->guiRevision_, down ),
                                Qt::HighEventPriority );
                        }
                    }
                    if( down && entry.owner->trace_ && ( consume || key == Qt::Key_F || key == Qt::Key_G || key == Qt::Key_H || arrow ) )
                        std::fprintf( stderr, "Flubber native key: thread=%lu hwnd=%p key=%d modifiers=%x arrows=%d foreground=%d claimed=%d\n",
                            static_cast<unsigned long>( GetCurrentThreadId() ), static_cast<void *>( message.hwnd ), key,
                            int( mods ), int( entry.arrows ), int( foreground ), int( consume ) );
                    if( consume ) { message.message = WM_NULL; message.wParam = 0; message.lParam = 0; }
                }
            }
        }
        return CallNextHookEx( NULL, code, removed, parameter );
    }
    void detach( DWORD thread )
    {
        {
            QMutexLocker lock( &mutex() );
            auto found = entries().find( thread );
            if( found != entries().end() && found->owner == this ) entries().erase( found );
        }
        const Hook hook = hooks_.take( thread );
        if( hook.handle ) UnhookWindowsHookEx( hook.handle );
    }
public:
    FlubberVideoKeys( QObject *target, bool arrows ) : target_( target ), gui_( GetCurrentThreadId() ), arrows_( arrows ),
        codeAlive_( keepCallbackCodeAlive() ), trace_( qEnvironmentVariableIsSet( "VLC_FLUBBER_TRACE_KEYS" ) ), guiRevision_( 0 )
    { FlubberNativeKeyEvent::eventType(); }
    ~FlubberVideoKeys() { for( DWORD thread : hooks_.keys() ) detach( thread ); }
    void setArrowMode( bool arrows )
    {
        QMutexLocker lock( &mutex() ); arrows_ = arrows; ++guiRevision_;
        for( auto it = entries().begin(); it != entries().end(); ++it ) if( it->owner == this ) it->arrows = arrows;
    }
    quint64 guiRevision() const { QMutexLocker lock( &mutex() ); return guiRevision_; }
    QString error() const { return error_; }
    bool refresh( WId surfaceId )
    {
        error_.clear();
        if( !codeAlive_ || !target_ ) { error_ = QStringLiteral( "Windows video keyboard routing is unavailable." ); return false; }
        const HWND surface = reinterpret_cast<HWND>( surfaceId );
        Discovery found = { gui_, {} };
        if( surface && IsWindow( surface ) ) { collect( surface, reinterpret_cast<LPARAM>( &found ) ); EnumChildWindows( surface, collect, reinterpret_cast<LPARAM>( &found ) ); }
        for( DWORD thread : hooks_.keys() )
            if( !found.windows.contains( thread ) || GetWindowThreadProcessId( hooks_[thread].window, NULL ) != thread ) detach( thread );
        for( auto it = found.windows.begin(); it != found.windows.end(); ++it )
        {
            if( !hooks_.contains( it.key() ) )
            {
                QMutexLocker lock( &mutex() );
                if( entries().contains( it.key() ) ) { error_ = QStringLiteral( "The video keyboard thread is already in use." ); continue; }
                // Thread belongs to this process; no global hook or DLL injection.
                // Publish under the same lock as installation: an immediate
                // callback cannot see a missing recipient and leak a shortcut.
                HHOOK hook = SetWindowsHookExW( WH_GETMESSAGE, receive, NULL, it.key() );
                if( !hook ) { error_ = QStringLiteral( "Windows video keyboard hook failed (%1)." ).arg( qulonglong( GetLastError() ) ); continue; }
                hooks_.insert( it.key(), Hook{ hook, it.value() } );
                entries().insert( it.key(), Entry{ this, target_, surface, arrows_, false } );
            }
            else { QMutexLocker lock( &mutex() ); entries()[it.key()].surface = surface; }
        }
        return error_.isEmpty();
    }
private:
    QObject *target_;
    DWORD gui_;
    bool arrows_; // protected by mutex(), including eager native mode changes
    const bool codeAlive_, trace_;
    quint64 guiRevision_; // GUI intent supersedes older queued native commands
    QString error_; // GUI thread only
    QMap<DWORD, Hook> hooks_; // GUI thread only
};
#else
class FlubberVideoKeys
{
public:
    FlubberVideoKeys( QObject *, bool ) : revision_( 0 ) {}
    void setArrowMode( bool ) { ++revision_; }
    quint64 guiRevision() const { return revision_; }
    QString error() const { return QString(); }
    bool refresh( WId ) { return true; }
private:
    quint64 revision_;
};
#endif
#endif
