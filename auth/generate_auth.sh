#!/bin/bash

# Generate a CA certificate
openssl genrsa -passout pass:password -out ca.key 4096
openssl req -passin pass:password -new -x509 -key ca.key -out ca.crt -subj "/C=KR/ST=Seoul/L=Gangnam/O=Avikus Co./CN=Root CA"

# Generate a server certificate
openssl genrsa -out server.key 4096
openssl req -new -key server.key -out server.csr -subj "/C=KR/ST=Seoul/L=Gangnam/O=Avikus Co./CN=localhost"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out server.crt
rm server.csr