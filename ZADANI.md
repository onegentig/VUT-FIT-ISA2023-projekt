# TFTP Klient + Server #

Vedoucí: [Ing. Daniel Dolejška](https://www.vut.cz/lide/daniel-dolejska-196165)

## Popis ##

Vašim úkolem je implementovat klientskou a serverovou aplikaci pro přenos souborů prostřednictvím TFTP (Trivial File Transfer Protocol) a to přesně dle [korespondující RFC specifikace daného protokolu](https://datatracker.ietf.org/doc/html/rfc1350).

Výsledná řešení musí dále být v souladu s následujícími rozšířeními základní specifikace protokolu TFTP:

- [TFTP Option Extension](https://datatracker.ietf.org/doc/html/rfc2347)
- [TFTP Blocksize Option](https://datatracker.ietf.org/doc/html/rfc2348)
- [TFTP Timeout Interval and Transfer Size Options](https://datatracker.ietf.org/doc/html/rfc2349)

### Další požadavky implementační řešení ###

- Implementace v jazyce C či C++
- Bez použití nestandardních/externích knihoven
- Funkční v referenčním vývojovém prostředí (dle [návodu](https://git.fit.vutbr.cz/NESFIT/dev-envs), sekce ISA)
- Libovolná (ovšem vhodná) adresářová struktura i umístění a počty souborů (složky src, include, obj, bin apod.)
- Dále se očekává souhlad s [všeobecnými podmínkami k vypracovávání projektů v předmětu ISA](https://moodle.vut.cz/course/view.php?id=268266#section-3)

<details>

<summary>📝 Všeobečné požadavky na vypracovávání projektů v předmětu ISA</summary><br />

Vytvořte komunikující aplikaci podle konkrétní vybrané specifikace pomocí síťové knihovny BSD sockets (pokud není ve variantě zadání uvedeno jinak). Projekt bude vypracován v jazyce C/C++. Pokud individuální zadání nespecifikuje vlastní referenční systém, musí být projekt přeložitelný a spustitelný na serveru `merlin.fit.vutbr.cz` pod operačním systémem GNU/Linux. Program bude přenositelný. Hodnocení projektů může probíhat na jiném počítači s nainstalovaným OS GNU/Linux, včetně jiných architektur než Intel/AMD, distribucí či verzí knihoven. Pokud vyžadujete minimální verzi knihovny (dostupnou na serveru merlin), jasně tuto skutečnost označte v dokumentaci a README.

- Vypracovaný projekt uložený v archívu .tar a se jménem `xlogin00.tar` odevzdejte elektronicky přes IS VUT. Soubor nekomprimujte.
- Termín odevzdání je **20.11.2023 (hard deadline)**. Odevzdání e-mailem po uplynutí termínu, dodatečné opravy či doplnění kódu není možné.
- Odevzdaný projekt musí obsahovat:
     1. soubor se zdrojovým kódem - dodržujte jména souborů uvedená v konkrétním zadání, v záhlaví každého zdrojového souboru by mělo být jméno autora a login,
     2. funkční `Makefile` pro překlad zdrojového souboru,
     3. dokumentaci ve formátu PDF (soubor `manual.pdf`), která bude obsahovat uvedení do problematiky, návrhu aplikace, popis implementace, základní informace o programu, návod na použití. Struktura dokumentace odpovídá technické zprávě a měla by obsahovat následující části: titulní stranu, obsah, logické strukturování textu včetně číslování kapitol, přehled nastudovaných informací z literatury, popis zajímavějších pasáží implementace, použití vytvořených programů a literatura. Pro dokumentaci je možné použít upravenou šablonu pro [bakalářské práce](https://www.fit.vut.cz/study/theses/bachelor-theses/.cs),
     4. soubor `README` obsahující jméno a login autora, datum vytvoření, krátký textový popis programu s případnými rozšířeními či omezeními, příklad spuštění a seznam odevzdaných souborů,
     5. další požadované soubory podle konkrétního typu zadání.
- Pokud v projektu nestihnete implementovat všechny požadované vlastnosti, je nutné veškerá omezení jasně uvést v dokumentaci a v souboru README.
- Co není v zadání jednoznačně uvedeno, můžete implementovat podle vlastního uvážení. Zvolené řešení popište v dokumentaci.
- Při řešení projektu respektujte zvyklosti zavedené v OS unixového typu (jako je například formát textového souboru).
- Vytvořené programy by měly být použitelné a smysluplné, řádně okomentované, formátované a členěné do funkcí a modulů. Program by měl obsahovat nápovědu informující uživatele o činnosti programu a očekávaných parametrech. V případě neočekávaného vstupu by měl  vypsat chybové hlášení, případně help.
- Aplikace nesmí v žádném případě skončit s chybou SEGMENTATION FAULT ani jiným násilným systémovým ukončením (např. při dělení nulou).
- Pokud přejímáte krátké pasáže zdrojových kódů z různých tutoriálů či příkladů z Internetu (ne mezi sebou), tak je nutné vyznačit tyto sekce a jejich autory dle licenčních podmínek, kterými se distribuce daných zdrojových kódů řídí. V případě nedodržení bude na projekt nahlíženo jako na plagiát.
- Za plagiát se považuje i kód, který byl vygenerován externím nástrojem a kde student není autorem kódu ve smyslu autorského zákona.
- Konzultace k projektu podává vyučující, který zadání vypsal. Pro své otázky můžete využít diskuzní fórum k projektům.
- Sledujte fórum k projektu, kde se může objevit dovysvětlení či upřesnění zadání.
- Před odevzdáním zkontrolujte, zda projekt obsahuje všechny potřebné soubory a také jste dodrželi jména odevzdávaných souborů pro konkrétní zadání. Zkontrolujte před odevzdáním, zda je projekt přeložitelný na cílové platformě.

----------------------------------------------

</details>

### Rozhraní aplikace ###

Následující sekce pojednávají o požadovaném rozhraní obou implementovaných aplikací.

#### Překlad ####

Kompletní překlad aplikace musí proběhnout prostřednictvím příkazu `make all` (t.j. `make`). Výsledkem takového překladu budou dva spustitelné soubory `tftp-client`, `tftp-server`.

#### Spuštění ####

Je očekáváno, že se program ukončí s chybovou hláškou v případě, že jsou mu předány nevalidní/nekompletní argumenty. Program nesmí v žádném případě havarovat.

```sh
tftp-client -h hostname [-p port] [-f filepath] -t dest_filepath
```

- `-h` IP adresa/doménový název vzdáleného serveru
- `-p` port vzdáleného serveru
  - pokud není specifikován předpokládá se výchozí dle specifikace
- `-f` cesta ke stahovanému souboru na serveru (download)
  - pokud není specifikován používá se obsah stdin (upload)
- `-t` cesta, pod kterou bude soubor na vzdáleném serveru/lokálně uložen

```sh
tftp-server [-p port] root_dirpath
```

- `-p` místní port, na kterém bude server očekávat příchozí spojení
- cesta k adresáři, pod kterým se budou ukládat příchozí soubory

#### Výstup ####

Obsah standardního výstupu (stdin) nebude předmětem hodnocení. Doporučuji vypisovat vlastní logovací/debugovací zprávy a libovolné další informace pro uživatele (umožní po hodnocení lépe pochopit co se s programem skutečně dělo). Výpisy můžete zanechat při odevzdání (mimo kompletního logování datového obsahu).

Program bude na standardní chybový výstup průběžně vypisovat zprávy v následujícím formátu (každý z formátů koresponduje s jedním typem zprávy aplikačního protokolu dle specifikací):

```sh
RRQ {SRC_IP}:{SRC_PORT} "{FILEPATH}" {MODE} {$OPTS}
WRQ {SRC_IP}:{SRC_PORT} "{FILEPATH}" {MODE} {$OPTS}
ACK {SRC_IP}:{SRC_PORT} {BLOCK_ID}
OACK {SRC_IP}:{SRC_PORT} {$OPTS}
DATA {SRC_IP}:{SRC_PORT}:{DST_PORT} {BLOCK_ID}
ERROR {SRC_IP}:{SRC_PORT}:{DST_PORT} {CODE} "{MESSAGE}"
```

Jednotlivé extension options `{$OPTS}` pak ve formátu dle pořadí v datovém přenosu:

```sh
{OPT1_NAME}={OPT1_VALUE} ... {OPTn_NAME}={OPTn_VALUE}
```

### Možnosti rozšíření nad rámec zadání ###

Můžete implementovat jakákoliv užitečná a smysluplná rozšíření tak, jak uznáte za vhodné a to s využitím vlastního typu přenosu (MODE z RRQ/WRQ) či extension options. Jakákoliv implementovaná rozšíření řádně zdokumentujte a případně i popište v rámci souboru README.

Při implementaci rozšíření není možné získat více bodů, než je stanovené maximum pro hodnocení daného zadání.
