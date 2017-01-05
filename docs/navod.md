#ImtJSON - česká příručka

## Popis knihovny ImtJSON

Knihovna ImtJSON zajišťuje parsování a serializaci formátu JSON, dále pak správu dat načtenych z JSON, úpravu dat a tvorbu nových dat ve objektech, které lze serializovat do formátu JSON.

Knihovna je určena pro programovací jazyk C++ verze 11 a vyšší

Od ostatních běžně dostupných knihoven se odlišuje zejména důrazem na immutabilitu objektové reprezentace JSON struktury.

## Immutabilita

Immutabilita - neměnnost - se u JSONu projevuje zejména při práci s kontejnery. Týká se to hodnot typů **array** a **object**. Zatímco u jiných knihoven a v *javascriptu* se změna kteréhokoliv prvku pole nebo objektu projeví všude tam, kde je příslušný kontejner referencovaný, u knihovny **imtjson** toto (záměrně) není možné. Příklad

```
var a = {"x":10,"y":20,"z":30}
var b = a;
b.x = 40;
console.log(a.x); //< prints '40'
```

Přepisem proměnné 'x' v objektu, který obsahuje proměnná 'b' se přepis projeví i v proměnné 'a'.Výše uvedená technika není v knihovně ImtJSON přípustná. Knihovna ImtJSON to řeší tak, že každá změna v obsahu kontejneru způsobí vytvoření nové instance kontejneru, původní kontejner se nezmění. Toto pravidlo může zdánlivě způsobovat zpomalení programu, nicméně přináší spoustu výhod, které mohou mít v celkovém souhrnu zjednodušení kódu a možná i zrychlení.

## Výhody a nevýhody immutability

### Výhody

 - **Sdílení** - data není třeba kopírovat. Kopírování se realizuje sdílením.
 - **Lockfree přístup** - Sdílení Json objeků mezi vlákny nevyžaduje žádné zámky ani synchronizaci!
 - **Neexistence cyklů** - Aby bylo možné v json struktuře vyrobit cyklus, je třeba umožnost vložení nadřazeného kontejneru do podřazeného. To u immutable objektů není možné, protože vložením nadřazeného kontejneru do podřazeného vznikné nová instance podřazeného kontejneru který neobsahuje nadřazený kontejner. 
 - **Garbage collector** - Neexistence cyklů umožňuje jednoduše sledovat počty referencí
 - **Konstantní hodnoty** - JSON je často plný běžných konstant, třeba hodnot 0, prázdných řetězců, prázdných polí nebo objektů. Všechny tyhle konstanty jsou v knihovně pevně zadefinované a nealokují se. Stejně tak hodnoty **true**, **false** a **null** jsou v paměti uloženy jen 1x a kdekoliv se vyskytují, tam se pouze uloží ukazatel na jejich instanci. Díky neměnnosti kontejnerů je takto sdíleno prázdné pole, prázdný objekt a prázdný řetězec. 
 - **Zaručení neměnnosti** - některé nástroje vyžaduji zaručení neměnnosti. Například databázové cache pro NoSQL databáze pracující s JSONem mohou umožnit vracet již cacheovaný parsovaný JSON a chtějí mít jistotu, že příjemce cacheovaného výsledku nebude měnit vnitřky kontejnerů a tím rozbíjet obsah cache. Dalším příkladem může být obyčejný požadavek uložení nějakého kontejneru z předaných dat a mít přitom jistotu, že jiná část programu, která ten kontejner také získala, ho nezmění. Bez téhle jistoty by příjemce takových dat musel (často zbytečně) vytvářet kopii celého kontejneru (rekurzivně), nebo s daty předávat nějaký slib o neměnnosti dat, aby se podle toho uměl zařídit.

### Nevýhody

 - každá změna generuje novou instanci (avšak provádí se jen mělká kopie)
 - Při změně hodnoty v nižší úrovni je třeba změnit všechny kontejnery na vyšších úrovních (například při změně hodnoty ve dvourozměrném poli je třeba provést kopii celého pole, ovšem není potřeba kopírovat celé řádky, pouze ten řádek ve kterém dochází ke změně)
 
 
## Jak na změny v prostředí immutability?

Každá změna znamená vytvoření nové instance kontejneru, ať je to pole nebo objekt. Aby během modifikace kontejneru nedocházelo k vytváření mnoho instancí, které jsou následně zahozeny při dalších modifikaci, nabízí knihovna ImtJSON speciální objekty, tzv "buildery". To jsou objekty ve kterých dochází k dílčím úpravám v kontejneru. Tyto úpravy probíhájí pouze v rámci aktuální funkce, která změnu realizuje. V okamžiku, kdy je změna hotová, provede se operace "commit", která způsobí vytvoření nové instance JSON hodnoty, která je již plně immutabilní. Vnitřní obsah builderu přitom není možné sdílet, ani začlenit do struktury, jediná cesta vede přes commit.

K dispozici jsou dva základní buildery. **Array** a **Object**, podle typu kontejneru

```
Value a = Value::fromString("{\"x\":10,\"y\":20,\"z\":30}");
Value b = a;
Object c(b);
c.set("x",40);
b = c; //< commit
cout << a["x"].getUInt() << endl;  //< prints '10'
cout << b["x"].getUInt() << endl;  //< prints '40'
```


Proměnná 'b' se použije ke konstrukcí builderu **Object**. Poté dojde ke změně proměnné 'x'. Výsledek se uloží zpět do proměnné 'b', což je operace commit, protože dochází ke konverzi z Object do Value. Proměnná 'b' nyní obsahuje novou instanci, která není závislá na proměnné 'a'.

Výše uvedený kód lze zapsat kratším způsobem

```
Value a = Value::fromString("{\"x\":10,\"y\":20,\"z\":30}");
Value b = a;
b = Object(b).set("x",40);
cout << a["x"].getUInt() << endl;  //< prints '10'
cout << b["x"].getUInt() << endl;  //< prints '40'
```

Velice podobně se pracuje s **Array**.

# Přehled možností knihovny 

Detaily jednotlivých funkcí lze najít přímo v dokumentaci ve zdrojácích. V této části jsou uvedeny jen ty důležité části a aspekty práce s ImtJSON, které nejsou při čtení referenčí příručky hned patrné

## Parsování formátu JSON

K pársování formátu JSON existuje několik metod:

 * **`Value::fromString(str)`** - parsuje z std::string
 * **`Value::fromStream(cin)`** - parsuje z std::istream
 * **`Value::fromFile(stdin)`** - parsuje z FILE *
 
Třída **Value** ale nabízí obecnější parser, který může přijímat data z libovolného zdroje, i třeba z nestandardního. Stačí,pokud existuje funkce, která vrací ze zdroje další znak.

 * **`Value::parse(funkce)`** - kde **funkce** má prototyp **int funkce()**. Funkce by měla při každém zavolání vrátit další znak ve streamu dokud není konec. Pak by měla vrátit hodnotu -1.
 
## Serializace formátu JSON

Téměř kteroukoliv hodnotu uloženou v proměnné Value lze serializovat - jedinou hodnotu, kterou nelze serializovat je hodnota ***undefined***. K serializaci lze použít funkce

 * **`Value.stringify()`** - serializuje jako string. Vrací interní typ String, který lze převést na std::string
 * **`Value.toStream(cout)`** - serializuje do std::ostream
 * **`Value.toFile(stdout)`** - serializuje do FILE *
 
Stejně jako u parseru i zde třída **Value** nabízí obecnější serializátor. který umožňuje provést serializaci prakticky kamkoliv.

 * **`Value.serialize(funkce)`** - kde **funkce** má prototyp **void funkce(char c)**. Funkce by měla při každém zavolání nějak zpracovat předaný znak, například zapsat do výstupního stream
 
```
int s = socket(...);
connect(s,...);
Value v = ...

v.serialize( [s](char c) {send(s, &c, 1, 0);} );
```

Výše uvedený příklad serializuje JSON do otevřeného soketu.

## Typy, které lze uložit v instanci třídy Value

 * **`number`** - jakékoliv číslo. Znamenková a neznamenková celá čísla využívají bitovou šířku aktuální platformy. Čísla s plovoucí čárkou a číslo s rozsahem mimo počet bitů platformy se ukládají jako **double**. Typ čísla nerozlišuje způsob uložení, nicméně rozhranní nabízí funkci, přes kterou lze získat informaci o způsobu uložení - viz **Value::flags()**
 * **`string`** - aktuální verze ImtJSON podporuje pouze 8bitové znaky. Znaky mezinárodních abeced jsou kódované pomocí UTF-8. Pokud se v JSONu objevuje escape sekvence \uXXXX je při parsování převedena na UTF-8 sekvenci. Parser také normalizuje UTF-8 sekvence, pokud by byly prodloužené.
 * **`boolean`** - hodnota true nebo false
 * **`object`** - kontejner, kde lze hodnoty adresovat pomocí klíče. Objekty jsou vždy řazeny podle klíče (binární řazení). Na pořadí klíčů ve zdrojovém JSONu se nebere ohled.
 * **`array`** - kontejner hodnot adresovaných pomocí pořadí (pozice). Hodnoty jsou indexované od čísla 0 do čísla velikost-1
 * **`null`** - ImtJSON považuje hodnotu **null** za běžnou hodnotu, kterou lze zapsat do libovolného kontejnetu 
 * **`undefined`** - Hodnota tohoto typu představuje proměnnou bez hodnoty, tedy i bez hodnotu null. Nelze ji serializovat, nejde ji uložit do kontejneru. Naopak, pokud je hodnotou **undefined** přepsána jiná hodnota v kontejneru, je to považováno za smazání přepisované hodnoty. Tato hodnota se také objevuje v případě, že dojde k pokusu adresovat neexistující hodnotu v kontejneru. Pozor také, že **(undefined == undefined) == false**
 
## Přístupy k obsahu

Třída Value nabízí několik metod, které umožňují získat vlastní obsah. 

 * **`getUInt`** - přečte číslo jako unsigned int
 * **`getInt`** - přečte číslo jako int
 * **`getNumber`** - přečte číslo jako double
 * **`getBool`** - přečte hodnotu bool
 * **`getString`** - přečte řetězec
 * **`isNull`** - vrací true, pokud je hodnota null
 * **`defined`** - vrací true, pokud hodnota není <undefined>
 * **`size`** - vrací počet prvků v kontejneru (pole i objekt!)
 * **`toString`** - vrací obsah jako řetězec (čísla převede na řetězec, boolean na klíčová slova "true"/"false", null na "null", kontejnery jako JSON reprezentaci a undefined jako "undefined"). Oproti **getString** tato funkce alokuje nový řetězec, zatímco **getString** vrací přímo řetězec uložený v proměnné - takže **getString** je rychlejší než **toString**.

Pro přístup do kontejneru lze použít operátor hranatých závorek

Pár příkladů

```
obj["key"].getString();
arr[10].getUInt();
arr2[5][12].getNumber();
arrObj[7]["key"].getBool();
```

### Poznámka k implementaci

Původně se zvažovalo, že by třída Value uměla automatické konverze na danný typ, aby se  s proměnnou tohoto typu dalo pracovat přímočaře bez nutnosti explicitně uvádět, jaký typ hodnoty se z proměnné vyzvedává. Nakonec byla zvolena vyše uvedená forma, která je lépe čitelná a redukuje množství ambignuit v situacích, kdy je Value poslána do přetížené funkce, která přijímá vícero typů, a které jsou zároveň podporované třídou Value. Zároveň lze Value použít u šablon bez nutnosti používat přetypovací závorky před proměnnou, které znepřehledňují čtení kódu. Funkce se také hodi při použiití deklarace **auto**.
```
auto str = v.getString();
auto n = v.getUInt();
```

### Co vrací metody pokud je v proměnné jiný typ než je očekáván

|         | null  | number  | string  | boolean  | object  | array | undefined |
|---|---|---|---|---|---|---|---|
| `getUInt` | 0     | hodnota   | konverze/0 | false=0, true=1 | 0 | 0   |    0      |
| `getInt`  | 0     | hodnota   | konverze/0 | false=0, true=1 | 0 | 0   |    0      |
| `getNumber`  | 0.0| hodnota   | konverze/0 | false=0.0, true=1.0 | 0.0 | 0.0   |    0.0      |
| `getBool`  | false| 0=false jinak true   | ""=false jinak true | hodnota | !empty() | !empty()   |    false      |
| `getString`  | "" | ""   | hodnota | "" | "" | ""   |    ""      |
| `isNull`  | true | false   | false | false | false | false   |  false  |
| `defined`  | true | true    | true  | true  | true  | true    |  false  
| `size()` | 0 | 0 | 0 | 0 | hodnota | hodnota | 0 |

**Legenda**

 - *hodnota* - vrací skutečnou hodnotu
 - *konverze/0* - konverze řetězce na číslo, pokud je možná, jinak 0
 - *!empty()* - true, pokud kontejner není prázdny
 
### Enumerace polí a hodnot v kontejneru

K dispozici je funkce size(), která vrací počet prvků v kontejneru. Enumerovat lze nejen pole, ale také objekt, protože i **u objektu lze přistupovat k hodnotám pomocí index(!)**

Dále je možné použít funkci **forEach**

```
container.forEach([&](Value v){
   //nějaká operace
   return true;  //<enumeraci lze přerušit pomocí false
} 
```

Třída Value také podporuje iterátory, lze tedy použít ranged-for

```
for(auto &&v : container) {
  //nějaká operace s v
} 
```

### Získání jména klíče z hodnoty při enumeraci

Všechny způsoby enumerace vrací pouze hodnoty, ale nikde se nevrací klíč. Pokud je enumerován objekt, klíč každé hodnoty je dostupný přímo přes třídu Value a funkci **getKey()**

```
for(auto &&v : container) {
  auto klic = v.getKey();
  auto hodn = v.getUInt();
  ...
} 
```
Pokud je funkce getKey volána na hodnotu, která nebyla získána z objektu, pak vrací prázdný řetězec "". 

**Poznámka:** Rozhraní třídy Value také nabízí funkci **setKey**, která umí svázat hodnotu s klíčem, takže se pak na ní dá zavolat **getKey**. I tady přitom vzniká nová instance hodnoty.

## Zápis jsonu v kódu

Rozhraní ImtJSON klade důraz na přehledný a přitom jednoznačný zápis JSONu přímo do kódu. V C++ není možné zapisovat JSON tak jak je zvykem u Javascriptu, přesto má C++ dost syntaxtických prostředků,jak se pohodlí javascriptu co nejvíce přiblížit

### Zápis hodnot

Hodnoty lze přiřadit přímo, pokud existuje konverze

```
Value a=10;    //< číslo
Value b=20.3;  //< číslo
Value c=true;  //< boolean
Value d="nejaky string" //< string
Value e=nullptr;  //< null
Value f;         //< undefined
```

### Zápis polí

```
Value p = {10,15,20.3,true,false,"nejaky string","hello",nullptr};
```

### Zápis objektu
```
Value obj = Object
           ("name","Alena")
           ("surname","Gore")
           ("age",19)
           ("sex","female");
```
### Kobinace zápisu

```
Value lide= {
	Object("name","Alena")
           ("surname","Gore")
           ("age",19)
           ("sex","female"),
	Object("name","Kamil")
           ("surname","Frama")
           ("age",26)
           ("sex","male")
};
```

## Buildery - modifikace existujících objektů

### Object

Prázdný objekt
```
Object obj;
```

Konverze Value na Object pro jeho modifikaci
```
Object obj(v);

```
Změna kliče
```
obj("klic",hodnota);
obj.set("klic",hodnota);
```
Smazání kliče
```
obj("klic",Value());
obj("klic",json::undefined);
obj.set("klic",Value());
obj.set("klic",json::undefined);
obj.unset("klic");
```
Modifikace podobjektu
```
obj.object("sub")("klic",hodnota)   //{"sub":{"klic":hodnota,
                 ("klic2",hodnota); //        "klic2":hodnota}}

// -- nebo --

auto sub=obj.object("sub");        //vytvoří nebo modifikuje podobjekt
sub.set("klic",hodnota);    //zapíše klic:hodnota
sub.set("klic2",hodnota);   //zapíše klic2:hodnota
sub.commit();               //commit změn a zápis do "sub"
```
Přidání prvků do pole v objektu
```
obj.array("data").addSet({10,20,30});

// -- nebo --

auto sub=obj.array("data");        //vytvoří nebo modifikuje pole
sub.addSet({10,20,30});        //přidá prvky na konec
sub.commit();               //commit změn a zápis do "sub"
```
Všechny změny provedené přes Object se nezapistují do původního objektu. Je třeba instanci třídy Object převést na instanci třídy Value a případně zapsat nový objekt namísto původního (do původní proměnné) jak je ukázáno v příkladu na začátku dokumentu.

### Array

Prázdné pole
```
Array arr;
```

Konverze Value na Array pro jeho modifikaci
```
Arrayt arr(v);

```
Přidání prvku
```
arr.add(x)
arr.push_back(x)

```
Přidání množiny prvků
```
arr.addSet({a,b,c});

```
Smazání prvku
```
arr.erase(i);
```
Změna prvku
```
arr.set(i, x);
```
Přidání objektu
```
arr.addObject()("klic",hodnota)   //[...,{"klic":hodnota,
                 ("klic2",hodnota); //        "klic2":hodnota}]

// -- nebo --

auto sub=arr.addObject();     //vytvoří nebo modifikuje podobjekt
sub.set("klic",hodnota);    //zapíše klic:hodnota
sub.set("klic2",hodnota);   //zapíše klic2:hodnota
sub.commit();               //commit změn a zápis do "sub"
```
Modifikace objektu na pozici
```
arr.object(i)("klic",hodnota)   //[...,{"klic":hodnota,
             ("klic2",hodnota); //        "klic2":hodnota},..]

// -- nebo --

auto sub=arr.object(i);     // modifikuje podobjekt
sub.set("klic",hodnota);    //zapíše klic:hodnota
sub.set("klic2",hodnota);   //zapíše klic2:hodnota
sub.commit();               //commit změn a zápis do "sub"
```

Všechny změny provedené přes Array se nezapistují do původního objektu. Je třeba instanci třídy Array převést na instanci třídy Value a případně zapsat nové pole namísto původního (do původní proměnné) jak je ukázáno v příkladu na začátku dokumentu.

## Další pomocné objekty

### String

Pro snažší manipulaci se stringem a také pro uložení výhradně řetězcových hodnot nabízí knihovna ImtJSON třídu **String**. Využívá hodnotu typu string, kterou při kopírování sdílí stejně jako jakoukoliv jinou hodnotu. String v knihovně nelze měnit stejně jako ostatní objekty, pouze lze vytvořit změněnou kopii. Lze jej tedy použít pro
uložení řetězců v různých datových struktůrách, které se zároveň různě přesouvají nebo kopírují, přičemž samotný string se pouze po celou dobu
sdílí.

### StringView

Tato třída slouží k předávání řetězců mezi různými částmi knihovny a také mezi knihovnou a STL. StringView pouze obsahuje ukazatel na řetězec a délku, nicméně lze jej použít všude tam, kde se očekává String nebo std::string. 

Tato třída vznikla protože STL doposud takovou třídu nemá a její zavedení se pravděpodobně chystá do verze 17.

### Path

Path slouží k vytváření cest po struktuře JSONu. Instanci třídy Path
lze použít namísti indexu u operátoru [] při adresaci prvků v kontejnerech.

Path existuje ve dvou verzích. První se jmenuje **Path** a durhá **PPath**. Instanci třídy **PPath** lze přitom automaticky převést na **Path**. Rozdíl mezi nimi ve způsobu alokace cesty. Zatímco **Path** je určena při sestavování cesty rekurzivním sestupem od rootu k prvku a je platná pouze v dané funkci, PPath je již sestavená immutabilní cesta alokovaná v heapu.

**Path** lze použít k sestavení cesty. **PPath** se posléze použije k uložení cesty do proměnné mimo aktuální zásobník.

Cesta se konstruuje tak, že se volá konstruktor, kterému se předá proměnná nadřazené úrovně a pak index nebo jméno klíče v aktuální úrovní. Nejvyšší úroveň se jmenuje **Path::root**

```
Path p1(Path::root, "abc");  // {"abc":?}
Path p2(p1,2);               // {"abc":[x,x,?,y,y,...] 
Path p3(p2,"xyz");           // {"abc":[x,x,{"xyz":?},y,y,...]
```
Cestu lze také sestavit pomocí lomítek
```
cout << v[Path::root/"abc"/2/"xyz"].toString()
```
Pozor jen, že cesta sestavená pomocí lomitek **se nesmí uložit** do proměnné **Path**. Celé se to komplikuje tím, že překladač často nedokáže takové použití cesty podchytit a upozornit na chybu. Knihovna ImtJSON implementuje jakýsi mechanismus detekce, kdy neplatné použití cesty způsobí výjimku

```
//NESPRÁVNÝ ZPŮSOB (ale překladač ho přeloží)
Path x = Path::root/"abc"/2/"xyz";
```
Důvodem, proč je tento způsob nesprávný je ten, že element "xyz" se odkazuje na element 2, který je po provedení příkazu zdestruován. V x je pak uložen jen elemeny "xyz" který odkazuje na zdestruovaný objekt

K výše uvedenému účelu použijeme objekt **PPath**
```
//Správný způsob 
PPath x = Path::root/"abc"/2/"xyz";
```

Kromě toho lze cestu převést na JSON
```
x.toValue()  //["abc",2,"xyz"]
```

a naopak (`fromValue()`)

### RefCntObj a RefCntPtr

Tyto dva objekty realizuji čítání referencí, hlavní součást garbage collectoru. Bylo zvoleno intruzivní čítání, protože vytváří menší overhead (není třeba alokovat extra paměť pro counter) a navíc je to asi 10x kratší kód, než kód pro  **std::shared_ptr** (a díky neexistenci cyklů není potřeba ani podpora slabých referencí). Oba objekty jsou MT safe

