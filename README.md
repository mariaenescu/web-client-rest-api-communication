Enescu Maria

# Client web. Comunicatie cu REST API.

In realizarea temei am folosit integral scheletul cu implementarea din
`laboratorul 9` careia i-am adaugat: 
- In `requests.c`:
    - parametrul `flag_delete` regasit in functia `compute_get_request`
    il folosesc pentru a nu-mi crea o functie separata de `DELETE request`
    care ar fi codul duplicat din `compute_get_request`;
    - parametrul `token` in functiile `compute_get_request` si
    `compute_post_request` pentru transmiterea datelor intre client si server,
    fara ca un posibil atacator sa poata modifica datele;
- Pentru parsarea JSON, am adaugat `parson.c` si `pason.h`
[https://github.com/kgabis/parson], fiindu-mi utile functiile: de serializare,
de formatare, de creare si de extragere a datelor din continuturile de tip JSON;

## client.c

In functia `main` din fisierul `client.c` este logica de baza pentru comenzile
pe care clientul le poate da, astfel:
- Dupa declararea variabilelor utile (login_cookie, token, sockfd, command,
is_login, permission_library):
    - `is_login` reprezinta starea logarii in care se afla utilizatorul, prin
    inexistenta sau existenta continului in `login_cookie`:
        `=0` -> `logout`;
        `=1` -> `login`;
    - `permission_library` reprezinta permisiunea de acces in biblioteca, prin
     inexistenta sau existenta continului in `token`:
        `=0` -> nu a fost data comanda `enter_library`;
        `=1` -> a fost data comanda `enter_library` cu succes, deci avem
        permisiune;

- Programul citeste repetitiv comanzi de la tastatura:
    1. `register`:
        - daca `is_login = 1` se va intoarce un mesaj de eroare ce semnaleaza
        neputinta inregistrarii unui nou user in timpul conectarii la altul;
        - altfel, daca utilizator nu e conectat, se apeleaza functia
        `register_login` in care:
            - se apeleaza functia `username_password` in care se introduc
            usernameul si parola de la tastatura, acestea trebuie sa fie
            valide(cuvinte fara spatii, nu pot fii goale - in caz contrar
            afisam un mesaj de eroare) si trimite un `POST request`(functia
            `post_message_response` - inglobeaza mesajul cat si raspunsul)
            pe ruta `REGISTER_ROUTE`;
            - in `register_response` am cuprins mesajele trimise de
            server care mai pot aparea:
                - `400 Bad Reques` -> eroare, userul era deja creat;
                - `201 Created` -> succes, utilizatorul si-a creat un user;

    2. `login`:
        - daca `is_login = 1` se va intoarce un mesaj de eroare ce semnaleaza
        neputinta conectarii la alt cont in timp ce esti deja conectat;
        - altfel, daca utilizator nu e conectat, se apeleaza functia
        `register_login` in care:
            - se apeleaza functia `username_password` in care se introduc
            usernameul si parola de la tastatura, acestea trebuie sa fie
            valide(cuvinte fara spatii, nu pot fii goale - in caz contrar
            afisam un mesaj de eroare) si trimite un `POST request`(functia
            `post_message_response` - inglobeaza mesajul cat si raspunsul)
            pe ruta `LOGIN_ROUTE`;
            - in `login_response` am cuprins mesajele trimise de
            server care mai pot aparea:
                - `400 Bad Request` -> eroare, nu exista acest user;
                - `200 OK` -> succes, utilizatorul s-a logat si clientu va
                retine un cookie pentru autentificare;

    3. `enter_library`:
        - daca `is_login = 0` se va intoarce un mesaj de eroare ce semnaleaza
        neputinta accesarii bibliotecii, deoarece utilizatorul nu e conectat;
        - altfel, daca utilizator e conectat, se apeleaza functia
        `enter_library` in care:
            - se trimite un `GET request`(functia `get_message_response`)
            pe ruta `ACCESS_ROUTE`; 
            - apoi va genera un `token` retinut de client pentru a avea
            permisiunea de acces in biblioteca;
            - se afiseaza mesajul de succes pentru intrarea in biblioteca;

    4. `get_books`:
        - daca `is_login = 0` se va intoarce un mesaj de eroare ce indica
        ca ar trebui mai intai sa fii autentificat;
        - daca utilizator e conectat(`is_login = 1`) si are permisiuni de
        accesare a bibliotecii(`permission_library = 1`), se apeleaza functia
        `get_books` in care:
            - se trimite un `GET request`(functia `get_message_response`)
            pe ruta `BOOKS_ROUTE`; 
            - cu functie `str_between_ends` delimitez lista de carti din
            `response`-ul serverului;
            - apoi cu functia `parse_books_to_json` serializez si afisez
            cartile in format JSON ;
        - altfe, utilizatorul e logat, dar nu are acces la biblioteca, asadar,
        se va intoarce un mesaj de eroare;

    5. `add_book`:
        - daca `is_login = 0` se va intoarce un mesaj de eroare ce indica
        ca ar trebui mai intai sa fii autentificat;
        - daca utilizator e conectat(`is_login = 1`) si are permisiuni de
        accesare a bibliotecii(`permission_library = 1`), se apeleaza functia
        `add_books` in care:
            - se introduc informatiile cerute despre carte de la tastatura,
            acestea trebuie sa fie valide (fara spatii goale, nr. de pagini
            trebuie sa fie nr. intreg - in caz contrar afisam un mesaj de
            eroare), apoi trimite un `POST request`(functia
            `post_message_response`) pe ruta `BOOKS_ROUTE`; 
            - se afiseaza mesajul de succes pentru adaugarea cartii in
            biblioteca;
        - altfet, utilizatorul e logat, dar nu are acces la biblioteca, asadar,
        se va intoarce un mesaj de eroare;

    6. `get_book`:
        - daca `is_login = 0` se va intoarce un mesaj de eroare ce indica
        ca ar trebui mai intai sa fii autentificat;
        - daca utilizator e conectat(`is_login = 1`) si are permisiuni de
        accesare a bibliotecii(`permission_library = 1`), se apeleaza functia
        `get_book` in care:
            - se introduce id-ul cartii de la tastatura, acestea trebuie sa fie
            valide (fara spatii goale, id-ul trebuie sa fie nr. intreg - in
            caz contrar afisam un mesaj de eroare) 
            - se trimite un `GET request`(functia `get_message_response`)
            pe ruta `ID_BOOK_ROUTE/id`; 
            - daca `response`-ul de la server este `200 OK` -> succes,  
            delimitez cu `str_between_ends` cartea dorita, apoi cu functia
            `parse_books_to_json` afisez cartea in format JSON;
            - daca `response`-ul de la server este `404 Not Found` -> eroare,
            nu exista o carte cu acel id;
        - altfe, utilizatorul e logat, dar nu are acces la biblioteca, asadar,
        se va intoarce un mesaj de eroare;

    7. `delete_book`:
        - daca `is_login = 0` se va intoarce un mesaj de eroare ce indica
        ca ar trebui mai intai sa fii autentificat;
        - daca utilizator e conectat(`is_login = 1`) si are permisiuni de
        accesare a bibliotecii(`permission_library = 1`), se apeleaza functia
        `get_book` in care:
            - se introduce id-ul cartii de la tastatura, acestea trebuie sa fie
            valide (fara spatii goale, id-ul trebuie sa fie nr. intreg - in
            caz contrar afisam un mesaj de eroare) 
            - se trimite un `DELETE request` (functia `delete_message_response`,
            in care se apeleaza `compute_delete_request` definita ca un
            macro prin functia `compute_get_request`) pe ruta `ID_BOOK_ROUTE/id`; 
            - daca `response`-ul de la server este `200 OK` -> succes, cartea
            a fost stearsa;
            - daca `response`-ul de la server este `404 Not Found` -> eroare,
            nu exista o carte cu acel id;
        - altfe, utilizatorul e logat, dar nu are acces la biblioteca, asadar,
        se va intoarce un mesaj de eroare;

    8. `logout`:
        - daca `is_login = 0` se va intoarce un mesaj de eroare ce semnaleaza
        neputinta delogarii daca nu esti deja logat;
        - altfel, daca utilizator e conectat, se apeleaza functia
        `logout` in care:
            - se trimite un `GET request`(functia `get_message_response`)
            pe ruta `LOGOUT_ROUTE`; 
            - daca `response`-ul de la server este `200 OK` -> mesaj de succes,
            userul a fost delogat;
            - clientul va sterge cookie-ul asociat conectarii si token-ul de
            acces;
    
    9. `exit`:
        - Conexiunea clientul se inchide, programul se incheie, eliberand si
        resursele alocate.
    
    - Orice alta comanda va fi semnalata cu un mesaj de eroare.
