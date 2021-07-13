// Copyright (C) 2021 Cocioran Stefan 321 CA

Pentru rezolvarea problemei m-am folosit de scheletul laboratoarelor 6 si 8.
Atunci cand serverul accepta o conexiune de la un client TCP, se verifica daca s-au primit numere 
de la clientii UDP si se formeaza un mesaj conform enuntului care este trimis imediat catre client.
Daca lista nu este goala (s-au primit numere), apelez functia "compute_payload" care imi intoarce 
un string ce contine lista de numere si media acestora. 
Atunci cand clientul TCP primeste raspunsul de la server, il afiseaza si se inchide.
Serverul nu o sa primesca niciodata date/informatii propriu-zise de la un client TCP, doar cereri 
de conectare (si atunci li se raspunde imediat).
Atunci cand serverul primeste date/un numar de la un client UDP, il adaug intr-un vector
de numere si afisez un mesaj conform enuntului "Am primit NUMAR de la IP:PORT".
Am implementat functia "compute_payload" care primeste ca parametru lista de numere primite, 
calculez media si formez un payload sub forma "{ LISTA_NUMERE }\nMedia: AVERAGE" pe care il returnez.
Serverul poate primi o singura comanda si anume "exit" care va inchide serverul. Orice alta comanda 
nu este acceptata si se va afisa un mesaj de eroare.
