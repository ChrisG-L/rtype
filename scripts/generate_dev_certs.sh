#!/bin/bash
#===============================================================================
# R-Type - Generation de certificats TLS pour le developpement
#===============================================================================

set -e

CERT_DIR="${1:-certs}"
DAYS_VALID=365
KEY_SIZE=4096

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║     R-Type - Generation de Certificats TLS (Developpement)   ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""
echo "Repertoire de sortie: $CERT_DIR"
echo "Validite: $DAYS_VALID jours"
echo "Taille cle RSA: $KEY_SIZE bits"
echo ""

# Creer le repertoire
mkdir -p "$CERT_DIR"

# Generer la cle privee RSA
echo "[1/3] Generation de la cle privee RSA..."
openssl genrsa -out "$CERT_DIR/server.key" $KEY_SIZE 2>/dev/null

# Generer le certificat auto-signe avec SAN
echo "[2/3] Generation du certificat auto-signe..."
openssl req -new -x509 \
    -key "$CERT_DIR/server.key" \
    -out "$CERT_DIR/server.crt" \
    -days $DAYS_VALID \
    -subj "/CN=localhost/O=R-Type Development/OU=Game Server/C=FR" \
    -addext "subjectAltName=DNS:localhost,DNS:rtype.local,IP:127.0.0.1,IP:::1" \
    -addext "basicConstraints=critical,CA:FALSE" \
    -addext "keyUsage=digitalSignature,keyEncipherment" \
    -addext "extendedKeyUsage=serverAuth"

# Verifier le certificat
echo "[3/3] Verification du certificat..."
echo ""
openssl x509 -in "$CERT_DIR/server.crt" -noout -subject -dates -ext subjectAltName

# Definir les permissions
chmod 600 "$CERT_DIR/server.key"
chmod 644 "$CERT_DIR/server.crt"

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                  Certificats generes !                       ║"
echo "╠══════════════════════════════════════════════════════════════╣"
echo "║  Certificat: $CERT_DIR/server.crt"
echo "║  Cle privee: $CERT_DIR/server.key"
echo "╠══════════════════════════════════════════════════════════════╣"
echo "║  IMPORTANT: Ces certificats sont pour le DEVELOPPEMENT      ║"
echo "║  En production, utilisez Let's Encrypt ou une CA reconnue   ║"
echo "╚══════════════════════════════════════════════════════════════╝"
