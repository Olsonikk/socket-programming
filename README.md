# Matematyczny Wyścig - Opis Projektu

## **Opis**

Gracz łączy się z serwerem i wysyła swój nick (jeśli nick jest już zajęty, serwer prosi o podanie innego nicku).

Po wybraniu nicku gracz trafia do lobby, w którym widzi bieżącą listę pokoi i liczbę graczy obecnych w każdym z nich.

Z poziomu lobby gracz może dołączyć do istniejącego pokoju lub stworzyć nowy, jeśli nie osiągnięto maksymalnej ustalonej z góry liczby pokoi. W każdej chwili gracz może również wrócić do lobby.

Jeśli w pokoju trwa gra, gracz dołącza jako uczestnik, rywalizujący o jak najszybsze rozwiązanie kolejnych zadań. Jeśli gra nie jest aktywna, gracz widzi listę obecnych graczy i oczekuje na rozpoczęcie rozgrywki.

Rozpocząć grę może gracz, który najdłużej czeka w pokoju, pod warunkiem że w pokoju znajduje się przynajmniej dwóch graczy.

Serwer przesyła wszystkim uczestnikom pierwsze zadanie matematyczne. Wszyscy uczestnicy mają ten sam czas na **wpisanie** odpowiedzi. Wpisane odpowiedzi nie są widziane przez wszystkich graczy, ale gracze dostają informację, jeśli przeciwnik odpowie poprawnie. Jeśli gracz odpowie poprawnie, to otrzymuje punkty na podstawie czasu odpowiedzi - im szybciej, tym lepiej. Dodatkowo pierwszy gracz, który odpowie poprawnie, otrzymuje premię za bycie najszybszym. Jeśli kilku graczy poda poprawną odpowiedź w tym samym momencie, serwer decyduje kto powinien dostać premię.

Jeśli żaden z graczy nie odpowie poprawnie na pytanie, to nikt nie otrzymuje punktów.

Po rozwiązaniu zadania serwer przesyła kolejne pytanie, a rozgrywka trwa aż do zdobycia przez jednego z graczy określonej liczby punktów lub zmniejszenia liczby graczy w pokoju poniżej dwóch.

Gracze na bieżąco widzą ranking punktów.

