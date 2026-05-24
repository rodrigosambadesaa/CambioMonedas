#!/bin/sh
set -e
echo "Compilando programa de prueba..."
make debug
echo "Ejecutando batch de ejemplo..."
./progvoraz --input tests/sample_batch.csv --output tests/sample_batch_out.csv --log tests/batch.log
echo "Salida escrita en tests/sample_batch_out.csv"
