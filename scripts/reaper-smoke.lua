-- Run only in the disposable Windows CI REAPER instance launched by the PS script.
local report = assert(os.getenv('BASS_REAPER_REPORT'))
local function log(message)
  local f = assert(io.open(report, 'a'))
  f:write(message .. '\n')
  f:close()
end
log('REAPER ' .. reaper.GetAppVersion())
if not reaper.GetAppVersion():match('^7%.86') then
  log('FAIL: unexpected REAPER version')
  return
end
reaper.InsertTrackAtIndex(0, true)
local track = reaper.GetTrack(0, 0)
local fx, phase, cycles, nextTime = -1, 0, 0, 0
local function step()
  if reaper.time_precise() < nextTime then reaper.defer(step); return end
  local ok, problem = pcall(function()
    if phase == 0 then
      fx = reaper.TrackFX_AddByName(track, 'VST3: Foundation Bass (Aulamix)', false, -1)
      if fx < 0 then fx = reaper.TrackFX_AddByName(track, 'Foundation Bass', false, -1) end
      assert(fx >= 0, 'VST3 not found or could not be instantiated')
      log('Inserted VST3, cycle ' .. (cycles + 1))
      reaper.TrackFX_Show(track, fx, 3)
      assert(reaper.TrackFX_GetFloatingWindow(track, fx), 'Editor window did not open')
      log('Opened editor')
      reaper.Main_OnCommand(1007, 0) -- Play: activate the audio graph.
    elseif phase == 1 then
      reaper.TrackFX_SetParamNormalized(track, fx, 1, 0.65)
      reaper.TrackFX_Show(track, fx, 2)
      log('Automated parameter and closed editor')
    elseif phase == 2 then
      reaper.TrackFX_Show(track, fx, 3)
      assert(reaper.TrackFX_GetFloatingWindow(track, fx), 'Editor did not reopen')
      log('Reopened editor')
    else
      reaper.Main_OnCommand(1016, 0) -- Stop.
      reaper.TrackFX_Show(track, fx, 2)
      reaper.TrackFX_Delete(track, fx)
      cycles = cycles + 1
      if cycles == 3 then
        log('PASS: REAPER 7.86 x64 VST3 insertion, native editor, automation, reopen and removal (3 cycles)')
        return
      end
    end
    phase = (phase + 1) % 4
    nextTime = reaper.time_precise() + 0.4
    reaper.defer(step)
  end)
  if not ok then log('FAIL: ' .. tostring(problem)) end
end
reaper.defer(step)
