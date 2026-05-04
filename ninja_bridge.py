#!/usr/bin/env python3
import sys
import json
import requests

# URL del servidor de evaluación (Aprendif Docker)
SERVER_URL = "http://aprendif.ugr.es:9821/get_move"

def connect_to_ninja_server():
    if len(sys.argv) < 6:
        print("0 0")
        return

    try:
        # Preparamos el JSON con los datos del tablero
        payload = {
            "filas": int(sys.argv[1]),
            "cols": int(sys.argv[2]),
            "n": int(sys.argv[3]),
            "id": int(sys.argv[4]),
            "nivel": int(sys.argv[5]),
            "fase_actual": int(sys.argv[6]),
            "movimientos_restantes": int(sys.argv[7]),
            "turno_actual": int(sys.argv[8]),
            "tablero": [int(x) for x in sys.argv[9:]]
        }
        
        # Hacemos la petición con un timeout de 60s
        r = requests.post(SERVER_URL, json=payload, timeout=60.0)
        res = r.json()
        
        if 'fila' in res and 'columna' in res:
            print(f"{res['fila']} {res['columna']}")
        else:
            print(f"Error en respuesta del servidor: {res}")
            
    except Exception as e:
        print(f"Error de red en el puente: {e}")

if __name__ == "__main__":
    connect_to_ninja_server()
