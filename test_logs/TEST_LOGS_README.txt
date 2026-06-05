Pokreci svaki test iz root foldera projekta.

Prvo build

make clean
make

Test 1: jedan direktorijum
Pokreni:

make run

Zatim ukucaj:

add test_logs/dir1

Sacekaj bar 7 sekundi, pa:

stats
top 5
stop

Ocekivano za stats:

Broj fajlova: 3
Broj linija: 11
ERROR: 4
WARNING: 2
INFO: 5

U top 5 treba da vidis ove poruke, redoslijed moze da varira:

Connection timeout (2)
User login ok (2)
User login failed (2)
Txt log processed (2)

Test 2: dva direktorijuma
Pokreni program ponovo:

make run

Ukucaj:

add test_logs/dir1
add test_logs/dir2

Sacekaj bar 7 sekundi, pa:

stats
top 5
stop

Ocekivano za stats:

Broj fajlova: 4
Broj linija: 17
ERROR: 5
WARNING: 4
INFO: 8

U top 5 najbitnije je da vidis:

Connection timeout (4)

Takodje treba da se pojave neke od poruka sa (2), npr. User login ok, User login failed, Txt log processed, Queue
delayed.

Test 3: append postojeceg fajla
Pokreni program:

make run

Ukucaj:

add test_logs/append_case

Sacekaj bar 7 sekundi, pa:

stats

Ocekivano prije append-a:

Broj fajlova: 2
Broj linija: 5
ERROR: 1
WARNING: 2
INFO: 2

U drugom terminalu, iz istog foldera projekta, pokreni:

cat test_logs/append_source/append_these_lines.txt >> test_logs/append_case/growing.log

Vrati se u prvi terminal, sacekaj bar 7 sekundi, pa ukucaj:

stats
top 5
stop

Ocekivano poslije append-a:

Broj fajlova: 2
Broj linija: 8
ERROR: 2
WARNING: 2
INFO: 4

U top 5 treba da vidis:

Batch started (2)
Static txt processed (2)
