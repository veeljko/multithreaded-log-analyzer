#!/usr/bin/env bash
set -euo pipefail

fail() {
    echo "FAIL: $1"
    echo
    echo "Kompletan izlaz programa je sacuvan u:"
    echo "$OUT_FILE"
    exit 1
}

pass() {
    echo "OK: $1"
}

assert_contains() {
    local pattern="$1"
    local description="$2"

    if grep -Fq "$pattern" "$OUT_FILE"; then
        pass "$description"
    else
        fail "$description"
    fi
}

assert_not_contains() {
    local pattern="$1"
    local description="$2"

    if grep -Fq "$pattern" "$OUT_FILE"; then
        fail "$description"
    else
        pass "$description"
    fi
}

make clean
make

TEST_ROOT="$(mktemp -d)"
LOG_DIR_1="$TEST_ROOT/logovi-1"
LOG_DIR_2="$TEST_ROOT/logovi-2"
OUT_FILE="$TEST_ROOT/output.txt"
mkdir -p "$LOG_DIR_1" "$LOG_DIR_2" "$LOG_DIR_1/poddir"

cleanup() {
    local exit_code=$?

    if [ "$exit_code" -eq 0 ]; then
        rm -rf "$TEST_ROOT"
    else
        echo
        echo "Privremeni test folder nije obrisan zbog debug-a:"
        echo "$TEST_ROOT"
    fi
}
trap cleanup EXIT

cat > "$LOG_DIR_1/app.log" <<'LOG'
1. 2026-05-29T00:16:59.705Z severity="INFO" Alpha started
2. 2026-05-29T00:17:00.705Z severity="ERROR" Connection timeout
3. 2026-05-29T00:17:01.705Z severity="WARNING" Disk almost full
LOG

cat > "$LOG_DIR_1/ignore.csv" <<'LOG'
1. 2026-05-29T00:17:02.705Z severity="ERROR" This csv file must be ignored
LOG

cat > "$LOG_DIR_1/poddir/ignored.log" <<'LOG'
1. 2026-05-29T00:17:03.705Z severity="ERROR" This subdir log must be ignored
LOG

cat > "$LOG_DIR_2/service.log" <<'LOG'
1. 2026-05-29T00:18:00.705Z severity="INFO" Alpha started
2. 2026-05-29T00:18:01.705Z severity="INFO" Cache warmed
3. 2026-05-29T00:18:02.705Z severity="ERROR" Connection timeout
4. 2026-05-29T00:18:03.705Z severity="ERROR" Connection timeout
LOG

echo "Test folder:"
echo "$TEST_ROOT"
echo
echo "Pokrecem program i saljem komande..."

{
    # Faza 1: jedan direktorijum, jedan .log fajl.
    printf "add %s\n" "$LOG_DIR_1"
    sleep 7
    printf "stats\n"
    printf "top 5\n"

    # Faza 2: drugi direktorijum i dodatni worker posao.
    printf "add %s\n" "$LOG_DIR_2"
    sleep 7
    printf "stats\n"
    printf "top 5\n"

    # Faza 3: izmjena postojecih fajlova i dodavanje novog .log fajla.
    cat >> "$LOG_DIR_1/app.log" <<'LOG'
4. 2026-05-29T00:19:00.705Z severity="INFO" Alpha started
5. 2026-05-29T00:19:01.705Z severity="ERROR" Database down
LOG

    cat > "$LOG_DIR_1/new.log" <<'LOG'
1. 2026-05-29T00:19:02.705Z severity="WARNING" Disk almost full
2. 2026-05-29T00:19:03.705Z severity="INFO" Alpha started
LOG

    cat >> "$LOG_DIR_2/service.log" <<'LOG'
5. 2026-05-29T00:19:04.705Z severity="WARNING" Queue delayed
LOG

    sleep 7
    printf "stats\n"
    printf "top 5\n"
    printf "stop\n"
} | ./program > "$OUT_FILE"

echo
echo "=== Izlaz programa ==="
cat "$OUT_FILE"

echo
echo "=== Provere ==="

# Poslije prve faze treba da budu obradjene 3 linije iz LOG_DIR_1/app.log.
assert_contains "Broj fajlova: 1" "prva faza vidi 1 procitan log fajl"
assert_contains "Broj linija: 3" "prva faza vidi 3 log linije"
assert_contains "ERROR: 1" "prva faza broji 1 ERROR"
assert_contains "WARNING: 1" "prva faza broji 1 WARNING"
assert_contains "INFO: 1" "prva faza broji 1 INFO"

# Poslije druge faze ukupno treba da bude 7 linija:
# LOG_DIR_1/app.log = 3, LOG_DIR_2/service.log = 4.
assert_contains "Broj fajlova: 2" "druga faza vidi 2 procitana log fajla"
assert_contains "Broj linija: 7" "druga faza vidi 7 log linija"
assert_contains "ERROR: 3" "druga faza broji 3 ERROR zapisa"
assert_contains "WARNING: 1" "druga faza i dalje broji 1 WARNING"
assert_contains "INFO: 3" "druga faza broji 3 INFO zapisa"
assert_contains "Connection timeout (3)" "top poslije druge faze vidi Connection timeout 3 puta"
assert_contains "Alpha started (2)" "top poslije druge faze vidi Alpha started 2 puta"

# Poslije trece faze ukupno treba da bude 12 linija:
# prethodnih 7 + 2 dopisane u app.log + 2 iz new.log + 1 dopisana u service.log.
assert_contains "Broj fajlova: 3" "treca faza vidi 3 procitana log fajla"
assert_contains "Broj linija: 12" "treca faza vidi 12 log linija"
assert_contains "ERROR: 4" "treca faza broji 4 ERROR zapisa"
assert_contains "WARNING: 3" "treca faza broji 3 WARNING zapisa"
assert_contains "INFO: 5" "treca faza broji 5 INFO zapisa"
assert_contains "Alpha started (4)" "finalni top vidi Alpha started 4 puta"
assert_contains "Connection timeout (3)" "finalni top vidi Connection timeout 3 puta"
assert_contains "Disk almost full (2)" "finalni top vidi Disk almost full 2 puta"

# Ovi zapisi ne smiju biti obradjeni: .csv nije log fajl, a poddirektorijum se ignorise.
assert_not_contains "This csv file must be ignored" "program ignorise .csv fajl"
assert_not_contains "This subdir log must be ignored" "program ignorise log fajl iz poddirektorijuma"

echo
echo "Svi automatizovani testovi su prosli."
echo
echo "Rucni watch test:"
echo "1. Pokreni: make run"
echo "2. Unesi: add test_logs/dir1/"
echo "3. Sacekaj nekoliko sekundi, pa unesi: watch"
echo "4. Iz watch rezima izadji sa CTRL+D"
echo "5. Unesi: stop"
