Test 1: jedan direktorijum
Komande:
add test_logs/dir1
stats
top 5

Ocekivano nakon sto worker-i obrade fajlove:
Ukupno log fajlova: 2
Ukupno linija: 9
ERROR: 3
WARNING: 2
INFO: 4

Najcesce poruke u dir1:
Connection timeout (2)
User login ok (2)
User login failed (2)

Napomena: test_logs/dir1/notes.txt i test_logs/dir1/fake_directory.log ne smeju da se obrade.


Test 2: dva scanner direktorijuma
Komande:
add test_logs/dir1
add test_logs/dir2
stats
top 5

Ocekivano:
Ukupno log fajlova: 3
Ukupno linija: 15
ERROR: 4
WARNING: 4
INFO: 7

Najcesca poruka:
Connection timeout (4)


Test 3: izmena postojeceg fajla append-om
Komande:
add test_logs/append_case
stats

Ocekivano pre append-a:
Ukupno log fajlova: 1
Ukupno linija: 3
ERROR: 1
WARNING: 1
INFO: 1

Zatim u drugom terminalu appenduj sadrzaj fajla append_these_lines.txt u growing.log:
cat test_logs/append_case/append_these_lines.txt >> test_logs/append_case/growing.log

Sacekaj vise od 5 sekundi, pa pokreni:
stats
top 5

Ocekivano posle append-a:
Ukupno log fajlova: 1
Ukupno linija: 6
ERROR: 2
WARNING: 1
INFO: 3

Najcesca poruka u append_case:
Batch started (2)
