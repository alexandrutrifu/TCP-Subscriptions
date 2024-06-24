# Tema 2 - PCOM

### Structură

- `server.cpp` 
  - parsează argumentele de pornire a server-ului și apelează funcția `run_server()`

- `subscriber.cpp` 
  - parsează argumentele de pornire a server-ului și apelează funcția `login_subscriber()`

- `server_backend.h`
  - definește namespace-ul `server`; acesta conține principalele funcții pe baza cărora se modelează *comportamentul server-ului* (multiplexarea conexiunilor UDP și TCP, interpretarea request-urilor trimise de către clienți etc.)

- `subscriber_backend.h`
  - definește namespace-ul `subscriber`; acesta conține principalele funcții pe baza cărora se modelează *comportamentul unui client TCP* (conectarea la server, trimiterea comenzilor de `subscribe` & `unsubscribe`)

- `subscription_protocol.h`
  - implementează protocolul de nivel Aplicație necesar eficientizării procesului de comunicare
  - definește namespace-ul cu același nume; acesta conține *structurile folosite în modelarea pachetelor UDP și TCP*, respectiv a principalelor atribute ale clienților TCP. În plus, tot aici se regăsesc funcțiile aferente mecanismelor de `subscribe`/`unsubscribe`, notificare și formatare de mesaje

- `helpers.h`
  - include bibliotecile necesare pentru rularea programului
  - definește namespace-ul `connection`; acesta conține funcțiile `receive_full_message()` & `send_full_message()`, responsabile cu citirea/trimiterea completă a fluxurilor de octeți trimise prin TCP


---

### [1] Implementarea protocolului de nivel Aplicație

Este realizată în cadrul header-ului `subscription_protocol.h`.

În cadrul procesului de comunicare au fost utilizate următoarele structuri:

- `struct udp_packet`
  - încapsulează câmpurile dintr-un mesaj UDP, conform specificațiilor din enunț
- `struct subscription_packet`
  - încapsulează un mesaj generic, în vederea comunicării cu clienții TCP

- `std::unordered_map<int, struct TCP_Client *> tcp_clients`
  - HashMap ce asociază fiecărui socket TCP o structură de tip `TCP_Client`; aceasta conține detalii precum ID-ul clientului, socket-ul folosit pentru comunicarea cu server-ul, starea de conectare (*isActive*) IP și port
  - inițializat odată cu pornirea server-ului, aici **[2]**
- `std::unordered_map<std::string, std::vector<struct TCP_Client *>> subscriptions`
  - HashMap ce asociază fiecărui topic înregistrat de către server un vector de clienți TCP abonați
  - inițializat odată cu pornirea server-ului, aici **[2]**

Manipularea structurilor menționate anterior este efectuată prin intermediul unor funcții specifice:

1) `subscribe_to_topic(client, topic, subscriptions)`
   - actualizează map-ul abonamentelor fie prin introducerea unei intrări noi (dacă server-ul nu a înregistrat topic-ul dat ca parametru), fie prin adăugarea client-ului curent în lista de abonați aferentă intrării `topic` din map
2) `unsubscribe_from_topic(client, topic_wildcard, subscriptions)`
   - elimină clientul curent din lista de abonați aferentă intrării `topic` din map

*Obs:* Matching-ul wildcard-urilor și gestionarea caracterelor `*`/`+` sunt realizate cu ajutorul **expresiilor regulate**, conform cerinței (spre exemplu, funcția de unsubscribe va folosi un matching strict, care să nu dezaboneze clientul decât de la topicul precizat ca parametru, fără potriviri suplimentare).

Funcția **`format_notification(packet)`** primește ca argument mesajul trimis de către un client UDP (încapsulat într-o structură `udp_packet`) și formatează un string-rezultat conform specificațiilor din enunț.

Funcția **`notify_subscribers(topic, notification, subscriptions)`** parcurge map-ul abonamentelor și verifică matching-ul dintre topic-urile disponibile și cel precizat ca parametru. În cazul unei potriviri, se trimite mesajul din `notification` tuturor clienților abonați la topic-ul găsit.

---

### [2] Pornirea server-ului

Realizată prin apelul `run_server()`.

Implică multiplexarea unor conexiuni de tip UDP și TCP, respectiv a comenzilor date de către user la STDIN; în acest sens, este folosit mecanismul de **poll()-ing**. În urma introducerii descriptorilor necesari în vectorul cu elemente de tip `struct pollfd`, server-ul tratează cererile primite cu ajutorul următoarelor funcții:

- `handle_tcp_connection()`, pentru cererile TCP de conectare (primite pe socket-ul pasiv al server-ului)
  - acceptă conexiuni TCP
  - verifică dacă există o intrare corespunzătoare clientului curent în map-ul asociat utilizatorilor înregistrați
    - dacă clientul este înregistrat și activ, se generează o eroare de conectare
    - dacă clientul s-a deconectat în trecut, se actualizează socket-ul aferent intrării sale din map și se setează câmpul *isActive* cu *true*
  - dacă clientul nu este înregistrat, se alocă o nouă structură `TCP_Client` și se introduce în map-urile server-ului
  - dacă operațiile de mai sus au avut loc cu succes, se trimite un mesaj de confirmare către client
- `handle_stdin_command()`, pentru comenzile primite de la STDIN (comanda *exit* determină închiderea tuturor descriptorilor urmăriți)
- `process_udp_message()`, pentru request-urile clienților UDP
  - primește mesajul trimis de client (într-o structură `udp_packet`)
  - apelează `format_notification()`, respectiv `notify_subscribers()` pentru a trimite un mesaj corespunzător clienților TCP abonați la topic-ul e extras din `udp_packet`
- `process_client_request()`, pentru cererile de *(un)subscribe* venite din partea clienților TCP (+ notificarea *Quit*)
  - se parsează topic-ul solicitat de către client, apoi se apelează funcțiile corespunzătoare din **[1]**

---

### [3] Pornirea clienților TCP

Realizată prin apelul `login_subscriber()`.

Implică multiplexarea între conexiunea cu server-ul (printr-un socket TCP) și comenzile primite de la STDIN; la fel ca în cazul server-ului, se folosește un vector cu elemente de tip `struct pollfd`.

Funcția **`connect_to_server()`** este responsabilă de stabilirea conexiunii TCP dintre client și server, aceasta fiind asigurată doar în urma primirii unui mesaj de confirmare din partea server-ului (pentru a evita conectarea simultană a doi clienți cu același ID).

Funcția **`parse_user_command()`** se ocupă de parsarea input-ului trimis de utilizator. Astfel, pentru fiecare comandă a acestuia din urmă (*subscribe*, *unsubscribe* sau *exit*), se va încapsula informația utilă a mesajului într-un pachet de tip `struct subscription_packet`, trimis către server în vederea prelucrării sale. 

*Obs*: Și de data aceasta, se va aștepta un mesaj de confirmare din partea server-ului. În cazul în care comanda nu poate fi executată, se va afișa un mesaj de eroare corespunzător.

---

