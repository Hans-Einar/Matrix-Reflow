# Studie: Matrix-Reflow på Linux og OpenGL

Dato: 11. september 2026. Undersøkt revisjon: `2577d25f747d629a08e19cd5ad44c9e1b4609002`.
Arbeidskopi: `~/git/Matrix-Reflow`.
Upstream: [Delmay441/Matrix-Reflow](https://github.com/Delmay441/Matrix-Reflow).
Vårt repository: [Hans-Einar/Matrix-Reflow](https://github.com/Hans-Einar/Matrix-Reflow).

## Vurdering og anbefaling

**Dette er gjennomførbart, og jeg anbefaler Reflow som utgangspunkt dersom målet
er regnets utseende i denne varianten.** Simuleringen er fortsatt plattformuavhengig
C99 som allerede kompilerer og kjører på Linux. Den medfølgende Matrix-fonten gjør
tegnene mer forutsigbare enn i den første versjonen. Mer omfattende lysbehandling,
regnlogikk og CRT-effekt øker derimot arbeidet med grafikk og visuell kontroll.

Anbefalt grunnlag er **C99-kjernen + C++17 + OpenGL 3.3/GLSL 330 + X11/GLX +
libepoxy + FreeType**, bygget med CMake. Første mål er XScreenSaver under Window
Maker, med SDR-utgang. XScreenSaver beholder ansvaret for aktivering, låsing og
autentisering; selve effekten tegner bare i vinduet verten gir den.

Anslag for én utvikler med erfaring i C/C++, OpenGL og X11:

| Leveranse | Samlet innsats fra start | Resultat |
| --- | ---: | --- |
| Visuell prototype | 3–5 persondager | Eget vindu med riktig font, regn, kamera og første bloom-versjon |
| Brukbar lokal XScreenSaver-versjon | 7–12 persondager | Regn og bloom i vertsvindu, grunninnstillinger og installasjon på labtop |
| Stabil X11/SDR-utgave | 18–30 persondager | Også CRT, fullført visuell kontroll, konfigurasjon, preview, flerskjerm, feilbehandling og dokumentert bygg |

Intervallene er **kumulative**, ikke tre pakker som skal summeres. En persondag
er omtrent 7,5 timer; siste trinn tilsvarer 135–225 timer eller omtrent 4–6
arbeidsuker. Dette er et planleggingsanslag, ikke målt implementasjonstid.
Egen GTK-konfigurator, native Wayland og ekte skjerm-HDR er separate leveranser.

Den forrige studien anslo 12–20 persondager for stabil Modern Matrix på X11/SDR.
Reflow anslås dermed omtrent 6–10 persondager høyere ved sammenlignbart omfang.
Det er ikke gjort en Linux-renderer i noen av prosjektene som vi allerede kan
gjenbruke: den første leveransen var også en studie. Det vil være enklere å porte
Reflow direkte enn å porte den eldre motoren først og deretter ettermontere alle
endringene.

## Repository og sammenligningsgrunnlag

Reflow ligger i samme GitHub-forknettverk som `DigitalChewie/ModernMatrixScreensaver`.
Kontoen hadde allerede `Hans-Einar/ModernMatrixScreensaver`. Forsøket på å opprette
en ekstra fork fikk GitHub til å gjenbruke og omdøpe denne; navnet ble straks satt
tilbake, og den opprinnelige forken er beholdt med uendret hovedgren.

`Hans-Einar/Matrix-Reflow` er derfor **et separat repository med upstreams
Git-historikk bevart, ikke en ekstra formell GitHub-fork**. Lokal `origin` peker
til vårt repository, `upstream` til Delmay441. `main` er pushet med samme revisjon
som upstream. Denne studien er skrevet lokalt og er ikke en implementasjon av porten.

Sammenligningen gjelder Modern Matrix ved
`2d1fa5a121e6c366955cc58f9f67500ea8fe9a94`; den tidligere lokale studien ligger i
`~/git/ModernMatrixScreensaver/Linux/study.md`.

## Hva som har endret seg

| Del | Modern Matrix | Matrix-Reflow | Betydning for porten |
| --- | --- | --- | --- |
| C-kjerne, `.c` + `.h` | 276 + 105 linjer | 838 + 137 linjer | Fortsatt direkte gjenbruk, men flere tilstander og parametere å validere |
| Windows-renderer, `.cpp` + `.h` | 579 + 120 linjer | 833 + 156 linjer | Mer etterbehandling, tidsstyring og skjermtilpasning |
| HLSL | 162 linjer | 406 linjer | Større GLSL-port med femnivås bloom og separat CRT-pass |
| Atlasbygging | 125 linjer | 161 linjer | Egen innebygd font erstatter systemfontvalg |
| Konfigurasjon og lagring | 204 + 84 linjer | 578 + 199 linjer | Flere valg, farger og fem lagrede brukerprofiler |
| Tegninstans | 32 byte | 40 byte | Nye vertex-attributter og større instansbuffer |
| Tegnsett | Seks alternative kodinger | 57 faste tegn, alternativ binærmodus | Gamle encoding-valg skal ikke videreføres som om de finnes i Reflow |

Linjetall er målt med `wc -l`, inklusive kommentarer og blanklinjer. Kildehenvisninger:
[kjernen](../core/mmcore.c), [API](../core/mmcore.h),
[renderer](../windows/renderer.cpp), [shadere](../windows/shaders.hlsl),
[atlas](../windows/atlas.cpp), [konfigurasjon](../windows/config.cpp),
[lagring](../windows/settings_store.cpp) og [profildefinisjon](../windows/settings_store.h).

### Hvorfor regnet kan oppleves mer autentisk

Dette er en vurdering av koden, ikke en måling mot filmopptak:

* En egen `Matrix-Code`-font bestemmer tegnformene. Tegn kan speiles separat
  horisontalt og vertikalt; tilfeldige tomrom bryter opp kolonnene.
* Kolonner bruker baner med avstandsregler, varierende lengde og gradvis
  oppstart. Hastigheten varierer langsomt fremfor at alle strømmer faller likt.
* Tegn muterer individuelt, særlig nær strømmenes hoder. Lysstyrken holdes høy
  tidlig i halen og avtar deretter eksponentielt, med små fargevariasjoner.
* Bloom fordeles over fem størrelser for både tett glød og bredere lysspredning.
  Valgfri CRT-behandling legger til blant annet scanlines og RGB-maskemønster.
* Standarddybden er null, som gir et flatere uttrykk. Romlig dybde og kamera
  finnes fortsatt som innstillinger. Ultrawide-utvidelse er implementert i kjernen.

CRT-passet har ingen historikktekstur og skaper derfor ikke ekte tidsmessig
fosforetterglød. Halene kommer fra simuleringen; glød og CRT er romlig behandling
av gjeldende bilde. En eventuell ny etterglødeffekt vil være en egen funksjon.

README og arvede dokumenter må kontrolleres mot koden. README omtaler både
atlasbygging ved bygging og ved oppstart; faktisk rasteriserer `atlas.cpp` fonten
ved oppstart, mens HLSL kompileres ved bygging. De tre bildene `hero.png`,
`encodings.png` og `toggles.png` under `windows/docs` er byteidentiske med bildene
i den eldre arbeidskopien. De dokumenterer ikke i seg selv Reflows nye utseende.
Windows-programmet er ikke kjørt eller visuelt sammenlignet i denne studien.

## Lokale kontroller

### Kompilering og simulering

`core/mmcore.c` ble kompilert uendret med GCC, C99, `-O2` og
`-Wall -Wextra -Wpedantic -Werror`. Den eksisterende `windows/_linktest.cpp`
ble bygget som C++17 og lenket med C-objektet og `libm`. Begge lyktes:

```text
settings: density=0.90 speed=0.35 bloom=0.90
derived : strips=137 fallSpeed=12.88 mutation=0.69
world   : halfWidth=38.0 topY=28.0 spacing=0.48 slotCount=118
sim     : emitted=2371 instances after 2s (expect >0)
LINKTEST OK
```

Den testen er bare en enkel lenkeprøve: den bruker 58 tegn selv om atlaset har
57, og sender `1.0` som resttid til tegningen. Derfor ble det også kjørt en
midlertidig C-test med korrekt tegnantall og realistisk resttid:

* 12 scenarier: 16:9, 21:9, 32:9 og portrett, kombinert med dybde 0, 0,75 og 1,5.
* 600 tidssteg à 1/60 sekund per scenario. Instanser ble undersøkt hver 30. frame
  og ved avslutning, med resttid både 0 og 1/120 sekund.
* To simuleringer med samme seed ble sammenlignet byte for byte. Alle ti float-
  felt ble kontrollert for endelige verdier, samt tegnindekser, speiling,
  lysstyrke, dybdesortering og buffergrenser med en liten buffer og vaktområde.
* Endringer under kjøring omfattet tetthet, tegnstørrelse, dybde, skjermformat,
  binærmodus og speiling. Klokkeverdier gikk inn og ut av 11:11. Kameraets
  rebasering ble utløst to ganger med en syntetisk rask kameraforflytning.

**Alle kontrollene besto; 2 551 223 tegninstanser ble undersøkt.** Største kapasitet
i disse scenariene var 185 328 instanser, mens høyeste observerte antall synlige
instanser ved prøvetaking var 20 714. Dette er ikke globale maksimumsverdier.
Testene viser funksjonell kjøring, ikke visuell korrekthet, sanntidsytelse eller
full robusthet. Det er ikke kjørt sanitizer-test for Reflow i denne studien.
Testkode og binærer ble laget i en midlertidig katalog og fjernet etter kjøringen.

### Font og maskin

`windows/Matrix-Code.ttf` er 7 896 byte. Fonten kodet i `windows/font_data.h`
er byteidentisk. Alle **57 av 57** kodepunkter fra atlasets faktiske tabell finnes
i fonten, kontrollert via FreeType på Linux. Ingen systemfontfallback er nødvendig
for dette tegnsettet. Undersøkte metadata oppgir familien `Matrix-Code`, men ga
ingen lisens- eller opphavstekst.

En ny `glxinfo -B` på labtop viser AlmaLinux 10.2, Intel HD Graphics 620,
Mesa 25.2.7, direkte og akselerert rendering, OpenGL 4.6 og GLSL 4.60.
GL 3.3 er dermed innenfor maskinens rapporterte støtte. Byggmiljøet som ble
kontrollert i den første studien har GCC 14.3.1, CMake 3.31.8 og utviklingsfiler
for X11, OpenGL, FreeType og libepoxy. Ingen ny pakkeinstallasjon var nødvendig
for forsøkene her. GPU-ytelsen til en Reflow-port er ennå ikke målt.

## Konkret portering

### 1. Behold kjernen, tilpass driveren

Simuleringen bør forbli i `core/`, kompilert som C99. Plattformlaget leverer
monoton tid, lokal klokke, skjermformat og kameraforflytning. Det må bevare disse
detaljene i dagens [renderer](../windows/renderer.cpp) og [kjerne](../core/mmcore.c):

* Fysikken bruker faste steg på 1/60 sekund; oppsamlet tidsunderskudd begrenses
  til 0,1 sekund. Etter suspend eller lang pause skal vi ikke spille inn flere
  minutters simulering i én frame.
* `renderAlpha` er misvisende navngitt: koden forventer **gjenstående sekunder**,
  normalt i intervallet `[0, 1/60)`, ikke en normalisert interpolasjonsfaktor.
  Koden fremskriver hodeposisjonen med resttiden; den interpolerer ikke mellom
  to lagrede tilstander. Bruk for eksempel lokalnavnet `residualSeconds`.
* Fremoverkameraet og `mm_sim_set_camera_travel()` må være samordnet. Når
  kameraet sentreres på nytt etter 72 enheter, må `mm_sim_rebase_depth()` også
  flytte kolonnene. Ellers vil nye og eksisterende dybdelag skille lag.
* `mm_world()` avhenger nå av innstillinger. Tegnstørrelse, dybde og skjermformat
  påvirker kapasiteten; ingen fast grense fra originalens 38 slots må gjenbrukes.
* Instansene kommer allerede dybdesortert fra kjernen. Behold rekkefølgen ved
  opplasting og tegning, slik at transparent overlapping ikke endrer uttrykk.

### 2. Fontatlas med FreeType

Bruk den innebygde fonten privat gjennom `FT_New_Memory_Face`, med fontdata i live
så lenge fontobjektet brukes. Fonten trenger ikke installeres på maskinen.
Fontconfig er derfor ikke nødvendig i denne portens normale tegnbane.

Behold tabellrekkefølgen fra `atlas.cpp` nøyaktig: indeks 3 er `0`, indeks 4 er `1`
og indeks 12 er kolon. Binærmodus og 11:11-effekten bruker disse indeksene direkte.
Ikke sorter tabellen eller fyll inn «manglende» tall. Atlaset inneholder 57 tegn,
ikke 58 slik lenketestens kommentar sier.

Med dagens 72 piksler per celle og 16 kolonner blir atlaset 1152 × 288 piksler:
omtrent 324 KiB som `GL_R8` før mipnivåer. FreeType erstatter DirectWrite,
Direct2D og WIC. Bruk gråtonedekning, tilsvarende plassering og filtrering.
Kontroller baseline, sidekanter og texelgrenser visuelt; FreeType og DirectWrite
vil ikke nødvendigvis gi identiske rasterpiksler.

Tegnene i atlaset skal være uspeilet. Reflow gjør speiling per instans i shaderen;
den eldre portens atlas-speiling skal ikke kopieres inn i tillegg.

### 3. OpenGL-renderer og GLSL

Direct3D-ressurser, DXGI, DirectXMath og forhåndskompilerte DXBC-shadere erstattes.
Det er åtte shaderinnganger å oversette: to vertex-shadere og seks fragment-
shadere for tegn, terskel, nedskalering, oppskalering, sluttkomposisjon og CRT.
OpenGL 3.3 har instancing som denne rendererstrukturen trenger; GL 4.6-spesifikke
shaderbinærer eller compute-shadere er ikke et krav.

`MMGlyphInstance` har ti float-felt, 40 byte. En mulig attributtfordeling er
`vec4(position, cell)` ved offset 0, `vec3(flipX, flipY, bright)` ved offset 16 og
`vec3(color)` ved offset 28, alle med stride 40 og divisor 1. Ikke anta at en
GLSL-bufferstruktur med `vec3` får identisk pakking uten eksplisitt kontroll.
Uniformblokkens layout må tilsvarende speiles eksplisitt.

Følgende krever særskilt kontroll ved oversettelsen:

* D3D- og GL-konvensjoner for matriser, UV-retning, frontside og klipperommets
  z-intervall. Lag en GL-kompatibel projeksjonsmatrise fremfor å kreve GL 4.5
  `glClipControl` når grunnmålet er 3.3.
* Tegnenes blanding bruker `ONE, ONE_MINUS_SRC_ALPHA`. Bloom-oppbyggingen bruker
  additiv `ONE, ONE`. Vanlig `SRC_ALPHA` overalt vil endre lysstyrken.
* Bloom har fem eksplisitte teksturer fra halv oppløsning til 1/32. Pipeline er
  terskeluttrekk, fire nedskaleringer og fire additive oppskaleringer, så
  sluttkomposisjon. Automatisk mip-generering erstatter ikke filterkjeden.
* Ved oppskalering legges mindre nivå til innholdet som allerede ligger i det
  større nivået. Ikke tøm målet først, og ikke les og skriv samme tekstur i passet.
* CRT bruker skjermens fysiske pikselkoordinater til maskemønster og scanlines.
  D3Ds `SV_Position` og GLs `gl_FragCoord` må få konsistent y-retning, også ved resize.

**Sluttalfa må behandles eksplisitt.** Komposisjonsshaderen beregner alfa fra
bloomens luminans; med bloom av kan alfa være null selv om RGB har et bilde.
Windows ignorerer alfa gjennom `DXGI_ALPHA_MODE_IGNORE`. På Linux må vi bruke
et passende opakt mål og/eller skrive sluttalfa 1. Et ARGB-vindu med compositor
kan ellers gjøre regnet transparent eller usynlig. Dette er adskilt fra
tegnblandingen inne i sceneteksturen.

Effekten bør først gjenskape dagens SDR-resultat. Flyttalls scene og bloom er
ikke det samme som HDR-utgang til skjermen. Gamma/sRGB og eksponering må velges
bevisst og sammenlignes med Windows-referanse; ikke legg inn en ny tonemapper
som en skjult del av porteringen.

### 4. XScreenSaver og konfigurasjon

Lag et vanlig testvindu for utvikling og en modus som tegner direkte i vinduet
fra `XSCREENSAVER_WINDOW`, eventuelt med eksplisitt `--window-id` for preview og
testing. XScreenSaver 6.16 på denne maskinen dokumenterer miljøvariabelen i
`~/src/xscreensaver-6.16/driver/xscreensaver.man`; `utils/vroot.h` viser parsing.

Et GLX-context må være kompatibelt med vertsvinduets eksisterende visual/FBConfig.
Det er en konkret integrasjonstest, ikke noe vi kan anta fordi eget testvindu
fungerer. Håndter resize, små previewvinduer, vindusødeleggelse, flere skjermer,
signaler og feil ved oppretting av GPU-ressurser. Effekten skal ikke opprette et
nytt fullscreen-overlegg eller overta tastatur, mus eller autentisering.

Første konfigurasjon bør være kommandolinjeflagg med XML for `xscreensaver-settings`.
Ta med hastighet, tetthet, størrelse, lengde, dybde, kamerafart, mutasjon, farger,
bloom, speiling, binærmodus, hull, hodekontrast, CRT og bildefrekvensbegrensning.
XScreenSavers eksisterende programmer/XML er referanse for hvilke kontroller
som passer; avansert farge- og profilredigering trenger ikke en egen GUI i første trinn.

Den lokale installasjonen bruker `/usr/local/libexec/xscreensaver` og
`/usr/local/share/xscreensaver/config`. Byggsystemet bør gjøre installasjonsstiene
konfigurerbare. En senere GTK-app kan tilby fargevelger og de fem brukerprofilene;
velg da én tydelig lagringsmodell med CLI-overstyring, ikke motstridende verdier
i både en privat profilfil og `.xscreensaver`.

Windows-spesifikk VRR/tearing, frame-latency-objekter og trådprioritet kan ikke
kopieres til GLX. Start med swap interval, monoton tidsstyring og valgfritt
60-FPS-tak. Mål samspillet med compositor. Tilsvarende VRR-oppførsel på alle
Linux-oppsett inngår ikke som et løfte i første utgave.

## Ytelse og eksisterende svakheter

Full bloom gir 11 grafikkpass per bilde inklusive scene og komposisjon; CRT
øker til 12. Uten bloom blir det to pass, eventuelt tre med CRT. Antall pass
alene sier ikke bildefrekvensen: de fleste bloom-passene er små, men de leser
flere texeler per piksel.

Windows-versjonen allokerer to fullstore RGBA16F-mål, scene og CRT-mellombilde,
pluss fem bloom-mål. Beregnet lagring, uten backbuffer, atlas, instanser og
driveroverhead:

| Oppløsning | Som dagens allokering | Hvis CRT-målet opprettes bare ved behov og CRT er av |
| --- | ---: | ---: |
| 1920 × 1080 | 36,91 MiB | 21,09 MiB |
| 3840 × 2160 | 147,64 MiB | 84,35 MiB |

Dette er en størrelsesberegning, ikke målt VRAM-bruk. Instanskapasiteten i
røykprøven krevde på det meste omtrent 7,1 MiB per full instansbuffer, i tillegg
til simuleringsdata og eventuelle doble buffere. Små tegn, høy dybde og brede
skjermer kan øke minnebehov og CPU-arbeid kraftig. Den integrerte HD 620 har
nødvendige API-funksjoner, men stabil 60 FPS med alle effekter i 4K er ubekreftet.

Første ytelsestiltak ved behov: unngå ubrukt CRT-mål, ha eget kvalitetsvalg for
bloom-oppløsning, sett grenser for instanskapasitet og begrens FPS. Ikke endre
regnlogikken automatisk under belastning før vi har målt årsaken og konsekvensen.

Noen kildefunn bør registreres som egne rettelser under implementasjon:

* Allokeringsfeil håndteres ikke konsekvent i C-kjernen. Valider innstillinger
  og skjermstørrelser og innfør minnebudsjett før store allokeringer.
* Shaderens eksisterende `whiteFlash` bruker `smoothstep(0.95, 1.0, bright)`,
  mens kjernen begrenser lysstyrken til 0,8. Dette tillegget bidrar dermed ikke.
  Den separate `extraContrastHeads`-masken bruker passende terskler ved 0,76–0,80.
  Ikke «reparer» førstnevnte ubemerket og kall lysendringen en nødvendig GL-port.
* `waves` finnes i innstillingene og har en standardverdi, men leses ikke som
  bryter av dagens kjernekode. Hastighetsbølgene er dermed ikke styrt av dette
  flagget slik man kunne anta. En GUI må beskrive faktisk funksjonalitet.
* Lenketestens 58 tegn og resttid 1,0 er ikke en korrekt rendererreferanse.
  En varig test bør bruke atlasets tabellstørrelse og resttid i sekunder.

## Arbeidspakker og akseptanse

| Arbeid | Persondager før reserve |
| --- | ---: |
| CMake, Linux-vert og GL-context | 1–2 |
| FreeType-atlas med medfølgende font | 0,5–1 |
| Simuleringsdriver, kamera og resttid | 1,5–2,5 |
| Instancing, tegnshader, projeksjon og farger | 2–3 |
| Femnivås bloom og sluttkomposisjon | 2–3 |
| CRT-pass og pikselkontroll | 1–2 |
| XScreenSaver-vindu, preview og livssyklus | 1,5–2,5 |
| CLI og XML-innstillinger | 1–2 |
| Visuell kontroll, robusthet og ytelse | 2,5–4,5 |
| Installasjon og dokumentasjon | 1–1,5 |
| **Sum** | **14–24** |

Med omtrent 25 prosent reserve blir totalen avrundet **18–30 persondager**.
En egen GTK-konfigurator med fargevalg og profiler anslås foreløpig til ytterligere
5–8 persondager. Wayland og ekte skjerm-HDR trenger egne undersøkelser og er ikke
gjemt i denne reserven. Særlig visuell sammenligning og GLX-integrasjon kan flytte anslaget.

Før en stabil utgave bør vi ha kontrollert:

1. Samme seed, parametere og tid mot Windows-bilder: fontretning, hastighet,
   hoder/haler, farger, lysfordeling og CRT. Skaff oppdaterte referanser først.
2. Standardregn, dybde, ultrawide og portrett, binærmodus, farger, speiling,
   hull, 11:11 og raskere testinjeksjon av de sjeldne glitch-hendelsene.
3. Bloom og CRT i alle av/på-kombinasjoner, også med compositor, slik at
   sluttalfa og blending ikke skjuler eller dobbelteksponerer bildet.
4. Preview, fullscreen via verten, flere skjermer, resize, suspend/resume,
   avslutning og ryddig håndtering av manglende GL-context eller font.
5. Målt CPU/GPU-tid og minne ved 1080p og skjermens faktiske oppløsning,
   med fornuftige kvalitetsvalg og uten å svekke responsen til låseverten.

## Kilder, opphav og avgrensning

Vurderingen bygger primært på kildefilene lenket over ved angitt revisjon,
lokale kompileringer og maskinens rapporterte grafikkstøtte. Nyttige primærreferanser:

* [Reflow upstream](https://github.com/Delmay441/Matrix-Reflow) for prosjekt og historikk.
* [Khronos: glVertexAttribDivisor](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Refpages/main/gl4/glVertexAttribDivisor.xml)
  for instancing og tilgjengelighet fra OpenGL 3.3.
* [Khronos: glClipControl](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Refpages/main/gl4/glClipControl.xml)
  for koordinatkonvensjoner og hvorfor dette ikke er en GL 3.3-baselinefunksjon.
* [FreeType: opprette et font-face](https://freetype.org/freetype2/docs/tutorial/step1.html)
  for fil- og minnebasert fontlasting.
* Lokal XScreenSaver 6.16-kilde, særlig `driver/xscreensaver.man`, `utils/vroot.h`
  og `hacks/config/glmatrix.xml`, for vertsvindu og konfigurasjon.

Jeg fant ingen toppnivålisens i Reflow-arbeidskopien. Opphav/lisens for både
kode og medfølgende font bør avklares før vi pakker og distribuerer en Linux-utgave.
At [Rezmason/matrix har en MIT-lisens](https://raw.githubusercontent.com/Rezmason/matrix/master/LICENSE)
fastslår ikke automatisk lisensen til alle filene i dette prosjektet. Studien
legger derfor ikke til noen antatt lisens på andres kode eller font.

Denne oppgaven har opprettet repository og lokal arbeidskopi og skrevet studien.
Ingen renderer, skjermsparerinnstillinger, innlogging eller aktiv X11-sesjon er endret.
