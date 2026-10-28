[简体中文](wifi-saved-network-recovery.zh_CN.md) · English

# Automatic recovery across saved Wi-Fi networks

Included in build **2.3.3a**. The owner confirmed the preceding Wi-Fi recovery
test build works on the Passport. This build adds a four-wave Wi-Fi icon plus
a connection dot, retaining green connected and red disconnected states.
Audio DSP, buttons, display settings and stored profile/password formats are unchanged.

At startup the manager first tries the last successfully used saved profile.
If it cannot connect, or a working connection is lost and its normal bounded
retries fail, the service worker scans for available saved networks. It prefers
the last-used SSID if it remains eligible; otherwise it tries the strongest
visible saved alternative. Duplicate access points count as one profile. It
examines every scan result, including saved networks beyond the UI's first 32
results, using one driver AP record at a time rather than another large buffer.

Each candidate has bounded retries and a 30-second association/DHCP deadline.
Authentication, driver-start and DHCP failures advance to the next candidate.
After all candidates fail, or none are visible, a fresh scan runs after 30
seconds. Failed trials never change saved passwords or delete profiles.
Successful DHCP updates only the last-used SSID when necessary. Legacy profile
migration is idempotent and no longer overwrites slot zero or resets that choice.
Only saved networks are selected automatically; a broadcast SSID is needed for
alternative-network discovery. Wi-Fi availability does not prove Internet access.

A healthy connection is retained, even if another saved network becomes stronger.
Opening the Wi-Fi list/password editor suspends automatic selection. Scanning
while connected preserves that connection. Leaving Wi-Fi setup resumes recovery
if disconnected; a manual connection already in progress is allowed to finish.
No new task, Bluetooth service or scan while normally connected is introduced.

## Validation and device checks

The production Wi-Fi manager is compiled against host driver/NVS stubs. Tests
cover relocation in both directions, an alternative after 35 unrelated APs,
last-used preference, RSSI ordering, duplicate SSIDs, saved open networks,
incorrect saved passwords, driver/DHCP/scan timeouts, 30-second backoff, retained
profiles, last-used persistence, manual setup during scan/handoff, and legacy
migration. These tests verify decisions, not actual radio or DHCP timing.

On the Passport, save networks A and B, connect A, then remove A while B is in
range. Check automatic B connection, correct About/SSID/IP and restored audio.
Repeat after reboot and in the opposite direction. Check no switches while the
current network works, no lost profiles, and password editing during recovery.
The 2.3.3a icon/version rebuild still requires owner hardware validation before release.
