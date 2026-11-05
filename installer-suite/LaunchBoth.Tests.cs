using System;
using System.Collections.Generic;
using System.IO;

internal static class LaunchBothTests {
    private static void Check(bool value, string message) {
        if (!value) throw new Exception(message);
    }
    private static int Main() {
        try { return Run(); }
        catch (Exception error) { Console.Error.WriteLine(error.Message); return 1; }
    }
    private static int Run() {
        string folder = Path.Combine(Path.GetTempPath(), "vlc suite \u00e9 test " + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(folder);
        string recorder = Path.Combine(folder, "remote-lsl-recorder.exe");
        string player = Path.Combine(folder, "vlc.exe");
        File.WriteAllText(recorder, "test fixture, never executed");
        File.WriteAllText(player, "test fixture, never executed");
        try {
            Check(LaunchBoth.ResolveExecutable("\"" + folder + "\"", "vlc.exe") == player, "Quoted installation path must resolve");
            Check(LaunchBoth.ResolveExecutable("relative", "vlc.exe") == null, "Relative roots must be rejected");
            Check(LaunchBoth.ResolveExecutable(@"C:relative", "vlc.exe") == null, "Drive-relative roots must be rejected");
            Check(LaunchBoth.ResolveExecutable(@"\\server\share", "vlc.exe") == null, "Network roots must be rejected");
            Check(LaunchBoth.ResolveExecutable(null, "vlc.exe") == null, "Missing installation must be rejected");
            Check(LaunchBoth.ResolveExecutable(folder, "absent.exe") == null, "Missing executable must be rejected");
            List<string> starts = new List<string>();
            LaunchBoth.OpenPair(recorder, player, path => false, path => starts.Add(path));
            Check(starts.Count == 2 && starts[0] == recorder && starts[1] == player, "Both apps must launch in one invocation");
            starts.Clear();
            LaunchBoth.OpenPair(recorder, player, path => true, path => starts.Add(path));
            Check(starts.Count == 0, "Existing app instances must be reused");
            LaunchBoth.OpenPair(recorder, player, path => path == recorder, path => starts.Add(path));
            Check(starts.Count == 1 && starts[0] == player, "Only missing app may start");
            starts.Clear();
            try {
                LaunchBoth.OpenPair(recorder, Path.Combine(folder, "missing.exe"), path => false, path => starts.Add(path));
                throw new Exception("Expected missing-component failure");
            } catch (FileNotFoundException) { }
            Check(starts.Count == 0, "Validate both apps before starting either");
            Console.WriteLine("PASS: installed paths, both-app launch, duplicate prevention, missing-component fence");
            return 0;
        } finally {
            File.Delete(recorder); File.Delete(player); Directory.Delete(folder);
        }
    }
}
