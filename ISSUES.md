# Issue: Phase 0 Start Time Incorrect on Fresh Start

## Description
On a fresh start, phase 0 should have its start time set to 0 epoch. However, upon device connect, phase 0 is being pushed with the updated system time instead. This causes phase 0 to update to phase 1 on a reboot, which is not the intended behavior.

## Steps to Reproduce
1. Start the device with no persisted data (fresh start).
2. Observe the start time of phase 0.
3. Connect to the device and check the phase 0 start time in storage.
4. Reboot the device and observe that phase 0 updates to phase 1.

## Expected Behavior
- Phase 0 should retain a start time of 0 epoch on a fresh start.
- Phase 0 should update to phase immediately as elapsed > duration.

## Actual Behavior
- Phase 0 start time is set to the current system time on connect.
- Phase 0 updates to phase 1 on reboot, even if the phase duration has not elapsed.
- Phase 0 doesn't update without reboot even if phase duration has elapsed

## Notes
- Needs debugging to identify why phase 0's start time is being overwritten.
- See `StateMachine.cpp` for relevant logic.

---
