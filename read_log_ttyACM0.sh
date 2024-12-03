#!/bin/bash

stty -F /dev/ttyACM0 raw 115200
# Čtení ze souboru nebo standardního vstupu
cat /dev/ttyACM0 | while IFS= read -r line; do
  # Přidání aktuálního času na začátek řádky
  echo "$(date '+%Y-%m-%d %H:%M:%S') $line"
done < "${1:-/dev/stdin}"
