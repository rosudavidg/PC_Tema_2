```
================================================================================
Nume:    Rosu
Prenume: Gabriel - David
Grupa:   321CD
Materia: Protocoale de comunicatii
Titlul:  Tema 2 - Sistem monetar de tip Internet Banking
Data:    5 mai 2018
================================================================================
                                    Tema 2
                      Sistem monetar de tip Internet Banking
================================================================================
[0] Cuprins
--------------------------------------------------------------------------------

    1. Introducere
    2. Continutul arhivei
    3. Implementare 
    4. Rulare
    5. Feedback

================================================================================
[1] Introducere
--------------------------------------------------------------------------------

    Aceasta tema reprezinta dezvoltarea unei aplicatii de tip
Internet Banking, folosind socketi UDP si TCP.

================================================================================
[2] Continului arhivei
--------------------------------------------------------------------------------
    
    +=================================================================+
    |        NUME        |               DESCRIERE                    |
    |=================================================================|
    | server.c           | Fisierul sursa pentru server               |
    | client.c           | Fisierul sursa pentru client               |
    | utils.h            | Header folosit de ksender.c si kreceiver.c |
    | Makefile           | Makefile pentru compilare / stergere       |
    | README             | Prezentul readme                           |
    +=================================================================+
    
================================================================================
[3] Implementare
--------------------------------------------------------------------------------
    SERVER
--------------------------------------------------------------------------------

[*]    Structura user reprezinta o intrare din fisierul primit ca argument.
Aceasta contine toate campurile necesare (first_name, last_name, card_no,
pin, secret_password, balance), dar si:
    locked - 1, daca cardul este blocat, 0 altfel
    logged - 1, daca cineva este conectat la acest cont la un moment de timp, 0
altfel
    attempt - numarul de incercari esuate de conectare

[*]    Variabile globale:
    PORT     - portul primit ca argument
    users_no - numarul de useri din "baza de date"
    users    - toti utilizatorii cititi din fisier (de tip User*)
    logged   - logged[x] = -1, daca clientul x nu este conectat la niciun
user, altfel inseamna ca este conectat la user-ul x
    last_command_was_transfer[i] = -1, daca ultima comanda data de clientul
i NU este transfer, x altfel, unde x reprezinta nr.crt din variabila users,
pentru destinatarul fondurilor transferate
    float values[i] = valoarea pe care clientul i urmeaza sa o transfere

[*]    main:
    Verific numarul de argumente primit.
    Citesc si stochez toti user-ii.
    Setez toate variabilele globale.
    Pornesc efectiv serverul
    Scriu inapoi in fisier user-ii cu valorile lor modificate.
    Eliberez memoria alocata pentru user-i.

[*] run:
    Implementez protocolul TCP/UDP ca in laborator.
    Adaug un socket pentru citirea de la stdin.

    Daca se primeste ceva la stdin, se verifica daca este QUIT. Daca da,
atunci serverul isi incheie conexiunea cu succes, altfel, se trateaza
ca o eroare.
    Daca se conecteaza un nou client, acesta se adauga conexiunea.

    Daca se primeste ceva de la un client deja aflat in lista, atunci:
    - se primeste mesajul
    - daca clientul a inchis conexiunea, atunci acesta este scos din lista
    - altfel, se extrage primul cuvant din mesaj si se verifica ce este
        - Daca este "login", se extrage numarul cardului si se cauta
in lista de useri. Daca nu este gasit, se intoarce eroare. Daca este
gasit, dar este deschisa deja o alta conexiune, se intoarce eroare.
Daca este blocat se intoarce eroare. Altfel, se verifica parola.
Daca parola este gresita, se verifica a cata incercare este. Daca
este a 3-a, se blocheaza cardul si se afiseaza aceasta eroare. Daca
nu este a 3-a, se incrementeaz variabila attempt. Daca pin-ul este corect,
atunci se accepta conectarea.
        - Daca este "logout", se considerea ca este deja conectat clientul,
deci acesta va fi doar deconectat si va primi mesajul valid.
        - Daca este "listsold", se va afisa direct variabila balance.
        - Daca este "transfer", se cauta destinatarul. Daca nu se gaseste,
se intoarce eroare. Daca se gaseste, se verifica daca fondurile sunt suficiente.
Daca nu sunt, se intoarce eroare. Daca sunt, se seteaza variabilele globale
necesare urmatoarei comenzi (adica last_command_was_transfer si values) si
se intoarce mesajul de confirmare (vezi ce urmeaza dupa QUIT).
        - Daca este "quit", se inchide cu succes clientul.
        - Daca nu se recunoaste comanda si last_command_was_transfer este
activat pentru socket-ul curent, atunci mesajul este sigur confirmarea
transferului, si se verifica acesta. Daca este pozitiv, adica "y",
se efectueaza transferul. Altfel, se intoarce eroarea de operatie anulata.

--------------------------------------------------------------------------------
    CLIENT
--------------------------------------------------------------------------------
[*]    Variabile globale:
    PID = process id al clientului
    log_fd = fd pentru fisierul de log
    logged = 1, daca clientul este conectat, 0 altfel
    last_command_was_transfer = 1, daca ultima comanda a fost transfer,
0, altfel
    my_card_no = numarul de card, dupa o conectare/ incercare de conectare
(numarul cardului exista).

[*] main
    Deschid fisierul pentru log
    Verific numarul de argumente primite
    Pornesc efectiv clientul
    La oprirea clientului, se inchide fisierul de log

[*] run
    Implementez protocolul TCP/UDP ca in laborator.
    Primesc comenzi de la tastatura, pana la primirea "quit".
    Daca primesc "login" si exista deja o sesiune deschisa pe acest
client (adica logged = 1), atunci se va afisa direct eroarea.
    Daca primesc "quit", atunci se va inchide clientul cu succes.
    Daca primesc orice comanda in afara de quit, unlock sau login si nu sunt
conectat, atunci se va afisa eroarea.
    Daca primesc "logout", atunci setez variabila globala de logged pe 0.
    Daca primesc "transfer", atunci trimit mesajul si setez variabila
last_command_was_transfer pe 1.
    Daca primesc "login", atunci voi trimite comanda catre server.
    Daca primesc "unlock"... nu merge :).
    Daca variabila last_command_was_transfer este setata, atunci
se va astepta de la intrare raspunsul si se va trimite catre server.

================================================================================
[4] Rulare
--------------------------------------------------------------------------------

(*) Makefile

    build: compileaza sursele
    
    clean: sterge executabilele si fisierele de log

Exemplu rulare server
    ./server 1234 users.txt

Exemplu rulare client
    ./client 127.0.0.1 1234

================================================================================
[5] Feedback
--------------------------------------------------------------------------------
    
    + O tema usoara, care isi atinge obiectivele.

================================================================================
                                    SFARSIT
================================================================================
```