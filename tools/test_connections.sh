#!/usr/bin/env bash
# Interactive connection test for plain, SSC, SCA, and BSK ADS.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_DIR/build}"

# Network / credentials
LOCAL_IP="192.168.1.220"
LOCAL_AMS="192.168.1.220.1.1"
PLC_IP="192.168.1.180"
PLC_AMS="39.178.175.124.1.1"
ADS_USER="Administrator"
ADS_PASS=""

# Certificate paths
CERTS_DIR="$REPO_DIR/certs"
SSC_KEY="$CERTS_DIR/ssc_client.key"
SSC_CERT="$CERTS_DIR/ssc_client.crt"
SCA_KEY="$CERTS_DIR/sca_client.key"
SCA_CERT="$CERTS_DIR/sca_client.crt"
SCA_CA="/home/ick3/certsonPlc/rootCA.pem"
SCA_CA_KEY="/home/ick3/certsonPlc/rootCA.key"

ADSTOOL="$BUILD_DIR/adstool"
TEST_BIN="$BUILD_DIR/test_connections"

PASS=0
FAIL=0

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
green()  { printf '\033[0;32m%s\033[0m\n' "$*"; }
red()    { printf '\033[0;31m%s\033[0m\n' "$*"; }
yellow() { printf '\033[0;33m%s\033[0m\n' "$*"; }
bold()   { printf '\033[1m%s\033[0m\n' "$*"; }

run_test() {
    local label="$1"; shift
    bold "--- $label ---"
    if "$@"; then
        green "PASS: $label"
        PASS=$((PASS + 1))
    else
        red "FAIL: $label (exit $?)"
        FAIL=$((FAIL + 1))
    fi
    echo
}

print_summary() {
    bold "=== Results ==="
    green "PASSED: $PASS"
    if [[ "$FAIL" -gt 0 ]]; then
        red "FAILED: $FAIL"
    else
        echo "FAILED: 0"
    fi
}

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
do_build() {
    bold "=== Building ==="
    if [ ! -d "$BUILD_DIR" ]; then
        meson setup "$BUILD_DIR" "$REPO_DIR"
    fi
    ninja -C "$BUILD_DIR" adstool test_connections
    echo
}

# ---------------------------------------------------------------------------
# Certificate helpers
# ---------------------------------------------------------------------------
ensure_ssc_cert() {
    mkdir -p "$CERTS_DIR"
    if [ ! -f "$SSC_KEY" ] || [ ! -f "$SSC_CERT" ]; then
        bold "Generating self-signed SSC client cert (CN=cachyos-x8664)..."
        openssl req -x509 -newkey rsa:2048 -nodes \
            -keyout "$SSC_KEY" -out "$SSC_CERT" \
            -days 3650 -subj "/CN=cachyos-x8664" 2>/dev/null
        echo "Created $SSC_CERT"
    fi
}

ensure_sca_cert() {
    mkdir -p "$CERTS_DIR"
    if [ ! -f "$SCA_KEY" ] || [ ! -f "$SCA_CERT" ]; then
        if [ ! -f "$SCA_CA" ] || [ ! -f "$SCA_CA_KEY" ]; then
            yellow "WARNING: CA files not found at $SCA_CA_KEY / $SCA_CA"
            return 1
        fi
        bold "Generating SCA client cert signed by rootCA..."
        openssl genrsa -out "$SCA_KEY" 2048 2>/dev/null
        openssl req -new -key "$SCA_KEY" \
            -out "$CERTS_DIR/sca_client.csr" \
            -subj "/CN=cachyos-x8664" 2>/dev/null
        openssl x509 -req \
            -in "$CERTS_DIR/sca_client.csr" \
            -CA "$SCA_CA" -CAkey "$SCA_CA_KEY" -CAcreateserial \
            -out "$SCA_CERT" -days 3650 2>/dev/null
        rm -f "$CERTS_DIR/sca_client.csr"
        echo "Created $SCA_CERT"
    fi
    [ -f "$SCA_CA" ] || { yellow "WARNING: CA cert $SCA_CA not found"; return 1; }
}

# ---------------------------------------------------------------------------
# Individual test runners
# ---------------------------------------------------------------------------
test_plain() {
    bold "=== Test: Plain ADS ==="
    echo "Registering route on PLC via adstool..."
    "$ADSTOOL" "$PLC_IP" addroute \
        --netid="$LOCAL_AMS" --addr="$LOCAL_IP" \
        --password="$ADS_PASS" || true
    echo
    run_test "plain ADS" \
        "$TEST_BIN" "$PLC_AMS" \
            --gw="$PLC_IP" \
            --localams="$LOCAL_AMS"
}

test_ssc() {
    ensure_ssc_cert
    bold "=== Test: SSC first-time registration ==="
    run_test "SSC (first-time, with credentials)" \
        "$TEST_BIN" "$PLC_AMS" \
            --gw="$PLC_IP" --localams="$LOCAL_AMS" \
            --mode=ssc --cert="$SSC_CERT" --key="$SSC_KEY" \
            --username="$ADS_USER" --password="$ADS_PASS"

    bold "=== Test: SSC established (no credentials) ==="
    run_test "SSC (established)" \
        "$TEST_BIN" "$PLC_AMS" \
            --gw="$PLC_IP" --localams="$LOCAL_AMS" \
            --mode=ssc --cert="$SSC_CERT" --key="$SSC_KEY"
}

test_sca() {
    if ! ensure_sca_cert; then
        yellow "SKIP: SCA test (CA files missing)"
        return
    fi
    bold "=== Test: SCA ==="
    run_test "SCA" \
        "$TEST_BIN" "$PLC_AMS" \
            --gw="$PLC_IP" --localams="$LOCAL_AMS" \
            --mode=sca --cert="$SCA_CERT" --key="$SCA_KEY" \
            --ca="$SCA_CA"
}

test_psk() {
    bold "=== Test: PSK (Pre-Shared Key) ==="
    printf "PSK identity: "
    read -r PSK_IDENTITY
    printf "PSK password: "
    read -rs PSK_PASSWORD
    echo
    if [[ -z "$PSK_IDENTITY" || -z "$PSK_PASSWORD" ]]; then
        yellow "SKIP: PSK test (identity or password empty)"
        return
    fi
    run_test "PSK" \
        "$TEST_BIN" "$PLC_AMS" \
            --gw="$PLC_IP" --localams="$LOCAL_AMS" \
            --mode=psk \
            --psk-identity="$PSK_IDENTITY" \
            --password="$PSK_PASSWORD"
}

test_bsk_Prefilled() {
    bold "=== Test: PSK (Pre-Shared Key) ==="
    run_test "PSK" \
        "$TEST_BIN" "$PLC_AMS" \
            --gw="$PLC_IP" --localams="$LOCAL_AMS" \
            --mode=psk \
            --psk-identity="MY_IDENTITY" \
            --password="MySecret"
}

# ---------------------------------------------------------------------------
# Interactive menu
# ---------------------------------------------------------------------------
echo
bold "================================================"
bold "  ADS Connection Test Suite"
bold "================================================"
echo
echo "  1) All tests( Depends on Route Configuration PSK or SCA will fail because only one can be active at time)"
echo "  2) SSC  (Self-Signed Certificate)"
echo "  3) SCA  (Shared CA Certificate)"
echo "  4) Plain ADS"
echo "  5) PSK  (Pre-Shared Key)"
echo "  6) PSK Prefilled"
echo
printf "Select test [1-6]: "
read -r CHOICE
echo

do_build

case "$CHOICE" in
    1)  test_psk      # Does not work If not called first. After a SSC no
        test_plain
        test_ssc
        test_sca

        ;;
    2) test_ssc   ;;
    3) test_sca   ;;
    4) test_plain ;;
    5) test_psk   ;;
    6) test_bsk_Prefilled;;   # Only Runable if the Prefilled Values are used for the PSK!

    *)
        red "Invalid selection: $CHOICE"
        exit 1
        ;;
esac

echo
print_summary
[[ "$FAIL" -gt 0 ]] && exit 1 || exit 0