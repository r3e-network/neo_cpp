Consensus CLI sync with RPC controls: console start/stop/restart now call ConsensusService, auto-start toggles keep config consistent.
Extended consensus controls: CLI now mirrors RPC start/stop/restart/autostart, persists auto-start flag, exposes detailed status.
Consensus CLI tracking RPC: manual start/stop/restart/autostart implemented with shared lifecycle and persisted config.
