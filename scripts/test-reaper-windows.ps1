param([string]$Bundle = 'build/FoundationBass_artefacts/Release/VST3/Foundation Bass.vst3')
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$testRoot = Join-Path $env:RUNNER_TEMP ('bass-reaper-' + [guid]::NewGuid())
New-Item $testRoot -ItemType Directory | Out-Null
$installer = Join-Path $testRoot 'reaper786_x64-install.exe'
Invoke-WebRequest 'https://www.reaper.fm/files/7.x/reaper786_x64-install.exe' -OutFile $installer
$signature = Get-AuthenticodeSignature $installer
if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'Cockos') {
  throw "Official REAPER installer signature could not be verified: $($signature.Status)"
}
$installDir = Join-Path $testRoot 'app'
$install = Start-Process -FilePath $installer -ArgumentList "/S /D=$installDir" -Wait -PassThru
if ($install.ExitCode -ne 0) { throw "REAPER installation failed: $($install.ExitCode)" }
$reaper = Join-Path $installDir 'reaper.exe'
if (!(Test-Path $reaper)) { throw 'REAPER executable not found' }
$pluginDir = Split-Path (Resolve-Path $Bundle).Path
$config = Join-Path $testRoot 'reaper.ini'
@"
[REAPER]
vstpath64=$pluginDir
"@ | Set-Content -Path $config -Encoding ascii
$report = Join-Path (Get-Location) 'reaper-windows.log'
if (Test-Path $report) { throw 'Refusing to reuse a stale REAPER test report' }
$env:BASS_REAPER_REPORT = $report
$script = (Resolve-Path 'scripts/reaper-smoke.lua').Path

# Use the normal evaluation UI, waiting for its button to become enabled.
# No licensing files, registry credentials, or user DAW settings are imported.
Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class BassReaperSmokeNative {
  public delegate bool EnumCallback(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumCallback f, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr h, EnumCallback f, IntPtr l);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint id);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr h);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
}
'@
$process = Start-Process -FilePath $reaper -ArgumentList "-newinst -nosplash -cfgfile `"$config`" `"$script`"" -PassThru
$deadline = (Get-Date).AddSeconds(100)
try {
  while ((Get-Date) -lt $deadline) {
    $process.Refresh()
    if ($process.HasExited) { throw "REAPER exited before completing the test, code $($process.ExitCode)" }
    if (Test-Path $report) {
      $content = Get-Content $report -Raw
      if ($content -match '(?m)^FAIL:') { throw $content }
      if ($content -match '(?m)^PASS:') { Write-Output $content; exit 0 }
    }
    [BassReaperSmokeNative]::EnumWindows({ param($window, $unused)
      [uint32]$ownerId = 0
      [BassReaperSmokeNative]::GetWindowThreadProcessId($window, [ref]$ownerId) | Out-Null
      if ($ownerId -eq $process.Id) {
        [BassReaperSmokeNative]::EnumChildWindows($window, { param($control, $unusedChild)
          $text = New-Object System.Text.StringBuilder 256
          [BassReaperSmokeNative]::GetWindowText($control, $text, 256) | Out-Null
          if ($text.ToString() -match 'Still Evaluating' -and [BassReaperSmokeNative]::IsWindowEnabled($control)) {
            [BassReaperSmokeNative]::PostMessage($control, 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
          }
          return $true
        }, [IntPtr]::Zero) | Out-Null
      }
      return $true
    }, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 250
  }
  $partial = if (Test-Path $report) { Get-Content $report -Raw } else { 'No ReaScript report was produced' }
  throw "REAPER smoke test timed out. $partial"
} finally {
  # Stop only this disposable test process; it has no user project to preserve.
  if (!$process.HasExited) { Stop-Process -Id $process.Id -Force }
}
