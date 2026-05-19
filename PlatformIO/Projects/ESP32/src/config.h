#pragma once

// ── WiFi ──────────────────────────────────────────────────────
#define WIFI_SSID   "MrSa2660"
#define WIFI_PASS   "salih2660"

// ── MQTT broker ───────────────────────────────────────────────
#define MQTT_HOST   "192.168.0.161"
#define MQTT_PORT   8883
#define MQTT_USER   "user1"
#define MQTT_PASS   "YOUR_MQTT_PASSWORD"
#define MQTT_TOPIC  "iot/del2/league/feedback"
#define DEVICE_ID   "ESP32-Smiley"

// ── CA certificate (PEM, from wilson.local) ───────────────────
// Must be a variable, not a #define — raw string literals don't work in macros.
static const char CA_CERT[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFBTCCAu2gAwIBAgIURkMg/bKi8HPnQn4OqtpDwhqoK8owDQYJKoZIhvcNAQEL
BQAwEjEQMA4GA1UEAwwHTVFUVC1DQTAeFw0yNjA1MTcxODMyMTBaFw0zNjA1MTQx
ODMyMTBaMBIxEDAOBgNVBAMMB01RVFQtQ0EwggIiMA0GCSqGSIb3DQEBAQUAA4IC
DwAwggIKAoICAQC8nf5IARgAB3FirRPveOW/4ju40nB/aL0kWLy5w3VLUY+AUZRE
Xkez6YYeLXEJhKlBkI46EdRaXBgP457rcC3EW3ZZHRxxu94u8rtvrjtbk9w5HVRz
bkwY2TS4f/GTB+/DOE5j1c8bzZK936/RILxkqlJvYV/3PSigCmEFxlLhegm5aWnk
tRTzk6/pwbqt71iIbIlSScG1g4PDO9daDcBMd72ZTjvm8Fd48fEmZxhTAnXbMvx1
1irAg9jGYzlgLdlxFYifwE4vFJeL4IZptJF9PRHTmjUsohBErMB+5aYKwqqp13pg
QxVxzqPlZ4p9CrIryaVXAV/xAGyFUMV5yQZ3A06UWu/JPeDUHqd0+42LxLWOLy9x
9Xde0sZHuQJbnBbNflj7wzvF9Jl1RknvZkN4Qf+r7I0TuybLNez1JRofBrKX42yN
WEeq8Kf7iGlr9I5QUgX7kq/mFCkzKsCdVfB2gAc90nwiE7fQmPc8JioBQpH7hqMi
OpvABic/U0kkoC7I9gfR63DZkhSsgQZnh9/IBmNLUkeRm7WDYVIc6EM7tcWJ1ltQ
4sjXPknhDNfj5t1xcuGl8FlrNSue9KaJyhft6zFNafU6nzYUAsAp8elsMHVhtlHy
XhW6LYxL/urFfYFy38sXBKUgIxQu4ssEh5vd/uhswRYMSAilJhebgDdR/wIDAQAB
o1MwUTAdBgNVHQ4EFgQUdmvBC6kBGEQzmRtxc12shylrNmQwHwYDVR0jBBgwFoAU
dmvBC6kBGEQzmRtxc12shylrNmQwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0B
AQsFAAOCAgEABIGGXdoQvNp5D/3vcZyK8dU6ur9JeLaEJawN7zhSVoevisEnsUaK
1n9GP8ubnFVyA0aqOnuMdXzVHzvP33rfYrJwGUfTlgy1R177OGki8Eo21l5717eg
S5wniJ+z3gPL83h0mZ+H55kBCJpYmDdRZOKsqpluKefrlK2ywHVU2d6/i0QsJfSb
xEvzhdD3ARtNtZaykd2cx0K0gGM54lWDQdla4ufRNkfGdUo+/C+JytSv+64GIts7
h1sVcWjwixkqFRQTwkTYrWhx/8/0ngAomFAhijl6taO77bbltT6PDtb6eAwkOarH
xDsfjf2hvZz2bqq+20DeNPLWrV+NsMaNhwF6pszfrenAtKrRcH8GTJdYwLTEhPF6
NQRaM7VavxPexM9tIH5MW+O/accoXGCSweFojdtfVrAqxWgcg4zHw3VXFKQGsm5k
pof0zBVGVDNhdQJf3NGQXWi31N2uttDKyQAwZzhaic+U8hk8n300YbXMojM2k729
9+TccZUPn/cbvcNjvxB9UxKWhtFUQ6TvgoAHljaOuB/50Wgq0z0NH6U/uFpb2f+i
PRcL6u5V8ILV7fXIyh4HPmJwDreNG9zYuBoVshMkxw+d3X0E7KLmEA6kvetTfKj9
0EH/sy778DnPpMA1+6AN3XaO9Fy2g78qgiKyScI3i1HEuWpQAECAwAA=
-----END CERTIFICATE-----
)EOF";
