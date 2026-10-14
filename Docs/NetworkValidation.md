# Foundation network validation
Engine source checked locally: Engine/Classes/Engine/NetDriver.h and Engine/Private/Net/NetEmulationHelper.cpp, UE 5.8.3 CL 58210709.
-PktLag delays outgoing packets in milliseconds. Pass 50 to each endpoint for 100 ms added round-trip delay; pass 100 to each for 200 ms added RTT. Local baseline/network processing adds additional delay. This is configured delay, not a measured ping guarantee.
-PktLoss is percent of outgoing packets per endpoint/direction. Do not also apply incoming loss/delay. No jitter, duplication or ordering simulation enabled. Round-trip exchanges can lose either leg; loss is not a combined RTT probability.
Every process log must confirm PktLag and PktLoss parsing. A command-line argument alone is not evidence settings applied.

A-package runs:
- 100 ms configured RTT / 2% per-direction loss: Network-20261003-063935, PASS; authoritative and late-join count 40.
- 200 ms configured RTT / 5% per-direction loss: Network-20261003-064045, PASS; authoritative and late-join count 39.

B-package commands (run results recorded in Evidence.md):
```powershell
./Scripts/NetworkSmoke.ps1 -Players 4 -RttMs 100 -LossPercent 2
./Scripts/NetworkSmoke.ps1 -Players 4 -RttMs 200 -LossPercent 5
./Scripts/NetworkSmoke.ps1 -Players 4 -Rendered
```
Four-player mode uses host + two initially interacting clients + fourth late-joining client. Requires three distinct authoritative requesters, matching counts on all peers and confirmed emulation settings in every log.
The diagnostic uses the real owning component RPC with local/server cadence limits. Test process handles are retained and only those processes are stopped.
Scope: interaction count convergence and late-join initial state only. Does not validate future loot uniqueness, mission/outpost state, saves, shot validation, malicious cross-owner RPCs, reconnection or matchmaking. Listen server/direct-IP; no external services.
