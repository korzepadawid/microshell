# unix-shell
## Instalacja
```
$ sudo apt-get install libreadline-dev
```

```
$ make
```

```
$ ./microshell
```
## Polecenie

Zadanie polega na wykorzystaniu języka ANSI C do napisania prostego programu powłoki – Microshell. Program ten powinien przyjmować na wejściu polecenia, a następnie wykonywać działania zgodne z ich treścia. Powłoka powinna:

- wyświetlać znak zachęty w postaci [{path}] $, gdzie {path} jest scieżką do bieżącego katalogu roboczego

- obsługiwać polecenie cd, działające analogicznie do tego znanego nam z powłoki bash

- obsługiwać polecenie exit, kończące działanie programu powłoki

- obsługiwać polecenie help, wyświetlające na ekranie informacje o autorze programu i oferowanych przez niego funkcjonalnościach

- obsługiwać dwa inne, dowolnie wybrane polecenia powłoki (chodzi tutaj np. o własną, samodzielną, prostą implementację dwóch poleceń)

- przyjmować polecenia odwołujące się przez nazwę do skryptow i programów znajdujacych się w katalogach opisanych wartoscią zmiennej środowiskowej PATH oraz umożliwiać wywołanie tych skryptów i programów z argumentami

- wypisywać komunikat błędu, gdy niemożliwe jest poprawne zinterpretowanie polecenia

- posiadać tzw. dodatkowe bajery (w zależności od stopnia skomplikowania problemu), np. wyświetlanie loginu aktualnie zalogowanego użytkownika, obsługę kolorów, obsługę argumentów w cudzysłowach, sensowną obsługę sygnałów (np. Ctrl+Z), obsługę historii poleceń, uzupełnianie składni, itp.
