# Studie: Modern Matrix på Linux og OpenGL

Dato: 11. september 2026. Undersøkt revisjon: `2d1fa5a121e6c366955cc58f9f67500ea8fe9a94`.
Arbeidskopi: `~/git/ModernMatrixScreensaver`, fork `Hans-Einar/ModernMatrixScreensaver`,
upstream `DigitalChewie/ModernMatrixScreensaver`.

## Vurdering og anbefaling

**Porteringen er godt gjennomførbar.** Dette er først og fremst arbeid med en ny
grafikk- og plattformdel: regnsimuleringen finnes allerede i ren C99 og kan brukes
direkte på Linux. Den eksisterende Windows-porten gir dessuten en kort og konkret
referanse for rendererens oppbygning. Vi trenger ikke porte Swift, Metal-rammeverket
eller Direct3D som sådan.

Jeg anbefaler **C99-kjernen + C++17 + OpenGL 3.3/GLSL 330 + X11/GLX**, med FreeType
og Fontconfig for tegnene. Første leveranse bør være en separat XScreenSaver-effekt
med SDR-utgang og samme bloom, kamera og regn som originalen. XScreenSaver fortsetter
å håndtere aktivering, låsing og passorddialogen vi allerede har tilpasset.

Anslag for én utvikler som kjenner C/C++, OpenGL og X11:

| Leveranse | Samlet innsats fra start | Hva vi har da |
| --- | ---: | --- |
| Visuell prototype | 2–4 persondager | Regn med tegnatlas i et eget vindu; nok til å sammenligne utseende |
| Brukbar XScreenSaver-versjon på labtop | 5–8 persondager | Rendering i vertens vindu, bloom, grunninnstillinger og installasjon for lokal testing |
| Stabil X11/SDR-utgave for forken | 12–20 persondager | Innstillinger i XScreenSaver, preview, feilbehandling, tester, flerskjermkontroll og dokumentert bygg/installasjon |

Tallene er **kumulative**, ikke tre arbeidspakker som skal summeres. En persondag
betyr omtrent 7,5 arbeidstimer. Det siste intervallet tilsvarer rundt 90–150 timer,
eller 2½–4 arbeidsuker for én person. Dette er et planleggingsanslag basert på
kodelesing og forsøkene nedenfor, ikke målt implementasjonstid eller et løfte om
kalendertid. Egen GTK-konfigurator, native Wayland og ekte skjerm-HDR er ikke inkludert.

## Hva som faktisk finnes i prosjektet

| Del | Omfang i undersøkt revisjon | Gjenbruk på Linux |
| --- | ---: | --- |
| [core/mmcore.c](../core/mmcore.c) og [mmcore.h](../core/mmcore.h) | 276 + 105 linjer | Direkte: simulering, PRNG, innstillinger, tegnsett og instansformat |
| [Windows-renderer](../windows/renderer.cpp) og [header](../windows/renderer.h) | 579 + 120 linjer | Struktur, parametere og kamerabevegelse; GPU-kall og DirectXMath erstattes |
| [HLSL-shadere](../windows/shaders.hlsl) | 162 linjer | Formler og passinndeling oversettes til GLSL |
| [Metal-shadere](../Resources/Shaders.metal) | 144 linjer | Andre referanse for samme grafikk |
| [Windows-atlas](../windows/atlas.cpp) | 125 linjer | Atlasformat og speiling gjenbrukes; DirectWrite/Direct2D/WIC erstattes |
| [Windows-vert](../windows/host.cpp) | 285 linjer | Livssyklus som referanse; Win32-vinduer, meldinger og `.scr`-argumenter erstattes |
| [Windows-konfigurasjon](../windows/config.cpp) og [lagring](../windows/settings_store.cpp) | 204 + 84 linjer | Innstillingsmodellen gjenbrukes; dialog og registerlagring erstattes |
| [Swift-renderer](../Sources/Core/Renderer.swift), [atlas](../Sources/Core/GlyphAtlas.swift), øvrig macOS-vert | Plattformspesifikt | Visuell referanse, ikke en avhengighet i Linux-bygget |

Linjetallene inkluderer kommentarer og blanklinjer. De viser at kodebasen er liten,
men sier ikke at grafikk- og vertskoden kan kopieres uendret. Et rimelig størrelsesanslag
for nye Linux-filer er 1 500–3 000 linjer inklusive GLSL, bygg, integrasjon og tester,
uten en egen avansert innstillingsapp. Estimatet har lavere sikkerhet enn oversikten
over eksisterende kode.

Dokumentasjonen må leses sammen med koden:

* [PORTING.md](../PORTING.md) beskriver Windows-porten og er nyttig som visuell spesifikasjon.
* [core/README.md](../core/README.md) kaller fortsatt Windows-porten «planned», men den er implementert.
* README-omtalen av GLMatrix som bare en x86-64-binær må ikke brukes som en generell
  beskrivelse av Linux. Her finnes allerede både XScreenSavers `hacks/glx/glmatrix.c`
  og den bygde `/usr/local/libexec/xscreensaver/glmatrix`. Modern Matrix er interessant
  på grunn av utseende, bloom og den felles motoren, ikke fordi Linux mangler Matrix-effekter.

## Lokale kontroller og hva de viser

Arbeidskopien var ren før studien. Ingen applikasjonskode eller aktive
skjermsparerinnstillinger er endret som del av denne oppgaven.

### C-kjernen kjører allerede på Linux

`core/mmcore.c` ble kompilert med GCC som C99, med
`-Wall -Wextra -Wpedantic -Werror`. Deretter ble den eksisterende
`windows/_linktest.cpp` kompilert som C++20 og lenket mot C-objektet og `libm`.
Begge deler lyktes uten kildeendringer. Resultatet var blant annet:

```text
settings: density=0.42 speed=0.08 bloom=0.53 enc=0
derived : strips=413 fallSpeed=4.10 mutation=1.94
world   : halfWidth=38.0 topY=28.0 spacing=1.55 slotCount=38
matrix  : glyphCount=66 first=U+FF66 last=U+0039
sim     : emitted=209 instances after 2s (expect >0)
LINKTEST OK
```

En ekstra, midlertidig C-test kjørte alle seks tegnsett ved tetthet 0, 0,42 og 1:
18 scenarier, hvert med 600 tidssteg på 1/60 sekund. Testen kontrollerte at to
simuleringer med samme seed ga identiske instanser, at posisjon/lysstyrke var endelige
tall, at tegnindeksene var gyldige, og at en liten resultatbuffer ikke ble overskrevet.
Den prøvde også endring av tetthet, hastighet og antall tegn. Alle kontrollene besto.

Dette er en funksjonell røykprøve, ikke en full robusthets- eller ytelsestest.
Et forsøk med AddressSanitizer/UndefinedBehaviorSanitizer kunne ikke lenkes fordi
GCC-oppsettet mangler `/usr/lib64/libasan.so.8.0.0`; det er derfor **ikke** dokumentert
en bestått sanitizer-kjøring. Testfiler og binærer lå i midlertidige kataloger og ble fjernet.

### Grafikkstøtte på labtop

`glxinfo -B` rapporterte:

| Egenskap | Målt verdi |
| --- | --- |
| Operativsystem | AlmaLinux 10.2, nåværende X11/Window Maker-sesjon |
| GPU | Intel HD Graphics 620, Kaby Lake GT2 |
| Driver | Mesa 25.2.7 |
| Direkte rendering / akselerasjon | Ja / ja |
| OpenGL core / GLSL | 4.6 / 4.60 |
| OpenGL ES | 3.2 |
| Byggeverktøy | GCC 14.3.1 og CMake 3.31.8 |

`pkg-config` finner GL, EGL, X11, FreeType, Fontconfig, GTK 3, libepoxy og libpng.
GLFW og SDL2 ble ikke funnet via `pkg-config`. De er ikke nødvendige for anbefalt
X11/GLX-løsning. `pkg-config gl` sin modulversjon er ikke GPU-ens OpenGL-versjon.

Maskinen oppfyller dermed det foreslåtte API-nivået. Vi har ennå ikke kjørt en
Linux-port av rendereren, testet dens FP16-framebuffer eller målt faktisk FPS,
strømforbruk og temperatur.

### En konkret fontfelle

`fc-match ':charset=30a1'` velger her `DroidSansFallbackFull.ttf`. En kontroll med
FreeTypes `FT_Get_Char_Index` fant **142 av de 152 tegnene** i Unicode-modusen:
alle katakana-tegnene finnes, men sifrene `0`–`9` mangler. For ASCII-siffer velger
Fontconfig en annen font, Noto Sans.

Linux-atlaset må derfor sjekke faktisk dekning per kodepunkt og bruke en reservefont
ved behov. Det holder ikke å velge én «japansk font» som ble åpnet uten feil.
Det er en konkret integrasjonsoppgave, ikke bare en hypotetisk forskjell i fontutvalg.

## Foreslått teknisk løsning

```text
core/mmcore.c ── instanser og innstillinger ──> Linux/OpenGL-renderer
                                                    ↑
                                     FreeType + Fontconfig-atlas
                                                    ↓
                              X11/GLX-vert eller senere GTK-preview
                                                    ↓
                                  XScreenSavers tildelte X-vindu
```

**Førstevalg: Xlib + GLX + libepoxy.** Dette passer den eksisterende X11-sesjonen og
gir direkte kontroll over å tegne i et vindu som XScreenSaver eier. libepoxy håndterer
GL-funksjonsoppslag. Bruk CMake og `pkg-config`, og hold rendereren fri for X11-kall,
slik at den kan brukes fra en annen vert senere.

GTK 3 med `GtkGLArea` er et godt senere valg for et eget vindu med innstillinger og
live preview, men bør ikke være selve verten inne i XScreenSaver. GLFW/SDL er nyttige
for vanlige applikasjonsvinduer; støtte for den fremmede X-vindusflaten må uansett
avklares, så de gir ingen åpenbar gevinst for første leveranse. En større overgang
til Vulkan/wgpu eller en felles renderer for alle tre operativsystemene vil utvide
oppgaven betydelig og er ikke nødvendig for denne porten.

Foreslått struktur, **ikke implementert i denne studien**:

```text
Linux/
  study.md
  CMakeLists.txt
  src/main.cpp             # argumenter og valg av kjøremodus
  src/host_x11.cpp          # eget vindu / eksisterende XScreenSaver-vindu
  src/renderer_gl.cpp       # scene, instanser, FBO-er, bloom
  src/glyph_atlas.cpp       # FreeType + Fontconfig
  src/settings.cpp         # validering og kommandolinje
  shaders/                 # glyph.vert/.frag, fullscreen.vert, tre post-fragmenter
  data/modernmatrix.xml    # XScreenSaver-innstillinger
  tests/                   # kjerne, atlas, bilder og vertslivssyklus
```

### GPU-koden: lite omfang, men noen viktige konvensjoner

Regnet tegnes som instansierte, kameravendte firkanter. HLSL bruker
`StructuredBuffer<GlyphInstance>` og `SV_InstanceID`; Linux trenger ikke samme
bufferteknikk. `MMGlyphInstance` er nøyaktig 32 byte, og kan lastes som to `vec4`
vertex-attributter med divisor 1: posisjon/tegncelle og lysstyrke/RGB. Da holder
OpenGL 3.3 og `glDrawArraysInstanced`; vi trenger verken compute-shadere eller SSBO-er.
[Khronos beskriver divisor-funksjonen og tilgjengelighet fra OpenGL 3.3.](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Refpages/main/gl4/glVertexAttribDivisor.xml)

| Eksisterende teknikk | Linux/OpenGL |
| --- | --- |
| Dynamisk D3D-instansbuffer / Metal-buffer | VBO med streaming; start med buffer-orphaning og opplasting |
| `DrawInstanced(4, n)` | VAO og `glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, n)` |
| R8-tegntekstur | `GL_R8`, lineær filtrering og mipmaps |
| FP16 scene og to bloom-teksturer | FBO-er med `GL_RGBA16F` |
| Premultiplisert over-blending | `GL_ONE, GL_ONE_MINUS_SRC_ALPHA` |
| Present/vsync | GLX swap interval når støttet, med tidsstyrt reserve |
| PNG-snapshot via WIC/CoreGraphics | Offscreen-FBO, `glReadPixels`, libpng og korrekt radretning |

Shaderne er to vertex-funksjoner og fire fragment-funksjoner. Følgende må håndteres
eksplisitt, ellers kan en tilsynelatende vellykket oversettelse gi feil bilde:

* **Matriser og dybde:** Windows bruker radvektorer og DirectXMath, Swift bruker
  kolonnevektorer. Begge referanseprojeksjonene gir NDC-dybde `[0,1]`, mens OpenGL
  normalt bruker `[-1,1]`. Velg én kolonnematrise-konvensjon og en OpenGL-projeksjon;
  alternativt konverter `clip.z = 2*clip.z - clip.w`. `glClipControl` kan endre
  konvensjonen fra OpenGL 4.5, men bør ikke være et krav i en 3.3-port.
  [Khronos: klippevolum og dybdekonvensjoner.](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Refpages/main/gl4/glClipControl.xml)
* **Teksturretning:** atlaset har rad 0 øverst i referansekoden. OpenGL-FBO-er og
  bildeeksport må få en konsekvent UV-/radkonvensjon. Atlas- og postprosess-UV skal
  vurderes separat; ikke legg på en tilfeldig ekstra Y-speiling i alle passene.
* **Bufferlayout:** dagens uniforms er 144 byte med skalare felt. Dette kan beholdes
  ved tilsvarende skalarlayout, eller pakkes eksplisitt som `mat4` + fem `vec4` i
  `std140`. En omskriving til tettpakkede C++-`vec3` uten kontroll av GLSL-offset er feilutsatt.
* **Tegnrekkefølge:** behold rekkefølgen fra C-kjernen og samme blending. Ikke legg til
  depth writes eller sorter instansene uten en bevisst visuell endring; referanserendereren
  tegner scenen uten et tilkoblet dybdebuffer.
* **Farger:** ikke aktiver sRGB-konvertering tilfeldig. Avklar sluttbuffer og
  overføringsfunksjon mot SDR-referansebilder. macOS velger dessuten Display P3/EDR,
  så identiske RGB-tall er ikke automatisk identiske synlige farger mellom plattformene.

Kamera og seks panoreringsposisjoner kan oversettes nesten direkte fra
`Renderer::UpdateUniforms()` eller Swift `PanController`: 46° synsvinkel,
standardkamera `(0,0,48)`, ni sekunders segmenter og bevegelse i første 45 % av segmentet.

### Tegnatlas

Behold 16 kolonner, 72 × 72 piksler per celle og rekkefølgen fra
`mm_encoding_codepoints()`. Tegnsettstørrelsene er 66, 2, 16, 10, 4 og 152.
Hvert tegn sentreres etter glyfens faktiske utstrekning og speiles horisontalt
innenfor cellen, ikke ved å speile hele atlaset.

FreeType gir både bitmap og mål; Fontconfig finner skrifter med nødvendige tegn.
Vi må håndtere bitmapens pitch/radretning, kontrollere tegnindeks 0, og gi en synlig
reserveglyf hvis ingen font dekker et tegn. Kodepunktene er enkeltstående tegn, så
avansert tekstforming er ikke nødvendig i første atlasimplementasjon.
[FreeType beskriver kodepunktoppslag, glyph-loading og bitmap-koordinater.](https://freetype.org/freetype2/docs/tutorial/step1.html)

Ikke forvent pikselidentisk tekst mot Core Text eller DirectWrite med ulike fonter.
Sammenlign lesbarhet, sentrering, speiling og flimring. En medfølgende font kan gi mer
forutsigbart utseende, men krever et bevisst font- og lisensvalg.

### Bloom og HDR er to forskjellige leveranser

Bloom finnes allerede som en oversiktlig passkjede:

```text
FP16 scene → terskel 0,72 → halv oppløsning
           → horisontal blur → vertikal blur → gjenta én gang
           → scene + bloom × (intensitet × 1,6) → SDR-utgang
```

Det er sju draw-pass med bloom inkludert scenen. Gaussian-vektene og terskelen kan
oversettes direkte fra HLSL. Unngå å lese fra samme tekstur som det aktuelle FBO-et
skriver til; bruk de to eksisterende ping-pong-teksturene.

Et internt FP16-buffer bevarer sterke lysverdier til bloom også på en vanlig
SDR-skjerm. **Det innebærer ikke ekte HDR-presentasjon.** Windows-koden har en egen
DXGI/scRGB-bane, og macOS har EDR. Vi har ikke påvist en tilsvarende komplett
presentasjonsbane i Window Maker/X11-oppsettet, og første port bør derfor rapportere
SDR-utgang ærlig selv om intern rendering er FP16.

Ekte HDR bør få en egen mulighetsprøve med valgt skjerm, driver og compositor.
Wayland har protokoller for eksplisitt fargerom og dynamikkområde, men støtte må
avklares i den konkrete kombinasjonen; det løses ikke bare ved å oversette shaderne.
[Waylands egen beskrivelse av fargestyring og HDR.](https://wayland.freedesktop.org/docs/book/Color.html)

## XScreenSaver og vårt eksisterende skrivebord

Dette er den viktigste vertstilpasningen. XScreenSaver 6.16 dokumenterer at en effekt
skal tegne i vinduet gitt av **`XSCREENSAVER_WINDOW`**. Å åpne et eget fullscreen-vindu
oppå skrivebordet er ikke samme integrasjon. Kontrakten er kontrollert i den lokale
`~/src/xscreensaver-6.16/driver/xscreensaver.man` og `utils/vroot.h`; sistnevnte viser
også tolkning av vindus-ID i både heksadesimal og desimal form.

Den anbefalte Linux-verten skal:

1. Støtte et vanlig utviklingsvindu, et eksplisitt `--window-id`, og XScreenSavers
   `XSCREENSAVER_WINDOW`. Ved `--root` må vertens vindu prioriteres fremfor ekte X-root.
2. Lese mål-vinduets visual, skjerm og dimensjoner. Velge en kompatibel GLX-konfigurasjon
   og kontekst, ikke anta at en vilkårlig standard-visual kan brukes. Dette er et tidlig
   punkt for praktisk verifikasjon, også i XScreenSaver-preview.
3. Håndtere endret størrelse, ødelagt vertsvindu og avslutning. Frigjøre egne
   GL-ressurser uten å ødelegge et vindu som XScreenSaver eier.
4. La XScreenSaver styre input og låsing i innebygd modus. Windows-portens «avslutt ved
   tast/musebevegelse» skal ikke kopieres som låselogikk. Escape kan avslutte et separat
   utviklingsvindu.
5. Bruke monoton klokke og begrense `dt`, blant annet etter at effekten har vært
   stoppet og fortsatt med signaler. Unngå å simulere hele hvileperioden ved gjenopptakelse.
6. La XScreenSaver starte effekten for sine skjermer/vinduer, fremfor å lage alle
   skjermvinduer selv slik Windows-verten gjør. Preview, ulike sideforhold, skjermendring
   og forskjellig oppdateringsfrekvens må fortsatt testes.

Registrer en effektkommando som `GL: modernmatrix --root` og en XML-beskrivelse med
`gl="yes"`. Den eksisterende `hacks/config/glmatrix.xml` viser hvordan skyveknapper,
avkryssinger, valg av tegnsett og kommandoargumenter kobles til XScreenSaver-settings.
På denne maskinen bruker byggoppsettet `/usr/local/libexec/xscreensaver` for effekter
og `/usr/local/share/xscreensaver/config` for XML. Installasjonen må kontrollere faktisk
prefix; en distribusjonspakke kan bruke andre kataloger.

**Første versjon trenger ikke en egen GTK-app eller D-Bus-tjeneste.** La innstillingene
lagres som effektens kommandolinje i XScreenSaver. Bruk samme standardverdier fra
`mm_settings_default()` og la eksplisitte argumenter overstyre dem. Det unngår to
parallelle sett med innstillinger i JSON og `.xscreensaver`.

`wmmatrix`-dockappen, `wm-screensaver activate` og vår nye passorddialog kan fortsette
å fungere som nå. Modern Matrix bytter ut den visuelle effekten, og trenger ingen
ny Window Maker-patch eller endring i PAM.

## Ytelse, robusthet og risikopunkter

Maksimal tetthet er 900 kolonner × 38 plasser = **34 200 mulige instanser**. Med
32 byte per instans gir dette omtrent 1,04 MiB opplasting per bilde, eller 62,6 MiB/s
ved 60 FPS i et teoretisk fullt bilde. Faktisk antall synlige tegn er vanligvis lavere.
Dette er en mengdeberegning, ikke et målt båndbredde- eller FPS-resultat.

Én fulloppløst RGBA16F-scene og to halvoppløste bloom-buffere krever omtrent:

| Oppløsning | Bare de tre effektbufferne |
| --- | ---: |
| 1920 × 1080 | 23,7 MiB |
| 3840 × 2160 | 94,9 MiB |

Atlas, instansbuffere, driver, compositor og swapchain kommer i tillegg, og hver
skjermprosess kan få egne ressurser. Min vurdering er at fillrate og bloom ved høy
oppløsning er mer sannsynlige begrensninger på HD 620 enn antallet firkanter.
Det må måles. Start med halvoppløst bloom som originalen; vurder kvartoppløsning
eller valgfri FPS-grense hvis strømforbruket blir høyt. Ikke bruk en ubegrenset
busy-loop når swap interval mangler.

| Risiko | Konsekvens / tiltak |
| --- | --- |
| GLX-kontekst i vertens visual | Tidlig prøve både i preview og ved aktiv skjermsparer; tydelig feil hvis kombinasjonen ikke støttes |
| Fontdekning og tegnmål | Per-tegn reservefont er nødvendig allerede på labtop; bildeprøve av alle seks tegnsett |
| UV, projeksjon og farger | Faste referansebilder og målbare kamera-/atlasprøver før visuell finjustering |
| Resume, DPMS og endret størrelse | Gjenopprett FBO-er riktig, begrens tidssteg og test gjentatte sykluser |
| Ugyldig konfigurasjon / minnemangel | Valider endelige tall, intervaller og enum-verdier før kall til kjernen; kontroller alle nye GL/allokeringsresultater |
| Forskjellig PRNG-seed | macOS bruker tilfeldig seed, Windows bruker 0. Gi testverten eksplisitt `--seed` og fast `dt` for sammenlignbare bilder |
| Dokumentasjon av lisens | Ingen LICENSE/COPYING-fil ble funnet i denne revisjonen. Avklar prosjektets lisens med upstream før ekstern distribusjon; dette hindrer ikke selve studien |

C-kjernen har enkelte eksisterende forbedringspunkter: `malloc`/`calloc`-resultater
blir ikke gjennomgående sjekket, og grensesnittet forutsetter gyldige innstillinger.
Dessuten bruker den konvertering til `int` for `headSlot`, mens portingsdokumentet
beskriver `floor`; disse er forskjellige for negative verdier ved starten av et fall.
Linux bør først bruke den samme C-kjernen for konsistent oppførsel, og eventuelle
semantiske rettelser bør gjøres som separate endringer for alle plattformer.

## Arbeidspakker og avgrensninger

| Arbeid | Anslag, persondager |
| --- | ---: |
| CMake, avhengigheter, GL-kontekst og vanlig testvindu | 0,5–1 |
| FreeType-atlas, reservefonter, speiling og mipmaps | 1–2 |
| Instanser, GLSL, kamera, fog/waves og visuell grunnparitet | 2–3 |
| Bloom, SDR-farger og PNG-snapshot | 1–2 |
| XScreenSaver-vert, preview og livssyklus | 1–2 |
| Argumenter, innstillingsvalidering, XML og FPS-visning | 1–2 |
| Robusthet, flerskjerm, ytelse og regresjonskontroller | 2–3 |
| Installering, byggdokumentasjon og enkel CI | 0,5–1 |
| **Sum før reserve** | **9–16** |

Med omtrent 25 % reserve er et avrundet planleggingsrom **12–20 persondager** for
den stabile X11/SDR-utgaven. Intervallene forutsetter tilgang til målmaskinen,
at vi bruker eksisterende simuleringskode og at hovedfunksjonene ikke redesignes.

Mulige tillegg, utenfor summen:

* Egen GTK-konfigurator med live preview: omtrent **3–5 persondager**.
* RPM-pakke og kontroll på flere distribusjoner/GPU-er: omtrent **2–4 persondager**,
  avhengig av hvor mye ekstern testmaskinvare som er tilgjengelig.
* Native Wayland og ekte HDR: start med en egen **2–3 dagers mulighetsprøve**.
  Det er ikke et leveranseestimat for full støtte. Velg først compositor,
  skjermsparer-/låsevert, fargeprotokoll og støttet skjerm. Å tegne i et vanlig
  Wayland-vindu er en annen oppgave enn å erstatte en låst skjermsparer der.

## Anbefalt rekkefølge og kriterier for ferdig port

Start med en liten, tidsavgrenset prøve: opprett GLX-kontekst i et XScreenSaver-vindu,
tegn instansene fra `mmcore`, og bygg et atlas som dekker alle 152 kodepunktene.
Dette avklarer de to mest konkrete plattformrisikoene før vi bygger resten.

Deretter legg til GLSL-paritet, bloom og et deterministisk snapshot-grensesnitt.
Sammenlign etter tilstrekkelig oppvarming: simuleringen starter over skjermkanten,
og standardhastigheten er lav, så et nesten svart tidlig bilde er ikke nødvendigvis
en rendererfeil. Bruk samme seed, tidssteg, innstillinger, kameratilstand og atlas
ved en presis renderersammenligning; ulike systemfonter gir ellers forventede forskjeller.

Før den stabile utgaven bør følgende være dokumentert:

* Alle seks tegnsett, tetthet/hastighet, fog, waves, panning, wireframe, teksturvalg,
  bloom-intensitet og FPS-valg virker og lagres gjennom XScreenSaver.
* Preview og faktisk skjermsparer tegner i riktig vertsvindu, uten ekstra appikoner
  eller egne fullscreen-vinduer i innebygd modus.
* Låsen forblir aktiv hvis effekten avsluttes eller krasjer; rendererens feil skal
  ikke kunne tolkes som en vellykket passordkontroll.
* Gjentatte resize-, DPMS-, suspend/resume- og flerskjermforløp fungerer, uten
  vedvarende vekst i minnebruk eller en varm CPU i en busy-loop.
* 1080p på labtop måles med og uten bloom. Velg 60 FPS som første målepunkt,
  og dokumenter faktisk resultat fremfor å love skjermens maksimale frekvens.
* Minst Intel/Mesa på labtop og en separat programvarerenderer brukes i testene;
  støtte for andre GPU-er beskrives som testet først når det faktisk er gjort.
* Endringer i `core/` kontrolleres mot eksisterende macOS-/Windows-bygg når slike
  byggmiljøer er tilgjengelige. Hold første port mest mulig innenfor `Linux/`.

Studien har dermed verifisert at motoren og utviklingsmaskinen er et godt utgangspunkt.
Det som gjenstår før et sikrere estimat, er en liten praktisk OpenGL-/XScreenSaver-prototype;
ingen Linux-renderer er implementert eller installert som del av denne studien.
