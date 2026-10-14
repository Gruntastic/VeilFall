# Memory preflight — October 5, 2026

Changes are project-local and reversible. No user applications were terminated, prior packages/maps/saves deleted, pagefile or machine settings changed, or paid services used. The accepted G3 binary remains VF-G3-20261003-A; no new compilation/package is claimed. These launch overrides work with that actual package and the installed UE 5.8.3 Editor. G4 movement/art implementation is still separate work.

## Findings and changes

The most recent user's Editor log reports 16 GB RAM, roughly 3 GB free at startup, and an RTX 4060 Laptop with 7956 MB dedicated GPU memory. It shows ten shader workers. No out-of-memory crash or texture-pool-over-budget message was found in that recent log. Earlier cooker logs contain memory-pressure warnings. The original reported incident has not been reproduced or attributed to a leak; its exact message is pending.

* Default PlayG3.cmd and G3.ps1 Host/Join/Editor now use MemoryProfile.ps1. It sets a 768 MiB texture streaming budget, a 200 MiB render-target pool minimum, 85% internal resolution, mostly Medium scalability and 60 FPS. Startup INI overrides apply pool/resolution settings before viewport allocation. These are budgets/minima, not hard total GPU/RAM caps. The preset reduces visual detail and is not the 1080p High/100% acceptance target.
* Source DefaultEngine.ini reserves 75% of logical cores from shader compilation and twelve while running a game. Actual new Editor logs confirm five workers instead of the previous ten. Compilation may take longer; aggregate worker-memory savings were not measured.
* DefaultEditor.ini raises cooker free-memory GC thresholds to 3072 MiB physical / 4096 MiB commit and enables Critical pressure-triggered GC. This is earlier collection, not an allocation limit. Cooking with these new settings is NOT RUN.
* G3NetworkSmoke.ps1 -HostRendered renders only the host, with headless guest processes for state tests. This avoids several simultaneous graphics windows. All diagnostic processes are now capped at 60 FPS. This is not a substitute for four rendered players or human co-op presentation review.

## Actual measurements

Each pair used sequential owned processes, the same G3 map and DX12. Packaged samples lasted 20 seconds per process; full Editor samples lasted 45 seconds per process. CPU working set/private commitment was sampled every 500 ms. Game runs exercised scripted relay AI/navigation/hostile damage and blocked-sight memory checks; full Editor runs loaded the map and viewport without playing. Processes were deliberately stopped after sampling, not reported as clean shutdowns. Saved roots for gameplay fixtures were isolated.

Final startup-cap runs:

| Target / metric (MiB) | Baseline | Reduced |
|---|---:|---:|
| Packaged end private commitment | 1587.54 | 1495.99 |
| Packaged peak working set | 1023.39 | 1018.58 |
| Packaged peak sampled private commitment | 1587.54 | 1612.86 |
| Full Editor end private commitment | 3651.30 | 3519.15 |
| Full Editor peak working set | 3347.17 | 3413.89 |
| Full Editor peak sampled private commitment | 3899.55 | 3828.19 |

End private commitment fell about 92 MiB (5.8%) for the game and 132 MiB (3.6%) for the full Editor. Peak physical RAM did NOT meaningfully improve; full Editor peak working set increased in this pair. This is a modest commitment reduction, not a demonstrated fix for an out-of-memory crash. Different background pressure, shader/DDC warming and fixed baseline-first order can influence results; no statistical significance claim. GPU allocated/resident bytes were NOT MEASURED.

The most effective observed workflow distinction is packaged play instead of keeping the full Editor open: these runs measured ~1.0 GiB packaged peak working set versus ~3.3 GiB full Editor. The scenarios differ (game combat versus Editor viewport), so this is Editor overhead evidence, not a matched content optimization percentage. Use packaged play for movement/combat feel, and the Editor for authoring.

Evidence: Logs/Memory-Packaged-20261005-070356/Results.csv and samples/logs; Logs/Memory-Editor-20261005-070437/Results.csv and samples/logs. Earlier post-startup-only pairs are retained at Memory-Packaged-20261005-065650, Memory-EditorGame-20261005-065911 and Memory-Editor-20261005-070041. An initial 20-second Editor-game probe at 065748 ended before its AI completion marker and is NOT a passing gameplay check. Later 45-second Editor-game pairs passed both AI markers. Old Result.txt boilerplate described all targets as game fixtures; the table/target here distinguishes actual full Editor versus game behavior.

Host-rendered-only baseline and 100 ms RTT / 2% directional loss lifecycle checks passed at G3-Network-20261005-070607 and 070717. The latter sampled aggregate test-process peaks of 2269.57 MiB working set / 2419.45 MiB private commitment, including the transient fifth-player rejection probe; unrelated applications and GPU memory are excluded.

The first 200 ms / 5% host-only run at 070739 FAILED after reconnect: uncapped headless observers repeated the opt-in ServerDiagnosticAck at their very high frame rate, overflowing a reliable network buffer. Logs/Failure.txt are retained. This was not an out-of-memory crash. The runner now caps all test processes at 60 FPS; its rerun status is recorded below. The binary's opt-in diagnostic still repeats acknowledgments per tick; future G4 code should send each reliable acknowledgment once. Ordinary gameplay does not execute this G3Smoke-only helper. Do not conceal this limitation behind the old G3 passes.

## Run

```powershell
./Scripts/MemoryProfile.ps1 Host
./Scripts/MemoryProfile.ps1 Join
./Scripts/MemoryProfile.ps1 Editor
./Scripts/MemoryProfile.ps1 Editor -FullQuality
./Scripts/MeasureMemory.ps1 -Target Packaged
./Scripts/MeasureMemory.ps1 -Target EditorGame -Seconds 45
./Scripts/MeasureMemory.ps1 -Target Editor -Seconds 45
./Scripts/G3NetworkSmoke.ps1 -HostRendered -RttMs 100 -LossPercent 2
```

PlayG3.cmd is the double-click entry. FullQuality omits the testing reductions and uses normal existing quality settings (still capped at 60 FPS); art approval must use the intended reference settings. Limit concurrent rendered test instances on this 16 GB/8 GB machine. Existing fully rendered G3 runs remain historical evidence; low-memory host-only checks do not replace them.

NOT RUN: reproduction of the original warning/crash, long-session leak/fragmentation analysis, actual GPU memory comparison, new cooker pressure thresholds, four-player combat-heavy frame/memory/VRAM budgets, reference 1080p High/100% measurements, physical input or visual approval of this new preset. No assertion that the issue is resolved until the original test is repeated.

Stress rerun PASS: Logs/G3-Network-20261005-071046/Result.txt, configured 200 ms RTT / 5% directional loss, host rendered and all diagnostic processes capped at 60 FPS. Aggregate sampled peak: 2278.45 MiB working set / 2430.10 MiB private commitment across owned test processes, including transient capacity probing. Full pickup/revive/recovery/late-join/logout/reconnect/host-restart lifecycle completed. This resolves this runner's uncapped diagnostic overflow; it does not claim the original memory incident is resolved.
