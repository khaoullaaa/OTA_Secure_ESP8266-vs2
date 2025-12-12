#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Serveur OTA local pour tester les mises � jour
Version corrig�e pour UTF-8
"""

import http.server
import socketserver
import ssl
import os
import json
from datetime import datetime

# Configuration
PORT = 8443
HOST = "0.0.0.0"  # Écouter sur toutes les interfaces
CERT_FILE = "server.crt"
KEY_FILE = "server.key"
FIRMWARE_FILE = "firmware.bin"

# Obtenir l'IP locale automatiquement
import socket
def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return "127.0.0.1"

LOCAL_IP = get_local_ip()

class OTAServer(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/manifest.json':
            self.send_manifest()
        elif self.path == '/firmware.bin':
            self.send_firmware()
        else:
            self.send_error(404, "File not found")
    
    def send_manifest(self):
        # Generer un manifest dynamique
        manifest = {
            "version": "2.0.0",
            "firmware_url": f"https://{LOCAL_IP}:{PORT}/firmware.bin",
            "sha256": self.calculate_sha256(),
            "build_date": datetime.now().isoformat(),
            "description": "Test local OTA",
            "min_version": "1.0.0",
            "size": os.path.getsize(FIRMWARE_FILE) if os.path.exists(FIRMWARE_FILE) else 0
        }
        
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(manifest).encode('utf-8'))
    
    def send_firmware(self):
        if os.path.exists(FIRMWARE_FILE):
            self.send_response(200)
            self.send_header('Content-Type', 'application/octet-stream')
            self.send_header('Content-Length', str(os.path.getsize(FIRMWARE_FILE)))
            self.end_headers()
            
            with open(FIRMWARE_FILE, 'rb') as f:
                self.wfile.write(f.read())
        else:
            self.send_error(404, "Firmware not found")
    
    def calculate_sha256(self):
        import hashlib
        if os.path.exists(FIRMWARE_FILE):
            with open(FIRMWARE_FILE, 'rb') as f:
                return hashlib.sha256(f.read()).hexdigest()
        return "0" * 64
    
    def log_message(self, format, *args):
        # Desactiver les logs de requetes
        pass

def generate_self_signed_cert():
    """Genere un certificat auto-signe pour les tests"""
    print("Installation de la bibliotheque cryptography...")
    os.system("pip install cryptography")
    
    from cryptography import x509
    from cryptography.x509.oid import NameOID
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import rsa
    import datetime
    
    print("Generation de la cle privee...")
    # Generer une cle privee
    key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
    )
    
    print("Creation du certificat auto-signe...")
    # Creer un certificat auto-signe
    subject = issuer = x509.Name([
        x509.NameAttribute(NameOID.COUNTRY_NAME, "FR"),
        x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, "Test"),
        x509.NameAttribute(NameOID.LOCALITY_NAME, "Test"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, "OTA Test"),
        x509.NameAttribute(NameOID.COMMON_NAME, LOCAL_IP),
    ])
    
    cert = x509.CertificateBuilder().subject_name(
        subject
    ).issuer_name(
        issuer
    ).public_key(
        key.public_key()
    ).serial_number(
        x509.random_serial_number()
    ).not_valid_before(
        datetime.datetime.utcnow()
    ).not_valid_after(
        datetime.datetime.utcnow() + datetime.timedelta(days=365)
    ).sign(key, hashes.SHA256())
    
    # Sauvegarder le certificat et la cle
    with open(KEY_FILE, "wb") as f:
        f.write(key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption()
        ))
    
    with open(CERT_FILE, "wb") as f:
        f.write(cert.public_bytes(serialization.Encoding.PEM))
    
    print(f"Certificat genere: {CERT_FILE}")
    print(f"Cle generee: {KEY_FILE}")

def main():
    print("=== Serveur OTA Local ===")
    print(f"IP détectée: {LOCAL_IP}")
    print(f"Adresse: https://{LOCAL_IP}:{PORT}")
    
    # Verifier si Python a les dependances
    try:
        import cryptography
    except ImportError:
        print("Installation des dependances...")
        os.system("pip install cryptography")
    
    # Verifier si les fichiers de certificat existent
    if not os.path.exists(CERT_FILE) or not os.path.exists(KEY_FILE):
        print("Generation du certificat auto-signe...")
        generate_self_signed_cert()
    
    # Creer un fichier firmware.bin factice si inexistant
    if not os.path.exists(FIRMWARE_FILE):
        print(f"Creation du fichier {FIRMWARE_FILE}...")
        with open(FIRMWARE_FILE, 'wb') as f:
            # Donnees factices pour le firmware
            f.write(bytes([0xE9] * 100000))  # 100KB de donnees de test
    
    # Demarrer le serveur HTTPS
    try:
        httpd = socketserver.TCPServer((HOST, PORT), OTAServer)
        httpd.allow_reuse_address = True
        
        # Configurer SSL
        httpd.socket = ssl.wrap_socket(
            httpd.socket,
            keyfile=KEY_FILE,
            certfile=CERT_FILE,
            server_side=True,
            ssl_version=ssl.PROTOCOL_TLS
        )
        
        print(f"\n✅ Serveur demarre sur le port {PORT}")
        print("Endpoints disponibles:")
        print(f"  GET https://{LOCAL_IP}:{PORT}/manifest.json")
        print(f"  GET https://{LOCAL_IP}:{PORT}/firmware.bin")
        print(f"\n📝 Mettre à jour manifestURL dans le firmware:")
        print(f"   const char* manifestURL = \"https://{LOCAL_IP}:{PORT}/manifest.json\";")
        print("\n⚠️  N'oubliez pas de désactiver la vérification SSL pour les tests locaux")
        print("\nAppuyez sur Ctrl+C pour arreter\n")
        
        httpd.serve_forever()
        
    except OSError as e:
        print(f"Erreur: {e}")
        print("Essayez de changer le port (ex: 8444) ou attendez 1 minute")
        input("Appuyez sur Entree pour quitter...")
    except KeyboardInterrupt:
        print("\nArret du serveur")
    finally:
        if 'httpd' in locals():
            httpd.server_close()

if __name__ == "__main__":
    main()