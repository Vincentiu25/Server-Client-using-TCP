Tarsoaga Vincentiu, 324Ca
--- Tema PCOM 2 - Server / Client TCP / UDP ---

--- Descriere ---

- Server TCP

Acest server primeste mesaje de la clienti UDP si le trimite
clientilor TCP abonati la un topic. De asemenea, primeste mesaje
de la clientii TCP de tipul subscribe sau unsubscribe si le modifica
lista de abonari a clientului respectiv la topicuri.

- Client TCP

Acest client se conecteaza la serverul TCP si trimite mesaje de tipul
subscribe sau unsubscribe pentru a se abona sau dezabona la un topic.
De asemenea, poate trimite mesaje de tipul exit pentru a se deconecta
de la server.

--- Implementare ---

- Server TCP

Serverul TCP foloseste epoll pentru a putea primi mesaje de la mai
multi clienti in acelasi timp. In functie de tipul mesajului primit,
acesta este procesat.

Pentru mesajele de tipul subscribe sau unsubscribe, serverul cauta
clientul in lista de clienti TCP si modifica lista de abonari a
clientului respectiv la topicuri.

Pentru mesajele de tipul exit, serverul cauta clientul in lista de
clienti TCP si il elimina din lista de clienti.

Pentru mesajele de tipul datagrama UDP, serverul cauta topicul in
lista de topicuri ale fiecarui client si trimite mesajul catre toti
clientii TCP abonati la acel topic.

Ca si stocare a datelor, am folosit un vector STL de clienti TCP,
fiecare client TCP avand un vector STL de topicuri la care este abonat.

- Client TCP

Clientul TCP foloseste epoll pentru a putea primi mesaje de la server,
dar si de la tastatura in acelasi timp. In functie de tipul mesajului
primit, acesta este procesat.

Acesta se conecteaza la serverul TCP si trimite mesaje de tipul
subscribe sau unsubscribe pentru a se abona sau dezabona la un topic.
De asemenea, poate trimite mesaje de tipul exit pentru a se deconecta
de la server.

De asemenea, clientul TCP primeste mesaje de la server si le afiseaza
in functie de tipul mesajului primit.

--- Compilare ---

- Server TCP

```
make server
```

- Client TCP

```
make client
```

--- Rulare ---

- Server TCP

```
./server_tcp <port>
```

- Client TCP

```
./client_tcp <id_client> <adresa_server> <port_server>
```

--- Informatii suplimentare ---

Nu am implementat:
    - Optiunea de store-forward pentru mesajele care nu
    pot fi livrate clientilor TCP abonati la un topic

--- Bibliografie ---

- https://man7.org/linux/man-pages/man7/epoll.7.html - folosire epoll
- laborator PCOM - folosire socket API pentru TCP si UDP
- laborator PCOM - functiile din utils.cpp (recv_all si send_all pentru
a primi si a trimite mesaje in intregime mesaje de lungime variabila)


--- Concluzii ---

Aceasta tema a fost dificil, dar interesant de realizat. Am invatat
sa lucrez cu socketi TCP si UDP, sa trimit si sa primesc mesaje
de la server si de la clienti. Am invatat sa folosesc epoll pentru
a putea primi mesaje de la mai multi clienti in acelasi timp.