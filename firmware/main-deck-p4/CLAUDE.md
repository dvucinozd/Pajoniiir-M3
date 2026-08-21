# main-deck-p4 developer notes

Važeći firmware target je samo `main-deck-p4` na ESP-IDF v6.0.2.

P4 izravno hosta DDJ-FLX4 na USB2 (MIDI + UAC1), Rekordbox medij na USB3,
PCM5102A na GPIO1/2/3, DSI/FT5426 UI i C6/ESP-Hosted mrežu. `control_link` je
lokalni semantic-event/LED adapter, bez UART transporta.

Prije promjena pročitaj root `AGENTS.md` te relevantne dokumente u `docs/`.
Za firmware promjene pokreni:

```powershell
. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1
idf.py build
$env:Path = "$env:Path;C:\msys64\ucrt64\bin"
.\tests\run_p4_host_tests.ps1
```

Ne dodavati drugi MCU target, peer protocol, profile-transfer ili međupanački
audio put bez nove eksplicitne arhitektonske odluke.
