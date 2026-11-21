using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Reflection;
using System.Windows.Forms;

public class SetupForm : Form
{
    const string PayloadResource = "PhaseDrift.dll.bin";

    public static string BundleDir
    {
        get { return Path.Combine (Environment.GetFolderPath (Environment.SpecialFolder.CommonProgramFiles),
                                   @"VST3\Phase Drift.vst3"); }
    }

    public static string DllPath
    {
        get { return Path.Combine (BundleDir, @"Contents\x86_64-win\Phase Drift.vst3"); }
    }

    // Headless install: extract the embedded plug-in binary. Returns an error string, or null on success.
    public static string DoInstall (Action<int> progress)
    {
        try
        {
            Directory.CreateDirectory (Path.GetDirectoryName (DllPath));

            var asm = Assembly.GetExecutingAssembly ();
            using (var stream = asm.GetManifestResourceStream (PayloadResource))
            {
                if (stream == null) return "payload missing from the installer binary";

                using (var fs = new FileStream (DllPath, FileMode.Create, FileAccess.Write, FileShare.None))
                {
                    var buffer = new byte[65536];
                    long written = 0;
                    long total = stream.Length;
                    int read;
                    while ((read = stream.Read (buffer, 0, buffer.Length)) > 0)
                    {
                        fs.Write (buffer, 0, read);
                        written += read;
                        if (progress != null) progress ((int) (100 * written / total));
                    }
                }
            }
            return null;
        }
        catch (Exception ex)
        {
            return ex.Message;
        }
    }

    public static string DoUninstall ()
    {
        try
        {
            if (Directory.Exists (BundleDir)) Directory.Delete (BundleDir, true);
            return null;
        }
        catch (Exception ex) { return ex.Message; }
    }

    TextBox pathBox;
    Label status;
    Button installBtn, uninstallBtn, closeBtn;
    ProgressBar bar;

    public SetupForm ()
    {
        Text = "Phase Drift  -  Setup";
        ClientSize = new Size (620, 330);
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Color.FromArgb (0x0e, 0x13, 0x19);
        ForeColor = Color.FromArgb (0xdd, 0xe7, 0xee);
        Font = new Font ("Segoe UI", 9.5f);

        var title = new Label ();
        title.Text = "PHASE DRIFT";
        title.Font = new Font ("Segoe UI", 22f, FontStyle.Bold);
        title.ForeColor = Color.FromArgb (0xe2, 0xed, 0xf3);
        title.SetBounds (28, 22, 400, 40);
        Controls.Add (title);

        var sub = new Label ();
        sub.Text = "PER-BAND PHASE + INDEPENDENT STEREO ECHO    -    VST3 64-bit installer";
        sub.Font = new Font ("Segoe UI", 9f);
        sub.ForeColor = Color.FromArgb (0x8b, 0x9a, 0xa9);
        sub.SetBounds (30, 62, 560, 20);
        Controls.Add (sub);

        var rule = new Panel ();
        rule.BackColor = Color.FromArgb (0x7e, 0xcb, 0xd6);
        rule.SetBounds (30, 90, 560, 2);
        Controls.Add (rule);

        var lbl = new Label ();
        lbl.Text = "INSTALL TO";
        lbl.ForeColor = Color.FromArgb (0x8b, 0x9a, 0xa9);
        lbl.SetBounds (30, 108, 200, 18);
        Controls.Add (lbl);

        pathBox = new TextBox ();
        pathBox.Text = DllPath;
        pathBox.ReadOnly = true;
        pathBox.BorderStyle = BorderStyle.FixedSingle;
        pathBox.BackColor = Color.FromArgb (0x11, 0x16, 0x1d);
        pathBox.ForeColor = Color.FromArgb (0xdd, 0xe7, 0xee);
        pathBox.SetBounds (30, 128, 560, 24);
        Controls.Add (pathBox);

        bar = new ProgressBar ();
        bar.SetBounds (30, 168, 560, 6);
        bar.Style = ProgressBarStyle.Continuous;
        Controls.Add (bar);

        status = new Label ();
        status.Text = "Ready.";
        status.ForeColor = Color.FromArgb (0x9a, 0xa9, 0xb6);
        status.SetBounds (30, 182, 560, 44);
        Controls.Add (status);

        installBtn = MakeButton ("INSTALL", 30, 240, 150, Color.FromArgb (0x7e, 0xcb, 0xd6));
        installBtn.Click += delegate { Install (); };
        Controls.Add (installBtn);

        uninstallBtn = MakeButton ("UNINSTALL", 192, 240, 150, Color.FromArgb (0xcf, 0x8f, 0xa6));
        uninstallBtn.Click += delegate { Uninstall (); };
        Controls.Add (uninstallBtn);

        closeBtn = MakeButton ("CLOSE", 440, 240, 150, Color.FromArgb (0x3a, 0x46, 0x56));
        closeBtn.Click += delegate { Close (); };
        Controls.Add (closeBtn);

        var foot = new Label ();
        foot.Text = "Seater Saturn   -   AGPL-3.0";
        foot.ForeColor = Color.FromArgb (0x6f, 0x7f, 0x8e);
        foot.SetBounds (30, 298, 400, 18);
        Controls.Add (foot);

        RefreshState ();
    }

    Button MakeButton (string text, int x, int y, int w, Color accent)
    {
        var b = new Button ();
        b.Text = text;
        b.SetBounds (x, y, w, 34);
        b.FlatStyle = FlatStyle.Flat;
        b.FlatAppearance.BorderColor = accent;
        b.FlatAppearance.BorderSize = 1;
        b.BackColor = Color.FromArgb (0x11, 0x16, 0x1d);
        b.ForeColor = accent;
        b.Font = new Font ("Segoe UI", 9.5f, FontStyle.Bold);
        return b;
    }

    void RefreshState ()
    {
        bool installed = File.Exists (DllPath);
        installBtn.Text = installed ? "REINSTALL" : "INSTALL";
        uninstallBtn.Enabled = installed;
        status.Text = installed ? "Phase Drift is installed." : "Phase Drift is not installed yet.";
        bar.Value = installed ? 100 : 0;
    }

    void Install ()
    {
        installBtn.Enabled = uninstallBtn.Enabled = false;
        status.Text = "Extracting ...";
        Application.DoEvents ();

        string err = DoInstall (delegate (int pct) { bar.Value = pct; Application.DoEvents (); });

        if (err == null)
        {
            bar.Value = 100;
            status.Text = "Installed. Restart your DAW, then rescan plug-ins.";
        }
        else status.Text = "Install failed: " + err;

        installBtn.Enabled = true;
        RefreshState ();
    }

    void Uninstall ()
    {
        string err = DoUninstall ();
        status.Text = err == null ? "Removed from " + BundleDir : "Uninstall failed: " + err;
        RefreshState ();
    }

    [STAThread]
    public static void Main (string[] args)
    {
        bool silent = false, remove = false;
        foreach (var a in args)
        {
            if (a == "/install" || a == "-install" || a == "/S" || a == "/silent") silent = true;
            if (a == "/uninstall" || a == "-uninstall") { silent = true; remove = true; }
        }

        if (silent)
        {
            string log = Path.Combine (Path.GetTempPath (), "phase-drift-setup.log");
            string err = remove ? DoUninstall () : DoInstall (null);
            File.WriteAllText (log, (remove ? "uninstall" : "install") + ": " + (err == null ? "ok" : err)
                                    + Environment.NewLine + DllPath + Environment.NewLine);
            Environment.Exit (err == null ? 0 : 1);
        }

        Application.EnableVisualStyles ();
        Application.Run (new SetupForm ());
    }
}
