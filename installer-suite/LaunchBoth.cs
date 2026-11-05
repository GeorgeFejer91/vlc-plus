// GPL-2.0-or-later. This launcher owns no recording or remote-control state.
using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;
using Microsoft.Win32;

internal static class LaunchBoth {
    private const string VlcKey = @"Software\Microsoft\Windows\CurrentVersion\Uninstall\{F836C1D9-43B0-4A71-95D7-2854030B68AE}_is1";
    private const string RecorderKey = @"Software\Microsoft\Windows\CurrentVersion\Uninstall\Remote LSL Recorder";

    internal static string ResolveExecutable(string location, string fileName) {
        if (String.IsNullOrWhiteSpace(location)) return null;
        try {
            string folder = location.Trim().Trim('"');
            // Both installers are local/current-user. Never execute a relative
            // path or reach a network share through a damaged installation key.
            if (!Path.IsPathRooted(folder) || Path.GetPathRoot(folder).Length != 3) return null;
            string path = Path.GetFullPath(Path.Combine(folder, fileName));
            return File.Exists(path) ? path : null;
        } catch (ArgumentException) { return null; }
        catch (NotSupportedException) { return null; }
        catch (PathTooLongException) { return null; }
    }

    private static string RegisteredExecutable(string keyName, string fileName) {
        foreach (RegistryView view in new[] { RegistryView.Registry64, RegistryView.Registry32 }) {
            using (RegistryKey root = RegistryKey.OpenBaseKey(RegistryHive.CurrentUser, view))
            using (RegistryKey key = root.OpenSubKey(keyName)) {
                string path = ResolveExecutable(key == null ? null : key.GetValue("InstallLocation") as string, fileName);
                if (path != null) return path;
            }
        }
        throw new FileNotFoundException("Install both apps for this account using the joint installer. Missing: " + fileName);
    }

    internal static void OpenPair(string recorder, string player, Func<string, bool> running, Action<string> start) {
        if (!File.Exists(recorder) || !File.Exists(player))
            throw new FileNotFoundException("Both installed apps must be available before launch.");
        if (!running(recorder)) start(recorder);
        if (!running(player)) start(player);
    }

    private static bool Running(string path) {
        int session;
        using (Process current = Process.GetCurrentProcess()) session = current.SessionId;
        bool found = false;
        foreach (Process process in Process.GetProcessesByName(Path.GetFileNameWithoutExtension(path))) {
            using (process) {
                try { found |= process.SessionId == session && String.Equals(Path.GetFullPath(process.MainModule.FileName), path, StringComparison.OrdinalIgnoreCase); }
                catch (System.ComponentModel.Win32Exception) { }
                catch (InvalidOperationException) { }
            }
        }
        return found;
    }

    private static bool AppOpenInThisSession() {
        int session;
        using (Process current = Process.GetCurrentProcess()) session = current.SessionId;
        bool found = false;
        foreach (string name in new[] { "remote-lsl-recorder", "vlc" })
        foreach (Process process in Process.GetProcessesByName(name)) {
            using (process) {
                try { found |= process.SessionId == session; }
                catch (InvalidOperationException) { }
            }
        }
        return found;
    }

    private static void Start(string path) {
        using (Process process = Process.Start(new ProcessStartInfo(path) {
            UseShellExecute = false,
            WorkingDirectory = Path.GetDirectoryName(path)
        })) {
            if (process == null) throw new InvalidOperationException("Could not start " + Path.GetFileName(path));
        }
    }

    [STAThread] private static int Main(string[] args) {
        bool quiet = args.Length == 1 && (args[0] == "--check" || args[0] == "--install-ready");
        try {
            if (args.Length == 1 && args[0] == "--install-ready") return AppOpenInThisSession() ? 20 : 0;
            if (args.Length > 0 && !quiet) return 2;
            string recorder = RegisteredExecutable(RecorderKey, "remote-lsl-recorder.exe");
            string player = RegisteredExecutable(VlcKey, "vlc.exe");
            if (!quiet) OpenPair(recorder, player, Running, Start);
            return 0;
        } catch (Exception error) {
            if (!quiet) MessageBox.Show(error.Message, "VLC+ and LSL Recorder", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 2;
        }
    }
}
