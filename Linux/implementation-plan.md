# Implementasjonsplan: Matrix-Reflow for Linux

Dato: 11. september 2026. Status: iterasjon 1 er merget; iterasjon 3 er merget; I4-M1–M3 er passert (integrasjon følges i PR #4).
Grunnlag: [portstudien](study.md), upstream-revisjon
`2577d25f747d629a08e19cd5ad44c9e1b4609002` og brukerens ønskede iterasjonsrekkefølge.
Repository: `Hans-Einar/Matrix-Reflow`. Målplattform: AlmaLinux 10.2, X11,
Window Maker og XScreenSaver 6.16, først på labtop med Intel HD 620.

## Leveranserekkefølge

**Preview i XScreenSaver er en obligatorisk leveranse i iterasjon 2.** Den skal
vise bevegelig regn med prosjektets font gjennom den samme GLX-rendereren som
brukes videre. Vi utsetter bloom, CRT og visuell finjustering, men ikke integrasjonen
med vertsvinduet. Iterasjon 1 lager et fungerende fundament, ikke en separat
demomotor som senere erstattes.

| Iterasjon | Leveranse ved avslutning | Branch | PR-tittel |
| --- | --- | --- | --- |
| 1 | Linux-bygg, FreeType-atlas og GLX-renderer med statisk tegndemonstrasjon | `linux/01-build-atlas-glx` | `Linux 1: build system, font atlas and GLX renderer` |
| 2 | Animert regn som XScreenSaver-effekt, med fungerende preview | `linux/02-rain-xscreensaver` | `Linux 2: animated rain and XScreenSaver preview` |
| 3 | Bloom og ferdig SDR-komposisjon | `linux/03-bloom` | `Linux 3: bloom and SDR compositing` |
| 4 | Valgfritt CRT-pass | `linux/04-crt` | `Linux 4: optional CRT post-processing` |
| 5 | Full konfigurasjon, GTK-verktøy, profiler og ferdig installasjon | `linux/05-configuration` | `Linux 5: configuration tools and release integration` |

De fem iterasjonene kjøres sekvensielt. En iterasjon er ferdig når alle dens
milepæler er verifisert, tilhørende commits er pushet, og iterasjonens PR er merget
til vårt `main`. Arbeidet retter seg mot vårt repository, ikke en PR til Delmay441
eller det opprinnelige Modern Matrix-prosjektet.

## Branch-, commit- og PR-regler

1. Opprett iterasjonens branch fra oppdatert `origin/main` etter at forrige PR er
   merget. Åpne én draft-PR mot `Hans-Einar/Matrix-Reflow:main` etter første milepæl.
2. Hver milepæl nedenfor avsluttes med en egen, navngitt commit på iterasjonsbranchen.
   Committen inneholder implementasjon, relevant verifikasjon og dokumentasjon
   for akkurat den milepælen. Ikke lag en tom commit bare for å markere fremdrift.
3. Kryss av milepælen i planen i samme commit som leveransen. Registrer utførte
   kontroller og eventuelle begrensninger i `Linux/validation.md`; oppdater
   `Linux/CHANGELOG-WIP.md` med den brukerrelevante endringen.
4. Push etter hver passerte milepæl og oppdater PR-ens sjekkliste. Commit-ID og
   testresultat føres i PR-en etter commit, slik at dokumentet ikke trenger å
   inneholde hashen til sin egen commit.
5. Nødvendige rettelser mellom milepæler kan få egne commits. De skal ikke skjules
   ved å omskrive allerede publiserte milepælcommits. Ikke merk en milepæl ferdig
   hvis en obligatorisk kontroll fortsatt feiler eller ikke er utført.
6. Bruk **merge commit**, ikke squash, ved sammenslåing. Da bevares milepælene i
   historikken. PR-en oppsummerer sluttresultatet, tester og kjente begrensninger;
   den skal ikke bare vise en kronologisk arbeidslogg.
7. Neste iterasjon starter fra resultatet av denne sammenslåingen. Ikke stable
   alle fem branchene på uferdig kode eller opprette tomme PR-er på forhånd.

`study.md` og denne planen ble tatt med som grunnlagsdokumenter i I1-M1.
Fremtidige iterasjoner er fortsatt planlagt; bare verifiserte milepæler krysses av.

## Felles arkitektur og arbeidsmåte

### Én renderer gjennom alle iterasjoner

Grunnlaget er C99 for `core/mmcore.c` og C++17 for Linux-koden, med OpenGL 3.3,
GLSL 330, GLX, Xlib, libepoxy og FreeType. CMake bygger uten Windows-verktøy.
Alle Linux-spesifikke filer legges under `Linux/`; eksisterende C-kjerne gjenbrukes.
Delte kjernerettelser holdes små og forklares separat fra Linux-plattformkode.

Foreslått inndeling, som kan justeres innenfor milepælene:

| Område | Ansvar |
| --- | --- |
| `Linux/CMakeLists.txt`, `Linux/cmake/` | Avhengigheter, targets, tester, ressursbygging og installasjon |
| `Linux/src/main.cpp`, `options.*` | Argumenter, valg av vindusmodus, programlivsløp |
| `Linux/src/x11_host.*`, `glx_context.*` | Eget eller lånt vindu, riktig visual, context og hendelser |
| `Linux/src/font_atlas.*`, `glyph_table.*` | Privat font, rasterisering og eksakt tabell med 57 tegn |
| `Linux/src/renderer.*`, `gl_resources.*` | Instancing, GPU-ressurser, resize og tegning |
| `Linux/src/simulation.*` | Fast tidssteg, resttid, klokke og kamerarebasering |
| `Linux/src/postprocess.*`, `Linux/shaders/` | Komposisjon fra I1, bloom fra I3, CRT fra I4 |
| `Linux/src/settings.*`, `Linux/config-ui/` | Felles validering og konfigurasjon; GTK-app fra I5 |
| `Linux/xscreensaver/` | XML og hjelp til registrering fra I2 |
| `Linux/tests/` | Målrettede CPU-tester og grafiske kontrollscener |
| `Linux/README.md`, `validation.md`, `CHANGELOG-WIP.md` | Bygg, bruk, verifikasjon og kommende endringer |

Scene tegnes til RGBA16F allerede fra I1, med et enkelt fullskjermpass til et
opakt sluttbilde. I2 bruker samme pass uten effekter. I3 utvider komposisjonen med
bloom, og I4 kan sende resultatet gjennom CRT. Ingen GTK/Cairo-gjengivelse av regnet,
alternativ preview-shader eller egen forenklet XScreenSaver-renderer introduseres.

Fonten er allerede laget: det som må bygges er FreeType-rasteriseringen og
GPU-atlaset. Linux-bygget bruker eksisterende `windows/Matrix-Code.ttf` som én
kilde og genererer innebygde ressursdata i byggkatalogen. Vi lager ikke en ny font,
installerer den globalt eller kopierer inn enda en manuelt vedlikeholdt byte-array.
Shaderressurser bygges tilsvarende slik at installert program ikke avhenger av cwd.

### Vindusmodi og ansvar

Planlagt programnavn er `matrix-reflow`. Eget testvindu og XScreenSaver-modus
bruker samme binærfil og renderer. Plattformlaget skiller eksplisitt mellom
vinduer det eier og vinduer det låner. Et lånt vindu skal ikke lukkes, flyttes,
reparentes eller overtas som et nytt fullscreen-vindu.

Planlagt oppstartsgrensesnitt:

* `--windowed`: eget utviklings-/previewvindu.
* `--window-id ID`: eksplisitt lånt X11-vindu; aksepter desimal og `0x`-heksadesimal.
* `--root`: XScreenSaver-kompatibel modus som prioriterer `XSCREENSAVER_WINDOW`.
  Manglende eller ugyldig vertsvindu gir en tydelig feil fremfor å tegne på den
  virkelige desktop-roten. Dette kravet dokumenteres i hjelpen.
* Uten eksplisitt modus: bruk `XSCREENSAVER_WINDOW` dersom den er satt, ellers eget
  vindu. `--root --window-id ID` tillates fordi XScreenSaver legger til vindus-ID ved
  preview. Eksplisitt vindus-ID går foran miljøvariabelen; `--windowed` kan ikke
  kombineres med vertsvindusflagg og ignorerer miljøvariabelen.
* `--help`, `--version`, `--seed`, `--fps-limit` og nødvendige renderingsvalg.

XScreenSaver eier aktivering, inputhåndtering og skjermlås. Porten berører ikke
passorddialog, PAM eller innlogging. Utvikling og grafisk kontroll skjer først i
eget vindu, deretter preview; det er ikke nødvendig å låse den aktive sesjonen
eller restarte Window Maker for å passere milepælene.

### Verifikasjon og dokumentasjon

CPU-tester skal dekke reelle feilrisikoer: atlasindekser, parametergrenser,
resttid, kapasitet og konfigurasjon. Grafiske kontroller skal bruke samme renderer
som programmet. Automatiske tester uten skjerm kjøres separat fra GLX-tester.
En CI-maskin uten GLX er ikke bevis på at XScreenSaver-preview fungerer.

Bruk fast seed og kontrollert simuleringstid ved bildesammenligning. Referansebilder
må være fra Reflow-revisjonen, ikke de arvede bildene som studien fant. Mangler en
Windows-referanse, registreres det uttrykkelig: funksjonell milepæl kan være ferdig
uten at pixel- eller utseendeparitet hevdes. Visuell paritet rapporteres separat.

Byggkontrollene utvikles fra I1, for eksempel:

```sh
cmake -S Linux -B build/linux -DCMAKE_BUILD_TYPE=Debug
cmake --build build/linux --parallel
ctest --test-dir build/linux --output-on-failure
```

Byggkommandoene er implementert i iterasjon 1. Se `README.md` for grafiske tester.

## Iterasjon 1 — Bygg, fontatlas og GLX-motor

Branch: `linux/01-build-atlas-glx`. Avhengighet: dagens upstream-kode.
Sluttresultat: et byggbart Linux-program som viser de faktiske fonttegnene
statisk i et eget GLX-vindu gjennom den permanente rendereren.

### [x] I1-M1 — Reproduserbart Linux-bygg

Leveranse: CMake-target for C-kjernen og Linux-binæren, C99/C++17, eksplisitt
avhengighetssøk og ryddig Debug/Release-bygg. Legg til grunnlagsdokumentene,
kort byggeveiledning, WIP-notater og første CPU-røykprøve med 57 tegn.
Genererte filer og byggprodukter holdes utenfor Git. Verifiser nødvendige
pakkenavn mot maskinens pakkekilder ved implementasjon.

Bestått når: ren byggkatalog gir et vellykket bygg uten Windows-SDK, den faktiske
C-kjernen er lenket inn, testen gir instanser, og manglende avhengigheter gir
forståelige CMake-feil. Dokumenter installerte versjoner.

Commit: `build(linux): I1-M1 add CMake build and core smoke test`.

### [x] I1-M2 — FreeType-atlas fra originalfonten

Leveranse: ressursinnbygging, fontlasting fra minne og rasterisering til én
8-bits dekningsflate. Behold 57-tegnstabellens rekkefølge, indeks 3/4/12 og
uspeilede tegn. Standardatlas er 16 kolonner med 72-pikslers celler, 1152 × 288.
Håndter FreeType-bitmapenes pitch, metrikker og levetiden til fontdata.

Bestått når: alle 57 tegn har gyldig glyph-ID og ikke-tom rasterdekning, celler
ligger innenfor atlaset, og tegn ikke klippes eller blør over i naboens celle.
En atlasdump kan undersøkes uten at fonten er installert på systemet.
Test også manglende/ødelagte fontdata med en kontrollert feilbane.

Commit: `feat(linux): I1-M2 rasterize the embedded Matrix font with FreeType`.

### [x] I1-M3 — GLX-context og permanente GPU-ressurser

Leveranse: X11-testvindu, GL 3.3-context, shaderkompilering med gode feillogger,
RAII for GPU-ressurser, scene-FBO og opakt komposisjonspass. Legg grunnlaget for
lånte vinduer ved å skille vindusoppretting fra context- og rendereroppretting.
Resize håndterer viewport og FBO; null størrelse tegnes ikke.

Bestått når: vinduet tegner en kontrollscene, tåler gjentatt resize og lukking,
og frigjør egne ressurser. Bekreft FBO-status og faktisk GL-versjon. En ugyldig
shader eller manglende context gir en tydelig feil, ikke et stille sort vindu.

Commit: `feat(linux): I1-M3 add the shared GLX renderer and scene targets`.

### [x] I1-M4 — Tegn gjennom samme instancing som regnet skal bruke

Leveranse: GL_R8-atlas på GPU, glyph-shader og instansbuffer med stride 40 og
kontrollerte attributtoffsets. En statisk kontrollscene mater rendereren med
`MMGlyphInstance`, viser alle tegn og eksempler på X/Y-speiling og farger.
Ingen dynamisk simulering, bloom eller CRT kreves ennå.

Bestått når: riktige fonttegn vises i GLX-vinduet, inkludert `0`, `1` og kolon;
speiling skjer én gang; orientering, dekning og sluttalfa er riktige. Scenen
bruker nøyaktig samme atlas, glyph-shader og draw-funksjon som I2 skal bruke.
Legg kontrollbilde og verifikasjonsbeskrivelse til iterasjonens PR.

Commit: `feat(linux): I1-M4 render Matrix glyph instances in the GLX window`.

PR ferdig når: I1-M1–M4 er passert og programmet kan bygges og demonstreres fra
instruksjonene i `Linux/README.md`.

## Iterasjon 2 — Bevegelig regn og XScreenSaver-preview

Branch: `linux/02-rain-xscreensaver`. Avhengighet: merget iterasjon 1.
Sluttresultat: brukbar skjermsparer og faktisk preview i `xscreensaver-settings`.
Utseendet kan være enkelt, men tegn, animasjon og vertsintegrasjon skal fungere.

### [x] I2-M1 — Koble simuleringen til rendereren

Leveranse: fast steg 1/60 sekund, oppsamlet tid begrenset til 0,1 sekund og
resttid i sekunder til `mm_sim_write_instances`. Implementer projeksjon, kamera,
lokal klokke og samordnet rebasering ved 72 enheter. Bruk kapasitet fra kjernen
og bevar instansenes dybdesortering. Allokeringsfeil og store/ugyldige innstillinger
skal fanges før de gir ukontrollert minnebruk; nødvendige kjernerettelser inngår.

Bestått når: fontregnet beveger seg sammenhengende i eget vindu uten effekter,
og simulasjonshastigheten ikke avhenger av render-FPS. CPU-test dekker samme seed,
gyldige instanser, kapasitetsgrenser, innstillingsendring og kamerarebasering.
Kontroller 16:9, portrett og ultrawide, samt dybde null og større enn null.

Commit: `feat(linux): I2-M1 animate the shared rain simulation`.

### [x] I2-M2 — Tegn i et lånt X11-vindu

Leveranse: `--window-id`, `--root` og miljøvariabelen etter kontrakten over.
Hent vertsvinduets faktiske screen/visual/depth og velg kompatibel GLX-FBConfig.
Kontroller double buffering og bruk passende presentasjon for drawabletypen.
Renderer og simulasjon gjenbrukes uendret fra eget vindu.

Bestått når: en enkel testvert kan gi programmet et vindu som viser regnet,
endre størrelsen og ødelegge vinduet uten krasj eller at andre vinduer påvirkes.
Ugyldig XID og inkompatibel visual håndteres. Testen skal ikke «bestå» ved å
åpne et separat vindu dersom embedding feiler.

Commit: `feat(linux): I2-M2 render into XScreenSaver-compatible host windows`.

### [x] I2-M3 — Registrer effekten og få ekte XScreenSaver-preview

Leveranse: minimal `matrix-reflow.xml` med `gl="yes"`, riktig oppstartskommando,
CMake-installasjon av binær/XML og dokumentert registrering i programs-listen.
Bruk lokal `glmatrix.xml` og XScreenSavers visual-regler som referanse. Et lite
sett funksjonelle kontroller, som hastighet og tetthet, er nok nå.
Eventuell registreringshjelper skal være idempotent og bevare øvrige oppføringer.

Bestått når: **Matrix Reflow kan velges i `xscreensaver-settings`, og det faktiske
innebygde previewet viser bevegelige fonttegn.** Bytt til en annen effekt og tilbake;
ingen foreldreløse rendererprosesser eller ekstra appvinduer skal bli igjen.
Bekreft også XScreenSavers utvidede preview uten å aktivere skjermlås.
Dokumenter hvilken installasjon som ble brukt og hvordan oppføringen fjernes.

Commit: `feat(linux): I2-M3 integrate XScreenSaver registration and preview`.

### [x] I2-M4 — Stabiliser første kjørbare skjermsparer

Leveranse: ryddig SIGTERM/vindusavslutning, pausehåndtering, frame pacing,
grunnleggende `--help` og feilmeldinger. Valider grenser for rendererens CLI-valg.
Mål CPU, frame-tid og minne med effekter av som referanse for I3/I4.
Dokumenter foreløpige forskjeller i utseende og hvilke funksjoner som er utsatt.

Brukerjustering 11. september: 30-minutterstesten utgår fordi maskinen kjører
på batteri. Bruk korte kontroller videre; ikke start langtesten på nytt.

Bestått når: kort vinduskjøring, minst 20 preview-start/stopp/bytter og gjentatt
resize er kontrollert uten krasj eller voksende prosessantall. Langtidsstabilitet
og vedvarende minnevekst er ikke verifisert av disse korte kontrollene. Kontroller håndtering av en syntetisk lang tidsluke.
Test flere skjermformater nå; fysisk flerskjerm rapporteres som utestet dersom
utstyret ikke er tilgjengelig. Simulert geometri skal ikke kalles flerskjermtest.

Commit: `fix(linux): I2-M4 harden preview lifecycle and frame pacing`.

PR ferdig når: I2-M1–M4 er passert. En skjermdump fra et vanlig eget vindu er
ikke tilstrekkelig dokumentasjon for preview-milepælen.

## Iterasjon 3 — Bloom og SDR-komposisjon

Branch: `linux/03-bloom`. Avhengighet: merget iterasjon 2.
Sluttresultat: Reflows bloom-kjede i både eget vindu og XScreenSaver-preview.

Batterihensyn (brukerens instruksjon): bygg med høyst to jobber og bruk korte,
avgrensede funksjons-/ytelsesmålinger. Ingen langtester i denne økten.

### [x] I3-M1 — Terskel og fem nivåer med nedskalering

Leveranse: GLSL for terskeluttrekk og 13-taps nedskalering, RGBA16F-mål fra 1/2
til 1/32 oppløsning og riktig gjenoppretting ved resize. Hver dimensjon er minst
én piksel, også i svært små previewvinduer. Diagnostikk kan vise hvert nivå.

Bestått når: en kontrollscene med lyst tegn/punkt gir forventet terskeluttrekk
og avtagende detaljering gjennom alle fem nivåer. Ingen ugyldige FBO-er eller
lesing og skriving av samme tekstur i ett pass. Regnet uten bloom er uendret.

Commit: `feat(linux): I3-M1 add bloom extraction and downsampling`.

### [x] I3-M2 — Additiv oppskalering og komposisjon

Leveranse: 9-taps oppskalering som akkumulerer i eksisterende større nivå,
bloomstyrke og sluttkomposisjon. Oversett også barrel distortion, kromatisk
avvik, vignett, kantfade og oppstartsfade slik referansen bruker dem.
Sluttbildet forblir opakt; tegnblandingen og bloom-blandingen holdes adskilt.

Bestått når: effekten kan slås av/på og styrken endres, uten at regnet forsvinner
ved styrke null. Bekreft at oppskalering ikke mister nivåenes eksisterende bidrag.
Kontroller fargerom/gamma og hjørner uten strekkstriper. Ingen skjult endring av
den inaktive `whiteFlash`-termen gjøres som del av shaderoversettelsen.

Commit: `feat(linux): I3-M2 composite multiscale bloom into the rain scene`.

### [x] I3-M3 — Visuell kontroll og målt kostnad

Leveranse: sammenlignbare bilder ved fast seed/tid, frame-tidsmålinger mot I2
og eventuell eksplisitt bloom-kvalitetsinnstilling dersom målingene krever det.
Oppdater preview-XML med bloom av/på og styrke; full GUI venter til I5.

Bestått når: preview og eget vindu viser samme effekt med samme innstillinger;
bloom av/på, resize og små vinduer fungerer med compositor. Rapportér CPU/GPU-
tid der måleverktøy støttes, ikke bare gjennomsnittlig FPS. Registrer 1080p og
faktisk skjermoppløsning; 4K er en separat måling hvis tilgjengelig.
Kvalitetsgrenser bestemmes av resultatene, ikke et ubekreftet løfte om 60 FPS.

Commit: `perf(linux): I3-M3 validate bloom quality and rendering cost`.

PR ferdig når: I3-M1–M3 er passert og I2s preview-kontroller fortsatt består.

## Iterasjon 4 — CRT-pass

Branch: `linux/04-crt`. Avhengighet: merget iterasjon 3.
Sluttresultat: valgfri CRT-behandling etter komposisjon, av som standard slik
dagens Reflow-standard tilsier. CRT-forvrengning og CRT-emulering er ulike valg.
Batterihensyn videreføres: høyst to byggejobber og korte, avgrensede tester.

### [x] I4-M1 — Valgfritt mål og pass etter komposisjon

Leveranse: CRT-mellommål opprettes bare når det trengs. Tegnebanen velger direkte
sluttkomposisjon når CRT er av, ellers komposisjon til mellomtekstur og ett CRT-pass.
Legg til bryter i CLI og minimal preview-XML.

Bestått når: et foreløpig identitetspass gir samme synlige bilde som bypass,
gjentatt av/på og resize ikke lekker ressurser, og deaktivert CRT ikke etterlater
en ubrukt fullstor CRT-tekstur. HDR-sceneformat skal ikke forveksles med skjerm-HDR.

Commit: `feat(linux): I4-M1 add optional CRT render target and bypass`.

### [x] I4-M2 — Oversett CRT-filteret

Leveranse: scanlines, RGB-maskemønster, horisontal utjevning, kanalavvik,
sortnivå, høylysbehandling og vignett fra HLSL. Bruk fysiske outputpiksler og en
eksplisitt konvensjon for y-retning. Ingen historikkbuffer eller ny etterglød.

Bestått når: mønsteret ligger stabilt i skjermpikslene og ikke driver med kamera,
UV eller vindusstørrelse. Kontroller både lite preview og stort vindu, farger,
scanline-retning og kombinasjonen med den separate forvrengningsparameteren.

Commit: `feat(linux): I4-M2 port the CRT filter to GLSL`.

### [x] I4-M3 — Kombinasjoner og regresjonskontroll

Leveranse: dokumenterte resultater for bloom/CRT av-av, på-av, av-på og på-på,
inkludert ressursbruk og tillegg i frame-tid. Oppdater WIP-notater og bruksveiledning.

Bestått når: alle fire kombinasjoner virker i ekte XScreenSaver-preview og eget
vindu, også etter resize. Alpha gir aldri utilsiktet gjennomsiktighet. CRT av
bevarer resultatet fra I3. Bildene fra kontrollscenene er vurdert visuelt.

Commit: `test(linux): I4-M3 verify CRT and bloom combinations`.

PR ferdig når: I4-M1–M3 er passert og preview fortsatt er stabilt.

## Iterasjon 5 — Konfigurasjonsverktøy og ferdig leveranse

Branch: `linux/05-configuration`. Avhengighet: merget iterasjon 4.
Sluttresultat: full CLI/XML-konfigurasjon og en enkel GTK-app med fargevalg og
fem brukerprofiler. GTK-verktøyet er et valgfritt byggtarget; rendereren trenger
ikke GTK for å kjøre. Denne planen inkluderer GTK-arbeidet som studien estimerte separat.

### [ ] I5-M1 — Felles innstillingsmodell og lagring

Leveranse: én modell med typer, standarder, gyldige intervaller og mapping til
`MMSettings`, renderer og vert. Bruk prosjektets standarder, med midlertidige
effektbegrensninger fra I2 fjernet. Ikke presenter en virksom `waves`-bryter før
funksjonen faktisk finnes; dokumenter eksisterende shader-/kjerneavvik.

Planlagt lagring er versjonert INI via GLib `GKeyFile` under
`$XDG_CONFIG_HOME/matrix-reflow/settings.ini`, med vanlig XDG-fallback. GLib legges
til som felles parseravhengighet her; GTK er fortsatt kun for GUI-targetet.
Lagre atomisk og behold tidligere gyldig fil ved feil. Rekkefølgen er:
innebygde standarder → valgt lagret profil → eksplisitte CLI-overstyringer.

Bestått når: lagring/lasting bevarer verdier og farger, ugyldige intervaller,
NaN/inf og ukjent fremtidig filversjon håndteres forståelig. Manglende konfigfil
bruker standarder. CLI-overstyringer endrer ikke lagret profil. Legg til
`--print-effective-settings` for diagnostikk av verdiene som faktisk brukes.

Commit: `feat(linux): I5-M1 add validated settings and versioned profiles`.

### [ ] I5-M2 — Full CLI og XScreenSaver-innstillinger

Leveranse: fullfør `--help` og XML-kontroller for de innstillingene XScreenSavers
GUI kan uttrykke godt. Kartlegg hastighet, tetthet, størrelse, lengde, dybde,
kamera, mutasjon, farger, speiling, binærmodus, hull, hoder, bloom, CRT og FPS-tak.
Dokumenter hva som ligger i GTK-verktøyet og hvilke valg som er diagnostikk.

Bestått når: UI-grenser og standarder samsvarer med parseren, og kommandoene
XML genererer kan parses og gi forventet preview. Velg én dokumentert modus for
profilbasert oppstart og én for eksplisitte XScreenSaver-argumenter. Vis tydelig
at eksplisitte argumenter overstyrer profilen; ingen innstilling skal tilsynelatende
bli ignorert uten at den effektive konfigurasjonen forklarer hvorfor.

Commit: `feat(linux): I5-M2 complete CLI and XScreenSaver configuration`.

### [ ] I5-M3 — GTK-verktøy med fem profiler og fargevalg

Leveranse: `matrix-reflow-settings` som GTK 3-app, felles validering/lagring,
oversiktlige innstillingsgrupper, fargevelgere, fem navngitte brukerprofiler,
lagre/tilbakestille og en knapp for preview. Ikke lag en egen renderer i GUI-en:
preview starter `matrix-reflow --windowed` med en snapshot av valgene gjennom
argumentliste uten shell. Ulagrede valg kan prøves uten å overskrive brukerprofilen.

Bestått når: GUI og CLI gir identiske effektive verdier, profiler bevares etter
omstart, og endring av profil/farge vises i preview. Å lukke innstillingsappen
rydder opp dens egne previewprosesser og påvirker ikke XScreenSavers prosess.
Renderer-only-bygg og normal skjermsparerstart fungerer uten GTK installert.

Commit: `feat(linux): I5-M3 add GTK settings with color controls and profiles`.

### [ ] I5-M4 — Installasjon, oppgradering og endelig verifikasjon

Leveranse: konfigurerbare installasjonsstier, `DESTDIR`-staging, desktopfil for
GTK-verktøyet og dokumentert registrering/fjerning av XScreenSaver-oppføringen.
Verifiser start utenfor kildekatalogen. Fullfør README, funksjonsoversikt,
WIP-release notes og listen over kjente forskjeller fra Windows.

Bestått når: ren Release-bygging og staging virker, oppgradering bevarer profiler
og andre skjermsparere, og både installert GUI og effekt finner sine ressurser.
Gjenta den funksjonelle matrisen fra I2–I4 på sluttresultatet. Kjør minst én
totimers vindus-/previewøkt med normal bruk og gjennomfør tilgjengelig flerskjerm-
og suspend/resume-kontroll; noter maskinvaredekning presist.
Avklar opphavs-/lisensspørsmål fra studien før en redistribuerbar binærpakke;
RPM, publisering og en release-tag er ikke nødvendige for å merge kode-PR-en.

Commit: `build(linux): I5-M4 finish installation and release documentation`.

PR ferdig når: I5-M1–M4 er passert. Stabilitets- og visuelle begrensninger som
gjenstår er konkret dokumentert; ikke beskriv ubekreftet Windows-paritet som oppnådd.

## Innsats og kobling til studiens arbeidspakker

Estimatene under er arbeidstid for én erfaren utvikler, før reserve. De inkluderer
milepælenes dokumentasjon og målrettede tester; det legges ikke til en skjult
sjette iterasjon for alt integrasjonsarbeidet.

| Iterasjon | Arbeidspakker fra studien | Persondager |
| --- | --- | ---: |
| 1 | CMake/GL-context, fontatlas og første instancing | 2–3 |
| 2 | Simuleringsdriver, resterende tegnrendering, XScreenSaver/preview og stabilisering | 5–8 |
| 3 | Bloom, komposisjon, visuell kontroll og ytelse | 3–5 |
| 4 | CRT, kombinasjonskontroll og ressursbruk | 2–3 |
| 5, CLI/XML/installasjon | Konfigurasjon, sluttkontroll og dokumentasjon | 2–5 |
| 5, GTK/profiler i tillegg | Separat GUI-arbeid omtalt i studien | 5–8 |
| **Sum med GTK** | | **19–32** |

Med omtrent 25 prosent reserve er planrammen **24–40 persondager**, omtrent
180–300 timer. Uten GTK-tillegget samsvarer rammen med studiens 18–30 dager.
Forskjellen skyldes at denne planen konkret inkluderer GTK-verktøy og profiler.
Intervallene per iterasjon summeres; de er ikke kumulative slik milepælanslagene
i studiens første tabell var.

Største usikkerheter er GLX-visual i XScreenSavers faktiske preview, sammenligning
med Windows og ytelse på integrert GPU. GLX-risikoen tas ut i I2, før bloom og CRT.
Hvis en risiko slår til, løses eller dokumenteres den på den aktuelle branchen;
preview flyttes ikke stille til I5 for å holde kalenderen.

Native Wayland, ekte skjerm-HDR, ny tidsbasert fosforetterglød og endringer i
skjermlås/autentisering ligger utenfor denne planen. De fem iterasjonene leverer
Linux/X11-porten og konfigurasjonen beskrevet over.
