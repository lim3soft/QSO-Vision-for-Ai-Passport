[简体中文](USER_GUIDE.zh_CN.md) · English

# QSO VISION 2.3.3 — user guide

Developer: limesoft (9W3SPY). Device UI: English.

QSO VISION turns AI Passport into a portable BrandMeister talkgroup listener. Connect to internet-enabled Wi-Fi, select a talkgroup with the buttons, and listen. Current caller details and up to 15 recent QSOs (five visible) appear when the service provides them. This is a receive-only app; it does not transmit. The device interface is in English.

## 1. QUICK START

- Install QSO-Vision.bin through the community installation entry and restart. For manual flashing, this complete firmware starts at address 0x0. Installation may reset existing settings, requiring Wi-Fi setup again.
- From the home screen, short OK → Options → Wi-Fi connection. Select and connect to your network.
- From home, short OK → Change talkgroup. Choose a group; the app returns to the monitoring screen automatically.

## 2. MAIN BUTTON CONTROLS
On the monitoring screen:

- Short UP / DOWN: scroll one history entry toward newer / older QSOs.
- Hold UP / DOWN: continuously increase / decrease volume; release to stop.
- Double DOWN: mute / unmute.
- Short OK: open Options.
- Long OK: fade the screen off while audio continues.
- While the screen is off, another long OK fades it back on. Other buttons do not wake it.
Inside menus: UP / DOWN moves the selection, short OK confirms, and long OK goes back.
“Double-press” means two quick presses, rather than holding the button.

## 3. WI-FI SETUP AND PASSWORD ENTRY

- Path: short OK from home → Wi-Fi connection. Wait for scanning, use UP / DOWN to select the SSID, then short OK.
- Password entry: UP / DOWN selects a character; hold either button to scroll faster. Short OK adds the selected character. Double UP deletes the last character.
- Character order: 0–9, a–z, A–Z, then space and the supported special characters shown on screen. Passwords are limited to 25 characters and are displayed in clear text for checking.
- When finished, double OK starts connecting. Success saves the profile and automatically returns to monitoring.
- Open networks start connecting without a password prompt. If the password is wrong or connection fails, long OK returns to the list so you can reselect the network and correct the password, or choose another network.
- If no networks are found, press OK to rescan. Leaving and reopening Wi-Fi connection also starts a scan. Opening the scan list alone does not intentionally replace the active network profile.
- Network setup is performed on the device; no phone or computer is needed to enter credentials.

## 4. SAVED NETWORKS

- Select an available saved network and press OK to connect using its saved password.
- At startup, the app attempts to reconnect to the last saved network. Saved networks outside scan range show SAVED/OFFLINE.
- Edit a password: highlight the saved network in the list, double DOWN, edit the password, then double OK to connect.
- Delete a profile: highlight the saved network in the list and double UP. This deletes the network profile. Double UP deletes a single character only while you are in the password editor.

## 5. TALKGROUP SELECTION
Path: short OK from home → Change talkgroup.
Presets: 91, 45401, 46001, 46020 and 50210.
Use UP / DOWN to select, then short OK to apply and return to monitoring.
For Other / Custom, UP / DOWN chooses a digit, short OK appends it, double UP deletes the last digit, and double OK applies the number. Long OK cancels and goes back. Supported custom IDs range from 91 to 16777215.

## 6. STATUS AND QSO HISTORY

- MONITORING TALK GROUP shows the selected group.
- When available, the current callsign is orange, with name, country and duration below. Segmented EQ bars in the lower-right animate during QSO activity and settle to zero afterward (an activity animation). The lower-left clock uses hh:mm:ss DAY dd/mm/yy, existing network time synchronization and Malaysia time; placeholders appear until valid time is available.
- RECENT QSOs loads and retains up to 15 completed calls from BM, with five visible. Short UP / DOWN scrolls one entry; the heading shows range/total. Each row has a flag before the orange callsign and an uppercase cyan country. The next line contains date dd/mm/yy, time, duration m:ss, BER and LOSS. Missing/pending quality values show --; genuine reported zero values show 0.00%. Completed calls enter history automatically. Changing TG reloads history; the available count depends on BM.
- The bottom-left VOL-xx% / MUTED status stays fixed beside right-to-left scrolling button instructions.
- The Wi-Fi icon reflects wireless connection status. Being connected to Wi-Fi does not guarantee internet access.
- A green speaker connection icon means Hoseline is connected; red means disconnected. The icon changes when muted. Green does not guarantee someone is currently transmitting.
- Battery is green above 20%, yellow at 11–20%, and red at 10% or less. Live charging detection/animation is not provided in this release.
- Options → About shows SSID, signal strength, IP, subnet, gateway, DNS, battery and app information. UP / DOWN scrolls; long OK returns.

## 7. USEFUL NOTES

- If there is no sound, check mute, volume, the selected group, Wi-Fi and Hoseline connection. An idle talkgroup is silent.
- Callsign, name, country and history depend on external services and may be delayed or incomplete. Not every talkgroup has available audio.
- Custom talkgroups use button entry; voice recognition is not included in this release.
- Independently developed by limesoft (9W3SPY), with no affiliation with or endorsement by BrandMeister or Hoseline. Service names, data and trademarks belong to their respective owners. Use the services according to their applicable terms.

## 8. SCREEN BRIGHTNESS AND TIMEOUT

- Path: short OK from home → Screen brightness. Short UP / DOWN raises / lowers brightness by 1%; hold to adjust faster. Range: 1–100%. Changes preview immediately; OK saves, long OK cancels unsaved changes.
- The saved level is used at startup, in menus, after button activity and after manual screen-off/wake. It survives normal power cycles. Default is 100% when no valid value is saved.
- Screen timeout lists After 1 / 5 / 10 minutes, Smart, then Never. Choose a mode and press OK to save directly; there is no separate dim-level setting.
- All automatic timeout modes fade to 1%. Default Smart dims after five seconds without QSO or button activity on the monitoring page. New audio or a button restores the saved level; menus stay at the saved level. Timed modes count from the last button activity, including during a QSO.
- Never disables automatic dimming. Dimming and manual screen-off keep audio running. Incoming audio does not wake a manually blanked screen; hold OK to wake it at the saved brightness.
